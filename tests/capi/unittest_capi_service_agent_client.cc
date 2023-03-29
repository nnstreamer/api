/**
 * @file        unittest_capi_service_agent_client.cc
 * @date        20 Jul 2022
 * @brief       Unit test for ML Service C-API.
 * @see         https://github.com/nnstreamer/api
 * @author      Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug         No known bugs
 */

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <ml-api-internal.h>
#include <ml-api-service.h>
#include <ml-api-service-private.h>
#include <ml-api-inference-pipeline-internal.h>

#include <netinet/tcp.h>
#include <netinet/in.h>

/**
 * @brief Test base class for Database of ML Service API.
 */
class MLServiceAgentTest : public::testing::Test
{
protected:
  GTestDBus *dbus;

public:
  /**
   * @brief Setup method for each test case.
   */
  void SetUp () override
  {
    gchar *current_dir = g_get_current_dir ();
    gchar *services_dir = g_build_filename (current_dir, "tests/services", NULL);
    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    ASSERT_NE (nullptr, dbus);

    g_test_dbus_add_service_dir (dbus, services_dir);
    g_free (current_dir);
    g_free (services_dir);

    g_test_dbus_up (dbus);
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
    sin.sin_port = htons(0);

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
 * @brief use case of using service api and agent
 */
TEST_F (MLServiceAgentTest, usecase_00)
{
  int status;

  const gchar *service_name = "simple_query_server_for_test";
  gchar *pipeline_desc;

  guint port = _get_available_port ();

  /* create server pipeline */
  pipeline_desc = g_strdup_printf ("tensor_query_serversrc port=%u num-buffers=10 ! other/tensors,num_tensors=1,dimensions=3:4:4:1,types=uint8,format=static,framerate=0/1 ! tensor_query_serversink async=false", port);

  status = ml_service_set_pipeline (service_name, pipeline_desc);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *ret_pipeline;
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (pipeline_desc, ret_pipeline);
  g_free (ret_pipeline);

  ml_service_h service;
  ml_pipeline_state_e state;
  status = ml_service_launch_pipeline (service_name, &service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  status = ml_service_start_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PLAYING, state);

  /* create client pipeline */
  guint sink_port = _get_available_port ();
  gchar *client_pipeline_desc = g_strdup_printf ("videotestsrc num-buffers=10 ! videoconvert ! videoscale ! video/x-raw,width=4,height=4,format=RGB,framerate=10/1 ! tensor_converter ! other/tensors,num_tensors=1,format=static ! tensor_query_client dest-port=%u port=%u ! fakesink sync=true", port, sink_port);

  status = ml_service_set_pipeline ("client", client_pipeline_desc);
  EXPECT_EQ (ML_ERROR_NONE, status);

  ml_service_h client;
  status = ml_service_launch_pipeline ("client", &client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_start_pipeline (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_stop_pipeline (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_destroy (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_stop_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  /** destroy the pipeline */
  status = ml_service_destroy (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** delete finished service */
  status = ml_service_delete_pipeline (service_name);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** it would fail if get the removed service */
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  g_free (pipeline_desc);
  g_free (client_pipeline_desc);
}

/**
 * @brief use case of using service api and agent
 */
TEST_F (MLServiceAgentTest, usecase_01)
{
  int status;

  const gchar *service_name = "simple_query_server_for_test";
  gchar *pipeline_desc;

  guint port = _get_available_port ();

  /* create server pipeline */
  pipeline_desc = g_strdup_printf ("tensor_query_serversrc port=%u num-buffers=10 ! other/tensors,num_tensors=1,dimensions=3:4:4:1,types=uint8,format=static,framerate=0/1 ! tensor_query_serversink async=false", port);

  status = ml_service_set_pipeline (service_name, pipeline_desc);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *ret_pipeline;
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (pipeline_desc, ret_pipeline);
  g_free (ret_pipeline);

  ml_service_h service;
  ml_pipeline_state_e state;
  status = ml_service_launch_pipeline (service_name, &service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  status = ml_service_start_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PLAYING, state);

  /* create client pipeline */
  guint sink_port = _get_available_port ();
  gchar *client_pipeline_desc = g_strdup_printf ("videotestsrc num-buffers=10 ! videoconvert ! videoscale ! video/x-raw,width=4,height=4,format=RGB,framerate=10/1 ! tensor_converter ! other/tensors,num_tensors=1,format=static ! tensor_query_client dest-port=%u port=%u ! fakesink sync=true", port, sink_port);

  ml_pipeline_h client;
  status = ml_pipeline_construct (client_pipeline_desc, NULL, NULL, &client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_pipeline_start (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_pipeline_stop (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_pipeline_destroy (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_stop_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  /** destroy the pipeline */
  status = ml_service_destroy (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** delete finished service */
  status = ml_service_delete_pipeline (service_name);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** it would fail if get the removed service */
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  g_free (pipeline_desc);
  g_free (client_pipeline_desc);
}

/**
 * @brief Test ml_service_set_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, set_pipeline_00_n)
{
  int status;
  status = ml_service_set_pipeline (NULL, "some pipeline");
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_set_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, set_pipeline_01_n)
{
  int status;
  status = ml_service_set_pipeline ("some key", NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_get_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, get_pipeline_00_n)
{
  int status;
  gchar *ret_pipeline = NULL;
  status = ml_service_get_pipeline (NULL, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_get_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, get_pipeline_01_n)
{
  int status;
  status = ml_service_get_pipeline ("some key", NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_delete_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, delete_pipeline_00_n)
{
  int status;
  status = ml_service_delete_pipeline (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_delete_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, delete_pipeline_01_n)
{
  int status;
  status = ml_service_set_pipeline ("some key", "videotestsrc ! fakesink");
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_delete_pipeline ("invalid key");
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_delete_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, delete_pipeline_02_n)
{
  int status;
  status = ml_service_set_pipeline ("some key", "videotestsrc ! fakesink");
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_delete_pipeline ("some key");
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_delete_pipeline ("some key");
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_launch_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, launch_pipeline_00_n)
{
  int status;
  status = ml_service_launch_pipeline (NULL, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_launch_pipeline with invalid key.
 */
TEST_F (MLServiceAgentTest, launch_pipeline_01_n)
{
  int status;
  ml_service_h service_h = NULL;
  status = ml_service_launch_pipeline (NULL, &service_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_launch_pipeline ("invalid key", &service_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  /* service_h is still NULL */
  status = ml_service_destroy (service_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_launch_pipeline with invalid pipeline.
 */
TEST_F (MLServiceAgentTest, launch_pipeline_02_n)
{
  int status;
  ml_service_h h;

  status = ml_service_set_pipeline ("key", "invalid_element ! invalid_element");
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_launch_pipeline ("key", &h);
  EXPECT_EQ (ML_ERROR_STREAMS_PIPE, status);
}

/**
 * @brief Test ml_service_start_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, start_pipeline_00_n)
{
  int status;
  status = ml_service_start_pipeline (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_stop_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, stop_pipeline_00_n)
{
  int status;
  status = ml_service_stop_pipeline (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_get_pipeline_state with invalid param.
 */
TEST_F (MLServiceAgentTest, get_pipeline_state_00_n)
{
  int status;
  ml_service_h h;
  ml_pipeline_state_e state;

  status = ml_service_get_pipeline_state (NULL, &state);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_set_pipeline ("key", "videotestsrc ! fakesink");
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_launch_pipeline ("key", &h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_get_pipeline_state (h, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_destroy (h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_delete_pipeline ("key");
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_destroy with invalid param.
 */
TEST_F (MLServiceAgentTest, destroy_00_n)
{
  int status;
  status = ml_service_destroy (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_destroy with explicit invalid param.
 */
TEST_F (MLServiceAgentTest, destroy_01_n)
{
  int status;
  ml_service_h h;

  ml_service_s *mls = g_new0 (ml_service_s, 1);
  _ml_service_server_s *server = g_new0 (_ml_service_server_s, 1);
  mls->priv = server;
  mls->type = ML_SERVICE_TYPE_MAX;

  h = (ml_service_h) mls;
  status = ml_service_destroy (h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  g_free (server);
  g_free (mls);
}

/**
 * @brief Test ml_service APIs with invalid handle.
 */
TEST_F (MLServiceAgentTest, explicit_invalid_handle_00_n)
{
  int status;
  ml_service_h h;

  status = ml_service_set_pipeline ("key", "videotestsrc ! fakesink");
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_launch_pipeline ("key", &h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  ml_service_s *mls = (ml_service_s *) h;
  _ml_service_server_s *server = (_ml_service_server_s *) mls->priv;
  gint64 _id = server->id;
  server->id = -987654321; /* explicitly set id as invalid number */

  status = ml_service_start_pipeline (h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_stop_pipeline (h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  ml_pipeline_state_e state;
  status = ml_service_get_pipeline_state (h, &state);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_destroy (h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  server->id = _id;
  status = ml_service_destroy (h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_delete_pipeline ("key");
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief use case of using service api
 */
TEST_F (MLServiceAgentTest, query_client)
{
  int status;

  /** Set server pipeline and launch it */
  const gchar *service_name = "simple_query_server_for_test";
  int num_buffers = 5;
  guint server_port = _get_available_port ();
  gchar *server_pipeline_desc = g_strdup_printf ("tensor_query_serversrc port=%u num-buffers=%d ! other/tensors,num_tensors=1,dimensions=3:4:4:1,types=uint8,format=static,framerate=0/1 ! tensor_query_serversink async=false sync=false", server_port, num_buffers);

  status = ml_service_set_pipeline (service_name, server_pipeline_desc);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *ret_pipeline;
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (server_pipeline_desc, ret_pipeline);
  g_free (server_pipeline_desc);
  g_free (ret_pipeline);

  ml_service_h service;
  ml_pipeline_state_e state;
  status = ml_service_launch_pipeline (service_name, &service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  status = ml_service_start_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PLAYING, state);

  ml_service_h client;
  ml_option_h query_client_option = NULL;

  status = ml_option_create (&query_client_option);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *host = g_strdup ("localhost");
  status = ml_option_set (query_client_option, "host", host, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint client_port = _get_available_port ();
  status = ml_option_set (query_client_option, "port", &client_port, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *dest_host = g_strdup ("localhost");
  status = ml_option_set (query_client_option, "dest-host", dest_host, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint dest_port = server_port;
  status = ml_option_set (query_client_option, "dest-port", &dest_port, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *connect_type = g_strdup ("TCP");
  status = ml_option_set (query_client_option, "connect-type", connect_type, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint timeout = 10000U;
  status = ml_option_set (query_client_option, "timeout", &timeout, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *caps_str = g_strdup ("other/tensors,num_tensors=1,format=static,types=uint8,dimensions=3:4:4:1,framerate=0/1");
  status = ml_option_set (query_client_option, "caps", caps_str, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* set input tensor */
  ml_tensors_info_h in_info;
  ml_tensor_dimension in_dim;
  ml_tensors_data_h input;

  ml_tensors_info_create (&in_info);
  in_dim[0] = 3;
  in_dim[1] = 4;
  in_dim[2] = 4;
  in_dim[3] = 1;

  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  status = ml_service_query_create (query_client_option, &client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_TRUE (NULL != input);

  /* request output tensor with input tensor */
  for (int i = 0; i < num_buffers; ++i) {
    ml_tensors_data_h output;
    uint8_t *received;
    size_t input_data_size, output_data_size;
    uint8_t test_data = (uint8_t) i;

    ml_tensors_data_set_tensor_data (input, 0, &test_data, sizeof (uint8_t));

    status = ml_service_query_request (client, input, &output);
    EXPECT_EQ (ML_ERROR_NONE, status);
    EXPECT_TRUE (NULL != output);

    status = ml_tensors_info_get_tensor_size (in_info, 0, &input_data_size);
    EXPECT_EQ (ML_ERROR_NONE, status);

    status = ml_tensors_data_get_tensor_data (output, 0, (void **) &received, &output_data_size);
    EXPECT_EQ (ML_ERROR_NONE, status);
    EXPECT_EQ (input_data_size, output_data_size);
    EXPECT_EQ (test_data, received[0]);

    status = ml_tensors_data_destroy (output);
    EXPECT_EQ (ML_ERROR_NONE, status);
  }

  /** destroy client ml_service_h */
  status = ml_service_destroy (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** destroy server pipeline */
  status = ml_service_stop_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  status = ml_service_destroy (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** delete finished service */
  status = ml_service_delete_pipeline (service_name);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** it would fail if get the removed service */
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  ml_option_destroy (query_client_option);
  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);
}

/**
 * @brief Test ml_service_query_create with invalid param.
 */
TEST_F (MLServiceAgentTest, query_create_00_n)
{
  int status;
  ml_option_h option = NULL;

  status = ml_service_query_create (NULL, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_create (&option);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_query_create (option, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_destroy (option);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_query_create with invalid param. caps must be set.
 */
TEST_F (MLServiceAgentTest, query_create_01_n)
{
  int status;
  ml_service_h client = NULL;
  ml_option_h invalid_option = NULL;

  status = ml_option_create (&invalid_option);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_query_create (invalid_option, &client);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_destroy (invalid_option);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_query_create with invalid param.
 */
TEST_F (MLServiceAgentTest, query_create_02_n)
{
  int status;
  ml_service_h client = NULL;
  ml_option_h invalid_option = NULL;

  status = ml_option_create (&invalid_option);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *topic = g_strdup ("sample-topic");
  status = ml_option_set (invalid_option, "topic", topic, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gint some_int = 0;
  status = ml_option_set (invalid_option, "unknown-key", &some_int, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *caps_str = g_strdup ("some invalid caps");
  status = ml_option_set (invalid_option, "caps", caps_str, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_query_create (invalid_option, &client);
  EXPECT_EQ (ML_ERROR_STREAMS_PIPE, status);

  status = ml_option_destroy (invalid_option);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_query_request with invalid param.
 */
TEST_F (MLServiceAgentTest, query_request_00_n)
{
  int status;
  status = ml_service_query_request (NULL, NULL, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_model_register with invalid param.
 */
TEST_F (MLServiceAgentTest, model_register_00_n)
{
  int status;

  const gchar *name = "some_model_name";
  const gchar *path = "/valid/path/to/some/model.tflite";

  const bool is_active = true;
  const gchar *desc = "some valid description";
  guint version;

  status = ml_service_model_register (NULL, path, is_active, desc, &version);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_register (name, NULL, is_active, desc, &version);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_register (name, path, is_active, desc, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_model_update_description with invalid param.
 */
TEST_F (MLServiceAgentTest, model_update_description_00_n)
{
  int status;

  const gchar *name = "some_model_name";
  const gchar *desc = "some valid description";
  guint version = 12345U;

  status = ml_service_model_update_description (NULL, version, desc);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_update_description (name, 0U, desc);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_update_description (name, version, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_update_description (name, version, desc);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_model_activate with invalid param.
 */
TEST_F (MLServiceAgentTest, model_activate_00_n)
{
  int status;
  status = ml_service_model_activate (NULL, 0U);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_activate ("some_model_name", 0U);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_activate ("some_model_name", 12345U);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_model_get with invalid param.
 */
TEST_F (MLServiceAgentTest, model_get_00_n)
{
  int status;

  const gchar *name = "some_model_name";
  guint version = 12345U;
  ml_option_h info_h;

  status = ml_service_model_get (NULL, version, &info_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_get (name, version, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_get (name, version, &info_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_model_get with invalid param.
 */
TEST_F (MLServiceAgentTest, model_get_01_n)
{
  int status;

  const gchar *model_name = "some_invalid_model_name";

  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  gchar *test_model;
  unsigned int version;

  /* ml_service_model_register() requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "some_invalid_model_name", NULL);
  ASSERT_FALSE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_service_model_delete (model_name, 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE || status == ML_ERROR_INVALID_PARAMETER);

  status = ml_service_model_register (model_name, test_model, true, NULL, &version);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  ml_option_h info_h;
  status = ml_service_model_get (model_name, 987654321U, &info_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_delete (model_name, version);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  g_free (test_model);
}

/**
 * @brief Test ml_service_model_get_activated with invalid param.
 */
TEST_F (MLServiceAgentTest, model_get_activated_00_n)
{
  int status;

  const gchar *name = "some_model_name";
  ml_option_h info_h;

  status = ml_service_model_get_activated (NULL, &info_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_get_activated (name, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_get_activated (name, &info_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_model_get_all with invalid param.
 */
TEST_F (MLServiceAgentTest, model_get_all_00_n)
{
  int status;

  const gchar *name = "some_model_name";
  ml_option_h *info_list_h;
  guint list_size;

  status = ml_service_model_get_all (NULL, &info_list_h, &list_size);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_get_all (name, NULL, &list_size);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_get_all (name, &info_list_h, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_get_all (name, &info_list_h, &list_size);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_model_delete with invalid param.
 */
TEST_F (MLServiceAgentTest, model_delete_00_n)
{
  int status;

  const gchar *name = "some_model_name";
  guint version = 12345U;

  status = ml_service_model_delete (NULL, version);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_delete (name, version);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_delete (name, 0U);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_option_get with invalid param.
 */
TEST_F (MLServiceAgentTest, model_ml_option_get_00_n)
{
  int status;

  const gchar *key = "some_key";
  ml_option_h info_h;
  gchar *value;

  status = ml_option_create (&info_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_option_get (NULL, key, (void **) &value);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_get (info_h, NULL, (void **) &value);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_get (info_h, key, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_get (info_h, key, (void **) &value);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_destroy (info_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_option_get with invalid param.
 */
TEST_F (MLServiceAgentTest, model_ml_option_get_01_n)
{
  int status;

  const gchar *model_name = "some_model_name";
  guint version;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* ml_service_model_register() requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  gchar *test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  const gchar *key = "some_invalid_key";
  ml_option_h info_h = NULL;
  gchar *value;

  status = ml_service_model_delete (model_name, 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE || status == ML_ERROR_INVALID_PARAMETER);

  status = ml_service_model_register (model_name, test_model, true, NULL, &version);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_model_get (model_name, version, &info_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *path;
  status = ml_option_get (info_h, "path", (void **) &path);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (test_model, path);

  gchar *description;
  status = ml_option_get (info_h, "description", (void **) &description);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ ("", description);

  status = ml_option_get (info_h, key, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_get (info_h, key, (void **) &value);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_get (NULL, key, (void **) &value);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_destroy (info_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_model_delete (model_name, 0U);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_free (test_model);
}

/**
 * @brief Test the usecase of ml_servive for model. TBU.
 */
TEST_F (MLServiceAgentTest, model_scenario)
{
  int status;
  const gchar *key = "mobilenet_v1";
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  gchar *test_model1, *test_model2;
  unsigned int version;

  /* ml_service_model_register() requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  /* delete all model with the key before test */
  status = ml_service_model_delete (key, 0U);
  EXPECT_TRUE (status == ML_ERROR_NONE || status == ML_ERROR_INVALID_PARAMETER);

  test_model1 = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model1, G_FILE_TEST_EXISTS));

  status = ml_service_model_register (key, test_model1, true, "temp description", &version);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (1U, version);

  status = ml_service_model_update_description (key, version, "updated description");
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_model_update_description (key, 32U, "updated description");
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  test_model2 = g_build_filename (root_path, "tests", "test_models", "models",
      "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model2, G_FILE_TEST_EXISTS));


  status = ml_service_model_register (key, test_model2, false, "this is the temp tflite model", &version);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (version, 2U);

  ml_option_h activated_model_info;
  status = ml_service_model_get_activated (key, &activated_model_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_NE (activated_model_info, nullptr);

  gchar *test_description;
  status = ml_option_get (activated_model_info, "path", (void **) &test_description);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (test_description, test_model1);
  status = ml_option_destroy (activated_model_info);
  EXPECT_EQ (ML_ERROR_NONE, status);

  ml_option_h _model_info;
  status = ml_service_model_get (key, 2U, &_model_info);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_NE (_model_info, nullptr);

  status = ml_option_get (_model_info, "path", (void **) &test_description);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (test_description, test_model2);
  status = ml_option_destroy (_model_info);
  EXPECT_EQ (ML_ERROR_NONE, status);

  ml_option_h *info_list;
  guint info_num;

  status = ml_service_model_get_all (key, &info_list, &info_num);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (info_num, 2U);

  for (guint i = 0; i < info_num; i++) {
    gchar *version_str;

    status = ml_option_get (info_list[i], "version", (void **) &version_str);
    EXPECT_EQ (ML_ERROR_NONE, status);
    if (g_ascii_strcasecmp (version_str, "1") == 0) {
      gchar *is_active;
      status = ml_option_get (info_list[i], "active", (void **) &is_active);
      EXPECT_EQ (ML_ERROR_NONE, status);
      EXPECT_STREQ (is_active, "T");

      gchar *path;
      status = ml_option_get (info_list[i], "path", (void **) &path);
      EXPECT_EQ (ML_ERROR_NONE, status);
      EXPECT_STREQ (path, test_model1);
    } else if (g_ascii_strcasecmp (version_str, "2") == 0) {
      gchar *is_active;
      status = ml_option_get (info_list[i], "active", (void **) &is_active);
      EXPECT_EQ (ML_ERROR_NONE, status);
      EXPECT_STREQ (is_active, "F");

      gchar *path;
      status = ml_option_get (info_list[i], "path", (void **) &path);
      EXPECT_EQ (ML_ERROR_NONE, status);
      EXPECT_STREQ (path, test_model2);
    } else {
      EXPECT_TRUE (false);
    }

    status = ml_option_destroy (info_list[i]);
    EXPECT_EQ (ML_ERROR_NONE, status);
  }

  g_free (info_list);

  /* delete the active model */
  status = ml_service_model_delete (key, 1U);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* no active model */
  status = ml_service_model_get_activated (key, &activated_model_info);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_activate (key, 91243U);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_model_activate (key, 2U);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_model_get_activated (key, &activated_model_info);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_option_get (activated_model_info, "path", (gpointer *) &test_description);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (test_description, test_model2);

  status = ml_option_destroy (activated_model_info);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_model_delete (key, 2U);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_model_delete (key, 1U);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  g_free (test_model1);
  g_free (test_model2);
}

/**
 * @brief Negative testcase of pipeline gdbus call.
 */
TEST_F (MLServiceAgentTest, pipeline_gdbus_call_n)
{
  int ret;
  GError *error = NULL;

  MachinelearningServicePipeline *proxy_for_pipeline = machinelearning_service_pipeline_proxy_new_for_bus_sync (
    G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE,
    "org.tizen.machinelearning.service",
    "/Org/Tizen/MachineLearning/Service/Pipeline", NULL, &error);

  if (!proxy_for_pipeline || error) {
    g_critical ("Failed to create proxy_for_pipeline for machinelearning service pipeline");
    if (error) {
      g_critical ("Error Message : %s", error->message);
      g_clear_error (&error);
    }
    ASSERT_TRUE (false);
  }

  /* gdbus call with empty string */
  machinelearning_service_pipeline_call_set_pipeline_sync (
    proxy_for_pipeline, "", "", &ret, nullptr, nullptr);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);
}

/**
 * @brief Negative testcase of model gdbus call.
 */
TEST_F (MLServiceAgentTest, model_gdbus_call_n)
{
  int ret;
  GError *error = NULL;

  MachinelearningServiceModel *proxy_for_model = machinelearning_service_model_proxy_new_for_bus_sync (
    G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE,
    "org.tizen.machinelearning.service",
    "/Org/Tizen/MachineLearning/Service/Model", NULL, &error);

  if (!proxy_for_model || error) {
    g_critical ("Failed to create proxy_for_model for machinelearning service model");
    if (error) {
      g_critical ("Error Message : %s", error->message);
      g_clear_error (&error);
    }
    ASSERT_TRUE (false);
  }

  /* empty string */
  machinelearning_service_model_call_register_sync (
    proxy_for_model, "", "", false, "test", NULL, &ret, nullptr, nullptr);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  /* empty string */
  machinelearning_service_model_call_get_all_sync (
    proxy_for_model, "", NULL, &ret, nullptr, nullptr);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  g_object_unref (proxy_for_model);
}

/**
 * @brief Negative test for pipeline. With DBus unconnected.
 */
TEST (MLServiceAgentTestDbusUnconnected, pipeline_n)
{
  int status;

  status = ml_service_set_pipeline ("test", "test");
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  gchar *ret_pipeline;
  status = ml_service_get_pipeline ("test", &ret_pipeline);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);


  ml_service_h service;
  status = ml_service_launch_pipeline ("test", &service);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  ml_service_s *mls = g_new0 (ml_service_s, 1);
  _ml_service_server_s *server = g_new0 (_ml_service_server_s, 1);
  mls->priv = server;

  server->id = -987654321; /* explicitly set id as invalid number */

  service = (ml_service_h) mls;
  status = ml_service_start_pipeline (service);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  status = ml_service_stop_pipeline (service);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  ml_pipeline_state_e state;
  status = ml_service_get_pipeline_state (service, &state);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  mls->type = ML_SERVICE_TYPE_SERVER_PIPELINE;
  status = ml_service_destroy (service);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  g_free (server);
  g_free (mls);
}

/**
 * @brief Negative test for model. With DBus unconnected.
 */
TEST (MLServiceAgentTestDbusUnconnected, model_n)
{
  int status;

  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  unsigned int version;

  /* ml_service_model_register() requires absolute path to model, ignore this case. */
  if (root_path == NULL)
    return;

  gchar *test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_service_model_register ("test", test_model, false, "test", &version);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  g_free (test_model);

  status = ml_service_model_update_description ("test", 1U, "test");
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  status = ml_service_model_activate ("test", 1U);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  ml_option_h model_info;
  status = ml_service_model_get ("test", 1U, &model_info);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  status = ml_service_model_get_activated ("test", &model_info);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);

  ml_option_h *info_list;
  guint info_num;
  status = ml_service_model_get_all ("test", &info_list, &info_num);
  EXPECT_EQ (ML_ERROR_IO_ERROR, status);
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
