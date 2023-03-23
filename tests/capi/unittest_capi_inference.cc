/**
 * @file        unittest_capi_inference.cc
 * @date        13 Mar 2019
 * @brief       Unit test for ML inference C-API.
 * @see         https://github.com/nnstreamer/nnstreamer
 * @author      MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <glib.h>
#include <glib/gstdio.h> /* GStatBuf */
#include <nnstreamer.h>
#include <nnstreamer_plugin_api.h>
#include <nnstreamer_internal.h>
#include <nnstreamer-tizen-internal.h>
#include <ml-api-internal.h>
#include <ml-api-inference-internal.h>
#include <ml-api-inference-pipeline-internal.h>

#if defined (__APPLE__)
#define SO_FILE_EXTENSION ".dylib"
#else
#define SO_FILE_EXTENSION ".so"
#endif

static const unsigned int SINGLE_DEF_TIMEOUT_MSEC = 10000U;

#if defined (ENABLE_TENSORFLOW_LITE) || defined (ENABLE_TENSORFLOW2_LITE)
constexpr bool is_enabled_tensorflow_lite = true;
#else
constexpr bool is_enabled_tensorflow_lite = false;
#endif

/**
 * @brief Struct to check the pipeline state changes.
 */
typedef struct {
  gboolean paused;
  gboolean playing;
} TestPipeState;

/**
 * @brief Macro to wait for pipeline state.
 */
#define wait_for_start(handle, state, status) do { \
    int counter = 0; \
    while ((state == ML_PIPELINE_STATE_PAUSED || state == ML_PIPELINE_STATE_READY) \
           && counter < 20) { \
      g_usleep (50000); \
      counter++; \
      status = ml_pipeline_get_state (handle, &state); \
      EXPECT_EQ (status, ML_ERROR_NONE); \
    } \
  } while (0)

/**
 * @brief Macro to wait for expected buffers to arrive.
 */
#define wait_pipeline_process_buffers(received, expected) do { \
    guint timer = 0; \
    while (received < expected) { \
      g_usleep (10000); \
      timer += 10; \
      if (timer > SINGLE_DEF_TIMEOUT_MSEC) \
        break; \
    } \
  } while (0)

#if defined (__TIZEN__)
#if TIZENPPM
/**
 * @brief Test NNStreamer pipeline construct with Tizen cam
 * @details Failure case to check permission (camera privilege)
 */
TEST (nnstreamer_capi_construct_destruct, tizen_cam_fail_01_n)
{
  ml_pipeline_h handle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup_printf ("%s ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=320,height=240 ! tensor_converter ! tensor_sink",
      ML_TIZEN_CAM_VIDEO_SRC);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_PERMISSION_DENIED);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline construct with Tizen cam
 * @details Failure case to check permission (camera privilege)
 */
TEST (nnstreamer_capi_construct_destruct, tizen_cam_fail_02_n)
{
  ml_pipeline_h handle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup_printf ("%s ! audioconvert ! audio/x-raw,format=S16LE,rate=16000 ! tensor_converter ! tensor_sink",
      ML_TIZEN_CAM_AUDIO_SRC);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_PERMISSION_DENIED);

  g_free (pipeline);
}
#endif /* TIZENPPM */

/**
 * @brief Test NNStreamer pipeline construct with Tizen internal API.
 */
TEST (nnstreamer_capi_construct_destruct, tizen_internal_01_p)
{
  ml_pipeline_h handle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup_printf (
      "videotestsrc ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=320,height=240 ! tensor_converter ! tensor_sink");

  status = ml_pipeline_construct_internal (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline construct with Tizen internal API.
 */
TEST (nnstreamer_capi_construct_destruct, tizen_internal_02_p)
{
  ml_pipeline_h handle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup_printf (
      "audiotestsrc ! audioconvert ! audio/x-raw,format=S16LE,rate=16000 ! tensor_converter ! tensor_sink");

  status = ml_pipeline_construct_internal (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}
#endif /* __TIZEN__ */

/**
 * @brief Test NNStreamer pipeline construct & destruct
 */
TEST (nnstreamer_capi_construct_destruct, dummy_01)
{
  const char *pipeline = "videotestsrc num_buffers=2 ! fakesink";
  ml_pipeline_h handle;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline construct & destruct
 */
TEST (nnstreamer_capi_construct_destruct, dummy_02)
{
  const char *pipeline = "videotestsrc num_buffers=2 ! videoconvert ! videoscale ! video/x-raw,format=RGBx,width=224,height=224 ! tensor_converter ! fakesink";
  ml_pipeline_h handle;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline construct & destruct
 */
TEST (nnstreamer_capi_construct_destruct, dummy_03)
{
  const char *pipeline = "videotestsrc num_buffers=2 ! videoconvert ! videoscale ! video/x-raw,format=RGBx,width=224,height=224 ! tensor_converter ! valve name=valvex ! tensor_sink name=sinkx";
  ml_pipeline_h handle;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline construct with non-existent filter
 */
TEST (nnstreamer_capi_construct_destruct, failure_01_n)
{
  const char *pipeline = "nonexistsrc ! fakesink";
  ml_pipeline_h handle;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_STREAMS_PIPE);
}

/**
 * @brief Test NNStreamer pipeline construct with erroneous pipeline
 */
TEST (nnstreamer_capi_construct_destruct, failure_02_n)
{
  const char *pipeline = "videotestsrc num_buffers=2 ! audioconvert ! fakesink";
  ml_pipeline_h handle;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_STREAMS_PIPE);
}

/**
 * @brief Test NNStreamer pipeline construct & destruct
 */
TEST (nnstreamer_capi_playstop, dummy_01)
{
  const char *pipeline = "videotestsrc is-live=true ! videoconvert ! videoscale ! video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! valve name=valvex ! valve name=valvey ! input-selector name=is01 ! tensor_sink name=sinkx";
  ml_pipeline_h handle;
  ml_pipeline_state_e state;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status,
      ML_ERROR_NONE); /* At this moment, it can be READY, PAUSED, or PLAYING */
  EXPECT_NE (state, ML_PIPELINE_STATE_UNKNOWN);
  EXPECT_NE (state, ML_PIPELINE_STATE_NULL);

  g_usleep (50000); /** 50ms is good for general systems, but not enough for
                       emulators to start gst pipeline. Let a few frames flow.
                       */
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  wait_for_start (handle, state, status);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PLAYING);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000); /* 50ms. Let a few frames flow. */

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PAUSED);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline construct & destruct
 */
TEST (nnstreamer_capi_playstop, dummy_02)
{
  const char *pipeline = "videotestsrc is-live=true ! videoconvert ! videoscale ! video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! valve name=valvex ! valve name=valvey ! input-selector name=is01 ! tensor_sink name=sinkx";
  ml_pipeline_h handle;
  ml_pipeline_state_e state;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status,
      ML_ERROR_NONE); /* At this moment, it can be READY, PAUSED, or PLAYING */
  EXPECT_NE (state, ML_PIPELINE_STATE_UNKNOWN);
  EXPECT_NE (state, ML_PIPELINE_STATE_NULL);

  g_usleep (50000); /** 50ms is good for general systems, but not enough for
                       emulators to start gst pipeline. Let a few frames flow.
                       */
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  wait_for_start (handle, state, status);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PLAYING);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000); /* 50ms. Let a few frames flow. */

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PAUSED);

  /* Resume playing */
  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_NE (state, ML_PIPELINE_STATE_UNKNOWN);
  EXPECT_NE (state, ML_PIPELINE_STATE_NULL);

  g_usleep (50000); /* 50ms. Enough to empty the queue */
  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PAUSED);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline construct & destruct
 */
TEST (nnstreamer_capi_valve, test01)
{
  const gchar *_tmpdir = g_get_tmp_dir ();
  const gchar *_dirname = "nns-tizen-XXXXXX";
  gchar *fullpath = g_build_path ("/", _tmpdir, _dirname, NULL);
  gchar *dir = g_mkdtemp ((gchar *)fullpath);
  gchar *file1 = g_build_path ("/", dir, "valve1", NULL);
  gchar *pipeline = g_strdup_printf (
      "videotestsrc is-live=true ! videoconvert ! videoscale ! video/x-raw,format=RGBx,width=16,height=16,framerate=10/1 ! tensor_converter ! queue ! valve name=valve1 ! filesink location=\"%s\"",
      file1);
  GStatBuf buf;

  ml_pipeline_h handle;
  ml_pipeline_state_e state;
  ml_pipeline_valve_h valve1;

  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_TRUE (dir != NULL);

  status = ml_pipeline_valve_get_handle (handle, "valve1", &valve1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_valve_set_open (valve1, false); /* close */
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (50000); /* 50ms. Wait for the pipeline stgart. */
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status,
      ML_ERROR_NONE); /* At this moment, it can be READY, PAUSED, or PLAYING */
  EXPECT_NE (state, ML_PIPELINE_STATE_UNKNOWN);
  EXPECT_NE (state, ML_PIPELINE_STATE_NULL);

  wait_for_start (handle, state, status);
  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = g_lstat (file1, &buf);
  EXPECT_EQ (status, 0);
  EXPECT_EQ (buf.st_size, 0);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_valve_set_open (valve1, true); /* open */
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_valve_release_handle (valve1); /* release valve handle */
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (500000); /* 500ms. Let a few frames flow. (10Hz x 0.5s --> 5)*/

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = g_lstat (file1, &buf);
  EXPECT_EQ (status, 0);
  EXPECT_GE (buf.st_size, 2048); /* At least two frames during 500ms */
  EXPECT_LE (buf.st_size, 6144); /* At most six frames during 500ms */
  EXPECT_EQ (buf.st_size % 1024, 0); /* It should be divided by 1024 */

  g_free (fullpath);
  g_free (file1);
  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline valve
 * @detail Failure case to handle valve element with invalid param.
 */
TEST (nnstreamer_capi_valve, failure_01_n)
{
  ml_pipeline_valve_h valve_h;
  int status;

  /* invalid param : pipe */
  status = ml_pipeline_valve_get_handle (NULL, "valvex", &valve_h);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test NNStreamer pipeline valve
 * @detail Failure case to handle valve element with invalid param.
 */
TEST (nnstreamer_capi_valve, failure_02_n)
{
  ml_pipeline_h handle;
  ml_pipeline_valve_h valve_h;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : name */
  status = ml_pipeline_valve_get_handle (handle, NULL, &valve_h);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}
/**
 * @brief Test NNStreamer pipeline valve
 * @detail Failure case to handle valve element with invalid param.
 */
TEST (nnstreamer_capi_valve, failure_03_n)
{
  ml_pipeline_h handle;
  ml_pipeline_valve_h valve_h;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : wrong name */
  status = ml_pipeline_valve_get_handle (handle, "wrongname", &valve_h);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline valve
 * @detail Failure case to handle valve element with invalid param.
 */
TEST (nnstreamer_capi_valve, failure_04_n)
{
  ml_pipeline_h handle;
  ml_pipeline_valve_h valve_h;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : invalid type */
  status = ml_pipeline_valve_get_handle (handle, "sinkx", &valve_h);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline valve
 * @detail Failure case to handle valve element with invalid param.
 */
TEST (nnstreamer_capi_valve, failure_05_n)
{
  ml_pipeline_h handle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : handle */
  status = ml_pipeline_valve_get_handle (handle, "valvex", NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

G_LOCK_DEFINE_STATIC (callback_lock);
/**
 * @brief A tensor-sink callback for sink handle in a pipeline
 */
static void
test_sink_callback_dm01 (
    const ml_tensors_data_h data, const ml_tensors_info_h info, void *user_data)
{
  gchar *filepath = (gchar *)user_data;
  unsigned int i, num = 0;
  void *data_ptr;
  size_t data_size;
  int status;
  FILE *fp = g_fopen (filepath, "a");

  if (fp == NULL)
    return;

  G_LOCK (callback_lock);
  ml_tensors_info_get_count (info, &num);

  for (i = 0; i < num; i++) {
    status = ml_tensors_data_get_tensor_data (data, i, &data_ptr, &data_size);
    if (status == ML_ERROR_NONE)
      fwrite (data_ptr, data_size, 1, fp);
  }
  G_UNLOCK (callback_lock);

  fclose (fp);
}

/**
 * @brief A tensor-sink callback for sink handle in a pipeline
 */
static void
test_sink_callback_count (
    const ml_tensors_data_h data, const ml_tensors_info_h info, void *user_data)
{
  guint *count = (guint *)user_data;

  G_LOCK (callback_lock);
  *count = *count + 1;
  G_UNLOCK (callback_lock);
}

/**
 * @brief Pipeline state changed callback
 */
static void
test_pipe_state_callback (ml_pipeline_state_e state, void *user_data)
{
  TestPipeState *pipe_state;

  G_LOCK (callback_lock);
  pipe_state = (TestPipeState *)user_data;

  switch (state) {
  case ML_PIPELINE_STATE_PAUSED:
    pipe_state->paused = TRUE;
    break;
  case ML_PIPELINE_STATE_PLAYING:
    pipe_state->playing = TRUE;
    break;
  default:
    break;
  }
  G_UNLOCK (callback_lock);
}

/**
 * @brief compare the two files.
 */
static int
file_cmp (const gchar *f1, const gchar *f2)
{
  gboolean r;
  gchar *content1 = NULL;
  gchar *content2 = NULL;
  gsize len1, len2;
  int cmp = 0;

  r = g_file_get_contents (f1, &content1, &len1, NULL);
  if (r != TRUE)
    return -1;

  r = g_file_get_contents (f2, &content2, &len2, NULL);
  if (r != TRUE) {
    g_free (content1);
    return -2;
  }

  if (len1 == len2) {
    cmp = memcmp (content1, content2, len1);
  } else {
    cmp = 1;
  }

  g_free (content1);
  g_free (content2);

  return cmp;
}

/**
 * @brief Wait until the change in pipeline status is done
 * @return ML_ERROR_NONE success, ML_ERROR_UNKNOWN if failed, ML_ERROR_TIMED_OUT if timeout happens.
 */
static int
waitPipelineStateChange (ml_pipeline_h handle, ml_pipeline_state_e state, guint timeout_ms)
{
  int status = ML_ERROR_UNKNOWN;
  guint counter = 0;
  ml_pipeline_state_e cur_state = ML_PIPELINE_STATE_NULL;

  do {
    status = ml_pipeline_get_state (handle, &cur_state);
    EXPECT_EQ (status, ML_ERROR_NONE);
    if (cur_state == ML_PIPELINE_STATE_UNKNOWN)
      return ML_ERROR_UNKNOWN;
    if (cur_state == state)
      return ML_ERROR_NONE;
    g_usleep (10000);
  } while ((timeout_ms / 10) >= counter++);

  return ML_ERROR_TIMED_OUT;
}

/**
 * @brief Test NNStreamer pipeline sink
 */
TEST (nnstreamer_capi_sink, dummy_01)
{
  const gchar *_tmpdir = g_get_tmp_dir ();
  const gchar *_dirname = "nns-tizen-XXXXXX";
  gchar *fullpath = g_build_path ("/", _tmpdir, _dirname, NULL);
  gchar *dir = g_mkdtemp ((gchar *)fullpath);

  ASSERT_NE (dir, (gchar *)NULL);

  gchar *file1 = g_build_path ("/", dir, "original", NULL);
  gchar *file2 = g_build_path ("/", dir, "sink", NULL);
  gchar *pipeline = g_strdup_printf (
      "videotestsrc num-buffers=3 ! videoconvert ! videoscale ! video/x-raw,format=BGRx,width=64,height=48,famerate=30/1 ! tee name=t t. ! queue ! filesink location=\"%s\" buffer-mode=unbuffered t. ! queue ! tensor_converter ! tensor_sink name=sinkx",
      file1);
  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle;
  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback_dm01, file2, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle != NULL);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = waitPipelineStateChange (handle, ML_PIPELINE_STATE_PLAYING, 200);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* 200ms. Give enough time for three frames to flow. */
  g_usleep (200000);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (10000); /* 10ms. Wait a bit. */

  status = ml_pipeline_sink_unregister (sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);

  /* File Comparison to check the integrity */
  EXPECT_EQ (file_cmp (file1, file2), 0);

  g_free (fullpath);
  g_free (file1);
  g_free (file2);
}

/**
 * @brief Test NNStreamer pipeline sink
 */
TEST (nnstreamer_capi_sink, dummy_02)
{
  ml_pipeline_h handle;
  ml_pipeline_state_e state;
  ml_pipeline_sink_h sinkhandle;
  gchar *pipeline;
  int status;
  guint *count_sink;
  TestPipeState *pipe_state;

  /* pipeline with appsink */
  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! tensor_converter ! appsink name=sinkx sync=false");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  pipe_state = (TestPipeState *)g_new0 (TestPipeState, 1);
  ASSERT_TRUE (pipe_state != NULL);

  status = ml_pipeline_construct (pipeline, test_pipe_state_callback, pipe_state, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle != NULL);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (100000); /* 100ms. Let a few frames flow. */
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PLAYING);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (10000); /* 10ms. Wait a bit. */

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PAUSED);

  status = ml_pipeline_sink_unregister (sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_TRUE (*count_sink > 0U);
  EXPECT_TRUE (pipe_state->paused);
  EXPECT_TRUE (pipe_state->playing);

  g_free (pipeline);
  g_free (count_sink);
  g_free (pipe_state);
}

/**
 * @brief Test NNStreamer pipeline sink
 */
TEST (nnstreamer_capi_sink, register_duplicated)
{
  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle0, sinkhandle1;
  gchar *pipeline;
  int status;
  guint *count_sink0, *count_sink1;
  TestPipeState *pipe_state;

  /* pipeline with appsink */
  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! tensor_converter ! appsink name=sinkx sync=false");
  count_sink0 = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink0 != NULL);
  *count_sink0 = 0;

  count_sink1 = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink1 != NULL);
  *count_sink1 = 0;

  pipe_state = (TestPipeState *)g_new0 (TestPipeState, 1);
  ASSERT_TRUE (pipe_state != NULL);

  status = ml_pipeline_construct (pipeline, test_pipe_state_callback, pipe_state, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback_count, count_sink0, &sinkhandle0);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle0 != NULL);

  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback_count, count_sink1, &sinkhandle1);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle1 != NULL);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (100000); /* 100ms. Let a few frames flow. */

  status = ml_pipeline_sink_unregister (sinkhandle0);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sinkhandle1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_TRUE (*count_sink0 > 0U);
  EXPECT_TRUE (*count_sink1 > 0U);
  EXPECT_TRUE (pipe_state->paused);
  EXPECT_TRUE (pipe_state->playing);

  g_free (pipeline);
  g_free (count_sink0);
  g_free (count_sink1);
  g_free (pipe_state);
}

/**
 * @brief Test NNStreamer pipeline sink
 * @detail Failure case to register callback with invalid param.
 */
TEST (nnstreamer_capi_sink, failure_01_n)
{
  ml_pipeline_sink_h sinkhandle;
  int status;
  guint *count_sink;

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  /* invalid param : pipe */
  status = ml_pipeline_sink_register (
      NULL, "sinkx", test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline sink
 * @detail Failure case to register callback with invalid param.
 */
TEST (nnstreamer_capi_sink, failure_02_n)
{
  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle;
  gchar *pipeline;
  int status;
  guint *count_sink;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : name */
  status = ml_pipeline_sink_register (
      handle, NULL, test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  g_free (pipeline);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline sink
 * @detail Failure case to register callback with invalid param.
 */
TEST (nnstreamer_capi_sink, failure_03_n)
{
  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle;
  gchar *pipeline;
  int status;
  guint *count_sink;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : wrong name */
  status = ml_pipeline_sink_register (
      handle, "wrongname", test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  g_free (pipeline);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline sink
 * @detail Failure case to register callback with invalid param.
 */
TEST (nnstreamer_capi_sink, failure_04_n)
{
  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle;
  gchar *pipeline;
  int status;
  guint *count_sink;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : invalid type */
  status = ml_pipeline_sink_register (
      handle, "valvex", test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  g_free (pipeline);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline sink
 * @detail Failure case to register callback with invalid param.
 */
TEST (nnstreamer_capi_sink, failure_05_n)
{
  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle;
  gchar *pipeline;
  int status;
  guint *count_sink;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : callback */
  status = ml_pipeline_sink_register (handle, "sinkx", NULL, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  g_free (pipeline);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline sink
 * @detail Failure case to register callback with invalid param.
 */
TEST (nnstreamer_capi_sink, failure_06_n)
{
  ml_pipeline_h handle;
  gchar *pipeline;
  int status;
  guint *count_sink;

  pipeline = g_strdup ("videotestsrc num-buffers=3 ! videoconvert ! valve name=valvex ! tensor_converter ! tensor_sink name=sinkx");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : handle */
  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback_count, count_sink, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  g_free (pipeline);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline src
 */
TEST (nnstreamer_capi_src, dummy_01)
{
  const gchar *_tmpdir = g_get_tmp_dir ();
  const gchar *_dirname = "nns-tizen-XXXXXX";
  gchar *fullpath = g_build_path ("/", _tmpdir, _dirname, NULL);
  gchar *dir = g_mkdtemp ((gchar *)fullpath);
  gchar *file1 = g_build_path ("/", dir, "output", NULL);
  gchar *pipeline = g_strdup_printf (
      "appsrc name=srcx ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! filesink location=\"%s\" buffer-mode=unbuffered",
      file1);
  ml_pipeline_h handle;
  ml_pipeline_state_e state;
  ml_pipeline_src_h srchandle;
  int status;
  ml_tensors_info_h info;
  ml_tensors_data_h data1, data2;
  unsigned int count = 0;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  ml_tensor_dimension dim = {
    0,
  };

  int i;
  uint8_t *uintarray1[10];
  uint8_t *uintarray2[10];
  uint8_t *content = NULL;
  gsize len;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (dir != NULL);
  for (i = 0; i < 10; i++) {
    uintarray1[i] = (uint8_t *)g_malloc (4);
    ASSERT_TRUE (uintarray1[i] != NULL);
    uintarray1[i][0] = i + 4;
    uintarray1[i][1] = i + 1;
    uintarray1[i][2] = i + 3;
    uintarray1[i][3] = i + 2;

    uintarray2[i] = (uint8_t *)g_malloc (4);
    ASSERT_TRUE (uintarray2[i] != NULL);
    uintarray2[i][0] = i + 3;
    uintarray2[i][1] = i + 2;
    uintarray2[i][2] = i + 1;
    uintarray2[i][3] = i + 4;
    /* These will be free'ed by gstreamer (ML_PIPELINE_BUF_POLICY_AUTO_FREE) */
    /** @todo Check whether gstreamer really deallocates this */
  }

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (10000); /* 10ms. Wait a bit. */
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status,
      ML_ERROR_NONE); /* At this moment, it can be READY, PAUSED, or PLAYING */
  EXPECT_NE (state, ML_PIPELINE_STATE_UNKNOWN);
  EXPECT_NE (state, ML_PIPELINE_STATE_NULL);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_tensors_info (srchandle, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_get_count (info, &count);
  EXPECT_EQ (count, 1U);

  ml_tensors_info_get_tensor_type (info, 0, &type);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  ml_tensors_info_get_tensor_dimension (info, 0, dim);
  EXPECT_EQ (dim[0], 4U);
  EXPECT_EQ (dim[1], 1U);
  EXPECT_EQ (dim[2], 1U);
  EXPECT_EQ (dim[3], 1U);

  status = ml_tensors_data_create (info, &data1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (info);

  status = ml_tensors_data_set_tensor_data (data1, 0, uintarray1[0], 4);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_input_data (srchandle, data1, ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000); /* 50ms. Wait a bit. */

  status = ml_pipeline_src_input_data (srchandle, data1, ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000); /* 50ms. Wait a bit. */

  status = ml_pipeline_src_release_handle (srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_tensors_info (srchandle, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_get_count (info, &count);
  EXPECT_EQ (count, 1U);

  ml_tensors_info_get_tensor_type (info, 0, &type);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  ml_tensors_info_get_tensor_dimension (info, 0, dim);
  EXPECT_EQ (dim[0], 4U);
  EXPECT_EQ (dim[1], 1U);
  EXPECT_EQ (dim[2], 1U);
  EXPECT_EQ (dim[3], 1U);

  for (i = 0; i < 10; i++) {
    status = ml_tensors_data_set_tensor_data (data1, 0, uintarray1[i], 4);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_pipeline_src_input_data (srchandle, data1, ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_tensors_data_create (info, &data2);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_tensors_data_set_tensor_data (data2, 0, uintarray2[i], 4);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_pipeline_src_input_data (srchandle, data2, ML_PIPELINE_BUF_POLICY_AUTO_FREE);
    EXPECT_EQ (status, ML_ERROR_NONE);

    g_usleep (50000); /* 50ms. Wait a bit. */
  }

  status = ml_pipeline_src_release_handle (srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (50000); /* Wait for the pipeline to flush all */

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);

  EXPECT_TRUE (g_file_get_contents (file1, (gchar **)&content, &len, NULL));
  EXPECT_EQ (len, 8U * 11);
  EXPECT_TRUE (content != nullptr);

  if (content && len == 88U) {
    for (i = 0; i < 10; i++) {
      EXPECT_EQ (content[i * 8 + 0 + 8], i + 4);
      EXPECT_EQ (content[i * 8 + 1 + 8], i + 1);
      EXPECT_EQ (content[i * 8 + 2 + 8], i + 3);
      EXPECT_EQ (content[i * 8 + 3 + 8], i + 2);
      EXPECT_EQ (content[i * 8 + 4 + 8], i + 3);
      EXPECT_EQ (content[i * 8 + 5 + 8], i + 2);
      EXPECT_EQ (content[i * 8 + 6 + 8], i + 1);
      EXPECT_EQ (content[i * 8 + 7 + 8], i + 4);
    }
  }

  g_free (content);
  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (data1);

  for (i = 0; i < 10; i++) {
    g_free (uintarray1[i]);
    g_free (uintarray2[i]);
  }

  g_free (fullpath);
  g_free (file1);
}

/**
 * @brief Test NNStreamer pipeline src
 * @detail Failure case when pipeline is NULL.
 */
TEST (nnstreamer_capi_src, failure_01_n)
{
  int status;
  ml_pipeline_src_h srchandle;

  status = ml_pipeline_src_get_handle (NULL, "dummy", &srchandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test NNStreamer pipeline src
 * @detail Failure case when the name of source node is wrong.
 */
TEST (nnstreamer_capi_src, failure_02_n)
{
  const char *pipeline = "appsrc name=mysource ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! valve name=valvex ! tensor_sink";
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;

  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : name */
  status = ml_pipeline_src_get_handle (handle, NULL, &srchandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline src
 * @detail Failure case when the name of source node is wrong.
 */
TEST (nnstreamer_capi_src, failure_03_n)
{
  const char *pipeline = "appsrc name=mysource ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! valve name=valvex ! tensor_sink";
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;

  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : wrong name */
  status = ml_pipeline_src_get_handle (handle, "wrongname", &srchandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline src
 * @detail Failure case when the name of source node is wrong.
 */
TEST (nnstreamer_capi_src, failure_04_n)
{
  const char *pipeline = "appsrc name=mysource ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! valve name=valvex ! tensor_sink";
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;

  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : invalid type */
  status = ml_pipeline_src_get_handle (handle, "valvex", &srchandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline src
 * @detail Failure case when the name of source node is wrong.
 */
TEST (nnstreamer_capi_src, failure_05_n)
{
  const char *pipeline = "appsrc name=mysource ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! valve name=valvex ! tensor_sink";
  ml_pipeline_h handle;

  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : handle */
  status = ml_pipeline_src_get_handle (handle, "mysource", NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline src
 * @detail Failure case when the number of tensors is 0 or bigger than ML_TENSOR_SIZE_LIMIT;
 */
TEST (nnstreamer_capi_src, failure_06_n)
{
  const char *pipeline = "appsrc name=srcx ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! tensor_sink";
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;
  ml_tensors_data_h data;
  ml_tensors_info_h info;

  int status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_tensors_info (srchandle, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (info, &data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* null data */
  status = ml_pipeline_src_input_data (srchandle, NULL, ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_src_release_handle (srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_destroy (data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Internal function to push dummy into appsrc.
 */
static void
test_src_cb_push_dummy (ml_pipeline_src_h src_handle)
{
  ml_tensors_data_h data;
  ml_tensors_info_h info;

  if (ml_pipeline_src_get_tensors_info (src_handle, &info) == ML_ERROR_NONE) {
    ml_tensors_data_create (info, &data);
    ml_pipeline_src_input_data (src_handle, data, ML_PIPELINE_BUF_POLICY_AUTO_FREE);
    ml_tensors_info_destroy (info);
  }
}

/**
 * @brief appsrc callback - need_data.
 */
static void
test_src_cb_need_data (ml_pipeline_src_h src_handle, unsigned int length,
    void *user_data)
{
  /* For test, push dummy if given src handles are same. */
  if (src_handle == user_data)
    test_src_cb_push_dummy (src_handle);
}

/**
 * @brief Test NNStreamer pipeline src callback.
 */
TEST (nnstreamer_capi_src, callback_replace)
{
  const char pipeline[] = "appsrc name=srcx ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! tensor_sink name=sinkx";
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle1, srchandle2;
  ml_pipeline_sink_h sinkhandle;
  ml_pipeline_src_callbacks_s callback = { 0, };
  guint *count_sink;
  int status;

  callback.need_data = test_src_cb_need_data;

  count_sink = (guint *) g_malloc0 (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_set_event_cb (srchandle1, &callback, srchandle1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (
    handle, "sinkx", test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  test_src_cb_push_dummy (srchandle1);
  g_usleep (100000);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_TRUE (*count_sink > 1U);

  /* Set new callback with new handle. */
  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* New callback will not push dummy. */
  status = ml_pipeline_src_set_event_cb (srchandle2, &callback, srchandle1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (100000);
  *count_sink = 0;
  test_src_cb_push_dummy (srchandle2);
  g_usleep (100000);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_TRUE (*count_sink == 1U);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline src callback.
 */
TEST (nnstreamer_capi_src, callback_invalid_param_01_n)
{
  const char pipeline[] = "appsrc name=srcx ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! tensor_sink";
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;
  ml_pipeline_src_callbacks_s callback = { 0, };
  int status;

  callback.need_data = test_src_cb_need_data;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param */
  status = ml_pipeline_src_set_event_cb (NULL, &callback, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer pipeline src callback.
 */
TEST (nnstreamer_capi_src, callback_invalid_param_02_n)
{
  const char pipeline[] = "appsrc name=srcx ! other/tensor,dimension=(string)4:1:1:1,type=(string)uint8,framerate=(fraction)0/1 ! tensor_sink";
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;
  int status;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param */
  status = ml_pipeline_src_set_event_cb (srchandle, NULL, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Check decoded orange.png with raw data.
 */
static void
check_orange_output (const ml_tensors_data_h data, const ml_tensors_info_h info, void *user_data)
{
  int status;
  size_t data_size;
  uint8_t *raw_content;
  gsize raw_content_len;
  gchar *orange_raw_file;

  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  orange_raw_file = g_build_filename (
      root_path, "tests", "test_models", "data", "orange.raw", NULL);
  ASSERT_TRUE (g_file_test (orange_raw_file, G_FILE_TEST_EXISTS));

  EXPECT_TRUE (g_file_get_contents (orange_raw_file, (gchar **) &raw_content, &raw_content_len, NULL));

  status = ml_tensors_data_get_tensor_data (data, 0, (void **) &data, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_EQ (raw_content_len, data_size);

  status = 0;
  for (size_t i = 0; i < data_size; ++i) {
    if (*(((uint8_t *) data) + i) != *(raw_content + i)) {
      status = 1;
      break;
    }
  }

  EXPECT_EQ (status, 0);

  g_free (raw_content);
  g_free (orange_raw_file);
}

/**
 * @brief Test NNStreamer pipeline src (appsrc with png file)
 */
TEST (nnstreamer_capi_src, pngfile)
{
  int status;

  ml_pipeline_h handle;
  ml_pipeline_sink_h sinkhandle;
  ml_pipeline_src_h srchandle;
  ml_pipeline_state_e state;

  ml_tensors_info_h in_info;
  ml_tensor_dimension in_dim;
  ml_tensors_data_h input = NULL;

  gchar *orange_png_file, *pipeline;
  uint8_t *content;
  gsize content_len;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");
  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* start pipeline test with valid model file */
  orange_png_file = g_build_filename (
      root_path, "tests", "test_models", "data", "orange.png", NULL);
  ASSERT_TRUE (g_file_test (orange_png_file, G_FILE_TEST_EXISTS));

  pipeline = g_strdup_printf (
    "appsrc name=srcx caps=image/png ! pngdec ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=224,height=224,framerate=0/1 ! tensor_converter ! tensor_sink name=sinkx sync=false async=false"
  );

  /* construct pipeline */
  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* set sink callback */
  status = ml_pipeline_sink_register (handle, "sinkx", check_orange_output, NULL, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* set src_handle */
  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* get the data of input png file */
  EXPECT_TRUE (g_file_get_contents (orange_png_file, (gchar **) &content, &content_len, NULL));

  /* set ml_tensors_info */
  ml_tensors_info_create (&in_info);
  in_dim[0] = content_len;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;

  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);

  wait_for_start (handle, state, status);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PLAYING);

  status = ml_tensors_data_set_tensor_data (input, 0, content, content_len);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_input_data (srchandle, input, ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (1000 * 1000);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (1000 * 1000);

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PAUSED);

  /* release handles and allocated memory */
  status = ml_pipeline_src_release_handle (srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);
  g_free (content);
  g_free (orange_png_file);
  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline switch
 */
TEST (nnstreamer_capi_switch, dummy_01)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  ml_pipeline_sink_h sinkhandle;
  ml_pipeline_switch_e type;
  ml_pipeline_state_e state;
  gchar *pipeline;
  int status;
  guint *count_sink;
  TestPipeState *pipe_state;
  gchar **node_list = NULL;

  pipeline = g_strdup (
      "input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
      "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
      "videotestsrc num-buffers=3 is-live=true ! videoconvert ! ins.sink_1");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  pipe_state = (TestPipeState *)g_new0 (TestPipeState, 1);
  ASSERT_TRUE (pipe_state != NULL);

  status = ml_pipeline_construct (pipeline, test_pipe_state_callback, pipe_state, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_switch_get_handle (handle, "ins", &type, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_PIPELINE_SWITCH_INPUT_SELECTOR);

  status = ml_pipeline_switch_get_pad_list (switchhandle, &node_list);
  EXPECT_EQ (status, ML_ERROR_NONE);

  if (node_list) {
    gchar *name = NULL;
    guint idx = 0;

    while ((name = node_list[idx]) != NULL) {
      EXPECT_TRUE (g_str_equal (name, "sink_0") || g_str_equal (name, "sink_1"));
      idx++;
      g_free (name);
    }

    EXPECT_EQ (idx, 2U);
    g_free (node_list);
  }

  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle != NULL);

  status = ml_pipeline_switch_select (switchhandle, "sink_1");
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (50000);
  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  wait_for_start (handle, state, status);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PLAYING);

  wait_pipeline_process_buffers (*count_sink, 3);
  g_usleep (300000); /* To check if more frames are coming in  */
  EXPECT_EQ (*count_sink, 3U);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_switch_release_handle (switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_TRUE (pipe_state->paused);
  EXPECT_TRUE (pipe_state->playing);

  g_free (pipeline);
  g_free (count_sink);
  g_free (pipe_state);
}

/**
 * @brief Test NNStreamer pipeline switch
 */
TEST (nnstreamer_capi_switch, dummy_02)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  ml_pipeline_sink_h sinkhandle0, sinkhandle1;
  ml_pipeline_switch_e type;
  gchar *pipeline;
  int status;
  guint *count_sink0, *count_sink1;
  gchar **node_list = NULL;

  /**
   * Prerolling problem
   * For running the test, set async=false in the sink element
   * when using an output selector.
   * The pipeline state can be changed to paused
   * after all sink element receive buffer.
   */
  pipeline = g_strdup ("videotestsrc is-live=true ! videoconvert ! tensor_converter ! output-selector name=outs "
                       "outs.src_0 ! tensor_sink name=sink0 async=false "
                       "outs.src_1 ! tensor_sink name=sink1 async=false");

  count_sink0 = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink0 != NULL);
  *count_sink0 = 0;

  count_sink1 = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink1 != NULL);
  *count_sink1 = 0;

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_switch_get_handle (handle, "outs", &type, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_PIPELINE_SWITCH_OUTPUT_SELECTOR);

  status = ml_pipeline_switch_get_pad_list (switchhandle, &node_list);
  EXPECT_EQ (status, ML_ERROR_NONE);

  if (node_list) {
    gchar *name = NULL;
    guint idx = 0;

    while ((name = node_list[idx]) != NULL) {
      EXPECT_TRUE (g_str_equal (name, "src_0") || g_str_equal (name, "src_1"));
      idx++;
      g_free (name);
    }

    EXPECT_EQ (idx, 2U);
    g_free (node_list);
  }

  status = ml_pipeline_sink_register (
      handle, "sink0", test_sink_callback_count, count_sink0, &sinkhandle0);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle0 != NULL);

  status = ml_pipeline_sink_register (
      handle, "sink1", test_sink_callback_count, count_sink1, &sinkhandle1);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle1 != NULL);

  status = ml_pipeline_switch_select (switchhandle, "src_1");
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (200000); /* 200ms. Let a few frames flow. */

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sinkhandle0);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sinkhandle1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_switch_release_handle (switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_EQ (*count_sink0, 0U);
  EXPECT_TRUE (*count_sink1 > 0U);

  g_free (pipeline);
  g_free (count_sink0);
  g_free (count_sink1);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_01_n)
{
  ml_pipeline_switch_h switchhandle;
  ml_pipeline_switch_e type;
  int status;

  /* invalid param : pipe */
  status = ml_pipeline_switch_get_handle (NULL, "ins", &type, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_02_n)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  ml_pipeline_switch_e type;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
                       "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
                       "videotestsrc num-buffers=3 ! videoconvert ! ins.sink_1");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : name */
  status = ml_pipeline_switch_get_handle (handle, NULL, &type, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_03_n)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  ml_pipeline_switch_e type;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
                       "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
                       "videotestsrc num-buffers=3 ! videoconvert ! ins.sink_1");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : wrong name */
  status = ml_pipeline_switch_get_handle (handle, "wrongname", &type, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_04_n)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  ml_pipeline_switch_e type;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
                       "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
                       "videotestsrc num-buffers=3 ! videoconvert ! ins.sink_1");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : invalid type */
  status = ml_pipeline_switch_get_handle (handle, "sinkx", &type, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_05_n)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_e type;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
                       "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
                       "videotestsrc num-buffers=3 ! videoconvert ! ins.sink_1");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : handle */
  status = ml_pipeline_switch_get_handle (handle, "ins", &type, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_06_n)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
                       "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
                       "videotestsrc num-buffers=3 ! videoconvert ! ins.sink_1");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* succesfully get switch handle if the param type is null */
  status = ml_pipeline_switch_get_handle (handle, "ins", NULL, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : handle */
  status = ml_pipeline_switch_select (NULL, "invalidpadname");
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_switch_release_handle (switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_07_n)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
                       "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
                       "videotestsrc num-buffers=3 ! videoconvert ! ins.sink_1");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* succesfully get switch handle if the param type is null */
  status = ml_pipeline_switch_get_handle (handle, "ins", NULL, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : pad name */
  status = ml_pipeline_switch_select (switchhandle, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_switch_release_handle (switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer pipeline switch
 * @detail Failure case to handle input-selector element with invalid param.
 */
TEST (nnstreamer_capi_switch, failure_08_n)
{
  ml_pipeline_h handle;
  ml_pipeline_switch_h switchhandle;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("input-selector name=ins ! tensor_converter ! tensor_sink name=sinkx "
                       "videotestsrc is-live=true ! videoconvert ! ins.sink_0 "
                       "videotestsrc num-buffers=3 ! videoconvert ! ins.sink_1");

  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* succesfully get switch handle if the param type is null */
  status = ml_pipeline_switch_get_handle (handle, "ins", NULL, &switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invalid param : wrong pad name */
  status = ml_pipeline_switch_select (switchhandle, "wrongpadname");
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_pipeline_switch_release_handle (switchhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test NNStreamer Utility for checking plugin availability (invalid param)
 */
TEST (nnstreamer_capi_util, plugin_availability_fail_invalid_01_n)
{
  int status;

  status = _ml_check_plugin_availability (NULL, "tensor_filter");
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer Utility for checking plugin availability (invalid param)
 */
TEST (nnstreamer_capi_util, plugin_availability_fail_invalid_02_n)
{
  int status;

  status = _ml_check_plugin_availability ("nnstreamer", NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer Utility for checking nnfw availability with custom option
 */
TEST (nnstreamer_capi_util, nnfw_availability_full_01)
{
  int status;
  bool result;

  status = ml_check_nnfw_availability_full (ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY, NULL, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);
}

/**
 * @brief Test NNStreamer Utility for checking nnfw availability with custom option (invalid param)
 */
TEST (nnstreamer_capi_util, nnfw_availability_full_02_n)
{
  int status;

  status = ml_check_nnfw_availability_full (ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer Utility for checking nnfw availability (invalid param)
 */
TEST (nnstreamer_capi_util, nnfw_availability_fail_invalid_01_n)
{
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer Utility for checking nnfw availability (invalid param)
 */
TEST (nnstreamer_capi_util, nnfw_availability_fail_invalid_02_n)
{
  bool result;
  int status;

  /* any is unknown nnfw type */
  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ANY, ML_NNFW_HW_ANY, &result);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer Utility for checking availability of Tensorflow-lite backend
 */
TEST (nnstreamer_capi_util, availability_01)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_ANY, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_AUTO, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_CPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_CPU_NEON, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_CPU_SIMD, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_GPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_NPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, is_enabled_tensorflow_lite);
}

/**
 * @brief Test NNStreamer Utility for checking availability of Tensorflow-lite backend
 */
TEST (nnstreamer_capi_util, availability_fail_01_n)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_NPU_MOVIDIUS, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_NPU_EDGE_TPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_NPU_VIVANTE, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW_LITE,
      ML_NNFW_HW_NPU_SR, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);
}

#ifdef ENABLE_TENSORFLOW
/**
 * @brief Test NNStreamer Utility for checking availability of Tensorflow backend
 */
TEST (nnstreamer_capi_util, availability_02)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW,
      ML_NNFW_HW_ANY, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW,
      ML_NNFW_HW_AUTO, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);
}

/**
 * @brief Test NNStreamer Utility for checking availability of Tensorflow backend
 */
TEST (nnstreamer_capi_util, availability_fail_02_n)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW,
      ML_NNFW_HW_NPU_VIVANTE, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_TENSORFLOW,
      ML_NNFW_HW_NPU_MOVIDIUS, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);
}
#endif /* ENABLE_TENSORFLOW */

/**
 * @brief Test NNStreamer Utility for checking availability of custom backend
 */
TEST (nnstreamer_capi_util, availability_03)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_CUSTOM_FILTER,
      ML_NNFW_HW_ANY, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_CUSTOM_FILTER,
      ML_NNFW_HW_AUTO, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);
}

/**
 * @brief Test NNStreamer Utility for checking availability of custom backend
 */
TEST (nnstreamer_capi_util, availability_fail_03_n)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_CUSTOM_FILTER,
      ML_NNFW_HW_CPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_CUSTOM_FILTER,
      ML_NNFW_HW_GPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);
}

#ifdef ENABLE_NNFW_RUNTIME
/**
 * @brief Test NNStreamer Utility for checking availability of NNFW
 */
TEST (nnstreamer_capi_util, availability_04)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_NNFW, ML_NNFW_HW_ANY, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_NNFW, ML_NNFW_HW_AUTO, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_NNFW, ML_NNFW_HW_CPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_NNFW, ML_NNFW_HW_GPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_NNFW, ML_NNFW_HW_NPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);
}

/**
 * @brief Test NNStreamer Utility for checking availability of NNFW
 */
TEST (nnstreamer_capi_util, availability_fail_04_n)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_NNFW,
      ML_NNFW_HW_NPU_SR, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_NNFW,
      ML_NNFW_HW_NPU_MOVIDIUS, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);
}
#endif /* ENABLE_NNFW_RUNTIME */

#ifdef ENABLE_MOVIDIUS_NCSDK2
/**
 * @brief Test NNStreamer Utility for checking availability of NCSDK2
 */
TEST (nnstreamer_capi_util, availability_05)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_MVNC,
      ML_NNFW_HW_ANY, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_MVNC,
      ML_NNFW_HW_AUTO, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_MVNC,
      ML_NNFW_HW_NPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_MVNC,
      ML_NNFW_HW_NPU_MOVIDIUS, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);
}

/**
 * @brief Test NNStreamer Utility for checking availability of NCSDK2
 */
TEST (nnstreamer_capi_util, availability_fail_05_n)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_MVNC,
      ML_NNFW_HW_CPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_MVNC,
      ML_NNFW_HW_GPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);
}
#endif /* ENABLE_MOVIDIUS_NCSDK2 */

#ifdef ENABLE_ARMNN
/**
 * @brief Test NNStreamer Utility for checking availability of ARMNN
 */
TEST (nnstreamer_capi_util, availability_06)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ARMNN,
      ML_NNFW_HW_ANY, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ARMNN,
      ML_NNFW_HW_AUTO, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ARMNN,
      ML_NNFW_HW_CPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ARMNN,
      ML_NNFW_HW_CPU_NEON, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ARMNN,
      ML_NNFW_HW_GPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, true);
}

/**
 * @brief Test NNStreamer Utility for checking availability of ARMNN
 */
TEST (nnstreamer_capi_util, availability_fail_06_n)
{
  bool result;
  int status;

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ARMNN,
      ML_NNFW_HW_NPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);

  status = ml_check_nnfw_availability (ML_NNFW_TYPE_ARMNN,
      ML_NNFW_HW_NPU_EDGE_TPU, &result);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (result, false);
}
#endif /** ENABLE_ARMNN */

/**
 * @brief Test NNStreamer Utility for checking an element availability
 */
TEST (nnstreamer_capi_util, element_available_01_p)
{
  bool available;
  int status, n_elems,  i;
  /**
   * If the allowed element list of nnstreamer is changed, this should also be changed.
   * https://github.com/nnstreamer/nnstreamer/blob/main/packaging/nnstreamer.spec#L642 (# Element allowance in Tizen)
   */
  const gchar *allowed = "tensor_converter tensor_filter tensor_query_serversrc capsfilter input-selector output-selector queue tee valve appsink appsrc audioconvert audiorate audioresample audiomixer videoconvert videocrop videorate videoscale videoflip videomixer compositor fakesrc fakesink filesrc filesink audiotestsrc videotestsrc jpegparse jpegenc jpegdec pngenc pngdec tcpclientsink tcpclientsrc tcpserversink tcpserversrc xvimagesink ximagesink evasimagesink evaspixmapsink glimagesink theoraenc lame vorbisenc wavenc volume oggmux avimux matroskamux v4l2src avsysvideosrc camerasrc tvcamerasrc pulsesrc fimcconvert tizenwlsink gdppay gdpdepay join rtpdec rtspsrc rtspclientsink zmqsrc zmqsink mqttsrc mqttsink udpsrc udpsink multiudpsink audioamplify audiochebband audiocheblimit audiodynamic audioecho audiofirfilter audioiirfilter audioinvert audiokaraoke audiopanorama audiowsincband audiowsinclimit scaletempo stereo";
  /** This not_allowed list is written only for testing. */
  const gchar *not_allowed = "videobox videobalance aasink adder alpha alsasink x264enc ximagesrc webpenc wavescope v4l2sink v4l2radio urisourcebin uridecodebin typefind timeoverlay rtpstreampay rtpsession rtpgstpay queue2 fdsink fdsrc chromium capssetter cairooverlay autovideosink";
  gchar **elements;
  gboolean restricted;

  restricted = nnsconf_get_custom_value_bool ("element-restriction",
      "enable_element_restriction", FALSE);

  /* element restriction is disabled */
  if (!restricted)
    return;

  elements = g_strsplit (allowed, " ", -1);
  n_elems = g_strv_length (elements);

  for (i = 0; i < n_elems; i++) {
    /** If the plugin is not installed, the availability of the element cannot be tested. */
    GstElementFactory *factory = gst_element_factory_find (elements[i]);

    if (factory) {
      status = ml_check_element_availability (elements[i], &available);
      EXPECT_EQ (status, ML_ERROR_NONE);
      EXPECT_EQ (available, true);
      gst_object_unref (factory);
    }
  }
  g_strfreev (elements);

  elements = g_strsplit (not_allowed, " ", -1);
  n_elems = g_strv_length (elements);

  for (i = 0; i < n_elems; i++) {
    GstElementFactory *factory = gst_element_factory_find (elements[i]);
    if (factory) {
      status = ml_check_element_availability (elements[i], &available);
      EXPECT_EQ (status, ML_ERROR_NONE);
      EXPECT_EQ (available, false);
      gst_object_unref (factory);
    }
  }
  g_strfreev (elements);
}

/**
 * @brief Test NNStreamer Utility for checking an element availability (null param)
 */
TEST (nnstreamer_capi_util, element_available_02_n)
{
  bool available;
  int status;

  status = ml_check_element_availability (nullptr, &available);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_check_element_availability ("tensor_filter", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer Utility for checking an element availability (invalid element name)
 */
TEST (nnstreamer_capi_util, element_available_03_n)
{
  bool available;
  int status;

  status = ml_check_element_availability ("invalid-elem", &available);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (available, false);
}

/**
 * @brief Test NNStreamer Utility for checking tensors info handle
 */
TEST (nnstreamer_capi_util, tensors_info)
{
  ml_tensors_info_h info;
  ml_tensor_dimension in_dim, out_dim;
  ml_tensor_type_e out_type;
  gchar *out_name;
  size_t data_size;
  int status;

  status = ml_tensors_info_create (&info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  in_dim[0] = 3;
  in_dim[1] = 300;
  in_dim[2] = 300;
  in_dim[3] = 1;

  /* add tensor info */
  status = ml_tensors_info_set_count (info, 2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_type (info, 1, ML_TENSOR_TYPE_FLOAT64);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 1, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_name (info, 1, "tensor-name-test");
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_type (info, 2, ML_TENSOR_TYPE_UINT64);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
  status = ml_tensors_info_set_tensor_dimension (info, 2, in_dim);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* get tensor info */
  status = ml_tensors_info_get_tensor_type (info, 0, &out_type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (out_type, ML_TENSOR_TYPE_UINT8);

  status = ml_tensors_info_get_tensor_dimension (info, 0, out_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (out_dim[0], 3U);
  EXPECT_EQ (out_dim[1], 300U);
  EXPECT_EQ (out_dim[2], 300U);
  EXPECT_EQ (out_dim[3], 1U);

  status = ml_tensors_info_get_tensor_name (info, 0, &out_name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (out_name == NULL);

  status = ml_tensors_info_get_tensor_type (info, 1, &out_type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (out_type, ML_TENSOR_TYPE_FLOAT64);

  status = ml_tensors_info_get_tensor_dimension (info, 1, out_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (out_dim[0], 3U);
  EXPECT_EQ (out_dim[1], 300U);
  EXPECT_EQ (out_dim[2], 300U);
  EXPECT_EQ (out_dim[3], 1U);

  status = ml_tensors_info_get_tensor_name (info, 1, &out_name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (out_name && g_str_equal (out_name, "tensor-name-test"));
  g_free (out_name);

  status = ml_tensors_info_get_tensor_type (info, 2, &out_type);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_get_tensor_dimension (info, 2, out_dim);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_get_tensor_name (info, 2, &out_name);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* get tensor size */
  status = ml_tensors_info_get_tensor_size (info, 0, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (data_size == (3 * 300 * 300));

  status = ml_tensors_info_get_tensor_size (info, 1, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (data_size == (3 * 300 * 300 * 8));

  status = ml_tensors_info_get_tensor_size (info, -1, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (data_size == ((3 * 300 * 300) + (3 * 300 * 300 * 8)));

  status = ml_tensors_info_get_tensor_size (info, 2, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test NNStreamer Utility for checking extended tensors info handle
 */
TEST (nnstreamer_capi_util, tensors_info_extended)
{
  ml_tensors_info_h info;
  ml_tensor_dimension in_dim, out_dim;
  ml_tensor_type_e out_type;
  gchar *out_name;
  size_t data_size;
  int status, i;

  status = ml_tensors_info_create_extended (&info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0 ;i < ML_TENSOR_RANK_LIMIT ; i++) {
    in_dim[i] = i % 4 + 1;
  }

  /* add tensor info */
  status = ml_tensors_info_set_count (info, 2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_type (info, 1, ML_TENSOR_TYPE_FLOAT64);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 1, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_name (info, 1, "tensor-name-test");
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* get tensor info */
  status = ml_tensors_info_get_tensor_type (info, 0, &out_type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (out_type, ML_TENSOR_TYPE_UINT8);

  status = ml_tensors_info_get_tensor_dimension (info, 0, out_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0 ;i < ML_TENSOR_RANK_LIMIT ; i++) {
    EXPECT_EQ (out_dim[i], i % 4 + 1);
  }

  status = ml_tensors_info_get_tensor_name (info, 0, &out_name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (out_name == NULL);

  status = ml_tensors_info_get_tensor_type (info, 1, &out_type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (out_type, ML_TENSOR_TYPE_FLOAT64);

  status = ml_tensors_info_get_tensor_dimension (info, 1, out_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0 ;i < ML_TENSOR_RANK_LIMIT ; i++) {
    EXPECT_EQ (out_dim[i], i % 4 + 1);
  }

  status = ml_tensors_info_get_tensor_name (info, 1, &out_name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (out_name && g_str_equal (out_name, "tensor-name-test"));
  g_free (out_name);

  /* get tensor size */
  status = ml_tensors_info_get_tensor_size (info, 0, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (data_size == (2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4));

  status = ml_tensors_info_get_tensor_size (info, 1, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (data_size == ((2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4)* 8));

  status = ml_tensors_info_get_tensor_size (info, -1, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (data_size == (((2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4)) + ((2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4) * (2 * 3 * 4)* 8)));

  status = ml_tensors_info_destroy (info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions
 */
TEST (nnstreamer_capi_util, compare_info)
{
  ml_tensors_info_h info1, info2;
  ml_tensor_dimension dim;
  int status;

  status = ml_tensors_info_create (&info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_create (&info2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  dim[0] = 3;
  dim[1] = 4;
  dim[2] = 4;
  dim[3] = 1;

  ml_tensors_info_set_count (info1, 1);
  ml_tensors_info_set_tensor_type (info1, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info1, 0, dim);

  ml_tensors_info_set_count (info2, 1);
  ml_tensors_info_set_tensor_type (info2, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info2, 0, dim);

  /* compare info */
  EXPECT_TRUE (ml_tensors_info_is_equal (info1, info2));

  /* change type */
  ml_tensors_info_set_tensor_type (info2, 0, ML_TENSOR_TYPE_UINT16);
  EXPECT_FALSE (ml_tensors_info_is_equal (info1, info2));

  /* validate info */
  EXPECT_TRUE (ml_tensors_info_is_valid (info2));

  /* validate invalid dimension */
  dim[3] = 0;
  ml_tensors_info_set_tensor_dimension (info2, 0, dim);
  EXPECT_FALSE (ml_tensors_info_is_valid (info2));

  status = ml_tensors_info_destroy (info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (info2);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions
 */
TEST (nnstreamer_capi_util, compare_info_extended)
{
  ml_tensors_info_h info1, info2;
  ml_tensor_dimension dim;
  int status, i;

  status = ml_tensors_info_create_extended (&info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_create_extended (&info2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0 ;i < ML_TENSOR_RANK_LIMIT ; i++) {
    dim[i] = i + 1;
  }

  ml_tensors_info_set_count (info1, 1);
  ml_tensors_info_set_tensor_type (info1, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info1, 0, dim);

  ml_tensors_info_set_count (info2, 1);
  ml_tensors_info_set_tensor_type (info2, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info2, 0, dim);

  /* compare info */
  EXPECT_TRUE (ml_tensors_info_is_equal (info1, info2));

  /* change type */
  ml_tensors_info_set_tensor_type (info2, 0, ML_TENSOR_TYPE_UINT16);
  EXPECT_FALSE (ml_tensors_info_is_equal (info1, info2));

  /* validate info */
  EXPECT_TRUE (ml_tensors_info_is_valid (info2));

  /* validate invalid dimension */
  dim[3] = 0;
  ml_tensors_info_set_tensor_dimension (info2, 0, dim);
  EXPECT_FALSE (ml_tensors_info_is_valid (info2));

  status = ml_tensors_info_destroy (info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (info2);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions
 */
TEST (nnstreamer_capi_util, compare_info_extended_n)
{
  ml_tensors_info_h info1, info2;
  ml_tensor_dimension dim;
  int status, i;

  status = ml_tensors_info_create_extended (&info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_create (&info2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0 ;i < ML_TENSOR_RANK_LIMIT ; i++) {
    dim[i] = i + 1;
  }

  ml_tensors_info_set_count (info1, 1);
  ml_tensors_info_set_tensor_type (info1, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info1, 0, dim);

  ml_tensors_info_set_count (info2, 1);
  ml_tensors_info_set_tensor_type (info2, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info2, 0, dim);

  /* compare info */
  EXPECT_FALSE (ml_tensors_info_is_equal (info1, info2));
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_create_1_n)
{
  int status = ml_tensors_info_create (nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_create_2_n)
{
  ml_tensors_info_h i;
  int status = _ml_tensors_info_create_from_gst (&i, nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_create_3_n)
{
  GstTensorsInfo gi;
  int status = _ml_tensors_info_create_from_gst (nullptr, &gi);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_create_4_n)
{
  int status = ml_tensors_info_create_extended (nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_destroy_n)
{
  int status = ml_tensors_info_destroy (nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_init_n)
{
  int status = _ml_tensors_info_initialize (nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_valid_01_n)
{
  bool valid;
  int status = ml_tensors_info_validate (nullptr, &valid);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_valid_02_n)
{
  ml_tensors_info_h info;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };
  int status;

  status = ml_tensors_info_create (&info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_set_count (info, 1);
  ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info, 0, dim);

  status = ml_tensors_info_validate (info, nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_comp_01_n)
{
  ml_tensors_info_h info;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };
  bool equal;
  int status;

  status = ml_tensors_info_create (&info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_set_count (info, 1);
  ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info, 0, dim);

  status = _ml_tensors_info_compare (nullptr, info, &equal);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_comp_02_n)
{
  ml_tensors_info_h info;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  bool equal;
  int status;

  status = ml_tensors_info_create (&info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_set_count (info, 1);
  ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info, 0, dim);

  status = _ml_tensors_info_compare (info, nullptr, &equal);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_comp_03_n)
{
  ml_tensors_info_h info1, info2;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };
  int status;

  status = ml_tensors_info_create (&info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_set_count (info1, 1);
  ml_tensors_info_set_tensor_type (info1, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info1, 0, dim);

  status = ml_tensors_info_create (&info2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_set_count (info2, 1);
  ml_tensors_info_set_tensor_type (info2, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info2, 0, dim);

  status = _ml_tensors_info_compare (info1, info2, nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (info2);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_comp_0)
{
  bool equal;
  ml_tensors_info_h info1, info2;
  ml_tensors_info_s *is;
  int status = ml_tensors_info_create (&info1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_create (&info2);
  ASSERT_EQ (status, ML_ERROR_NONE);

  is = (ml_tensors_info_s *)info1;
  is->num_tensors = 1;
  is = (ml_tensors_info_s *)info2;
  is->num_tensors = 2;

  status = _ml_tensors_info_compare (info1, info2, &equal);
  ASSERT_EQ (status, ML_ERROR_NONE);
  ASSERT_FALSE (equal);

  status = ml_tensors_info_destroy (info1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info2);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, info_comp_1)
{
  ml_tensors_info_h info1, info2;
  ml_tensor_dimension dim = { 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 };
  int status;
  bool equal;

  status = ml_tensors_info_create (&info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_set_count (info1, 1);
  ml_tensors_info_set_tensor_type (info1, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info1, 0, dim);

  status = ml_tensors_info_create_extended (&info2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_set_count (info2, 1);
  ml_tensors_info_set_tensor_type (info2, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (info2, 0, dim);

  status = _ml_tensors_info_compare (info1, info2, &equal);
  ASSERT_EQ (status, ML_ERROR_NONE);
  ASSERT_FALSE (equal);

  status = ml_tensors_info_destroy (info1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (info2);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_count_n)
{
  int status = ml_tensors_info_set_count (nullptr, 1);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_count_1_n)
{
  unsigned int count;
  int status = ml_tensors_info_get_count (nullptr, &count);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_count_2_n)
{
  ml_tensors_info_h info;
  int status;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (info, nullptr);
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_tname_0_n)
{
  int status = ml_tensors_info_set_tensor_name (nullptr, 0, "fail");
  ASSERT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_tname_1_n)
{
  ml_tensors_info_h info;
  int status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 3);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_name (info, 3, "fail");
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_tname_1)
{
  ml_tensors_info_h info;
  int status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_name (info, 0, "first");
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_name (info, 0, "second");
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tname_01_n)
{
  int status;
  ml_tensors_info_h info;
  char *name = NULL;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_name (nullptr, 0, &name);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tname_02_n)
{
  int status;
  ml_tensors_info_h info;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_name (info, 0, nullptr);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tname_03_n)
{
  int status;
  ml_tensors_info_h info;
  char *name = NULL;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_name (info, 2, &name);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_ttype_01_n)
{
  int status;

  status = ml_tensors_info_set_tensor_type (nullptr, 0, ML_TENSOR_TYPE_INT16);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_ttype_02_n)
{
  int status;
  ml_tensors_info_h info;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UNKNOWN);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_ttype_03_n)
{
  int status;
  ml_tensors_info_h info;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_type (info, 2, ML_TENSOR_TYPE_INT16);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_ttype_01_n)
{
  int status;
  ml_tensor_type_e type;

  status = ml_tensors_info_get_tensor_type (nullptr, 0, &type);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_ttype_02_n)
{
  int status;
  ml_tensors_info_h info;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_type (info, 0, nullptr);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_ttype_03_n)
{
  int status;
  ml_tensors_info_h info;
  ml_tensor_type_e type;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_type (info, 2, &type);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_tdimension_01_n)
{
  int status;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  status = ml_tensors_info_set_tensor_dimension (nullptr, 0, dim);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_set_tdimension_02_n)
{
  int status;
  ml_tensors_info_h info;
  ml_tensor_dimension dim = { 1, 2, 3, 4 };

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_set_tensor_dimension (info, 2, dim);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tdimension_01_n)
{
  int status;
  ml_tensor_dimension dim;

  status = ml_tensors_info_get_tensor_dimension (nullptr, 0, dim);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tdimension_02_n)
{
  int status;
  ml_tensors_info_h info;
  ml_tensor_dimension dim;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_dimension (info, 2, dim);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tsize_01_n)
{
  int status;
  ml_tensors_info_h info;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_size (info, 0, nullptr);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tsize_02_n)
{
  int status;
  size_t data_size;

  status = ml_tensors_info_get_tensor_size (nullptr, 0, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_get_tsize_03_n)
{
  int status;
  size_t data_size;
  ml_tensors_info_h info;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_tensor_size (info, 2, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
  status = ml_tensors_info_get_tensor_size (info, 0, &data_size);
  EXPECT_TRUE (data_size == 0);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_clone)
{
  gint status;
  guint count = 0;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension in_dim, out_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;

  status = ml_tensors_info_create (&in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_create (&out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  in_dim[0] = 5;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;

  status = ml_tensors_info_set_count (in_info, 1);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_clone (out_info, in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_type (out_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  status = ml_tensors_info_get_tensor_dimension (out_info, 0, out_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (in_dim[0] == out_dim[0]);
  EXPECT_TRUE (in_dim[1] == out_dim[1]);
  EXPECT_TRUE (in_dim[2] == out_dim[2]);
  EXPECT_TRUE (in_dim[3] == out_dim[3]);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_clone_extended)
{
  gint status, i;
  guint count = 0;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension in_dim, out_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;

  status = ml_tensors_info_create_extended (&in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_create_extended (&out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0 ;i < ML_TENSOR_RANK_LIMIT ; i++) {
    in_dim[i] = i + 1;
  }

  status = ml_tensors_info_set_count (in_info, 1);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_clone (out_info, in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_type (out_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  status = ml_tensors_info_get_tensor_dimension (out_info, 0, out_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0 ;i < ML_TENSOR_RANK_LIMIT ; i++) {
    EXPECT_TRUE (in_dim[i] == out_dim[i]);
  }

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_clone_01_n)
{
  int status;
  ml_tensors_info_h src;

  status = ml_tensors_info_create (&src);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_clone (nullptr, src);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (src);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, info_clone_02_n)
{
  int status;
  ml_tensors_info_h desc;

  status = ml_tensors_info_create (&desc);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_clone (desc, nullptr);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (desc);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_create_01_n)
{
  int status;
  ml_tensors_data_h data = nullptr;

  status = ml_tensors_data_create (nullptr, &data);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_create_02_n)
{
  int status;
  ml_tensors_info_h info;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (info, nullptr);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_create_03_n)
{
  int status;
  ml_tensors_info_h info;
  ml_tensors_data_h data = nullptr;

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* invalid info */
  status = ml_tensors_data_create (info, &data);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_destroy (data);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (internal)
 */
TEST (nnstreamer_capi_util, data_create_internal_n)
{
  int status;

  status = _ml_tensors_data_create_no_alloc (NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_get_tdata_01_n)
{
  int status;
  size_t data_size;
  void *raw_data;

  status = ml_tensors_data_get_tensor_data (nullptr, 0, &raw_data, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_get_tdata_02_n)
{
  int status;
  size_t data_size;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_data_create (info, &data);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_get_tensor_data (data, 0, nullptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_destroy (data);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_get_tdata_03_n)
{
  int status;
  void *raw_data;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_data_create (info, &data);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_get_tensor_data (data, 0, &raw_data, nullptr);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_destroy (data);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_get_tdata_04_n)
{
  int status;
  size_t data_size;
  void *raw_data;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_data_create (info, &data);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_get_tensor_data (data, 2, &raw_data, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_destroy (data);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_set_tdata_01_n)
{
  int status;
  void *raw_data;

  raw_data = g_malloc (1024); /* larger than tensor */

  status = ml_tensors_data_set_tensor_data (nullptr, 0, raw_data, 16);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  g_free (raw_data);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_set_tdata_02_n)
{
  int status;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_data_create (info, &data);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_set_tensor_data (data, 0, nullptr, 16);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_destroy (data);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_set_tdata_03_n)
{
  int status;
  void *raw_data;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  raw_data = g_malloc (1024); /* larger than tensor */

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_data_create (info, &data);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_set_tensor_data (data, 2, raw_data, 16);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_destroy (data);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  g_free (raw_data);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_set_tdata_04_n)
{
  int status;
  void *raw_data;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  raw_data = g_malloc (1024); /* larger than tensor */

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_data_create (info, &data);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_set_tensor_data (data, 0, raw_data, 0);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_destroy (data);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  g_free (raw_data);
}

/**
 * @brief Test utility functions (public)
 */
TEST (nnstreamer_capi_util, data_set_tdata_05_n)
{
  int status;
  void *raw_data;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 2, 2, 2, 2 };

  raw_data = g_malloc (1024); /* larger than tensor */

  status = ml_tensors_info_create (&info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (info, 1);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UINT8);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_data_create (info, &data);
  ASSERT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_set_tensor_data (data, 0, raw_data, 1024);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_destroy (data);
  ASSERT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_destroy (info);
  ASSERT_EQ (status, ML_ERROR_NONE);
  g_free (raw_data);
}

/**
 * @brief Test utility functions - clone data.
 */
TEST (nnstreamer_capi_util, data_clone_01_p)
{
  int status;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensors_data_h data_out;
  ml_tensor_dimension dim = { 5, 1, 1, 1 };
  const int raw_data[5] = { 10, 20, 30, 40, 50 };
  int *result = nullptr;
  size_t data_size, result_size;

  ml_tensors_info_create (&info);
  ml_tensors_info_set_count (info, 1);
  ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ml_tensors_info_get_tensor_size (info, 0, &data_size);

  ml_tensors_data_create (info, &data);
  ml_tensors_data_set_tensor_data (data, 0, (const void *) raw_data, data_size);

  /* test code : clone data and compare raw value. */
  status = ml_tensors_data_clone (data, &data_out);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_get_tensor_data (data_out, 0, (void **) &result, &result_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  for (unsigned int i = 0; i < 5; i++)
    EXPECT_EQ (result[i], raw_data[i]);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (data);
  ml_tensors_data_destroy (data_out);
}

/**
 * @brief Test utility functions - clone data.
 */
TEST (nnstreamer_capi_util, data_clone_02_n)
{
  int status;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensor_dimension dim = { 5, 1, 1, 1 };

  ml_tensors_info_create (&info);
  ml_tensors_info_set_count (info, 1);
  ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ml_tensors_data_create (info, &data);

  status = ml_tensors_data_clone (data, nullptr);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (data);
}

/**
 * @brief Test utility functions - clone data.
 */
TEST (nnstreamer_capi_util, data_clone_03_n)
{
  int status;
  ml_tensors_data_h data_out;

  status = ml_tensors_data_clone (nullptr, &data_out);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);
}

/**
 * @brief Test utility functions - clone data.
 */
TEST (nnstreamer_capi_util, data_clone_04_p)
{
  int status, i;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  ml_tensors_data_h data_out;
  ml_tensor_dimension dim = { 5, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  int raw_data[25];
  int *result = nullptr;
  size_t data_size, result_size;

  for (i = 0; i < 25; i++)
    raw_data[i] = i;

  ml_tensors_info_create_extended (&info);
  ml_tensors_info_set_count (info, 1);
  ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (info, 0, dim);
  ml_tensors_info_get_tensor_size (info, 0, &data_size);

  ml_tensors_data_create (info, &data);
  ml_tensors_data_set_tensor_data (data, 0, (const void *) raw_data, data_size);

  /* test code : clone data and compare raw value. */
  status = ml_tensors_data_clone (data, &data_out);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_get_tensor_data (data_out, 0, (void **) &result, &result_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  for (unsigned int i = 0; i < 25; i++)
    EXPECT_EQ (result[i], raw_data[i]);

  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (data);
  ml_tensors_data_destroy (data_out);
}

/**
 * @brief Test to replace string.
 */
TEST (nnstreamer_capi_util, replaceStr01)
{
  gchar *result;
  guint changed;

  result = g_strdup ("sourceelement ! parser ! converter ! format ! converter ! format ! converter ! sink");

  result = _ml_replace_string (result, "sourceelement", "src", NULL, &changed);
  EXPECT_EQ (changed, 1U);
  EXPECT_STREQ (result, "src ! parser ! converter ! format ! converter ! format ! converter ! sink");

  result = _ml_replace_string (result, "format", "fmt", NULL, &changed);
  EXPECT_EQ (changed, 2U);
  EXPECT_STREQ (result, "src ! parser ! converter ! fmt ! converter ! fmt ! converter ! sink");

  result = _ml_replace_string (result, "converter", "conv", NULL, &changed);
  EXPECT_EQ (changed, 3U);
  EXPECT_STREQ (result, "src ! parser ! conv ! fmt ! conv ! fmt ! conv ! sink");

  result = _ml_replace_string (result, "invalidname", "invalid", NULL, &changed);
  EXPECT_EQ (changed, 0U);
  EXPECT_STREQ (result, "src ! parser ! conv ! fmt ! conv ! fmt ! conv ! sink");

  g_free (result);
}

/**
 * @brief Test to replace string.
 */
TEST (nnstreamer_capi_util, replaceStr02)
{
  gchar *result;
  guint changed;

  result = g_strdup ("source! parser ! sources ! mysource ! source ! format !source! conv source");

  result = _ml_replace_string (result, "source", "src", " !", &changed);
  EXPECT_EQ (changed, 4U);
  EXPECT_STREQ (result, "src! parser ! sources ! mysource ! src ! format !src! conv src");

  result = _ml_replace_string (result, "src", "mysource", "! ", &changed);
  EXPECT_EQ (changed, 4U);
  EXPECT_STREQ (result, "mysource! parser ! sources ! mysource ! mysource ! format !mysource! conv mysource");

  result = _ml_replace_string (result, "source", "src", NULL, &changed);
  EXPECT_EQ (changed, 6U);
  EXPECT_STREQ (result, "mysrc! parser ! srcs ! mysrc ! mysrc ! format !mysrc! conv mysrc");

  result = _ml_replace_string (result, "mysrc", "src", ";", &changed);
  EXPECT_EQ (changed, 0U);
  EXPECT_STREQ (result, "mysrc! parser ! srcs ! mysrc ! mysrc ! format !mysrc! conv mysrc");

  g_free (result);
}

/**
 * @brief Test to replace string.
 */
TEST (nnstreamer_capi_util, replaceStr03)
{
  gchar *result;
  guint changed;

  result = g_strdup ("source! parser name=source ! sources ! mysource ! source prop=temp ! source. ! filter model=\"source\" ! sink");

  result = _ml_replace_string (result, "source", "CHANGED", " !", &changed);
  EXPECT_EQ (changed, 2U);
  EXPECT_STREQ (result, "CHANGED! parser name=source ! sources ! mysource ! CHANGED prop=temp ! source. ! filter model=\"source\" ! sink");

  g_free (result);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_handle()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_handle_00_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_handle()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_handle_01_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_handle (handle, nullptr, &vsrc_h);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "WRONG_PROPERTY_NAME", &vsrc_h);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_release_handle()` API and check its results.
 */
TEST (nnstreamer_capi_element, release_handle_02_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_release_handle()` API and check its results.
 */
TEST (nnstreamer_capi_element, release_handle_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_bool_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  gchar *pipeline;
  int status;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_bool (selector_h, "sync-streams", FALSE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_bool (selector_h, "sync-streams", TRUE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_bool_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_bool (nullptr, "sync-streams", FALSE);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_bool_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  gchar *pipeline;
  int status;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_bool (selector_h, "WRONG_PROPERTY", TRUE);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_bool_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_bool (vscale_h, "sharpness", 10);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_bool_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  gchar *pipeline;
  int ret_sync_streams;
  int status;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_bool (selector_h, "sync-streams", FALSE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_bool (selector_h, "sync-streams", &ret_sync_streams);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_sync_streams, FALSE);

  status = ml_pipeline_element_set_property_bool (selector_h, "sync-streams", TRUE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_bool (selector_h, "sync-streams", &ret_sync_streams);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_sync_streams, TRUE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_bool_02_n)
{
  int ret_sync_streams;
  int status;

  /* Test Code */
  status = ml_pipeline_element_get_property_bool (nullptr, "sync-streams", &ret_sync_streams);
  ASSERT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_bool_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  gchar *pipeline;
  int ret_sync_streams;
  int status;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_bool (selector_h, "sync-streams", FALSE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_bool (selector_h, "WRONG_NAME", &ret_sync_streams);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_bool_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  gchar *pipeline;
  int status;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_bool (selector_h, "sync-streams", FALSE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_bool (selector_h, "sync-streams", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_bool()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_bool_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h udpsrc_h = nullptr;
  int status;
  int wrong_type;
  gchar *pipeline;

  pipeline = g_strdup ("udpsrc name=usrc port=5555 caps=application/x-rtp ! queue ! fakesink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "usrc", &udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "timeout", 123456789123456789ULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_bool (udpsrc_h, "timeout", &wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_string_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h filter_h = nullptr;
  gchar *pipeline, *test_model;
  int status;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Skip this test if enable-tensorflow-lite is false */
  if (!is_enabled_tensorflow_lite)
    return;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* start pipeline test with valid model file */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  pipeline = g_strdup_printf (
      "appsrc name=appsrc ! "
      "other/tensor,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
      "tensor_filter name=filter_h framework=tensorflow-lite model=%s ! tensor_sink name=tensor_sink",
      test_model);

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "filter_h", &filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_string (filter_h, "framework", "nnfw");
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
  g_free (test_model);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_string_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_string (nullptr, "framework", "nnfw");
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_string_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h filter_h = nullptr;
  gchar *pipeline, *test_model;
  int status;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Skip this test if enable-tensorflow-lite is false */
  if (!is_enabled_tensorflow_lite)
    return;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* start pipeline test with valid model file */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  pipeline = g_strdup_printf (
      "appsrc name=appsrc ! "
      "other/tensor,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
      "tensor_filter name=filter_h framework=tensorflow-lite model=%s ! tensor_sink name=tensor_sink",
      test_model);

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "filter_h", &filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_string (filter_h, "WRONG_NAME", "invalid");
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
  g_free (test_model);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_string_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  gchar *pipeline;
  int status;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_string (selector_h, "sync-streams", "TRUE");
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_string_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h filter_h = nullptr;
  gchar *pipeline, *test_model;
  gchar *ret_prop;
  int status;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Skip this test if enable-tensorflow-lite is false */
  if (!is_enabled_tensorflow_lite)
    return;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* start pipeline test with valid model file */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  pipeline = g_strdup_printf (
      "appsrc name=appsrc ! "
      "other/tensor,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
      "tensor_filter name=filter_h framework=tensorflow-lite model=%s ! tensor_sink name=tensor_sink",
      test_model);

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "filter_h", &filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_string (filter_h, "framework", &ret_prop);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (g_str_equal (ret_prop, "tensorflow-lite"));
  g_free (ret_prop);

#ifdef ENABLE_NNFW_RUNTIME
  status = ml_pipeline_element_set_property_string (filter_h, "framework", "nnfw");
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_string (filter_h, "framework", &ret_prop);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (g_str_equal (ret_prop, "nnfw"));
  g_free (ret_prop);
#endif

  status = ml_pipeline_element_release_handle (filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
  g_free (test_model);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_string_02_n)
{
  int status;
  gchar *ret_prop;

  /* Test Code */
  status = ml_pipeline_element_get_property_string (nullptr, "framework", &ret_prop);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_string_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h filter_h = nullptr;
  gchar *pipeline, *test_model;
  gchar *ret_prop;
  int status;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Skip this test if enable-tensorflow-lite is false */
  if (!is_enabled_tensorflow_lite)
    return;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* start pipeline test with valid model file */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  pipeline = g_strdup_printf (
      "appsrc name=appsrc ! "
      "other/tensor,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
      "tensor_filter name=filter_h framework=tensorflow-lite model=%s ! tensor_sink name=tensor_sink",
      test_model);

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "filter_h", &filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_string (filter_h, "WRONG_NAME", &ret_prop);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
  g_free (test_model);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_string_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h filter_h = nullptr;
  gchar *pipeline, *test_model;
  int status;
  const gchar *root_path = g_getenv ("MLAPI_SOURCE_ROOT_PATH");

  /* Skip this test if enable-tensorflow-lite is false */
  if (!is_enabled_tensorflow_lite)
    return;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* start pipeline test with valid model file */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  pipeline = g_strdup_printf (
      "appsrc name=appsrc ! "
      "other/tensor,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
      "tensor_filter name=filter_h framework=tensorflow-lite model=%s ! tensor_sink name=tensor_sink",
      test_model);

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "filter_h", &filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_string (filter_h, "framework", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (filter_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
  g_free (test_model);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_string()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_string_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h selector_h = nullptr;
  gchar *pipeline;
  gchar *ret_wrong_type;
  int status;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "is01", &selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_bool (selector_h, "sync-streams", FALSE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_string (selector_h, "sync-streams", &ret_wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (selector_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int32_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_int32 (vsrc_h, "kx", 10);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int32 (vsrc_h, "kx", -1234);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int32_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_int32 (nullptr, "kx", 10);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int32_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_int32 (vsrc_h, "WRONG_NAME", 10);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int32_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h demux_h = nullptr;
  gchar *pipeline;
  int status;

  pipeline = g_strdup (
      "videotestsrc ! video/x-raw,format=RGB,width=640,height=480 ! videorate max-rate=1 ! "
      "tensor_converter ! tensor_mux ! tensor_demux name=demux ! tensor_sink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "demux", &demux_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_int32 (demux_h, "tensorpick", 1);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (demux_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int32_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  int32_t ret_kx;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int32 (vsrc_h, "kx", 10);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int32 (vsrc_h, "kx", &ret_kx);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_kx == 10);

  status = ml_pipeline_element_set_property_int32 (vsrc_h, "kx", -1234);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_int32 (vsrc_h, "kx", &ret_kx);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_kx == -1234);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int32_02_n)
{
  int status;
  int32_t ret_kx;

  /* Test Code */
  status = ml_pipeline_element_get_property_int32 (nullptr, "kx", &ret_kx);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int32_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  int32_t ret_kx;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int32 (vsrc_h, "kx", 10);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int32 (vsrc_h, "WRONG_NAME", &ret_kx);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int32_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int32 (vsrc_h, "kx", 10);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int32 (vsrc_h, "kx", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int32_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  int wrong_type;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_double (vscale_h, "sharpness", 0.72);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int32 (vscale_h, "sharpness", &wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int64_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_int64 (vsrc_h, "timestamp-offset", 1234567891234LL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int64 (vsrc_h, "timestamp-offset", 10LL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int64_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_int64 (nullptr, "timestamp-offset", 1234567891234LL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int64_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_int64 (vsrc_h, "WRONG_NAME", 1234567891234LL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_int64_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_int64 (vsrc_h, "foreground-color", 123456);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int64_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  int64_t ret_timestame_offset;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int64 (vsrc_h, "timestamp-offset", 1234567891234LL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int64 (
      vsrc_h, "timestamp-offset", &ret_timestame_offset);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_timestame_offset == 1234567891234LL);

  status = ml_pipeline_element_set_property_int64 (vsrc_h, "timestamp-offset", 10LL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_int64 (
      vsrc_h, "timestamp-offset", &ret_timestame_offset);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_timestame_offset == 10LL);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int64_02_n)
{
  int status;
  int64_t ret_timestame_offset;

  /* Test Code */
  status = ml_pipeline_element_get_property_int64 (
      nullptr, "timestamp-offset", &ret_timestame_offset);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int64_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  int64_t ret_timestame_offset;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int64 (vsrc_h, "timestamp-offset", 1234567891234LL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int64 (vsrc_h, "WRONG_NAME", &ret_timestame_offset);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int64_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  int64_t wrong_type;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "foreground-color", 123456U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int64 (vsrc_h, "foreground-color", &wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_int64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_int64_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int64 (vsrc_h, "timestamp-offset", 1234567891234LL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_int64 (vsrc_h, "timestamp-offset", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint32_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "foreground-color", 123456U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "foreground-color", 4294967295U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint32_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_uint32 (nullptr, "foreground-color", 123456U);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint32_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "WRONG_NAME", 123456U);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint32_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "kx", 10U);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint32_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  uint32_t ret_foreground_color;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "foreground-color", 123456U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint32 (
      vsrc_h, "foreground-color", &ret_foreground_color);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_foreground_color == 123456U);

  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "foreground-color", 4294967295U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_uint32 (
      vsrc_h, "foreground-color", &ret_foreground_color);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_foreground_color == 4294967295U);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint32_02_n)
{
  int status;
  uint32_t ret_foreground_color;

  /* Test Code */
  status = ml_pipeline_element_get_property_uint32 (
      nullptr, "foreground-color", &ret_foreground_color);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint32_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  uint32_t ret_foreground_color;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "foreground-color", 123456U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint32 (vsrc_h, "WRONG_NAME", &ret_foreground_color);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint32_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  uint32_t ret_wrong_type;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int32 (vsrc_h, "kx", 10);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint32 (vsrc_h, "kx", &ret_wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint32()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint32_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint32 (vsrc_h, "foreground-color", 123456U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint32 (vsrc_h, "foreground-color", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint64_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h udpsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup ("udpsrc name=usrc port=5555 caps=application/x-rtp ! queue ! fakesink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "usrc", &udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "timeout", 123456789123456789ULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "timeout", 987654321ULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint64_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_uint64 (nullptr, "timeout", 123456789123456789ULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint64_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h udpsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup ("udpsrc name=usrc port=5555 caps=application/x-rtp ! queue ! fakesink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "usrc", &udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "WRONG_NAME", 123456789123456789ULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_uint64_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_uint64 (vsrc_h, "timestamp-offset", 12ULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint64_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h udpsrc_h = nullptr;
  int status;
  uint64_t ret_timeout;
  gchar *pipeline;

  pipeline = g_strdup ("udpsrc name=usrc port=5555 caps=application/x-rtp ! queue ! fakesink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "usrc", &udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "timeout", 123456789123456789ULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint64 (udpsrc_h, "timeout", &ret_timeout);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_timeout == 123456789123456789ULL);

  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "timeout", 987654321ULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_uint64 (udpsrc_h, "timeout", &ret_timeout);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (ret_timeout == 987654321ULL);

  status = ml_pipeline_element_release_handle (udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint64_02_n)
{
  int status;
  uint64_t ret_timeout;

  /* Test Code */
  status = ml_pipeline_element_get_property_uint64 (nullptr, "timeout", &ret_timeout);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint64_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h udpsrc_h = nullptr;
  int status;
  uint64_t ret_timeout;
  gchar *pipeline;

  pipeline = g_strdup ("udpsrc name=usrc port=5555 caps=application/x-rtp ! queue ! fakesink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "usrc", &udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "timeout", 123456789123456789ULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint64 (udpsrc_h, "WRONG_NAME", &ret_timeout);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint64_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  int status;
  uint64_t wrong_type;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int64 (vsrc_h, "timestamp-offset", 1234567891234LL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint64 (vsrc_h, "timestamp-offset", &wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_uint64()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_uint64_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h udpsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup ("udpsrc name=usrc port=5555 caps=application/x-rtp ! queue ! fakesink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "usrc", &udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint64 (udpsrc_h, "timeout", 123456789123456789ULL);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_uint64 (udpsrc_h, "timeout", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_double_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_double (vscale_h, "sharpness", 0.72);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_double (vscale_h, "sharpness", 1.43);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_double_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_double (nullptr, "sharpness", 0.72);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_double_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_double (vscale_h, "WRONG_NAME", 1.43);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_double_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_double (vscale_h, "method", 3.0);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_double_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  double ret_sharpness;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_double (vscale_h, "sharpness", 0.72);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_double (vscale_h, "sharpness", &ret_sharpness);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_sharpness, 0.72);

  status = ml_pipeline_element_set_property_double (vscale_h, "sharpness", 1.43);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_double (vscale_h, "sharpness", &ret_sharpness);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_sharpness, 1.43);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_double_02_n)
{
  int status;
  double ret_sharpness;

  /* Test Code */
  status = ml_pipeline_element_get_property_double (nullptr, "sharpness", &ret_sharpness);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_double_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  double ret_sharpness;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_double (vscale_h, "sharpness", 0.72);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_double (vscale_h, "WRONG_NAME", &ret_sharpness);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_double_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  double wrong_type;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_enum (vscale_h, "method", 3U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_double (vscale_h, "method", &wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_double()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_double_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_double (vscale_h, "sharpness", 0.72);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_double (vscale_h, "sharpness", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_enum_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_enum (vscale_h, "method", 3U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_enum (vscale_h, "method", 5U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_int32 (vscale_h, "method", 4);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_uint32 (vscale_h, "method", 2U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_enum_02_n)
{
  int status;

  /* Test Code */
  status = ml_pipeline_element_set_property_enum (nullptr, "method", 3U);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_enum_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_enum (vscale_h, "WRONG_NAME", 3U);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_set_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, set_property_enum_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h udpsrc_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup ("udpsrc name=usrc port=5555 caps=application/x-rtp ! queue ! fakesink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "usrc", &udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_set_property_enum (udpsrc_h, "timeout", 12345);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (udpsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_enum_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  uint32_t ret_method;
  int32_t ret_signed_method;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_enum (vscale_h, "method", 3U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_enum (vscale_h, "method", &ret_method);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_method, 3U);

  status = ml_pipeline_element_set_property_enum (vscale_h, "method", 5U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_enum (vscale_h, "method", &ret_method);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_method, 5U);

  status = ml_pipeline_element_set_property_uint32 (vscale_h, "method", 2U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_uint32 (vscale_h, "method", &ret_method);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_method, 2U);

  status = ml_pipeline_element_set_property_int32 (vscale_h, "method", 4);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_property_int32 (vscale_h, "method", &ret_signed_method);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (ret_signed_method, 4);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_enum_02_n)
{

  int status;
  uint32_t ret_method;

  /* Test Code */
  status = ml_pipeline_element_get_property_enum (nullptr, "method", &ret_method);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_enum_03_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  uint32_t ret_method;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_enum (vscale_h, "method", 3U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_enum (vscale_h, "WRONG_NAME", &ret_method);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_enum_04_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h demux_h = nullptr;
  gchar *pipeline;
  uint32_t ret_wrong_type;
  int status;

  pipeline = g_strdup (
      "videotestsrc ! video/x-raw,format=RGB,width=640,height=480 ! videorate max-rate=1 ! "
      "tensor_converter ! tensor_mux ! tensor_demux name=demux ! tensor_sink");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "demux", &demux_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_string (demux_h, "tensorpick", "1,2");
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_enum (demux_h, "tensorpick", &ret_wrong_type);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (demux_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Run the `ml_pipeline_element_get_property_enum()` API and check its results.
 */
TEST (nnstreamer_capi_element, get_property_enum_05_n)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vscale_h = nullptr;
  int status;
  gchar *pipeline;

  pipeline = g_strdup (
      "videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale name=vscale ! "
      "video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! tensor_converter ! "
      "valve name=valvex ! input-selector name=is01 ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vscale", &vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_set_property_enum (vscale_h, "method", 3U);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test Code */
  status = ml_pipeline_element_get_property_enum (vscale_h, "method", nullptr);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (vscale_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Construct the pipeline and run it during updating elements' property.
 */
TEST (nnstreamer_capi_element, scenario_01_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_element_h vsrc_h = nullptr;
  ml_pipeline_state_e state;
  gchar *pipeline;
  int status;

  pipeline = g_strdup ("videotestsrc name=vsrc is-live=true ! videoconvert ! videoscale ! video/x-raw,format=RGBx,width=224,height=224,framerate=60/1 ! "
                       "tensor_converter ! tensor_sink name=sinkx");

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "vsrc", &vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Test code: Set the videotestsrc pattern */
  status = ml_pipeline_element_set_property_enum (vsrc_h, "pattern", 4);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000);

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  wait_for_start (handle, state, status);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PLAYING);

  /* Stop playing */
  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000);

  /* Test code: Set the new videotestsrc pattern */
  status = ml_pipeline_element_set_property_enum (vsrc_h, "pattern", 12);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Resume playing */
  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000);

  status = ml_pipeline_get_state (handle, &state);
  EXPECT_EQ (status, ML_ERROR_NONE);
  wait_for_start (handle, state, status);
  EXPECT_EQ (state, ML_PIPELINE_STATE_PLAYING);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_usleep (50000);

  status = ml_pipeline_element_release_handle (vsrc_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief Test case of Element Property Control.
 * @detail Construct the pipeline and run it during updating elements' property.
 */
TEST (nnstreamer_capi_element, scenario_02_p)
{
  ml_pipeline_h handle = nullptr;
  ml_pipeline_sink_h sinkhandle = nullptr;
  ml_pipeline_element_h asink_h = nullptr;
  gchar *pipeline;
  guint *count_sink;
  int status;

  pipeline = g_strdup ("videotestsrc is-live=true ! videoconvert ! tensor_converter ! appsink name=sinkx sync=false");

  count_sink = (guint *)g_malloc (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  status = ml_pipeline_construct (pipeline, nullptr, nullptr, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_get_handle (handle, "sinkx", &asink_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (
      handle, "sinkx", test_sink_callback_count, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (sinkhandle != NULL);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (100000);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (*count_sink > 0U);

  /* Test Code */
  *count_sink = 0;

  status = ml_pipeline_element_set_property_bool (asink_h, "emit-signals", FALSE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_usleep (100000);

  /** Since `emit-signals` property of appsink is set as FALSE, *count_sink
   * should be 0 */
  EXPECT_TRUE (*count_sink == 0U);

  status = ml_pipeline_stop (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_element_release_handle (asink_h);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
  g_free (count_sink);
}

/**
 * @brief Test for internal function '_ml_tensors_info_copy_from_gst'.
 */
TEST (nnstreamer_capi_internal, copy_from_gst)
{
  int status;
  ml_tensors_info_h ml_info;
  ml_tensor_type_e type;
  ml_tensor_dimension dim;
  char *name;
  unsigned int count;
  GstTensorsInfo gst_info;
  guint i;

  gst_tensors_info_init (&gst_info);
  gst_info.num_tensors = 2;
  for (i = 0; i < NNS_TENSOR_RANK_LIMIT; i++) {
    gst_info.info[0].dimension[i] = i + 1;
    gst_info.info[1].dimension[i] = i + 1;
  }

  status = ml_tensors_info_create (&ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_count (ml_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 2U);
  status = ml_tensors_info_get_tensor_dimension (ml_info, 0, dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (dim[0], 1U);
  EXPECT_EQ (dim[1], 2U);
  EXPECT_EQ (dim[2], 3U);
  EXPECT_EQ (dim[3], 4U);

  gst_info.info[0].type = _NNS_INT32;
  gst_info.info[1].type = _NNS_UINT32;
  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_tensor_type (ml_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_INT32);
  status = ml_tensors_info_get_tensor_type (ml_info, 1, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT32);

  gst_info.info[0].type = _NNS_INT16;
  gst_info.info[1].type = _NNS_UINT16;
  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_tensor_type (ml_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_INT16);
  status = ml_tensors_info_get_tensor_type (ml_info, 1, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT16);

  gst_info.info[0].type = _NNS_INT8;
  gst_info.info[1].type = _NNS_UINT8;
  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_tensor_type (ml_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_INT8);
  status = ml_tensors_info_get_tensor_type (ml_info, 1, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  gst_info.info[0].type = _NNS_INT64;
  gst_info.info[1].type = _NNS_UINT64;
  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_tensor_type (ml_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_INT64);
  status = ml_tensors_info_get_tensor_type (ml_info, 1, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT64);

  gst_info.info[0].type = _NNS_FLOAT64;
  gst_info.info[1].type = _NNS_FLOAT32;
  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_tensor_type (ml_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT64);
  status = ml_tensors_info_get_tensor_type (ml_info, 1, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  gst_info.info[0].name = g_strdup ("tn1");
  gst_info.info[1].name = g_strdup ("tn2");
  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_tensor_name (ml_info, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (name, "tn1");
  g_free (name);
  status = ml_tensors_info_get_tensor_name (ml_info, 1, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (name, "tn2");
  g_free (name);

  status = ml_tensors_info_destroy (ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  gst_tensors_info_free (&gst_info);
}

/**
 * @brief Test for internal function '_ml_tensors_info_copy_from_gst'.
 */
TEST (nnstreamer_capi_internal, copy_from_gst_extended)
{
  int status;
  ml_tensors_info_h ml_info;
  ml_tensor_dimension dim;
  unsigned int count;
  GstTensorsInfo gst_info;
  guint i;

  gst_tensors_info_init (&gst_info);
  gst_info.num_tensors = 2;
  gst_info.info[0].type = _NNS_UINT32;
  gst_info.info[1].type = _NNS_UINT32;

  for (i = 0; i < NNS_TENSOR_RANK_LIMIT; i++) {
    gst_info.info[0].dimension[i] = i + 1;
    gst_info.info[1].dimension[i] = i + 1;
  }

  status = ml_tensors_info_create_extended (&ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *)ml_info, &gst_info);
  status = ml_tensors_info_get_count (ml_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 2U);
  status = ml_tensors_info_get_tensor_dimension (ml_info, 0, dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  for (i = 0; i < NNS_TENSOR_RANK_LIMIT; i++) {
    EXPECT_EQ (dim[i], i + 1);
  }

  status = ml_tensors_info_destroy (ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  gst_tensors_info_free (&gst_info);
}

/**
 * @brief Test for internal function '_ml_tensors_info_copy_from_gst'.
 */
TEST (nnstreamer_capi_internal, copy_from_gst_01_n)
{
  GstTensorsInfo gst_info;
  int status;

  gst_tensors_info_init (&gst_info);
  status = _ml_tensors_info_copy_from_gst (NULL, &gst_info);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test for internal function '_ml_tensors_info_copy_from_gst'.
 */
TEST (nnstreamer_capi_internal, copy_from_gst_02_n)
{
  ml_tensors_info_h ml_info;
  int status;

  status = ml_tensors_info_create (&ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = _ml_tensors_info_copy_from_gst ((ml_tensors_info_s *) ml_info, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test for internal function '_ml_tensors_info_copy_from_ml'.
 */
TEST (nnstreamer_capi_internal, copy_from_ml)
{
  int status;
  ml_tensors_info_h ml_info;
  ml_tensor_dimension dim = { 1, 2, 3, 4 };
  GstTensorsInfo gst_info;

  gst_tensors_info_init (&gst_info);

  status = ml_tensors_info_create (&ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_count (ml_info, 2);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (ml_info, 0, dim);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_dimension (ml_info, 1, dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  _ml_tensors_info_copy_from_ml (&gst_info, (ml_tensors_info_s *)ml_info);
  EXPECT_EQ (gst_info.num_tensors, 2U);
  EXPECT_EQ (gst_info.info[0].dimension[0], 1U);
  EXPECT_EQ (gst_info.info[0].dimension[1], 2U);
  EXPECT_EQ (gst_info.info[0].dimension[2], 3U);
  EXPECT_EQ (gst_info.info[0].dimension[3], 4U);

  status = ml_tensors_info_set_tensor_type (ml_info, 0, ML_TENSOR_TYPE_INT32);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (ml_info, 1, ML_TENSOR_TYPE_UINT32);
  EXPECT_EQ (status, ML_ERROR_NONE);
  _ml_tensors_info_copy_from_ml (&gst_info, (ml_tensors_info_s *)ml_info);
  EXPECT_EQ (gst_info.info[0].type, _NNS_INT32);
  EXPECT_EQ (gst_info.info[1].type, _NNS_UINT32);

  status = ml_tensors_info_set_tensor_type (ml_info, 0, ML_TENSOR_TYPE_INT16);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (ml_info, 1, ML_TENSOR_TYPE_UINT16);
  EXPECT_EQ (status, ML_ERROR_NONE);
  _ml_tensors_info_copy_from_ml (&gst_info, (ml_tensors_info_s *)ml_info);
  EXPECT_EQ (gst_info.info[0].type, _NNS_INT16);
  EXPECT_EQ (gst_info.info[1].type, _NNS_UINT16);

  status = ml_tensors_info_set_tensor_type (ml_info, 0, ML_TENSOR_TYPE_INT8);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (ml_info, 1, ML_TENSOR_TYPE_UINT8);
  EXPECT_EQ (status, ML_ERROR_NONE);
  _ml_tensors_info_copy_from_ml (&gst_info, (ml_tensors_info_s *)ml_info);
  EXPECT_EQ (gst_info.info[0].type, _NNS_INT8);
  EXPECT_EQ (gst_info.info[1].type, _NNS_UINT8);

  status = ml_tensors_info_set_tensor_type (ml_info, 0, ML_TENSOR_TYPE_INT64);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (ml_info, 1, ML_TENSOR_TYPE_UINT64);
  EXPECT_EQ (status, ML_ERROR_NONE);
  _ml_tensors_info_copy_from_ml (&gst_info, (ml_tensors_info_s *)ml_info);
  EXPECT_EQ (gst_info.info[0].type, _NNS_INT64);
  EXPECT_EQ (gst_info.info[1].type, _NNS_UINT64);

  status = ml_tensors_info_set_tensor_type (ml_info, 0, ML_TENSOR_TYPE_FLOAT64);
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_type (ml_info, 1, ML_TENSOR_TYPE_FLOAT32);
  EXPECT_EQ (status, ML_ERROR_NONE);
  _ml_tensors_info_copy_from_ml (&gst_info, (ml_tensors_info_s *)ml_info);
  EXPECT_EQ (gst_info.info[0].type, _NNS_FLOAT64);
  EXPECT_EQ (gst_info.info[1].type, _NNS_FLOAT32);

  status = ml_tensors_info_set_tensor_name (ml_info, 0, "tn1");
  EXPECT_EQ (status, ML_ERROR_NONE);
  status = ml_tensors_info_set_tensor_name (ml_info, 1, "tn2");
  EXPECT_EQ (status, ML_ERROR_NONE);
  _ml_tensors_info_copy_from_ml (&gst_info, (ml_tensors_info_s *)ml_info);
  EXPECT_STREQ (gst_info.info[0].name, "tn1");
  EXPECT_STREQ (gst_info.info[1].name, "tn2");

  status = ml_tensors_info_destroy (ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  gst_tensors_info_free (&gst_info);
}

/**
 * @brief Test for internal function '_ml_tensors_info_copy_from_ml'.
 */
TEST (nnstreamer_capi_internal, copy_from_ml_01_n)
{
  ml_tensors_info_h ml_info;
  int status;

  status = ml_tensors_info_create (&ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = _ml_tensors_info_copy_from_ml (NULL, (ml_tensors_info_s *) ml_info);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (ml_info);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test for internal function '_ml_tensors_info_copy_from_ml'.
 */
TEST (nnstreamer_capi_internal, copy_from_ml_02_n)
{
  GstTensorsInfo gst_info;
  int status;

  status = _ml_tensors_info_copy_from_ml (&gst_info, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Invoke callback for custom-easy filter.
 */
static int
test_custom_easy_cb (const ml_tensors_data_h in, ml_tensors_data_h out,
    void *user_data)
{
  /* test code, set data size. */
  if (user_data) {
    void *raw_data = NULL;
    size_t *data_size = (size_t *)user_data;

    ml_tensors_data_get_tensor_data (out, 0, &raw_data, data_size);
  }

  return 0;
}

/**
 * @brief Test for custom-easy registration.
 */
TEST (nnstreamer_capi_custom, register_filter_01_p)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_pipeline_h pipe;
  ml_pipeline_src_h src;
  ml_pipeline_sink_h sink;
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h in_data;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };
  int status;
  gchar *pipeline = g_strdup_printf (
      "appsrc name=srcx ! other/tensor,dimension=(string)2:1:1:1,type=(string)int8,framerate=(fraction)0/1 ! tensor_filter framework=custom-easy model=%s ! tensor_sink name=sinkx",
      test_custom_filter);
  guint *count_sink = (guint *)g_malloc0 (sizeof (guint));
  size_t *filter_data_size = (size_t *)g_malloc0 (sizeof (size_t));
  size_t data_size;
  guint i;

  ml_tensors_info_create (&in_info);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_info_create (&out_info);
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);
  ml_tensors_info_get_tensor_size (out_info, 0, &data_size);

  /* test code for custom filter */
  status = ml_pipeline_custom_easy_filter_register (test_custom_filter, in_info,
      out_info, test_custom_easy_cb, filter_data_size, &custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (
      pipe, "sinkx", test_sink_callback_count, count_sink, &sink);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (pipe, "srcx", &src);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0; i < 5; i++) {
    status = ml_tensors_data_create (in_info, &in_data);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_pipeline_src_input_data (src, in_data, ML_PIPELINE_BUF_POLICY_AUTO_FREE);
    EXPECT_EQ (status, ML_ERROR_NONE);

    g_usleep (50000); /* 50ms. Wait a bit. */
  }

  status = ml_pipeline_stop (pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_release_handle (src);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sink);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_custom_easy_filter_unregister (custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* check received data in sink node */
  EXPECT_TRUE (*count_sink > 0U);
  EXPECT_TRUE (*filter_data_size > 0U && *filter_data_size == data_size);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  g_free (pipeline);
  g_free (count_sink);
  g_free (filter_data_size);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_02_n)
{
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h in_info, out_info;
  int status;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);


  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);

  /* test code with null param */
  status = ml_pipeline_custom_easy_filter_register (
      NULL, in_info, out_info, test_custom_easy_cb, NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_03_n)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h out_info;
  int status;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };

  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);

  /* test code with null param */
  status = ml_pipeline_custom_easy_filter_register (
      test_custom_filter, NULL, out_info, test_custom_easy_cb, NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_04_n)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h in_info;
  int status;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };

  ml_tensors_info_create (&in_info);

  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  /* test code with null param */
  status = ml_pipeline_custom_easy_filter_register (
      test_custom_filter, in_info, NULL, test_custom_easy_cb, NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_05_n)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h in_info, out_info;
  int status;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);

  /* test code with null param */
  status = ml_pipeline_custom_easy_filter_register (
      test_custom_filter, in_info, out_info, NULL, NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_06_n)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_tensors_info_h in_info, out_info;
  int status;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);

  /* test code with null param */
  status = ml_pipeline_custom_easy_filter_register (
      test_custom_filter, in_info, out_info, test_custom_easy_cb, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_07_n)
{
  int status;

  /* test code with null param */
  status = ml_pipeline_custom_easy_filter_unregister (NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_08_n)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };
  int status;

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  /* test code with invalid output info */
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  status = ml_pipeline_custom_easy_filter_register (
      test_custom_filter, in_info, out_info, test_custom_easy_cb, NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_09_n)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };
  int status;

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  /* test code with invalid input info */
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);

  status = ml_pipeline_custom_easy_filter_register (
      test_custom_filter, in_info, out_info, test_custom_easy_cb, NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test for custom-easy registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_custom, register_filter_10_n)
{
  const char test_custom_filter[] = "test-custom-filter";
  ml_custom_easy_filter_h custom, custom2;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };
  int status;

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);

  /* test code for duplicated name */
  status = ml_pipeline_custom_easy_filter_register (
      test_custom_filter, in_info, out_info, test_custom_easy_cb, NULL, &custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_custom_easy_filter_register (test_custom_filter, in_info,
      out_info, test_custom_easy_cb, NULL, &custom2);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_custom_easy_filter_unregister (custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test for custom-easy unregistration.
 * @detail Failed if pipeline is constructed.
 */
TEST (nnstreamer_capi_custom, register_filter_11_n)
{
  ml_pipeline_h pipe1, pipe2;
  ml_custom_easy_filter_h custom;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension dim = { 2, 1, 1, 1 };
  int status;
  gchar *pipeline = g_strdup_printf (
      "appsrc name=srcx ! other/tensor,dimension=(string)2:1:1:1,type=(string)int8,framerate=(fraction)0/1 ! "
      "tensor_filter framework=custom-easy model=tfilter_unreg_test ! tensor_sink name=sinkx");

  ml_tensors_info_create (&in_info);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_info_create (&out_info);
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, dim);

  status = ml_pipeline_custom_easy_filter_register ("tfilter_unreg_test",
      in_info, out_info, test_custom_easy_cb, NULL, &custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &pipe1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &pipe2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* failed to unregister custom-easy filter if pipeline is constructed */
  status = ml_pipeline_custom_easy_filter_unregister (custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (pipe1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_custom_easy_filter_unregister (custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (pipe2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* succesfully done if all pipelines with custom-easy filter are destroyed */
  status = ml_pipeline_custom_easy_filter_unregister (custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  g_free (pipeline);
}

/**
 * @brief Callback for tensor_if custom condition.
 */
static int
test_if_custom_cb (const ml_tensors_data_h data, const ml_tensors_info_h info,
    int *result, void *user_data)
{
  void *data_ptr;
  guint sum = 0, i;
  size_t data_size;

  ml_tensors_data_get_tensor_data (data, 0, &data_ptr, &data_size);

  for (i = 0; i < data_size; i++)
    sum += ((guint8 *) data_ptr)[i];

  /* Sum value 30 means that the sixth buffer has arrived.*/
  if (sum >= 30)
    *result = 0;
  else
    *result = 1;

  return 0;
}

/**
 * @brief Test for tensor_if custom condition
 */
TEST (nnstreamer_capi_if, custom_01_p)
{
  const gchar *_tmpdir = g_get_tmp_dir ();
  const gchar *_dirname = "nns-tizen-XXXXXX";
  gchar *fullpath = g_build_path ("/", _tmpdir, _dirname, NULL);
  gchar *dir = g_mkdtemp ((gchar *)fullpath);
  gchar *file = g_build_path ("/", dir, "output", NULL);
  ml_pipeline_h pipe;
  ml_pipeline_src_h srchandle;
  ml_pipeline_sink_h sink_false;
  ml_pipeline_if_h custom;
  ml_tensors_info_h info;
  ml_tensors_data_h data;
  unsigned int count = 0;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  int status;
  uint8_t *uintarray[10];
  uint8_t *content = NULL;
  guint i;
  gsize len;
  gchar *pipeline = g_strdup_printf (
      "appsrc name=appsrc ! other/tensor,dimension=(string)4:1:1:1, type=(string)uint8,framerate=(fraction)0/1 ! "
      "tensor_if name=tif compared-value=CUSTOM compared-value-option=tif_custom_cb_name then=PASSTHROUGH else=PASSTHROUGH "
      "tif.src_0 ! queue ! filesink location=\"%s\" buffer-mode=unbuffered "
      "tif.src_1 ! queue ! tensor_sink name=sink_false sync=false async=false", file);

  guint *count_sink = (guint *)g_malloc0 (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);
  *count_sink = 0;

  /* test code for tensor_if custom */
  status = ml_pipeline_tensor_if_custom_register ("tif_custom_cb_name",
      test_if_custom_cb, NULL, &custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (
      pipe, "sink_false", test_sink_callback_count, count_sink, &sink_false);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (pipe, "appsrc", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (i = 0; i < 10; i++) {
    uintarray[i] = (uint8_t *)g_malloc (4);
    ASSERT_TRUE (uintarray[i] != NULL);
    uintarray[i][0] = i + 4;
    uintarray[i][1] = i + 1;
    uintarray[i][2] = i + 3;
    uintarray[i][3] = i + 2;
  }

  status = ml_pipeline_src_get_tensors_info (srchandle, &info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_get_count (info, &count);
  EXPECT_EQ (count, 1U);

  ml_tensors_info_get_tensor_type (info, 0, &type);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  status = ml_tensors_data_create (info, &data);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* Set tensor data and push buffers to source pad */
  for (i = 0; i < 10; i++) {
    status = ml_tensors_data_set_tensor_data (data, 0, uintarray[i], 4);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_pipeline_src_input_data (srchandle, data,
        ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
    EXPECT_EQ (status, ML_ERROR_NONE);

    g_usleep (50000); /* 50ms. Wait a bit. */
  }

  status = ml_pipeline_stop (pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_release_handle (srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_unregister (sink_false);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (pipe);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_tensor_if_custom_unregister (custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_TRUE (g_file_get_contents (file, (gchar **)&content, &len, NULL));
  EXPECT_EQ (len, 4U * 5);
  EXPECT_TRUE (content != nullptr);

  /* Check if the TRUE path data is received correctly.  */
  if (content && len == 20) {
    for (i = 0; i < 5; i++) {
      EXPECT_EQ (content[i * 4 + 0], i + 4);
      EXPECT_EQ (content[i * 4 + 1], i + 1);
      EXPECT_EQ (content[i * 4 + 2], i + 3);
      EXPECT_EQ (content[i * 4 + 3], i + 2);
    }
  }
  g_free (content);

  /* The FALSE path receives 5 buffers. */
  EXPECT_EQ (*count_sink, 5U);

  for (i = 0; i < 10; i++) {
    g_free (uintarray[i]);
  }
  ml_tensors_info_destroy (info);
  ml_tensors_data_destroy (data);
  g_free (pipeline);
  g_free (count_sink);
  g_free (fullpath);
  g_free (file);
}

/**
 * @brief Test for tensor_if custom registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_if, register_01_n)
{
  ml_pipeline_if_h custom;
  int status;

  /* test code with null param */
  status = ml_pipeline_tensor_if_custom_register (NULL, test_if_custom_cb,
      NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test for tensor_if custom registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_if, register_02_n)
{
  ml_pipeline_if_h custom;
  int status;

  /* test code with null param */
  status = ml_pipeline_tensor_if_custom_register ("tif_custom_cb_name",
      NULL, NULL, &custom);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test for tensor_if custom registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_if, register_03_n)
{
  int status;

  /* test code with null param */
  status = ml_pipeline_tensor_if_custom_register ("tif_custom_cb_name",
      test_if_custom_cb, NULL, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test for tensor_if custom registration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_if, register_04_n)
{
  ml_pipeline_if_h custom1, custom2;
  int status;

  status = ml_pipeline_tensor_if_custom_register ("tif_custom_cb_name",
      test_if_custom_cb, NULL, &custom1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* test to register tensor_if custom twice with same name */
  status = ml_pipeline_tensor_if_custom_register ("tif_custom_cb_name",
      test_if_custom_cb, NULL, &custom2);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_tensor_if_custom_unregister (custom1);
  EXPECT_EQ (status, ML_ERROR_NONE);
}

/**
 * @brief Test for tensor_if custom unregistration.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_if, unregister_01_n)
{
  int status;

  /* test code with null param */
  status = ml_pipeline_tensor_if_custom_unregister (NULL);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief Test for tensor_if custom unregistration.
 * @detail Failed if pipeline is constructed.
 */
TEST (nnstreamer_capi_if, unregister_02_n)
{
  ml_pipeline_h pipe1, pipe2;
  ml_pipeline_if_h custom;
  int status;
  gchar *pipeline = g_strdup_printf (
      "appsrc name=appsrc ! other/tensor,dimension=(string)4:1:1:1, type=(string)uint8,framerate=(fraction)0/1 ! "
      "tensor_if name=tif compared-value=CUSTOM compared-value-option=tif_unreg_test then=PASSTHROUGH else=PASSTHROUGH "
      "tif.src_0 ! queue ! tensor_sink name=sink_true sync=false async=false "
      "tif.src_1 ! queue ! tensor_sink name=sink_false sync=false async=false");

  status = ml_pipeline_tensor_if_custom_register ("tif_unreg_test",
      test_if_custom_cb, NULL, &custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &pipe1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_construct (pipeline, NULL, NULL, &pipe2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* failed to unregister tensor_if custom if pipeline is constructed */
  status = ml_pipeline_tensor_if_custom_unregister (custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (pipe1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_tensor_if_custom_unregister (custom);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_pipeline_destroy (pipe2);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* succesfully done if all pipelines with custom condition are destroyed */
  status = ml_pipeline_tensor_if_custom_unregister (custom);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (pipeline);
}

/**
 * @brief A tensor-sink callback for sink handle in a pipeline
 */
static void
test_sink_callback_flush (const ml_tensors_data_h data,
    const ml_tensors_info_h info, void *user_data)
{
  guint *count = (guint *) user_data;

  G_LOCK (callback_lock);
  *count = *count + 1;

  /* check first data */
  if (*count == 1) {
    gint *received;
    size_t data_size;

    ml_tensors_data_get_tensor_data (data, 0, (void **) &received, &data_size);
    EXPECT_EQ (data_size, 3 * sizeof (gint));
    EXPECT_EQ (received[0], 1);
    EXPECT_EQ (received[1], 2);
    EXPECT_EQ (received[2], 3);
  }
  G_UNLOCK (callback_lock);
}

/**
 * @brief Test NNStreamer pipeline flush.
 */
TEST (nnstreamer_capi_flush, success_01_p)
{
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;
  ml_pipeline_sink_h sinkhandle;
  ml_tensors_info_h in_info;
  ml_tensors_data_h in_data;
  ml_tensor_dimension dim = { 10, 1, 1, 1 };
  int status;
  gchar pipeline[] = "appsrc name=srcx ! "
      "other/tensor,dimension=(string)10:1:1:1,type=(string)int32,framerate=(fraction)0/1 ! "
      "tensor_aggregator frames-in=10 frames-out=3 frames-flush=3 frames-dim=0 ! "
      "tensor_sink name=sinkx";
  gint test_data[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  guint *count_sink = (guint *) g_malloc0 (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);

  /* prepare input data */
  ml_tensors_info_create (&in_info);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_data_create (in_info, &in_data);
  ml_tensors_data_set_tensor_data (in_data, 0, test_data, 10 * sizeof (gint));

  /* start pipeline */
  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (handle, "sinkx",
      test_sink_callback_flush, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* push input data */
  *count_sink = 0;
  status = ml_pipeline_src_input_data (srchandle, in_data,
        ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  wait_pipeline_process_buffers (*count_sink, 3);
  g_usleep (300000);
  EXPECT_EQ (*count_sink, 3U);

  /* flush pipeline */
  status = ml_pipeline_flush (handle, true);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* push input data again */
  *count_sink = 0;
  status = ml_pipeline_src_input_data (srchandle, in_data,
        ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
  EXPECT_EQ (status, ML_ERROR_NONE);

  wait_pipeline_process_buffers (*count_sink, 3);
  g_usleep (300000);
  EXPECT_EQ (*count_sink, 3U);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_data_destroy (in_data);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline flush.
 * @detail Failure case when the pipeline handle is invalid.
 */
TEST (nnstreamer_capi_flush, failure_02_n)
{
  int status;

  status = ml_pipeline_flush (NULL, true);
  EXPECT_NE (status, ML_ERROR_NONE);
}

/**
 * @brief A tensor-sink callback for sink handle in a pipeline
 */
static void
test_sink_callback_flex (const ml_tensors_data_h data,
    const ml_tensors_info_h info, void *user_data)
{
  guint *count = (guint *) user_data;
  gint status;
  gint *received;
  guint total = 0;
  size_t data_size;

  G_LOCK (callback_lock);
  *count = *count + 1;

  status = ml_tensors_info_get_count (info, &total);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (total, 3U);

  status = ml_tensors_data_get_tensor_data (data, 0, (void **) &received, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (data_size, 4 * sizeof (gint));
  EXPECT_EQ (received[0], 1);
  EXPECT_EQ (received[1], 2);
  EXPECT_EQ (received[2], 3);
  EXPECT_EQ (received[3], 4);

  status = ml_tensors_data_get_tensor_data (data, 1, (void **) &received, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (data_size, 2 * sizeof (gint));
  EXPECT_EQ (received[0], 5);
  EXPECT_EQ (received[1], 6);

  status = ml_tensors_data_get_tensor_data (data, 2, (void **) &received, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (data_size, 4 * sizeof (gint));
  EXPECT_EQ (received[0], 7);
  EXPECT_EQ (received[1], 8);
  EXPECT_EQ (received[2], 9);
  EXPECT_EQ (received[3], 10);

  G_UNLOCK (callback_lock);
}

/**
 * @brief Test NNStreamer pipeline for flexible tensors.
 */
TEST (nnstreamer_capi_flex, sink_multi)
{
  gchar pipeline[] = "appsrc name=srcx caps=application/octet-stream,framerate=(fraction)10/1 ! "
      "tensor_converter input-dim=4,2,4 input-type=int32,int32,int32 ! "
      "other/tensors,format=flexible ! tensor_sink name=sinkx sync=false";
  guint test_data[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;
  ml_pipeline_sink_h sinkhandle;
  ml_tensors_info_h in_info;
  ml_tensors_data_h in_data;
  ml_tensor_dimension dim = { 10, 1, 1, 1 };
  gint i, status;
  guint *count_sink;

  count_sink = (guint *) g_malloc0 (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);

  /* prepare input data */
  ml_tensors_info_create (&in_info);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim);

  ml_tensors_data_create (in_info, &in_data);
  ml_tensors_data_set_tensor_data (in_data, 0, test_data, 10 * sizeof (gint));

  /* start pipeline */
  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (handle, "sinkx",
      test_sink_callback_flex, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* push input data */
  *count_sink = 0;
  for (i = 0; i < 3; i++) {
    g_usleep (50000);
    status = ml_pipeline_src_input_data (srchandle, in_data,
          ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
    EXPECT_EQ (status, ML_ERROR_NONE);
  }

  wait_pipeline_process_buffers (*count_sink, 3);
  g_usleep (300000);
  EXPECT_EQ (*count_sink, 3U);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_data_destroy (in_data);
  g_free (count_sink);
}

/**
 * @brief Test NNStreamer pipeline for flexible tensors.
 */
TEST (nnstreamer_capi_flex, src_multi)
{
  gchar pipeline[] = "appsrc name=srcx caps=other/tensors,format=flexible,framerate=(fraction)10/1 ! "
      "tensor_converter input-dim=4,2,4 input-type=int32,int32,int32 ! "
      "tensor_sink name=sinkx sync=false";
  guint test_data[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  ml_pipeline_h handle;
  ml_pipeline_src_h srchandle;
  ml_pipeline_sink_h sinkhandle;
  ml_tensors_info_h in_info;
  ml_tensors_data_h in_data;
  ml_tensor_dimension dim1 = { 4, 1, 1, 1 };
  ml_tensor_dimension dim2 = { 2, 1, 1, 1 };
  ml_tensor_dimension dim3 = { 4, 1, 1, 1 };
  gint i, status;
  guint *count_sink;

  count_sink = (guint *) g_malloc0 (sizeof (guint));
  ASSERT_TRUE (count_sink != NULL);

  /* prepare input data */
  ml_tensors_info_create (&in_info);
  ml_tensors_info_set_count (in_info, 3);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, dim1);
  ml_tensors_info_set_tensor_type (in_info, 1, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (in_info, 1, dim2);
  ml_tensors_info_set_tensor_type (in_info, 2, ML_TENSOR_TYPE_INT32);
  ml_tensors_info_set_tensor_dimension (in_info, 2, dim3);

  ml_tensors_data_create (in_info, &in_data);
  ml_tensors_data_set_tensor_data (in_data, 0, &test_data[0], 4 * sizeof (gint));
  ml_tensors_data_set_tensor_data (in_data, 1, &test_data[4], 2 * sizeof (gint));
  ml_tensors_data_set_tensor_data (in_data, 2, &test_data[6], 4 * sizeof (gint));

  /* start pipeline */
  status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_src_get_handle (handle, "srcx", &srchandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_sink_register (handle, "sinkx",
      test_sink_callback_flex, count_sink, &sinkhandle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_pipeline_start (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* push input data */
  *count_sink = 0;
  for (i = 0; i < 3; i++) {
    g_usleep (50000);
    status = ml_pipeline_src_input_data (srchandle, in_data,
          ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
    EXPECT_EQ (status, ML_ERROR_NONE);
  }

  wait_pipeline_process_buffers (*count_sink, 3);
  g_usleep (300000);
  EXPECT_EQ (*count_sink, 3U);

  status = ml_pipeline_destroy (handle);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_data_destroy (in_data);
  g_free (count_sink);
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

  try {
    result = RUN_ALL_TESTS ();
  } catch (...) {
    g_warning ("catch `testing::internal::GoogleTestFailureException`");
  }

  set_feature_state (ML_FEATURE, NOT_CHECKED_YET);
  set_feature_state (ML_FEATURE_INFERENCE, NOT_CHECKED_YET);

  return result;
}
