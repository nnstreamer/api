/**
 * @file        unittest_capi_offloading_service.cc
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
class MLServiceTrainingOffloading : public ::testing::Test
{
  protected:
  GTestDBus *dbus;
  GBusType bus_type;
  int status;
  ml_service_h client_h;
  ml_service_h server_h;

  public:
  /**
   * @brief Setup method for each test case.
   */
  void SetUp () override
  {
    g_autofree gchar *services_dir = g_build_filename ("/usr/bin/ml-test/services", NULL);

    dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
    ASSERT_NE (nullptr, dbus);

    g_test_dbus_add_service_dir (dbus, services_dir);

    g_test_dbus_up (dbus);
#if defined(ENABLE_GCOV)
    bus_type = G_BUS_TYPE_SYSTEM;
#else
    bus_type = G_BUS_TYPE_SESSION;
#endif

    /* should start receiver */
    g_autofree gchar *receiver_config
        = _get_config_path ("training_offloading_receiver.conf");
    status = ml_service_new (receiver_config, &client_h);
    ASSERT_EQ (status, ML_ERROR_NONE);

    g_autofree gchar *sender_config = _get_config_path ("training_offloading_sender.conf");
    status = ml_service_new (sender_config, &server_h);
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

    status = ml_service_destroy (client_h);
    EXPECT_EQ (ML_ERROR_NONE, status);

    status = ml_service_destroy (server_h);
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
 * @brief Callback function for reply test.
 */
static void
_receive_trained_model_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  switch ((int) event) {

    case ML_SERVICE_EVENT_REPLY:
      {
        g_warning ("received trained model");
        break;
      }
    default:
      break;
  }
}

/**
 * @brief Start thread
*/
static void
server_start_thread (ml_service_h server_h)
{
  ml_service_start (server_h);
}

/**
 * @brief Client thread
*/
static void
receiver_start_thread (ml_service_h client_h)
{
  ml_service_start (client_h);
}

/**
 * @brief use case of pipeline registration using ml offloading service using conf file.
 */
TEST_F (MLServiceTrainingOffloading, registerPipeline)
{
  GThread *start_thread = NULL;
  GThread *receive_thread = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);
  EXPECT_TRUE (g_file_test (file_path, G_FILE_TEST_IS_DIR));
  g_autofree gchar *trained_model_path = g_build_filename (
      root_path, "tests", "test_models", "models", "trained-model.bin", NULL);

  /* A path to app rw path */
  status = ml_service_set_information (server_h, "path", file_path);
  /* A path to app rw path */
  status = ml_service_set_information (client_h, "path", file_path);

  status = ml_service_set_event_cb (server_h, _receive_trained_model_cb, NULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  start_thread = g_thread_new (
      "server_start_thread", (GThreadFunc) server_start_thread, server_h);
  receive_thread = g_thread_new (
      "receiver_start_thread", (GThreadFunc) receiver_start_thread, client_h);
  int loop = 120;
  while (loop--) {
    g_warning ("######## %s : %s: %d wait for thread %d", __FILE__,
        __FUNCTION__, __LINE__, loop);
    if (g_file_test (trained_model_path, G_FILE_TEST_EXISTS))
      break;
    g_usleep (100000);
  }

  ml_service_stop (server_h);
  ml_service_stop (client_h);

  if (start_thread) {
    g_thread_join (start_thread);
    start_thread = NULL;
  }
  if (receive_thread) {
    g_thread_join (receive_thread);
    receive_thread = NULL;
  }
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
