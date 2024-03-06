/**
 * @file        unittest_capi_service_training_offloading.cc
 * @date        5 April 2024
 * @brief       Unit test for ML Service C-API training offloading service.
 * @see         https://github.com/nnstreamer/api
 * @author      Hyunil Park <hyunil46.park@samsung.com>
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
#include "ml-api-service-training-offloading.h"

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
  int status;

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
};

/**
 * @brief Callback function for reply test.
 */
static void
_receive_trained_model_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  gboolean ret = FALSE;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);
  ASSERT_TRUE (g_file_test (file_path, G_FILE_TEST_IS_DIR));
  g_autofree gchar *registered_trained_model_path = g_build_filename (root_path,
      "tests", "test_models", "models", "registered-trained-model.bin", NULL);

  /** @todo  case value '4' not in enumerated type 'ml_service_event_e', so it is necessary casting  */
  switch ((int) event) {

    case ML_SERVICE_EVENT_REPLY:
      {
        g_debug ("Get ML_SERVICE_EVENT_REPLY and received trained model");
        ret = g_file_test (registered_trained_model_path, G_FILE_TEST_EXISTS);
        EXPECT_EQ (TRUE, ret);
        break;
      }
    default:
      break;
  }
}

/**
 * @brief Callback function for sink node.
 */
static void
_sink_register_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  ml_tensors_data_h data = NULL;
  double *output = NULL;
  size_t data_size = 0;
  int status;
  double result_data[4];
  char *output_node_name = NULL;

  switch (event) {
    case ML_SERVICE_EVENT_NEW_DATA:
      ASSERT_TRUE (event_data != NULL);

      status = ml_information_get (event_data, "name", (void **) &output_node_name);
      EXPECT_EQ (status, ML_ERROR_NONE);
      EXPECT_STREQ (output_node_name, "training_result");

      status = ml_information_get (event_data, "data", &data);
      EXPECT_EQ (status, ML_ERROR_NONE);

      status = ml_tensors_data_get_tensor_data (data, 0, (void **) &output, &data_size);
      EXPECT_EQ (status, ML_ERROR_NONE);
      break;
    default:
      break;
  }

  if (output) {
    for (int i = 0; i < 4; i++)
      result_data[i] = output[i];

    g_debug ("name:%s >> [training_loss: %f, training_accuracy: %f, validation_loss: %f, validation_accuracy: %f]",
        output_node_name, result_data[0], result_data[1], result_data[2],
        result_data[3]);
  }
}

/**
 * @brief Start thread
*/
static void
sender_start_thread (ml_service_h sender_h)
{
  ml_service_start (sender_h);
}

/**
 * @brief Client thread
*/
static void
receiver_start_thread (ml_service_h receiver_h)
{
  ml_service_start (receiver_h);
}

/**
 * @brief full test for training offloading services.
 */
TEST_F (MLServiceTrainingOffloading, trainingOffloading_p)
{
  ml_service_h receiver_h;
  ml_service_h sender_h;

  GThread *start_thread = NULL;
  GThread *receive_thread = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);
  ASSERT_TRUE (g_file_test (file_path, G_FILE_TEST_IS_DIR));
  g_autofree gchar *trained_model_path = g_build_filename (
      root_path, "tests", "test_models", "models", "trained-model.bin", NULL);

  /* If you run the sender without running the receiver first, a connect error occurs in nns-edge. */
  g_autofree gchar *receiver_config = _get_config_path ("training_offloading_receiver.conf");
  status = ml_service_new (receiver_config, &receiver_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

  g_autofree gchar *sender_config = _get_config_path ("training_offloading_sender.conf");
  status = ml_service_new (sender_config, &sender_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* A path to app rw path */
  status = ml_service_set_information (sender_h, "path", file_path);
  EXPECT_EQ (status, ML_ERROR_NONE);
  /* A path to app rw path */
  status = ml_service_set_information (receiver_h, "path", file_path);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_event_cb (sender_h, _receive_trained_model_cb, NULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_event_cb (receiver_h, _sink_register_cb, NULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /** Use thread, if the test app uses the same thread as the receiver and sender,
      when the receiver does lock for wait required file, the sender will not work. */
  start_thread = g_thread_new (
      "sender_start_thread", (GThreadFunc) sender_start_thread, sender_h);
  receive_thread = g_thread_new (
      "receiver_start_thread", (GThreadFunc) receiver_start_thread, receiver_h);
  int loop = 120;
  while (loop--) {
    if (g_file_test (trained_model_path, G_FILE_TEST_EXISTS))
      break;
    g_usleep (100000);
  }

  ml_service_stop (sender_h);
  ml_service_stop (receiver_h);

  if (start_thread) {
    g_thread_join (start_thread);
    start_thread = NULL;
  }
  if (receive_thread) {
    g_thread_join (receive_thread);
    receive_thread = NULL;
  }

  status = ml_service_destroy (receiver_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_destroy (sender_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_training_offloading_create with invalid param.
 */
TEST_F (MLServiceTrainingOffloading, createInvalidParam1_n)
{
  int status;
  ml_service_h service_h = NULL;
  ml_service_s *mls = NULL;

  service_h = _ml_service_create_internal (ML_SERVICE_TYPE_OFFLOADING);
  mls = (ml_service_s *) service_h;
  ASSERT_NE (nullptr, mls);

  status = ml_service_training_offloading_create (mls, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_training_offloading_create with invalid param.
 */
TEST_F (MLServiceTrainingOffloading, createInvalidParam2_n)
{
  int status;
  ml_service_h service_h = NULL;
  ml_service_s *mls = NULL;

  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (GError) err = NULL;
  g_autofree gchar *json_string = NULL;
  JsonNode *root;
  JsonObject *object;
  g_autofree gchar *receiver_config = _get_config_path ("training_offloading_receiver.conf");

  ASSERT_TRUE (g_file_get_contents (receiver_config, &json_string, NULL, NULL));
  parser = json_parser_new ();
  ASSERT_TRUE (json_parser_load_from_data (parser, json_string, -1, &err));
  root = json_parser_get_root (parser);
  ASSERT_NE (nullptr, root);
  object = json_node_get_object (root);
  ASSERT_NE (nullptr, object);

  service_h = _ml_service_create_internal (ML_SERVICE_TYPE_OFFLOADING);
  mls = (ml_service_s *) service_h;
  ASSERT_NE (nullptr, mls);

  status = ml_service_training_offloading_create (NULL, object);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_training_offloading_create.
 */
TEST_F (MLServiceTrainingOffloading, create_p)
{
  int status;
  ml_service_h service_h = NULL;
  ml_service_s *mls = NULL;
  ml_option_h option = NULL;

  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (GError) err = NULL;
  g_autofree gchar *json_string = NULL;
  JsonNode *root;
  JsonObject *object;
  g_autofree gchar *receiver_config = _get_config_path ("training_offloading_receiver.conf");

  JsonObject *offloading_object;
  const gchar *val = NULL;
  const gchar *key = NULL;
  GList *list = NULL, *iter;

  ASSERT_TRUE (g_file_get_contents (receiver_config, &json_string, NULL, NULL));
  parser = json_parser_new ();
  ASSERT_TRUE (json_parser_load_from_data (parser, json_string, -1, &err));
  root = json_parser_get_root (parser);
  ASSERT_NE (nullptr, root);
  object = json_node_get_object (root);
  ASSERT_NE (nullptr, object);

  service_h = _ml_service_create_internal (ML_SERVICE_TYPE_OFFLOADING);
  mls = (ml_service_s *) service_h;
  ASSERT_NE (nullptr, mls);

  status = ml_option_create (&option);
  EXPECT_EQ (ML_ERROR_NONE, status);

  offloading_object = json_object_get_object_member (object, "offloading");
  ASSERT_NE (nullptr, offloading_object);

  list = json_object_get_members (offloading_object);
  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    key = (gchar *) iter->data;
    if (g_ascii_strcasecmp (key, "training") == 0) {
      /* It is not a value to set for option. */
      continue;
    }
    val = json_object_get_string_member (offloading_object, key);
    status = ml_option_set (option, key, g_strdup (val), g_free);
    EXPECT_EQ (ML_ERROR_NONE, status);
  }
  g_list_free (list);

  status = ml_service_offloading_create (mls, option);
  /** nns-edge error occurs because there is no remote to connect to.*/
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* An offloading instance must be created first. */
  status = ml_service_training_offloading_create (mls, object);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_training_offloading_destroy (mls);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_offloading_release_internal (mls);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_option_destroy (option);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_training_offloading_destroy.
 */
TEST_F (MLServiceTrainingOffloading, destroyInvalidParam1_n)
{
  int status;

  status = ml_service_training_offloading_destroy (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test ml_service_training_offloading_set_path.
 */
TEST_F (MLServiceTrainingOffloading, setPathInvalidParam1_n)
{
  int status;

  ml_service_h service_h = NULL;
  ml_service_s *mls = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);

  g_autofree gchar *receiver_config = _get_config_path ("training_offloading_receiver.conf");
  status = ml_service_new (receiver_config, &service_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

  mls = (ml_service_s *) service_h;
  ASSERT_NE (nullptr, mls);

  status = ml_service_training_offloading_set_path (mls, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_training_offloading_set_path (mls, file_path);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_destroy (service_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_training_offloading_start.
 */
TEST_F (MLServiceTrainingOffloading, startInvalidParam1_n)
{
  int status;
  ml_service_h receiver_h = NULL;
  ml_service_s *mls = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);

  g_autofree gchar *receiver_config = _get_config_path ("training_offloading_receiver.conf");
  /* If you run the sender without running the receiver first, a connect error occurs in nns-edge. */
  status = ml_service_new (receiver_config, &receiver_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  mls = (ml_service_s *) receiver_h;
  ASSERT_NE (nullptr, mls);

  status = ml_service_training_offloading_set_path (mls, file_path);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_training_offloading_start (mls);
  /** Not receiving data needed for training*/
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_training_offloading_start (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_destroy (receiver_h);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test ml_service_training_offloading_start.
 */
TEST_F (MLServiceTrainingOffloading, stopInvalidParam1_n)
{
  int status;
  ml_service_h receiver_h = NULL;
  ml_service_s *mls = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);

  g_autofree gchar *receiver_config = _get_config_path ("training_offloading_receiver.conf");
  /* If you run the sender without running the receiver first, a connect error occurs in nns-edge. */
  status = ml_service_new (receiver_config, &receiver_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  mls = (ml_service_s *) receiver_h;
  ASSERT_NE (nullptr, mls);

  status = ml_service_training_offloading_set_path (mls, file_path);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* not start */
  status = ml_service_training_offloading_stop (mls);
  EXPECT_EQ (ML_ERROR_STREAMS_PIPE, status);

  status = ml_service_destroy (receiver_h);
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
