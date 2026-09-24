/**
 * @file        unittest_capi_service_training_offloading.cc
 * @date        5 April 2024
 * @brief       Unit test for ML Service C-API training offloading service.
 * @see         https://github.com/nnstreamer/api
 * @author      Hyunil Park <hyunil46.park@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <glib/gstdio.h>
#ifdef __GLIBC__
#include <malloc.h>
#endif
#include <ml-api-inference-pipeline-internal.h>
#include <ml-api-internal.h>
#include <ml-api-service-private.h>
#include <ml-api-service.h>
#include <nnstreamer-edge.h>

#include "ml-api-service-offloading.h"
#include "ml-api-service-training-offloading.h"
#include "unittest_util.h"

/**
 * @brief Test base class for Database of ML Service API.
 */
class MLServiceTrainingOffloading : public ::testing::Test
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
GTestDBus *MLServiceTrainingOffloading::dbus = nullptr;

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

  /** @todo  case value '4' not in enumerated type 'ml_service_event_e', so it is necessary casting. */
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
 * @brief Start thread (sender).
*/
static gpointer
sender_start_thread (gpointer data)
{
  ml_service_h sender_h = (ml_service_h) data;

  ml_service_start (sender_h);
  return NULL;
}

/**
 * @brief Start thread (receiver).
*/
static gpointer
receiver_start_thread (gpointer data)
{
  ml_service_h receiver_h = (ml_service_h) data;

  ml_service_start (receiver_h);
  return NULL;
}

/**
 * @brief full test for training offloading services.
 */
TEST_F (MLServiceTrainingOffloading, trainingOffloading_p)
{
  int status;
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
  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);
  g_autofree gchar *sender_config
      = prepare_test_config ("training_offloading_sender.conf", avail_port);

  status = ml_service_new (receiver_config, &receiver_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

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

  /**
   * Use thread, if the test app uses the same thread as the receiver and sender,
   * when the receiver does lock for wait required file, the sender will not work.
   */
  start_thread = g_thread_new (
      "sender_start_thread", (GThreadFunc) sender_start_thread, sender_h);
  receive_thread = g_thread_new (
      "receiver_start_thread", (GThreadFunc) receiver_start_thread, receiver_h);

  /**
   * The pipeline is designed to perform training and validation.
   * When nntrainer finishes training, the model is created.
   * Therefore, validation time is given for TC.
   */
  int loop = 120;
  while (loop--) {
    if (g_file_test (trained_model_path, G_FILE_TEST_EXISTS)) {
      g_usleep (1000000);
      break;
    }
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

  EXPECT_EQ (g_remove (receiver_config), 0);
  EXPECT_EQ (g_remove (sender_config), 0);
}

/**
 * @brief Test _ml_service_training_offloading_create with invalid param.
 */
TEST_F (MLServiceTrainingOffloading, createInvalidParam1_n)
{
  int status;
  ml_service_s *mls;

  mls = _ml_service_create_internal (ML_SERVICE_TYPE_OFFLOADING);
  ASSERT_NE (nullptr, mls);

  status = _ml_service_training_offloading_create (mls, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = _ml_service_offloading_release_internal (mls);
  EXPECT_EQ (ML_ERROR_NONE, status);
}

/**
 * @brief Test _ml_service_training_offloading_create with invalid param.
 */
TEST_F (MLServiceTrainingOffloading, createInvalidParam2_n)
{
  int status;

  g_autoptr (JsonParser) parser = NULL;
  g_autofree gchar *json_string = NULL;
  JsonNode *root;
  JsonObject *object;

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);

  ASSERT_TRUE (g_file_get_contents (receiver_config, &json_string, NULL, NULL));
  parser = json_parser_new ();
  ASSERT_TRUE (json_parser_load_from_data (parser, json_string, -1, NULL));
  root = json_parser_get_root (parser);
  ASSERT_NE (nullptr, root);
  object = json_node_get_object (root);
  ASSERT_NE (nullptr, object);

  JsonObject *offloading = json_object_get_object_member (object, "offloading");
  status = _ml_service_training_offloading_create (NULL, offloading);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  EXPECT_EQ (g_remove (receiver_config), 0);
}

/**
 * @brief Test _ml_service_training_offloading_create.
 */
TEST_F (MLServiceTrainingOffloading, create_p)
{
  int status;
  ml_service_s *mls;

  g_autoptr (JsonParser) parser = NULL;
  g_autofree gchar *json_string = NULL;
  JsonNode *root;
  JsonObject *object;

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);

  ASSERT_TRUE (g_file_get_contents (receiver_config, &json_string, NULL, NULL));
  parser = json_parser_new ();
  ASSERT_TRUE (json_parser_load_from_data (parser, json_string, -1, NULL));
  root = json_parser_get_root (parser);
  ASSERT_NE (nullptr, root);
  object = json_node_get_object (root);
  ASSERT_NE (nullptr, object);

  mls = _ml_service_create_internal (ML_SERVICE_TYPE_OFFLOADING);
  ASSERT_NE (nullptr, mls);

  status = _ml_service_offloading_create (mls, object);
  /* nns-edge error occurs because there is no remote to connect to. */
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* An offloading instance must be created first. */
  JsonObject *offloading = json_object_get_object_member (object, "offloading");
  status = _ml_service_training_offloading_create (mls, offloading);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = _ml_service_training_offloading_destroy (mls);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = _ml_service_offloading_release_internal (mls);
  EXPECT_EQ (ML_ERROR_NONE, status);

  EXPECT_EQ (g_remove (receiver_config), 0);
}

/**
 * @brief Pipeline the receiver would normally get from the remote sender.
 * It is self-contained on purpose: the teardown order, not the training
 * framework, is under test here.
 */
static const gchar *receiver_pipe_json
    = R"JSON({"pipeline":{"description":"videotestsrc is-live=true ! videoconvert ! video/x-raw,format=RGB,width=16,height=16,framerate=30/1 ! tensor_converter ! tensor_sink name=training_result async=false","output_node":[{"name":"training_result"}]}})JSON";

/**
 * @brief Time the sink callback stays in the pipeline, in microseconds.
 */
#define SINK_CB_HOLD_TIME (300000)

/**
 * @brief Handshake between the sink callback and the thread tearing the
 * service down.
 */
typedef struct {
  GMutex lock;
  GCond cond;
  gboolean entered;
} sink_hold_s;

/**
 * @brief Callback holding the streaming thread while the service is destroyed.
 *
 * The 'name' of the event data is the name of the node info owned by the
 * training offloading handle, handed over without a copy. Holding the callback
 * makes the teardown overlap with it, so reading the name afterwards fails if
 * the node info was released while the pipeline was still running.
 */
static void
_hold_new_data_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
{
  sink_hold_s *hold = (sink_hold_s *) user_data;
  char *node_name = NULL;

  if (event != ML_SERVICE_EVENT_NEW_DATA)
    return;

  g_mutex_lock (&hold->lock);
  hold->entered = TRUE;
  g_cond_broadcast (&hold->cond);
  g_mutex_unlock (&hold->lock);

  g_usleep (SINK_CB_HOLD_TIME);

  EXPECT_EQ (ml_information_get (event_data, "name", (void **) &node_name), ML_ERROR_NONE);
  EXPECT_STREQ (node_name, "training_result");
}

/**
 * @brief Bring a receiver service up to the point where the sink callback has
 * entered and is holding the streaming thread.
 */
static void
_start_receiver_pipeline (ml_service_h receiver_h, const gchar *path, sink_hold_s *hold)
{
  ml_service_s *mls = (ml_service_s *) receiver_h;
  nns_edge_data_h data_h = NULL;
  gint64 deadline;
  gboolean entered;
  int status;

  status = _ml_service_training_offloading_set_path (mls, path);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_service_set_event_cb (receiver_h, _hold_new_data_cb, hold);
  ASSERT_EQ (status, ML_ERROR_NONE);

  ASSERT_EQ (nns_edge_data_create (&data_h), NNS_EDGE_ERROR_NONE);
  status = _ml_service_training_offloading_process_received_data (mls, data_h,
      path, receiver_pipe_json, ML_SERVICE_OFFLOADING_TYPE_PIPELINE_RAW);
  nns_edge_data_destroy (data_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = _ml_service_training_offloading_start (mls);
  ASSERT_EQ (status, ML_ERROR_NONE);

  deadline = g_get_monotonic_time () + 10 * G_TIME_SPAN_SECOND;

  g_mutex_lock (&hold->lock);
  while (!hold->entered) {
    if (!g_cond_wait_until (&hold->cond, &hold->lock, deadline))
      break;
  }
  entered = hold->entered;
  g_mutex_unlock (&hold->lock);

  ASSERT_TRUE (entered);
}

/**
 * @brief Destroying a running service must not release the node info the sink
 * callback is still using.
 */
TEST_F (MLServiceTrainingOffloading, destroyWhileRunning_p)
{
  int status;
  sink_hold_s hold = {};
  ml_service_h receiver_h = NULL;
  g_autofree gchar *path = g_dir_make_tmp ("ml-training-offloading-XXXXXX", NULL);

  ASSERT_NE (nullptr, path);

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);

  g_mutex_init (&hold.lock);
  g_cond_init (&hold.cond);

  status = ml_service_new (receiver_config, &receiver_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _start_receiver_pipeline (receiver_h, path, &hold);

#ifdef __GLIBC__
  /**
   * Scrub memory as it is released, so that a node name read after the node
   * table is gone is caught rather than read back intact. glibc skips this for
   * a chunk that fits the tcache, which the node name normally does; there the
   * detection instead comes from tcache_put() writing its own link fields over
   * the first 16 bytes of the chunk. Neither is a property of the code under
   * test, so a sanitizer build remains the only airtight net for this.
   */
  mallopt (M_PERTURB, 0xAA);
#endif

  /* Destroy without stopping first. */
  status = ml_service_destroy (receiver_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

#ifdef __GLIBC__
  mallopt (M_PERTURB, 0);
#endif

  g_mutex_clear (&hold.lock);
  g_cond_clear (&hold.cond);

  EXPECT_EQ (g_remove (receiver_config), 0);
  EXPECT_EQ (g_rmdir (path), 0);
}

/**
 * @brief Stopping the service before destroying it keeps working.
 */
TEST_F (MLServiceTrainingOffloading, destroyAfterStop_p)
{
  int status;
  sink_hold_s hold = {};
  ml_service_h receiver_h = NULL;
  g_autofree gchar *path = g_dir_make_tmp ("ml-training-offloading-XXXXXX", NULL);

  ASSERT_NE (nullptr, path);

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);

  g_mutex_init (&hold.lock);
  g_cond_init (&hold.cond);

  status = ml_service_new (receiver_config, &receiver_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

  _start_receiver_pipeline (receiver_h, path, &hold);

  status = ml_service_stop (receiver_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_destroy (receiver_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  g_mutex_clear (&hold.lock);
  g_cond_clear (&hold.cond);

  EXPECT_EQ (g_remove (receiver_config), 0);
  EXPECT_EQ (g_rmdir (path), 0);
}

/**
 * @brief Test _ml_service_training_offloading_destroy.
 */
TEST_F (MLServiceTrainingOffloading, destroyInvalidParam1_n)
{
  int status;

  status = _ml_service_training_offloading_destroy (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);
}

/**
 * @brief Test _ml_service_training_offloading_destroy with a service that is
 * not in training mode.
 */
TEST_F (MLServiceTrainingOffloading, destroyInvalidParam2_n)
{
  int status;
  ml_service_s *mls;

  g_autoptr (JsonParser) parser = NULL;
  g_autofree gchar *json_string = NULL;
  JsonNode *root;
  JsonObject *object;

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("service_offloading_receiver.conf", avail_port);

  ASSERT_TRUE (g_file_get_contents (receiver_config, &json_string, NULL, NULL));
  parser = json_parser_new ();
  ASSERT_TRUE (json_parser_load_from_data (parser, json_string, -1, NULL));
  root = json_parser_get_root (parser);
  ASSERT_NE (nullptr, root);
  object = json_node_get_object (root);
  ASSERT_NE (nullptr, object);

  mls = _ml_service_create_internal (ML_SERVICE_TYPE_OFFLOADING);
  ASSERT_NE (nullptr, mls);

  /* The configuration has no 'training' member, so the mode stays NONE. */
  status = _ml_service_offloading_create (mls, object);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = _ml_service_training_offloading_destroy (mls);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = _ml_service_destroy_internal (mls);
  EXPECT_EQ (ML_ERROR_NONE, status);

  EXPECT_EQ (g_remove (receiver_config), 0);
}

/**
 * @brief Test _ml_service_training_offloading_set_path.
 */
TEST_F (MLServiceTrainingOffloading, setPathInvalidParam1_n)
{
  int status;

  ml_service_h service_h = NULL;
  ml_service_s *mls = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);

  status = ml_service_new (receiver_config, &service_h);
  ASSERT_EQ (status, ML_ERROR_NONE);

  mls = (ml_service_s *) service_h;
  ASSERT_NE (nullptr, mls);

  status = _ml_service_training_offloading_set_path (mls, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = _ml_service_training_offloading_set_path (mls, file_path);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = ml_service_destroy (service_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  EXPECT_EQ (g_remove (receiver_config), 0);
}

/**
 * @brief Test _ml_service_training_offloading_start.
 */
TEST_F (MLServiceTrainingOffloading, startInvalidParam1_n)
{
  int status;
  ml_service_h receiver_h = NULL;
  ml_service_s *mls = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);

  /* If you run the sender without running the receiver first, a connect error occurs in nns-edge. */
  status = ml_service_new (receiver_config, &receiver_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  mls = (ml_service_s *) receiver_h;
  ASSERT_NE (nullptr, mls);

  status = _ml_service_training_offloading_set_path (mls, file_path);
  EXPECT_EQ (ML_ERROR_NONE, status);

  status = _ml_service_training_offloading_start (mls);
  /* Not receiving data needed for training. */
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = _ml_service_training_offloading_start (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, status);

  status = ml_service_destroy (receiver_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  EXPECT_EQ (g_remove (receiver_config), 0);
}

/**
 * @brief Test _ml_service_training_offloading_start.
 */
TEST_F (MLServiceTrainingOffloading, stopInvalidParam1_n)
{
  int status;
  ml_service_h receiver_h = NULL;
  ml_service_s *mls = NULL;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  g_autofree gchar *file_path
      = g_build_filename (root_path, "tests", "test_models", "models", NULL);

  guint avail_port = get_available_port ();
  g_autofree gchar *receiver_config
      = prepare_test_config ("training_offloading_receiver.conf", avail_port);

  /* If you run the sender without running the receiver first, a connect error occurs in nns-edge. */
  status = ml_service_new (receiver_config, &receiver_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  mls = (ml_service_s *) receiver_h;
  ASSERT_NE (nullptr, mls);

  status = _ml_service_training_offloading_set_path (mls, file_path);
  EXPECT_EQ (ML_ERROR_NONE, status);

  /* not start */
  status = _ml_service_training_offloading_stop (mls);
  EXPECT_EQ (ML_ERROR_STREAMS_PIPE, status);

  status = ml_service_destroy (receiver_h);
  EXPECT_EQ (ML_ERROR_NONE, status);

  EXPECT_EQ (g_remove (receiver_config), 0);
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
