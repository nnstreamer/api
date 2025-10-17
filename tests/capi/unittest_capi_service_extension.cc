/**
 * @file        unittest_capi_service_extension.cc
 * @date        1 September 2023
 * @brief       Unittest for ML service extension C-API.
 * @see         https://github.com/nnstreamer/api
 * @author      Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug         No known bugs except for NYI items
 */

#include <gtest/gtest.h>
#include <glib.h>
#include <iostream>
#include <ml-api-service-private.h>
#include <ml-api-service.h>
#include "ml-api-service-extension.h"
#include "unittest_util.h"

#if defined(ENABLE_TENSORFLOW_LITE) || defined(ENABLE_TENSORFLOW2_LITE)
#define TEST_REQUIRE_TFLITE(Case, Name) TEST (Case, Name)
#define TEST_F_REQUIRE_TFLITE(Case, Name) TEST_F (Case, Name)
#else
#define TEST_REQUIRE_TFLITE(Case, Name) TEST (Case, DISABLED_##Name)
#define TEST_F_REQUIRE_TFLITE(Case, Name) TEST_F (Case, DISABLED_##Name)
#endif

#define TEST_SET_MAGIC(h, m)     \
  do {                           \
    if (h) {                     \
      *((uint32_t *) (h)) = (m); \
    }                            \
  } while (0);

/**
 * @brief Internal structure for test.
 */
typedef struct {
  gint received;
  gboolean is_pipeline;
} extension_test_data_s;

/**
 * @brief Test base class for database of ML Service API.
 */
class MLServiceExtensionTest : public ::testing::Test
{
  protected:
  static GTestDBus *dbus;

  /**
   * @brief Setup method for entire testsuite.
   */
  static void SetUpTestSuite ()
  {
    g_autofree gchar *services_dir
        = g_build_filename (EXEC_PREFIX, "ml-test", "services", NULL);

    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    ASSERT_NE (nullptr, dbus);

    g_test_dbus_add_service_dir (dbus, services_dir);
    g_test_dbus_up (dbus);
  }

  /**
   * @brief Teardown method for entire testsuite.
   */
  static void TearDownTestSuite ()
  {
    g_test_dbus_down (dbus);
    g_object_unref (dbus);
  }
};

/**
 * @brief GTestDbus object to run ml-agent.
 */
GTestDBus *MLServiceExtensionTest::dbus = nullptr;

/**
 * @brief Internal function to create test-data.
 */
static extension_test_data_s *
_create_test_data (gboolean is_pipeline)
{
  extension_test_data_s *tdata;

  tdata = g_try_new0 (extension_test_data_s, 1);
  if (tdata) {
    tdata->received = 0;
    tdata->is_pipeline = is_pipeline;
  }

  return tdata;
}

/**
 * @brief Internal function to free test-data.
 */
static void
_free_test_data (extension_test_data_s *tdata)
{
  g_free (tdata);
}

/**
 * @brief Internal function to get the data file path.
 */
static gchar *
_get_data_path (const gchar *data_name)
{
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Supposed to run test in build directory. */
  if (root_path == NULL)
    root_path = "..";

  gchar *data_file
      = g_build_filename (root_path, "tests", "test_models", "data", data_name, NULL);

  return data_file;
}

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
 * @brief Callback function for scenario test.
 */
static void
_extension_test_add_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  extension_test_data_s *tdata = (extension_test_data_s *) user_data;
  ml_tensors_data_h data = NULL;
  void *_raw = NULL;
  size_t _size = 0;
  int status;

  switch (event) {
    case ML_SERVICE_EVENT_NEW_DATA:
      ASSERT_TRUE (event_data != NULL);

      status = ml_information_get (event_data, "data", &data);
      EXPECT_EQ (status, ML_ERROR_NONE);

      status = ml_tensors_data_get_tensor_data (data, 0U, &_raw, &_size);
      EXPECT_EQ (status, ML_ERROR_NONE);

      /* (input 1.0 + invoke 2.0) */
      EXPECT_EQ (((float *) _raw)[0], 3.0f);

      if (tdata)
        tdata->received++;
      break;
    default:
      break;
  }
}

/**
 * @brief Internal function to run test with ml-service extension handle.
 */
static inline void
_extension_test_add (ml_service_h handle, gboolean is_pipeline)
{
  extension_test_data_s *tdata;
  ml_tensors_info_h info;
  ml_tensors_data_h input;
  int i, status, tried;

  tdata = _create_test_data (is_pipeline);
  ASSERT_TRUE (tdata != NULL);

  status = ml_service_set_event_cb (handle, _extension_test_add_cb, tdata);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Create and push input data. */
  status = ml_service_get_input_information (handle, NULL, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_create (info, &input);

  for (i = 0; i < 5; i++) {
    g_usleep (50000U);

    float tmp_input[] = { 1.0f };

    ml_tensors_data_set_tensor_data (input, 0U, tmp_input, sizeof (float));

    status = ml_service_request (handle, NULL, input);
    EXPECT_EQ (status, ML_ERROR_NONE);
  }

  /* Let the data frames are passed into ml-service extension handle. */
  tried = 0;
  do {
    g_usleep (30000U);
  } while (tdata->received < 3 && tried++ < 10);

  EXPECT_TRUE (tdata->received > 0);

  /* Clear callback before releasing tdata. */
  status = ml_service_set_event_cb (handle, NULL, NULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (input);

  _free_test_data (tdata);
}

/**
 * @brief Callback function for scenario test.
 */
static void
_extension_test_imgclf_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  extension_test_data_s *tdata = (extension_test_data_s *) user_data;
  ml_tensors_data_h data = NULL;
  gchar *name = NULL;
  void *_raw = NULL;
  size_t _size = 0;
  int status;

  switch (event) {
    case ML_SERVICE_EVENT_NEW_DATA:
      ASSERT_TRUE (event_data != NULL);

      if (tdata->is_pipeline) {
        status = ml_information_get (event_data, "name", (void **) (&name));
        EXPECT_EQ (status, ML_ERROR_NONE);
        EXPECT_STREQ (name, "result_clf");
      }

      status = ml_information_get (event_data, "data", &data);
      EXPECT_EQ (status, ML_ERROR_NONE);

      /* The output tensor has type ML_TENSOR_TYPE_UINT8, dimension 1001:1. */
      status = ml_tensors_data_get_tensor_data (data, 0U, &_raw, &_size);
      EXPECT_EQ (status, ML_ERROR_NONE);
      EXPECT_EQ (_size, 1001);

      /* Check max score, label 'orange' (index 951). */
      if (_raw != NULL && _size > 0) {
        size_t i, max_idx = 0;
        uint8_t cur, max_value = 0;

        for (i = 0; i < _size; i++) {
          cur = ((uint8_t *) _raw)[i];

          if (max_value < cur) {
            max_idx = i;
            max_value = cur;
          }
        }

        EXPECT_EQ (max_idx, 951);
      }

      if (tdata)
        tdata->received++;
      break;
    default:
      break;
  }
}

/**
 * @brief Internal function to run test with ml-service extension handle.
 */
static inline void
_extension_test_imgclf (ml_service_h handle, gboolean is_pipeline)
{
  extension_test_data_s *tdata;
  ml_tensors_info_h in_info = NULL;
  ml_tensors_info_h out_info = NULL;
  ml_tensors_data_h input = NULL;
  unsigned int count;
  ml_tensor_type_e type;
  ml_tensor_dimension in_dim = { 0 };
  ml_tensor_dimension out_dim = { 0 };
  int i, status, tried;
  void *_raw = NULL;
  size_t _size = 0;
  guint64 start_time, end_time;
  gdouble elapsed_time;

  g_autofree gchar *data_file = _get_data_path ("orange.raw");

  ASSERT_TRUE (g_file_get_contents (data_file, (gchar **) &_raw, &_size, NULL));
  ASSERT_TRUE (_size == 3U * 224 * 224);

  tdata = _create_test_data (is_pipeline);
  ASSERT_TRUE (tdata != NULL);

  status = ml_service_set_event_cb (handle, _extension_test_imgclf_cb, tdata);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Check input information. */
  status = ml_service_get_input_information (handle, "input_img", &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  count = 0U;
  status = ml_tensors_info_get_count (in_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  type = ML_TENSOR_TYPE_UNKNOWN;
  status = ml_tensors_info_get_tensor_type (in_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  status = ml_tensors_info_get_tensor_dimension (in_info, 0, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (in_dim[0], 3U);
  EXPECT_EQ (in_dim[1], 224U);
  EXPECT_EQ (in_dim[2], 224U);
  EXPECT_EQ (in_dim[3], 1U);
  EXPECT_LE (in_dim[4], 1U);

  /* Check output information. */
  status = ml_service_get_output_information (handle, "result_clf", &out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  count = 0U;
  status = ml_tensors_info_get_count (out_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  type = ML_TENSOR_TYPE_UNKNOWN;
  status = ml_tensors_info_get_tensor_type (out_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  status = ml_tensors_info_get_tensor_dimension (out_info, 0, out_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (out_dim[0], 1001U);
  EXPECT_EQ (out_dim[1], 1U);
  EXPECT_LE (out_dim[2], 1U);

  /* Create and push input data (orange). */
  ml_tensors_data_create (in_info, &input);
  ml_tensors_data_set_tensor_data (input, 0U, _raw, _size);

  for (i = 0; i < 5; i++) {
    g_usleep (50000U);

    status = ml_service_request (handle, "input_img", input);
    EXPECT_EQ (status, ML_ERROR_NONE);
  }

  /* Let the data frames are passed into ml-service extension handle. */
  tried = 0;
  start_time = g_get_monotonic_time ();
  do {
    g_usleep (300000U);
  } while (tdata->received < 3 && tried++ < 10);
  end_time = g_get_monotonic_time ();
  elapsed_time = (end_time - start_time) / (double) G_USEC_PER_SEC;
  _ml_logd ("[DEBUG] Data received cnt: %u, Elapsed time: %.6f second\n",
      tdata->received, elapsed_time);
  EXPECT_TRUE (tdata->received > 0);

  /* Clear callback before releasing tdata. */
  status = ml_service_set_event_cb (handle, NULL, NULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  if (in_info)
    ml_tensors_info_destroy (in_info);
  if (out_info)
    ml_tensors_info_destroy (out_info);
  if (input)
    ml_tensors_data_destroy (input);
  g_free (_raw);

  _free_test_data (tdata);
}

#if defined(ENABLE_LLAMACPP)
/**
 * @brief Macro to skip testcase if model file is not ready.
 */
#define skip_llm_tc(tc_name, model_name)                                                           \
  do {                                                                                             \
    g_autofree gchar *model_file = _get_model_path (model_name);                                   \
    if (!g_file_test (model_file, G_FILE_TEST_EXISTS)) {                                           \
      g_autofree gchar *msg = g_strdup_printf (                                                    \
          "Skipping '%s' due to missing model file. "                                              \
          "Please download model file from https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF.", \
          tc_name);                                                                                \
      GTEST_SKIP () << msg;                                                                        \
    }                                                                                              \
  } while (0)


/**
 * @brief Callback function for scenario test.
 */
static void
_extension_test_llm_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  extension_test_data_s *tdata = (extension_test_data_s *) user_data;
  ml_tensors_data_h data = NULL;
  void *_raw = NULL;
  size_t _size = 0;
  int status;

  switch (event) {
    case ML_SERVICE_EVENT_NEW_DATA:
      ASSERT_TRUE (event_data != NULL);

      status = ml_information_get (event_data, "data", &data);
      EXPECT_EQ (status, ML_ERROR_NONE);

      status = ml_tensors_data_get_tensor_data (data, 0U, &_raw, &_size);
      EXPECT_EQ (status, ML_ERROR_NONE);

      std::cout.write (static_cast<const char *> (_raw), _size); /* korean output */
      std::cout.flush ();

      if (tdata)
        tdata->received++;
      break;
    default:
      break;
  }
}

/**
 * @brief Internal function to run test with ml-service extension handle.
 */
static inline void
_extension_test_llm (const gchar *config, gchar *input_file, guint sleep_us, gboolean is_pipeline)
{
  extension_test_data_s *tdata;
  ml_service_h handle;
  ml_tensors_info_h info;
  ml_tensors_data_h input;
  int status;
  gsize len = 0;
  g_autofree gchar *contents = NULL;

  if (input_file != NULL) {

    g_autofree gchar *data_file = _get_data_path (input_file);
    ASSERT_TRUE (g_file_test (data_file, G_FILE_TEST_EXISTS));
    ASSERT_TRUE (g_file_get_contents (data_file, &contents, &len, NULL));
  } else {
    contents = g_strdup ("Hello my name is");
    len = strlen (contents);
  }

  tdata = _create_test_data (is_pipeline);
  ASSERT_TRUE (tdata != NULL);

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_event_cb (handle, _extension_test_llm_cb, tdata);

  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Create and push input data. */
  status = ml_service_get_input_information (handle, NULL, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_create (info, &input);

  ml_tensors_data_set_tensor_data (input, 0U, contents, len);

  status = ml_service_request (handle, NULL, input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (sleep_us);
  EXPECT_GT (tdata->received, 0);

  /* Clear callback before releasing tdata. */
  status = ml_service_set_event_cb (handle, NULL, NULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (input);

  _free_test_data (tdata);
}

/**
 * @brief Usage of ml-service extension API.
 */
TEST (MLServiceExtension, scenarioConfigLlamacpp)
{
  skip_llm_tc ("scenarioConfigLlamacpp", "llama-2-7b-chat.Q2_K.gguf");

  g_autofree gchar *config = get_config_path ("config_single_llamacpp.conf");

  _extension_test_llm (config, NULL, 5000000U, FALSE);
}

/**
 * @brief Usage of ml-service extension API.
 */
TEST (MLServiceExtension, scenarioConfigLlamacppAsync)
{
  skip_llm_tc ("scenarioConfigLlamacppAsync", "llama-2-7b-chat.Q2_K.gguf");

  g_autofree gchar *config = get_config_path ("config_single_llamacpp_async.conf");

  _extension_test_llm (config, NULL, 5000000U, FALSE);
}

/**
 * @brief Usage of ml-service extension API.
 *
 * Note: For test, copy modelfile to current dir
 * There are some commonly used functions, so Flare is temporarily put into ENABLE_LLAMACPP.
 */
TEST (MLServiceExtension, scenarioConfigFlare)
{
  g_autofree gchar *input_file = g_strdup ("flare_input.txt");

  skip_llm_tc ("scenarioConfigFlare", "sflare_if_4bit_3b.bin");

  g_autofree gchar *config = get_config_path ("config_single_flare.conf");

  _extension_test_llm (config, input_file, 40000000U, FALSE);
}
#endif /* ENABLE_LLAMACPP */


/**
 * @brief Usage of ml-service extension API.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, scenarioConfigAdd)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _extension_test_add (handle, FALSE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Usage of ml-service extension API.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, scenarioConfig1ImgClf)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _extension_test_imgclf (handle, FALSE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Usage of ml-service extension API.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, scenarioConfig2ImgClf)
{
  ml_service_h handle;
  int status;

  /* The configuration file includes model path only. */
  g_autofree gchar *config = get_config_path ("config_single_imgclf_file.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _extension_test_imgclf (handle, FALSE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Usage of ml-service extension API.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, scenarioConfig3ImgClf)
{
  ml_service_h handle;
  int status;

  /* The configuration file includes model path only. */
  g_autofree gchar *config = get_config_path ("config_pipeline_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _extension_test_imgclf (handle, TRUE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Usage of ml-service extension API.
 */
TEST_F_REQUIRE_TFLITE (MLServiceExtensionTest, scenarioConfig4ImgClf)
{
  ml_service_h handle;
  int status;

  const char test_name[] = "test-single-imgclf";
  unsigned int version = 0U;
  g_autofree gchar *config = get_config_path ("config_single_imgclf_key.conf");
  g_autofree gchar *model = _get_model_path ("mobilenet_v1_1.0_224_quant.tflite");

  /* Register test model. */
  ml_service_model_delete (test_name, 0U);
  ml_service_model_register (test_name, model, true, NULL, &version);

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _extension_test_imgclf (handle, FALSE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Clear test model. */
  ml_service_model_delete (test_name, 0U);
}

/**
 * @brief Usage of ml-service extension API.
 */
TEST_F_REQUIRE_TFLITE (MLServiceExtensionTest, scenarioConfig5ImgClf)
{
  ml_service_h handle;
  int status;

  const char test_name[] = "test-pipeline-imgclf";
  g_autofree gchar *config = get_config_path ("config_pipeline_imgclf_key.conf");
  g_autofree gchar *model = _get_model_path ("mobilenet_v1_1.0_224_quant.tflite");
  g_autofree gchar *pipeline = g_strdup_printf (
      "appsrc name=input_img "
      "caps=other/tensors,num_tensors=1,format=static,types=uint8,dimensions=3:224:224:1,framerate=0/1 ! "
      "tensor_filter framework=tensorflow-lite model=%s ! tensor_sink name=result_clf",
      model);

  /* Register test pipeline. */
  ml_service_pipeline_delete (test_name);
  ml_service_pipeline_set (test_name, pipeline);

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _extension_test_imgclf (handle, TRUE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Clear test pipeline. */
  ml_service_pipeline_delete (test_name);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam01_n)
{
  ml_service_h handle;
  int status;

  status = ml_service_new (NULL, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam02_n)
{
  ml_service_h handle;
  int status;

  status = ml_service_new ("", &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, createConfigInvalidParam03_n)
{
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam04_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file does not exist. */
  g_autofree gchar *config = get_config_path ("invalid_path.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam05_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file has invalid tensor information. */
  g_autofree gchar *config = get_config_path ("config_single_imgclf_invalid_info.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam06_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file has invalid type. */
  g_autofree gchar *config = get_config_path ("config_unknown_type.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam07_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file does not have model file. */
  g_autofree gchar *config = get_config_path ("config_single_no_model.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam08_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file has invalid information. */
  g_autofree gchar *config = get_config_path ("config_pipeline_invalid_info.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam09_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file does not have node information. */
  g_autofree gchar *config = get_config_path ("config_pipeline_no_info.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam10_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file has duplicated node name. */
  g_autofree gchar *config = get_config_path ("config_pipeline_duplicated_name.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, createConfigInvalidParam11_n)
{
  ml_service_h handle;
  int status;

  /* The configuration file does not have node name. */
  g_autofree gchar *config = get_config_path ("config_pipeline_no_name.conf");

  status = ml_service_new (config, &handle);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, destroyInvalidParam01_n)
{
  int status;

  status = ml_service_destroy (NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, destroyInvalidParam02_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_destroy (handle);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, setCallbackInvalidParam01_n)
{
  int status;

  status = ml_service_set_event_cb (NULL, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, setCallbackInvalidParam02_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_set_event_cb (handle, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_set_event_cb (handle, NULL, NULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, startInvalidParam01_n)
{
  int status;

  status = ml_service_start (NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, startInvalidParam02_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_pipeline_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_start (handle);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, stopInvalidParam01_n)
{
  int status;

  status = ml_service_stop (NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, stopInvalidParam02_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_pipeline_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_stop (handle);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, getInputInfoInvalidParam01_n)
{
  ml_tensors_info_h info;
  int status;

  status = ml_service_get_input_information (NULL, NULL, &info);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getInputInfoInvalidParam02_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_input_information (handle, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getInputInfoInvalidParam03_n)
{
  ml_service_h handle;
  ml_tensors_info_h info;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_get_input_information (handle, NULL, &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_get_input_information (handle, NULL, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  unsigned int count = 0U;
  status = ml_tensors_info_get_count (info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  status = ml_tensors_info_get_tensor_type (info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensor_dimension dimension = { 0 };
  status = ml_tensors_info_get_tensor_dimension (info, 0, dimension);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (dimension[0], 1U);
  EXPECT_LE (dimension[1], 1U);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getInputInfoInvalidParam04_n)
{
  ml_service_h handle;
  ml_tensors_info_h info;
  int status;

  g_autofree gchar *config = get_config_path ("config_pipeline_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_input_information (handle, NULL, &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_input_information (handle, "", &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_input_information (handle, "invalid_name", &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_input_information (handle, "result_clf", &info);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, getOutputInfoInvalidParam01_n)
{
  ml_tensors_info_h info;
  int status;

  status = ml_service_get_output_information (NULL, NULL, &info);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getOutputInfoInvalidParam02_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_output_information (handle, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getOutputInfoInvalidParam03_n)
{
  ml_service_h handle;
  ml_tensors_info_h info;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_get_output_information (handle, NULL, &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_get_output_information (handle, NULL, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  unsigned int count = 0U;
  status = ml_tensors_info_get_count (info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  status = ml_tensors_info_get_tensor_type (info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensor_dimension dimension = { 0 };
  status = ml_tensors_info_get_tensor_dimension (info, 0, dimension);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (dimension[0], 1U);
  EXPECT_LE (dimension[1], 1U);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getOutputInfoInvalidParam04_n)
{
  ml_service_h handle;
  ml_tensors_info_h info;
  int status;

  g_autofree gchar *config = get_config_path ("config_pipeline_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_output_information (handle, NULL, &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_output_information (handle, "", &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_output_information (handle, "invalid_name", &info);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_output_information (handle, "input_img", &info);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, setInfoInvalidParam01_n)
{
  int status;

  status = ml_service_set_information (NULL, "test-threshold", "0.1");
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, setInfoInvalidParam02_n)
{
  ml_service_h handle;
  char *value;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_set_information (handle, "test-threshold", "0.1");
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_set_information (handle, "test-threshold", "0.1");
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_service_get_information (handle, "test-threshold", &value);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (value, "0.1");
  g_free (value);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, setInfoInvalidParam03_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_information (handle, NULL, "0.1");
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_set_information (handle, "", "0.1");
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_set_information (handle, "test-threshold", NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_set_information (handle, "test-threshold", "");
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, getInfoInvalidParam01_n)
{
  char *value;
  int status;

  status = ml_service_get_information (NULL, "threshold", &value);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getInfoInvalidParam02_n)
{
  ml_service_h handle;
  char *value;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_get_information (handle, "threshold", &value);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_get_information (handle, "threshold", &value);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (value, "0.5");
  g_free (value);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getInfoInvalidParam03_n)
{
  ml_service_h handle;
  char *value;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_information (handle, NULL, &value);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_information (handle, "", &value);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_get_information (handle, "invalid_name", &value);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, getInfoInvalidParam04_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_information (handle, "threshold", NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST (MLServiceExtension, requestInvalidParam01_n)
{
  ml_tensors_info_h info;
  ml_tensors_data_h input;
  ml_tensor_dimension dimension = { 0 };
  int status;

  dimension[0] = 4U;
  ml_tensors_info_create (&info);
  ml_tensors_info_set_count (info, 1U);
  ml_tensors_info_set_tensor_type (info, 0U, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (info, 0U, dimension);
  ml_tensors_data_create (info, &input);

  status = ml_service_request (NULL, NULL, input);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (input);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, requestInvalidParam02_n)
{
  ml_service_h handle;
  ml_tensors_info_h info;
  ml_tensors_data_h input;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_input_information (handle, NULL, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_create (info, &input);

  /* Set invalid magic. */
  TEST_SET_MAGIC (handle, 0U);
  status = ml_service_request (handle, NULL, input);
  EXPECT_NE (status, ML_ERROR_NONE);
  TEST_SET_MAGIC (handle, 0xfeeedeed);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (input);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, requestInvalidParam03_n)
{
  ml_service_h handle;
  int status;

  g_autofree gchar *config = get_config_path ("config_single_add.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_request (handle, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Testcase with invalid param.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, requestInvalidParam04_n)
{
  ml_service_h handle;
  ml_tensors_info_h info;
  ml_tensors_data_h input;
  int status;

  g_autofree gchar *config = get_config_path ("config_pipeline_imgclf.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_input_information (handle, "input_img", &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_create (info, &input);

  status = ml_service_request (handle, NULL, input);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_request (handle, "", input);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_request (handle, "invalid_name", input);
  EXPECT_NE (status, ML_ERROR_NONE);
  status = ml_service_request (handle, "result_clf", input);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (input);
}

/**
 * @brief Testcase with max buffer.
 */
TEST_REQUIRE_TFLITE (MLServiceExtension, requestMaxBuffer_n)
{
  ml_service_h handle;
  ml_tensors_info_h info;
  ml_tensors_data_h input;
  char *value;
  int i, status;

  g_autofree gchar *config = get_config_path ("config_single_imgclf_max_input.conf");

  status = ml_service_new (config, &handle);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_get_information (handle, "max_input", &value);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (value, "5");
  g_free (value);

  ml_service_get_input_information (handle, NULL, &info);
  ml_tensors_data_create (info, &input);

  for (i = 0; i < 200; i++) {
    g_usleep (20000U);

    status = ml_service_request (handle, NULL, input);
    if (status != ML_ERROR_NONE) {
      /* Supposed max input data in queue. */
      break;
    }
  }

  EXPECT_EQ (status, ML_ERROR_STREAMS_PIPE);

  status = ml_service_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (input);
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
