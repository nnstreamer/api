/**
 * @file        unittest_capi_service_offloading.cc
 * @date        26 Jun 2023
 * @brief       Unit test for ML Service C-API offloading service.
 * @see         https://github.com/nnstreamer/api
 * @author      Gichan Jang <gichan2.jang@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <glib/gstdio.h>
#include <ml-api-inference-pipeline-internal.h>
#include <ml-api-internal.h>
#include <ml-api-service-private.h>
#include <ml-api-service.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "ml-api-service-offloading.h"
#include "nnstreamer.h"
#include "unittest_util.h"

/**
 * @brief Structure for ml-service event callback.
 */
typedef struct {
  ml_service_h handle;
  void *data;
  gboolean received_reply;
} _ml_service_test_data_s;

/**
 * @brief Test base class for Database of ML Service API.
 */
class MLOffloadingService : public ::testing::Test
{
  protected:
  static GTestDBus *dbus;
  ml_service_h client_h;
  ml_service_h server_h;
  _ml_service_test_data_s test_data;

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

  /**
   * @brief Setup method for each test case.
   */
  void SetUp () override
  {
    guint avail_port = get_available_port ();
    g_autofree gchar *receiver_config
        = prepare_test_config ("service_offloading_receiver.conf", avail_port);
    g_autofree gchar *sender_config
        = prepare_test_config ("service_offloading_sender.conf", avail_port);

    int status = ml_service_new (receiver_config, &server_h);
    ASSERT_EQ (status, ML_ERROR_NONE);
    test_data.handle = server_h;

    status = ml_service_new (sender_config, &client_h);
    ASSERT_EQ (status, ML_ERROR_NONE);

    ASSERT_EQ (g_remove (receiver_config), 0);
    ASSERT_EQ (g_remove (sender_config), 0);
  }

  /**
   * @brief Teardown method for each test case.
   */
  void TearDown ()
  {
    int status = ml_service_destroy (server_h);
    EXPECT_EQ (ML_ERROR_NONE, status);
    status = ml_service_destroy (client_h);
    EXPECT_EQ (ML_ERROR_NONE, status);
  }
};

/**
 * @brief GTestDbus object to run ml-agent.
 */
GTestDBus *MLOffloadingService::dbus = nullptr;

/**
 * @brief Callback function for scenario test.
 */
static void
_ml_service_event_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  int status;
  _ml_service_test_data_s *test_data = (_ml_service_test_data_s *) user_data;

  /** @todo remove typecast to int after new event type is added. */
  switch ((int) event) {
    case ML_SERVICE_EVENT_PIPELINE_REGISTERED:
      {
        g_autofree gchar *ret_pipeline = NULL;
        const gchar *service_key = "pipeline_registration_test_key";
        status = ml_service_pipeline_get (service_key, &ret_pipeline);
        EXPECT_EQ (ML_ERROR_NONE, status);
        EXPECT_STREQ ((gchar *) test_data->data, ret_pipeline);
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
        EXPECT_EQ (memcmp ((gchar *) test_data->data, activated_model_contents, activated_model_len),
            0);

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
 * @brief Create tensor data from the input string.
 */
static int
_create_tensor_data_from_str (const gchar *raw_data, const gsize len, ml_tensors_data_h *data_h)
{
  int status;
  ml_tensor_dimension in_dim = { 0 };
  ml_tensors_info_h in_info = NULL;

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = len;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  status = ml_tensors_data_create (in_info, data_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_set_tensor_data (*data_h, 0, raw_data, len);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (ML_ERROR_NONE, status);

  return status;
}

/**
 * @brief use case of pipeline registration using ml offloading service using conf file.
 */
TEST_F (MLOffloadingService, registerPipeline)
{
  int status;
  ml_tensors_data_h input = NULL;

  gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");
  test_data.data = pipeline_desc;
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = _create_tensor_data_from_str (pipeline_desc, strlen (pipeline_desc) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "pipeline_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_pipeline_delete ("pipeline_registration_test_key");
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_free (pipeline_desc);
}

/**
 * @brief use case of pipeline registration using ml offloading service using conf file.
 */
TEST_F (MLOffloadingService, registerPipelineURI)
{
  int status;
  ml_tensors_data_h input = NULL;

  g_autofree gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");
  test_data.data = pipeline_desc;
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  gchar *current_dir = g_get_current_dir ();
  g_autofree gchar *test_file_path
      = g_build_path (G_DIR_SEPARATOR_S, current_dir, "test.pipeline", NULL);

  EXPECT_TRUE (g_file_set_contents (
      test_file_path, pipeline_desc, strlen (pipeline_desc) + 1, NULL));

  g_autofree gchar *pipeline_uri = g_strdup_printf ("file://%s", test_file_path);

  status = _create_tensor_data_from_str (pipeline_uri, strlen (pipeline_uri) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "pipeline_registration_uri", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_pipeline_delete ("pipeline_registration_test_key");
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_offloading_create with invalid param.
 */
TEST_F (MLOffloadingService, createInvalidParam_n)
{
  int status;
  status = ml_service_offloading_create (NULL, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_create (server_h, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_offloading_request with invalid param.
 */
TEST_F (MLOffloadingService, registerInvalidParam01_n)
{
  int status;
  ml_tensors_data_h input = NULL;

  g_autofree gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");

  status = _create_tensor_data_from_str (pipeline_desc, strlen (pipeline_desc) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_offloading_request (NULL, "pipeline_registration_raw", input);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_request (client_h, NULL, input);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_request (client_h, "pipeline_registration_raw", NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_offloading_request_raw with invalid param.
 */
TEST_F (MLOffloadingService, registerInvalidParam02_n)
{
  int status;

  g_autofree gchar *data = g_strdup ("fakesrc ! fakesink");
  gsize len = strlen (data);

  status = ml_service_offloading_request_raw (NULL, "req_raw", data, len);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_request_raw (client_h, NULL, data, len);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_request_raw (client_h, "req_raw", NULL, len);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_offloading_request_raw (client_h, "req_raw", data, 0);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief use case of model registration using ml offloading service.
 */
TEST_F (MLOffloadingService, registerModel)
{
  int status;
  ml_tensors_data_h input = NULL;

  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  /* ml_service_offloading_request () requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  g_autofree gchar *model_dir
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);
  g_autofree gchar *test_model
      = g_build_filename (model_dir, "mobilenet_v1_1.0_224_quant.tflite", NULL);

  g_autofree gchar *contents = NULL;
  gsize len = 0;
  EXPECT_TRUE (g_file_get_contents (test_model, &contents, &len, NULL));

  test_data.data = contents;
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_offloading_set_information (server_h, "path", model_dir);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = _create_tensor_data_from_str (contents, len, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "model_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_model_delete ("model_registration_test_key", 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of model registration from URI using ml offloading service.
 */
TEST_F (MLOffloadingService, registerModelURI)
{
  int status;
  ml_tensors_data_h input = NULL;
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

  test_data.data = contents;
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_autofree gchar *model_uri = g_strdup_printf ("file://%s", test_model_path);

  status = _create_tensor_data_from_str (model_uri, strlen (model_uri) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "model_registration_uri", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_model_delete ("model_registration_test_key", 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of model registration using ml offloading service.
 */
TEST_F (MLOffloadingService, registerModelPath)
{
  int status;
  ml_tensors_data_h input = NULL;
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

  test_data.data = contents;
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = _create_tensor_data_from_str (contents, len, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, "model_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_model_delete ("model_registration_test_key", 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of pipeline registration using ml offloading service using conf file.
 */
TEST_F (MLOffloadingService, requestInvalidParam_n)
{
  int status;
  ml_tensors_data_h input = NULL;

  g_autofree gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");
  status = ml_service_set_event_cb (server_h, _ml_service_event_cb, pipeline_desc);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = _create_tensor_data_from_str (pipeline_desc, strlen (pipeline_desc) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_request (client_h, NULL, input);
  EXPECT_NE (ML_ERROR_NONE, status);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
}


/**
 * @brief Callback function for reply test.
 */
static void
_ml_service_reply_test_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  int status;

  switch ((int) event) {
    case ML_SERVICE_EVENT_PIPELINE_REGISTERED:
      {
        _ml_service_test_data_s *test_data = (_ml_service_test_data_s *) user_data;
        g_autofree gchar *ret_pipeline = NULL;
        void *_data;
        size_t _size;
        const gchar *service_key = "pipeline_registration_test_key";
        status = ml_service_pipeline_get (service_key, &ret_pipeline);
        EXPECT_EQ (ML_ERROR_NONE, status);
        status = ml_tensors_data_get_tensor_data (test_data->data, 0, &_data, &_size);
        EXPECT_EQ (ML_ERROR_NONE, status);
        EXPECT_STREQ ((gchar *) _data, ret_pipeline);

        ml_service_request (test_data->handle, "reply_to_client", test_data->data);
        break;
      }
    case ML_SERVICE_EVENT_REPLY:
      {
        gint *received = (gint *) user_data;
        (*received)++;
        break;
      }
    default:
      break;
  }
}

/**
 * @brief use case of replying to client.
 */
TEST_F (MLOffloadingService, replyToClient)
{
  int status;
  ml_tensors_data_h input = NULL;
  gint received = 0;

  gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");

  status = _create_tensor_data_from_str (pipeline_desc, strlen (pipeline_desc) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  test_data.data = input;
  status = ml_service_set_event_cb (server_h, _ml_service_reply_test_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_event_cb (client_h, _ml_service_reply_test_cb, &received);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_request (client_h, "pipeline_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  EXPECT_GT (received, 0);

  status = ml_service_pipeline_delete ("pipeline_registration_test_key");
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_free (pipeline_desc);
}


/**
 * @brief A tensor-sink callback for sink handle in a pipeline
 */
static void
test_sink_callback (const ml_tensors_data_h data, const ml_tensors_info_h info, void *user_data)
{
  gint *received = (gint *) user_data;

  (*received)++;
}


/**
 * @brief use case of launching the pipeline on server.
 */
TEST_F (MLOffloadingService, launchPipeline)
{
  int status;
  ml_tensors_data_h input = NULL;
  gint received = 0;

  guint server_port = get_available_port ();
  EXPECT_TRUE (server_port > 0);
  g_autofree gchar *server_pipeline_desc = g_strdup_printf (
      "tensor_query_serversrc port=%u ! other/tensors,num_tensors=1,dimensions=3:4:4:1,types=uint8,format=static,framerate=0/1 ! tensor_query_serversink async=false sync=false",
      server_port);

  status = _create_tensor_data_from_str (
      server_pipeline_desc, strlen (server_pipeline_desc) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  test_data.data = input;
  status = ml_service_set_event_cb (server_h, _ml_service_reply_test_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_event_cb (client_h, _ml_service_reply_test_cb, &received);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_request (client_h, "pipeline_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_request (client_h, "pipeline_launch_test", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1000000);

  /* create client pipeline */
  g_autofree gchar *client_pipeline_desc = g_strdup_printf (
      "videotestsrc num-buffers=100 ! videoconvert ! videoscale ! video/x-raw,width=4,height=4,format=RGB,framerate=60/1 ! tensor_converter ! other/tensors,num_tensors=1,format=static ! tensor_query_client dest-port=%u port=0 ! other/tensors,num_tensors=1,dimensions=3:4:4:1,types=uint8,format=static,framerate=0/1 ! tensor_sink sync=true name=sinkx",
      server_port);

  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle;
  status = ml_pipeline_construct (client_pipeline_desc, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  gint sink_received = 0;
  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback, &sink_received, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle != NULL);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = waitPipelineStateChange (handle, ML_PIPELINE_STATE_PLAYING, 200);
  EXPECT_EQ (status, ML_ERROR_NONE);

  guint tried = 0;
  do {
    g_usleep (500000U);
  } while (sink_received < 1 && tried++ < 10);

  EXPECT_GT (received, 0);
  EXPECT_GT (sink_received, 0);

  status = ml_service_pipeline_delete ("pipeline_registration_test_key");
  EXPECT_TRUE (status == ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief use case of launching the pipeline on server.
 */
TEST_F (MLOffloadingService, launchPipeline2)
{
  int status;
  ml_tensors_data_h input = NULL;
  gint received = 0;

  guint server_port = get_available_port ();
  EXPECT_TRUE (server_port > 0);
  g_autofree gchar *server_pipeline_desc = g_strdup_printf (
      "tensor_query_serversrc port=%u ! other/tensors,num_tensors=1,dimensions=3:4:4:1,types=uint8,format=static,framerate=0/1 ! tensor_query_serversink async=false sync=false",
      server_port);

  status = _create_tensor_data_from_str (
      server_pipeline_desc, strlen (server_pipeline_desc) + 1, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  test_data.data = input;
  status = ml_service_set_event_cb (server_h, _ml_service_reply_test_cb, &test_data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_event_cb (client_h, _ml_service_reply_test_cb, &received);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_request (client_h, "pipeline_registration_raw", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* Wait for the server to register and check the result. */
  g_usleep (1000000);

  status = ml_service_request (client_h, "pipeline_launch_test", input);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1000000);

  ml_option_h query_client_option = NULL;

  status = ml_option_create (&query_client_option);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint client_port = 0;

  status = ml_option_set (query_client_option, "port", &client_port, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint dest_port = server_port;
  status = ml_option_set (query_client_option, "dest-port", &dest_port, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint timeout = 200000U;
  status = ml_option_set (query_client_option, "timeout", &timeout, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_autofree gchar *caps_str = g_strdup (
      "other/tensors,num_tensors=1,format=static,types=uint8,dimensions=3:4:4:1,framerate=0/1");
  status = ml_option_set (query_client_option, "caps", caps_str, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* set input tensor */
  ml_tensors_info_h in_info;
  ml_tensor_dimension in_dim;
  ml_tensors_data_h query_input;

  ml_tensors_info_create (&in_info);
  in_dim[0] = 3;
  in_dim[1] = 4;
  in_dim[2] = 4;
  in_dim[3] = 1;

  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  ml_service_h query_h;
  status = ml_service_query_create (query_client_option, &query_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_tensors_data_create (in_info, &query_input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_TRUE (NULL != query_input);

  int num_buffers = 5;
  /* request output tensor with input tensor */
  for (int i = 0; i < num_buffers; ++i) {
    ml_tensors_data_h output;
    uint8_t *cnt;
    size_t input_data_size, output_data_size;
    uint8_t test_data = (uint8_t) i;

    ml_tensors_data_set_tensor_data (query_input, 0, &test_data, sizeof (uint8_t));

    status = ml_service_query_request (query_h, query_input, &output);
    EXPECT_EQ (ML_ERROR_NONE, status);
    EXPECT_TRUE (NULL != output);

    g_usleep (1000000);

    status = ml_tensors_info_get_tensor_size (in_info, 0, &input_data_size);
    EXPECT_EQ (ML_ERROR_NONE, status);

    status = ml_tensors_data_get_tensor_data (output, 0, (void **) &cnt, &output_data_size);
    EXPECT_EQ (ML_ERROR_NONE, status);
    EXPECT_EQ (input_data_size, output_data_size);
    EXPECT_EQ (test_data, cnt[0]);

    status = ml_tensors_data_destroy (output);
    EXPECT_EQ (ML_ERROR_NONE, status);
  }

  EXPECT_GT (received, 0);

  status = ml_service_pipeline_delete ("pipeline_registration_test_key");
  EXPECT_TRUE (status == ML_ERROR_NONE);


  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_tensors_data_destroy (query_input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_destroy (query_h);
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
