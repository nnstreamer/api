/**
 * @file        unittest_capi_remote_service.cc
 * @date        26 Jun 2023
 * @brief       Unit test for ML Service C-API remote service.
 * @see         https://github.com/nnstreamer/api
 * @author      Gichan Jang <gichan2.jang@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <gdbus-util.h>
#include <gio/gio.h>
#include <ml-api-inference-pipeline-internal.h>
#include <ml-api-internal.h>
#include <ml-api-service-private.h>
#include <ml-api-service.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

/**
 * @brief Test base class for Database of ML Service API.
 */
class MLRemoteService : public ::testing::Test
{
  protected:
  GTestDBus *dbus;

  public:
  /**
   * @brief Setup method for each test case.
   */
  void SetUp () override
  {
    g_autofree gchar *current_dir = g_get_current_dir ();
    g_autofree gchar *services_dir
        = g_build_filename (current_dir, "tests/services", NULL);

    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    ASSERT_NE (nullptr, dbus);

    g_test_dbus_add_service_dir (dbus, services_dir);

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
 * @brief use case of pipeline registration using ml remote service.
 */
TEST_F (MLRemoteService, registerPipeline)
{
  int status;

  /**============= Prepare client ============= **/
  ml_service_h client_h;
  ml_option_h client_option_h = NULL;

  status = ml_option_create (&client_option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *client_node_type = g_strdup ("remote_sender");
  status = ml_option_set (client_option_h, "node-type", client_node_type, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *client_dest_host = g_strdup ("127.0.0.1");
  status = ml_option_set (client_option_h, "host", client_dest_host, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint dest_port = 3000;
  status = ml_option_set (client_option_h, "port", &dest_port, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *client_connect_type = g_strdup ("TCP");
  status = ml_option_set (client_option_h, "connect-type", client_connect_type, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *topic = g_strdup ("remote_service_test_topic");
  status = ml_option_set (client_option_h, "topic", topic, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_remote_service_create (client_option_h, &client_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /**============= Prepare server ============= **/
  ml_service_h server_h;
  ml_option_h server_option_h = NULL;
  status = ml_option_create (&server_option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *server_node_type = g_strdup ("remote_receiver");
  status = ml_option_set (server_option_h, "node-type", server_node_type, g_free);

  gchar *dest_host = g_strdup ("127.0.0.1");
  status = ml_option_set (server_option_h, "dest-host", dest_host, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_option_set (server_option_h, "topic", topic, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_option_set (server_option_h, "dest-port", &dest_port, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *server_connect_type = g_strdup ("TCP");
  status = ml_option_set (server_option_h, "connect-type", server_connect_type, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_remote_service_create (server_option_h, &server_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  ml_option_h remote_service_option_h = NULL;
  status = ml_option_create (&remote_service_option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);


  gchar *service_type = g_strdup ("pipeline_raw");
  ml_option_set (remote_service_option_h, "service-type", service_type, g_free);

  gchar *service_key = g_strdup ("pipeline_test_key");
  ml_option_set (remote_service_option_h, "service-key", service_key, g_free);

  g_autofree gchar *pipeline_desc = g_strdup ("fakesrc ! fakesink");

  status = ml_remote_service_register (client_h, remote_service_option_h,
      pipeline_desc, strlen (pipeline_desc) + 1);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /** Wait for the server to register the pipeline. */
  g_usleep (1000000);

  g_autofree gchar *ret_pipeline = NULL;
  status = ml_service_get_pipeline (service_key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, status);
  EXPECT_STREQ (pipeline_desc, ret_pipeline);

  status = ml_service_destroy (server_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_service_destroy (client_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_option_destroy (server_option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_option_destroy (remote_service_option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
  status = ml_option_destroy (client_option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_remote_service_create with invalid param.
 */
TEST_F (MLRemoteService, createInvalidParam_n)
{
  int status;
  ml_option_h option_h = NULL;
  ml_service_h service_h = NULL;

  status = ml_option_create (&option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_remote_service_create (NULL, &service_h);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_remote_service_create (option_h, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_destroy (option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_remote_service_register with invalid param.
 */
TEST_F (MLRemoteService, registerInvalidParam_n)
{
  int status;
  ml_service_h service_h = NULL;
  ml_option_h option_h = NULL;
  g_autofree gchar *str = g_strdup ("Temp_test_str");
  size_t len = strlen (str) + 1;

  status = ml_option_create (&option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *client_node_type = g_strdup ("remote_sender");
  status = ml_option_set (option_h, "node-type", client_node_type, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *client_dest_host = g_strdup ("127.0.0.1");
  status = ml_option_set (option_h, "dest-host", client_dest_host, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  guint dest_port = 1883;
  status = ml_option_set (option_h, "dest-port", &dest_port, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *client_connect_type = g_strdup ("HYBRID");
  status = ml_option_set (option_h, "connect-type", client_connect_type, g_free);
  EXPECT_EQ (ML_ERROR_NONE, status);

  gchar *topic = g_strdup ("temp_test_topic");
  status = ml_option_set (option_h, "topic", topic, NULL);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_remote_service_create (option_h, &service_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_remote_service_register (NULL, option_h, str, len);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_remote_service_register (service_h, NULL, str, len);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_remote_service_register (service_h, option_h, NULL, len);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_remote_service_register (service_h, option_h, str, 0);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_option_destroy (option_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_destroy (service_h);
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
