/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file        unittest_capi_lxm_service.cc
 * @date        26 JULY 2025
 * @brief       Unit test for ml-lxm-service.
 * @see         https://github.com/nnstreamer/api
 * @author      Hyunil Park <hyunil46.park@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <glib.h>
#include <ml-api-common.h>
#include <ml-api-service-private.h>
#include <ml-api-service.h>
#include <string.h>
#include "ml-lxm-service-internal.h"
#include "unittest_util.h"

#if defined(ENABLE_LLAMACPP)

/**
 * @brief Internal function to get the model file path.
 */
static gchar *
_get_model_path (const gchar *model_name)
{
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Supposed to run test in build directory. */
  if (root_path == NULL)
    root_path = "..";

  gchar *model_file = g_build_filename (
      root_path, "tests", "test_models", "models", model_name, NULL);

  return model_file;
}

/**
 * @brief Macro to skip testcase if required files are not ready.
 */
#define skip_lxm_tc(tc_name)                                                                       \
  do {                                                                                             \
    g_autofree gchar *model_file = _get_model_path ("llama-2-7b-chat.Q2_K.gguf");                  \
    if (!g_file_test (model_file, G_FILE_TEST_EXISTS)) {                                           \
      g_autofree gchar *msg = g_strdup_printf (                                                    \
          "Skipping '%s' due to missing model file. "                                              \
          "Please download model file from https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF.", \
          tc_name);                                                                                \
      GTEST_SKIP () << msg;                                                                        \
    }                                                                                              \
  } while (0)

/**
 * @brief Test data structure to pass to the callback.
 */
typedef struct {
  int token_count;
  GString *received_tokens;
} lxm_test_data_s;

/**
 * @brief Callback function for LXM service token streaming.
 */
static void
_lxm_token_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  lxm_test_data_s *tdata = (lxm_test_data_s *) user_data;
  ml_tensors_data_h data = NULL;
  void *_raw = NULL;
  size_t _size = 0;
  int status;

  switch (event) {
    case ML_SERVICE_EVENT_NEW_DATA:
      ASSERT_TRUE (event_data != NULL);

      status = ml_information_get (event_data, "data", &data);
      EXPECT_EQ (status, ML_ERROR_NONE);
      if (status != ML_ERROR_NONE)
        return;

      status = ml_tensors_data_get_tensor_data (data, 0U, &_raw, &_size);
      EXPECT_EQ (status, ML_ERROR_NONE);
      if (status != ML_ERROR_NONE)
        return;

      if (tdata) {
        if (tdata->received_tokens) {
          g_string_append_len (tdata->received_tokens, (const char *) _raw, _size);
        }
        tdata->token_count++;
      }
      g_print ("%.*s", (int) _size, (char *) _raw); // Print received token
      break;
    default:
      // Handle unknown or unimplemented events if necessary
      g_printerr ("Received unhandled LXM service event: %d\n", event);
      break;
  }
}

/**
 * @brief Internal function to run a full LXM session test.
 */
static void
_run_lxm_session_test (const gchar *config_path, const gchar *input_text, ml_option_h options)
{
  ml_lxm_session_h session = NULL;
  ml_lxm_prompt_h prompt = NULL;
  lxm_test_data_s tdata = { 0, NULL };
  int status;

  tdata.received_tokens = g_string_new ("");

  // 1. Create session with callback
  status = ml_lxm_session_create (config_path, NULL, _lxm_token_cb, &tdata, &session);
  ASSERT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (session != NULL);

  // 2. Create prompt
  status = ml_lxm_prompt_create (&prompt);
  ASSERT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (prompt != NULL);

  status = ml_lxm_prompt_append_text (prompt, input_text);
  ASSERT_EQ (status, ML_ERROR_NONE);

  // 3. Generate response (callback is already set during session creation)
  status = ml_lxm_session_respond (session, prompt, options);
  ASSERT_EQ (status, ML_ERROR_NONE);

  // Wait for the callback to receive data.
  // 10 seconds should be enough for a simple response.
  g_usleep (10000000U);

  // 4. Verify results
  EXPECT_GT (tdata.token_count, 0);
  EXPECT_GT (tdata.received_tokens->len, 0U);

  g_print ("\nReceived total tokens: %d\n", tdata.token_count);
  g_print ("Full received text: %s\n", tdata.received_tokens->str);

  // 5. Cleanup
  status = ml_lxm_prompt_destroy (prompt);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_lxm_session_destroy (session);
  EXPECT_EQ (status, ML_ERROR_NONE);

  if (tdata.received_tokens) {
    g_string_free (tdata.received_tokens, TRUE);
  }
}

/**
 * @brief Test basic flow of LXM service.
 */
TEST (MLLxmService, basicFlow_p)
{
  skip_lxm_tc ("basicFlow_p");

  g_autofree gchar *config = get_config_path ("config_single_llamacpp.conf");
  ASSERT_TRUE (config != NULL);

  const gchar input_text[] = "Hello LXM, how are you?";
  ml_option_h options = NULL;
  int status;

  // Create options
  status = ml_option_create (&options);
  ASSERT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (options != NULL);

  // Set temperature option
  status = ml_option_set (options, "temperature", g_strdup_printf ("%f", 0.8), g_free);
  ASSERT_EQ (status, ML_ERROR_NONE);

  // Set max_tokens option
  status = ml_option_set (
      options, "max_tokens", g_strdup_printf ("%zu", (size_t) 32), g_free);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _run_lxm_session_test (config, input_text, options);

  // Cleanup options
  ml_option_destroy (options);
}


/**
 * @brief Test LXM service with invalid parameters.
 */
TEST (MLLxmService, invalidParams_n)
{
  ml_lxm_session_h session = NULL;
  ml_lxm_prompt_h prompt = NULL;
  int status;
  ml_option_h options = NULL;
  g_autofree gchar *valid_config = get_config_path ("config_single_llamacpp.conf");

  // Create options for testing
  status = ml_option_create (&options);
  ASSERT_EQ (status, ML_ERROR_NONE);
  ml_option_set (options, "temperature", g_strdup_printf ("%f", 0.5), g_free);
  ml_option_set (options, "max_tokens", g_strdup_printf ("%zu", (size_t) 10), g_free);

  // ml_lxm_session_create
  status = ml_lxm_session_create (valid_config, NULL, NULL, NULL, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
  status = ml_lxm_session_create (NULL, NULL, NULL, NULL, &session);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_lxm_session_create ("non_existent_config.conf", NULL, NULL, NULL, &session);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_lxm_session_create (valid_config, NULL, NULL, NULL, &session);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_lxm_session_create (valid_config, NULL, _lxm_token_cb, NULL, &session);
  if (status == ML_ERROR_NONE) {
    // ml_lxm_prompt_create
    status = ml_lxm_prompt_create (NULL);
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

    status = ml_lxm_prompt_create (&prompt);
    ASSERT_EQ (status, ML_ERROR_NONE);

    // ml_lxm_prompt_append_text
    status = ml_lxm_prompt_append_text (NULL, "text");
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
    status = ml_lxm_prompt_append_text (prompt, NULL);
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

    // ml_lxm_prompt_append_instruction
    status = ml_lxm_prompt_append_instruction (NULL, "instruction");
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
    status = ml_lxm_prompt_append_instruction (prompt, NULL);
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

    // ml_lxm_session_set_instructions
    status = ml_lxm_session_set_instructions (NULL, "new instructions");
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
    status = ml_lxm_session_set_instructions (session, NULL);
    EXPECT_EQ (status, ML_ERROR_NONE);
    status = ml_lxm_session_set_instructions (session, "new instructions");
    EXPECT_EQ (status, ML_ERROR_NONE);

    // ml_lxm_session_respond - callback is already set during session creation
    status = ml_lxm_session_respond (NULL, prompt, options);
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
    status = ml_lxm_session_respond (session, NULL, options);
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

    // Now ml_lxm_session_respond should succeed with valid parameters
    status = ml_lxm_session_respond (session, prompt, options);
    EXPECT_EQ (status, ML_ERROR_NONE);

    // ml_lxm_prompt_destroy
    status = ml_lxm_prompt_destroy (NULL);
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
    status = ml_lxm_prompt_destroy (prompt);
    EXPECT_EQ (status, ML_ERROR_NONE);
    prompt = NULL;

    // ml_lxm_session_destroy
    status = ml_lxm_session_destroy (NULL);
    EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
    status = ml_lxm_session_destroy (session);
    EXPECT_EQ (status, ML_ERROR_NONE);
    session = NULL;
  } else {
    g_print ("Skipping part of invalidParams_n as session creation failed (possibly due to missing models/config).\n");
  }

  // Cleanup options
  ml_option_destroy (options);
}

/**
 * @brief Main function to run the test.
 */
int
main (int argc, char **argv)
{
  int result = -1;

  try {
    testing::InitGoogleTest (&argc, argv);
  } catch (...) {
    g_warning ("catch 'testing::internal::<unnamed>::ClassUniqueToAlwaysTrue'");
  }

  /* ignore tizen feature status while running the testcases */
  set_feature_state (ML_FEATURE, SUPPORTED);
  set_feature_state (ML_FEATURE_INFERENCE, SUPPORTED);
  set_feature_state (ML_FEATURE_SERVICE, SUPPORTED);

  try {
    result = RUN_ALL_TESTS ();
  } catch (...) {
    g_warning ("catch `testing::internal::GoogleTestFailureException`");
  }

  set_feature_state (ML_FEATURE, NOT_CHECKED_YET);
  set_feature_state (ML_FEATURE_INFERENCE, NOT_CHECKED_YET);
  set_feature_state (ML_FEATURE_SERVICE, NOT_CHECKED_YET);

  return result;
}
#endif /* ENABLE_LLAMACPP */
