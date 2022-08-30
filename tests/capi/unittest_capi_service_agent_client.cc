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

#include <netinet/tcp.h>
#include <netinet/in.h>

/**
 * @brief Test base class for Database of ML Service API.
 */
class MLServiceAgentTest : public::testing::Test
{
protected:
  GTestDBus *dbus;
  GDBusProxy *proxy;

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

    GError *error = NULL;
    proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.tizen.machinelearning.service",
        "/Org/Tizen/MachineLearning/Service/Pipeline",
        "org.tizen.machinelearning.service.pipeline",
        NULL,
        &error);

    ASSERT_EQ (nullptr, error);
    ASSERT_NE (nullptr, proxy);
  }

  /**
   * @brief Teardown method for each test case.
   */
  void TearDown () override
  {
    if (proxy)
      g_object_unref (proxy);

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

  status = ml_service_getdesc_pipeline (service, &ret_pipeline);
  EXPECT_STREQ (pipeline_desc, ret_pipeline);

  status = ml_service_getstate_pipeline (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  status = ml_service_start_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_service_getstate_pipeline (service, &state);
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

  status = ml_service_destroy_pipeline (client);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_stop_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_usleep (1 * 1000 * 1000);

  status = ml_service_getstate_pipeline (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  /** destroy the pipeline */
  status = ml_service_destroy_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** delete finished service */
  status = ml_service_delete_pipeline (service_name);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** it would fail if get the removed service */
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  g_free (pipeline_desc);
  g_free (client_pipeline_desc);
  g_free (ret_pipeline);
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

  status = ml_service_getdesc_pipeline (service, &ret_pipeline);
  EXPECT_STREQ (pipeline_desc, ret_pipeline);

  status = ml_service_getstate_pipeline (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  status = ml_service_start_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_service_getstate_pipeline (service, &state);
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

  status = ml_service_getstate_pipeline (service, &state);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_EQ (ML_PIPELINE_STATE_PAUSED, state);

  /** destroy the pipeline */
  status = ml_service_destroy_pipeline (service);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** delete finished service */
  status = ml_service_delete_pipeline (service_name);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** it would fail if get the removed service */
  status = ml_service_get_pipeline (service_name, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  g_free (pipeline_desc);
  g_free (client_pipeline_desc);
  g_free (ret_pipeline);
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
 * @brief Test ml_service_destroy_pipeline with invalid param.
 */
TEST_F (MLServiceAgentTest, close_pipeline_00_n)
{
  int status;
  status = ml_service_destroy_pipeline (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
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
  set_feature_state (ML_FEATURE_SERVICE, NOT_CHECKED_YET);

  return result;
}
