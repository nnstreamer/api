/**
 * @file        unittest_capi_service_offloading.cc
 * @date        26 Jun 2023
 * @brief       Unit test for ML Service C-API offloading service.
 * @see         https://github.com/nnstreamer/api
 * @author      Gichan Jang <gichan2.jang@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <ml-api-inference-pipeline-internal.h>
#include <ml-api-internal.h>
#include <ml-api-service-private.h>
#include <ml-api-service.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "ml-api-service-offloading.h"

/**
 * @brief Internal function to get the config file path.
 */
static gchar *
_get_config_path (const gchar *config_name)
{
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Supposed to run test in build directory. */
  if (root_path == NULL)
    root_path = "..";

  gchar *config_file = g_build_filename (
      root_path, "tests", "test_models", "config", config_name, NULL);

  return config_file;
}

/**
 * @brief Test base class for Database of ML Service API.
 */
class MLOffloadingService : public ::testing::Test
{
  protected:
  GTestDBus *dbus;
  int status;
  ml_service_h client_h;
  ml_service_h server_h;

  public:
  /**
   * @brief Setup method for each test case.
   */
  void SetUp () override
  {
    g_autofree gchar *services_dir
        = g_build_filename (EXEC_PREFIX, "ml-test", "services", NULL);

    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    ASSERT_NE (nullptr, dbus);

    g_test_dbus_add_service_dir (dbus, services_dir);

    g_test_dbus_up (dbus);

    g_autofree gchar *sender_config = _get_config_path ("service_offloading_sender.conf");
    status = ml_service_new (sender_config, &client_h);
    ASSERT_EQ (status, ML_ERROR_NONE);

    g_autofree gchar *receiver_config
        = _get_config_path ("service_offloading_receiver.conf");
    status = ml_service_new (receiver_config, &server_h);
    ASSERT_EQ (status, ML_ERROR_NONE);
  }

  /**
   * @brief Teardown method for each test case.
   */
  void TearDown () override
  {
    if (dbus) {
      g_test_dbus_down (dbus);
      g_object_unref (dbus);
    }

    status = ml_service_destroy (server_h);
    EXPECT_EQ (ML_ERROR_NONE, status);
    status = ml_service_destroy (client_h);
    EXPECT_EQ (ML_ERROR_NONE, status);
  }

  /**
   * @brief Get available port number.
   */
  static guint _get_available_port (void)
  {
    struct sockaddr_in sin;
    guint port = 0;
    gint sock;
    socklen_t len = sizeof (struct sockaddr);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (0);

    sock = socket (AF_INET, SOCK_STREAM, 0);
    EXPECT_TRUE (sock > 0);
    if (sock < 0)
      return 0;

    if (bind (sock, (struct sockaddr *) &sin, sizeof (struct sockaddr)) == 0) {
      if (getsockname (sock, (struct sockaddr *) &sin, &len) == 0) {
        port = ntohs (sin.sin_port);
      }
    }
    close (sock);

    EXPECT_TRUE (port > 0);
    return port;
  }
};

/**
 * @brief Callback function for scenario test.
 */
static void
_ml_service_event_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  int status;

  /** @todo remove typecast to int after new event type is added. */
  switch ((int) event) {
    case ML_SERVICE_EVENT_PIPELINE_REGISTERED:
      {
        g_autofree gchar *ret_pipeline = NULL;
        const gchar *service_key = "pipeline_registration_test_key";
        status = ml_service_pipeline_get (service_key, &ret_pipeline);
        EXPECT_EQ (ML_ERROR_NONE, status);
        EXPECT_STREQ ((gchar *) user_data, ret_pipeline);
        break;
      }
    case ML_SERVICE_EVENT_MODEL_REGISTERED:
      {
        const gchar *service_key = "model_registration_test_key";
        ml_information_h activated_model_info;

        status = ml_service_model_get_activated (service_key, &activated_model_info);
        EXPECT_EQ (ML_ERROR_NONE, status);
        EXPECT_NE (activated_model_info, nullptr);

        gchar *activated_model_path;
        status = ml_information_get (
            activated_model_info, "path", (void **) &activated_model_path);
        EXPECT_EQ (ML_ERROR_NONE, status);

        g_autofree gchar *activated_model_contents = NULL;
        gsize activated_model_len = 0;
        EXPECT_TRUE (g_file_get_contents (activated_model_path,
            &activated_model_contents, &activated_model_len, NULL));
        EXPECT_EQ (memcmp ((gchar *) user_data, activated_model_contents, activated_model_len), 0);

        status = g_remove (activated_model_path);
        EXPECT_TRUE (status == 0);
        status = ml_information_destroy (activated_model_info);
        EXPECT_EQ (ML_ERROR_NONE, status);
        break;
      }
    default:
      break;
  }
}

/**
 * @brief use case of pipeline registration using ml offloading service using conf file.
 */
TEST_F (MLOffloadingService, registerPipeline)
{
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };

  gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, pipeline_desc);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = strlen (pipeline_desc) + 1;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_set_tensor_data (
      input, 0, pipeline_desc, strlen (pipeline_desc) + 1);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "pipeline_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_pipeline_delete ("pipeline_registration_test_key");
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_free (pipeline_desc);
}

/**
 * @brief use case of pipeline registration using ml offloading service using conf file.
 */
TEST_F (MLOffloadingService, registerPipelineURI)
{
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };

  g_autofree gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, pipeline_desc);
  EXPECT_EQ (status, ML_ERROR_NONE);

  gchar *current_dir = g_get_current_dir ();
  g_autofree gchar *test_file_path
      = g_build_path (G_DIR_SEPARATOR_S, current_dir, "test.pipeline", NULL);

  EXPECT_TRUE (g_file_set_contents (
      test_file_path, pipeline_desc, strlen (pipeline_desc) + 1, NULL));

  g_autofree gchar *pipeline_uri = g_strdup_printf ("file://%s", test_file_path);

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = strlen (pipeline_uri) + 1;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_set_tensor_data (
      input, 0, pipeline_uri, strlen (pipeline_uri) + 1);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "pipeline_registration_uri", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_pipeline_delete ("pipeline_registration_test_key");
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_offloading_create with invalid param.
 */
TEST_F (MLOffloadingService, createInvalidParam_n)
{
  int status;
  ml_option_h option_h = NULL;
  ml_service_h service_h = NULL;

  status = ml_option_create (&option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  service_h = _ml_service_create_internal (ML_SERVICE_TYPE_OFFLOADING);

  status = ml_service_offloading_create (NULL, option_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_create (service_h, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_destroy (option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_offloading_request with invalid param.
 */
TEST_F (MLOffloadingService, registerInvalidParam_n)
{
  g_autofree gchar *str = g_strdup ("Temp_test_str");
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };

  g_autofree gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = strlen (pipeline_desc) + 1;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_offloading_request (NULL, "pipeline_registration_raw", input);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_request (client_h, NULL, input);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_request (client_h, "pipeline_registration_raw", NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of model registration using ml offloading service.
 */
TEST_F (MLOffloadingService, registerModel)
{
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };

  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  /* ml_service_offloading_request () requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  g_autofree gchar *test_model = g_build_filename (root_path, "tests",
      "test_models", "models", "mobilenet_v1_1.0_224_quant.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  g_autofree gchar *contents = NULL;
  gsize len = 0;
  EXPECT_TRUE (g_file_get_contents (test_model, &contents, &len, NULL));

  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, contents);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = len;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_set_tensor_data (input, 0, contents, len);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "model_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_model_delete ("model_registration_test_key", 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of model registration from URI using ml offloading service.
 */
TEST_F (MLOffloadingService, registerModelURI)
{
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  /* ml_service_offloading_request () requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  g_autofree gchar *test_model_path = g_build_filename (root_path, "tests",
      "test_models", "models", "mobilenet_v1_1.0_224_quant.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model_path, G_FILE_TEST_EXISTS));

  g_autofree gchar *contents = NULL;
  gsize len = 0;
  EXPECT_TRUE (g_file_get_contents (test_model_path, &contents, &len, NULL));

  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, contents);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_autofree gchar *model_uri = g_strdup_printf ("file://%s", test_model_path);

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = strlen (model_uri) + 1;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_set_tensor_data (input, 0, model_uri, strlen (model_uri) + 1);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "model_registration_uri", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_model_delete ("model_registration_test_key", 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of model registration using ml offloading service.
 */
TEST_F (MLOffloadingService, registerModelPath)
{
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  /* ml_service_offloading_request () requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  g_autofree gchar *model_dir
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);
  EXPECT_TRUE (g_file_test (model_dir, G_FILE_TEST_IS_DIR));

  /** A path to save the received model file */
  status = ml_service_set_information (server_h, "path", model_dir);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_autofree gchar *test_model
      = g_build_filename (model_dir, "mobilenet_v1_1.0_224_quant.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  g_autofree gchar *contents = NULL;
  gsize len = 0;
  EXPECT_TRUE (g_file_get_contents (test_model, &contents, &len, NULL));

  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, contents);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = len;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_set_tensor_data (input, 0, contents, len);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "model_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_model_delete ("model_registration_test_key", 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of pipeline registration using ml offloading service using conf file.
 */
TEST_F (MLOffloadingService, requestInvalidParam_n)
{
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };

  g_autofree gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, pipeline_desc);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = strlen (pipeline_desc) + 1;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_set_tensor_data (
      input, 0, pipeline_desc, strlen (pipeline_desc) + 1);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, NULL, input);
  EXPECT_NE (ML_ERROR_NONE, status);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Main gtest
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

  _ml_initialize_gstreamer ();

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
