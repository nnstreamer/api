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
#include <nnstreamer.h>
#include <nnstreamer-single.h>
#include <nnstreamer_plugin_api.h>
#include <nnstreamer_internal.h>
#include <nnstreamer-tizen-internal.h>
#include <ml-api-inference-internal.h>
#include <ml-api-inference-single-internal.h>

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
 * @brief Test NNStreamer single shot (tensorflow-lite)
 */
TEST (nnstreamer_capi_singleshot, invoke_invalid_param_01_n)
{
  ml_single_h single;
  int status;
  ml_tensors_info_h in_info;
  ml_tensors_data_h input, output;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_invoke (NULL, input, &output);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_invoke (single, NULL, &output);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_invoke (single, input, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 */
TEST (nnstreamer_capi_singleshot, invoke_invalid_param_02_n)
{
  ml_single_h single;
  int status;
  ml_tensors_info_h in_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  ml_tensors_info_create (&in_info);

  in_dim[0] = 3;
  in_dim[1] = 224;
  in_dim[2] = 224;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  /* handle null data */
  status = _ml_tensors_data_create_no_alloc (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (input);

  /* set invalid type to test wrong data size */
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT32);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_data_destroy (input);

  /* set invalid input tensor number */
  ml_tensors_info_set_count (in_info, 2);
  ml_tensors_info_set_tensor_type (in_info, 1, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 1, in_dim);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 */
TEST (nnstreamer_capi_singleshot, invoke_01)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res, out_res;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim, res_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  char *name = NULL;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 3;
  in_dim[1] = 224;
  in_dim[2] = 224;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 1001;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  /* input tensor in filter */
  status = ml_single_get_input_info (single, &in_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (in_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (in_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (in_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
  EXPECT_TRUE (in_dim[0] == res_dim[0]);
  EXPECT_TRUE (in_dim[1] == res_dim[1]);
  EXPECT_TRUE (in_dim[2] == res_dim[2]);
  EXPECT_TRUE (in_dim[3] == res_dim[3]);

  /* output tensor in filter */
  status = ml_single_get_output_info (single, &out_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (out_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (out_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  ml_tensors_info_get_tensor_dimension (out_res, 0, res_dim);
  EXPECT_TRUE (out_dim[0] == res_dim[0]);
  EXPECT_TRUE (out_dim[1] == res_dim[1]);
  EXPECT_TRUE (out_dim[2] == res_dim[2]);
  EXPECT_TRUE (out_dim[3] == res_dim[3]);

  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);

skip_test:
  g_free (test_model);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Start pipeline without tensor info
 */
TEST (nnstreamer_capi_singleshot, invoke_02)
{
  ml_single_h single;
  ml_tensors_info_h in_info;
  ml_tensors_data_h input, output;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);
skip_test:
  g_free (test_model);
}

/**
 * @brief Measure the loading time and total time for the run
 */
static void
benchmark_single (const gboolean no_alloc, const gboolean no_timeout, const int count)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim;
  int status;
  unsigned long open_duration = 0, invoke_duration = 0, close_duration = 0;
  gint64 start, end;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 3;
  in_dim[1] = 224;
  in_dim[2] = 224;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 1001;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  /** Initial run to warm up the cache */
  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }
  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  for (int i = 0; i < count; i++) {
    start = g_get_monotonic_time ();
    status = ml_single_open (&single, test_model, in_info, out_info,
        ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
    end = g_get_monotonic_time ();
    open_duration += end - start;
    ASSERT_EQ (status, ML_ERROR_NONE);

    if (!no_timeout) {
      status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
      EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);
    }

    /* generate dummy data */
    input = output = NULL;

    status = ml_tensors_data_create (in_info, &input);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (input != NULL);

    if (no_alloc) {
      status = ml_tensors_data_create (out_info, &output);
      EXPECT_EQ (status, ML_ERROR_NONE);
      EXPECT_TRUE (output != NULL);
    }

    start = g_get_monotonic_time ();
    if (no_alloc)
      status = ml_single_invoke_fast (single, input, output);
    else
      status = ml_single_invoke (single, input, &output);
    end = g_get_monotonic_time ();
    invoke_duration += end - start;
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (output != NULL);

    start = g_get_monotonic_time ();
    status = ml_single_close (single);
    end = g_get_monotonic_time ();
    close_duration = end - start;
    EXPECT_EQ (status, ML_ERROR_NONE);

    ml_tensors_data_destroy (input);
    ml_tensors_data_destroy (output);
  }

  g_warning ("Time to open single = %f us", (open_duration * 1.0) / count);
  g_warning ("Time to invoke single = %f us", (invoke_duration * 1.0) / count);
  g_warning ("Time to close single = %f us", (close_duration * 1.0) / count);

skip_test:
  g_free (test_model);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @note Measure the loading time and total time for the run
 */
TEST (nnstreamer_capi_singleshot, benchmark_time)
{
  g_warning ("Benchmark (no timeout)");
  benchmark_single (FALSE, TRUE, 1);

  g_warning ("Benchmark (no alloc, no timeout)");
  benchmark_single (TRUE, TRUE, 1);
}

/**
 * @brief Test NNStreamer single shot (custom filter)
 * @detail Run pipeline with custom filter, handle multi tensors.
 */
TEST (nnstreamer_capi_singleshot, invoke_03)
{
  const gchar cf_name[] = "libnnstreamer_customfilter_passthrough_variable" SO_FILE_EXTENSION;
  gchar *lib_path = NULL;
  gchar *test_model = NULL;
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim;
  int status;
  unsigned int i;
  void *data_ptr;
  size_t data_size;

  lib_path = nnsconf_get_custom_value_string ("filter", "customfilters");
  if (lib_path == NULL) {
    /* cannot get custom-filter directory */
    goto skip_test;
  }

  test_model = g_build_filename (lib_path, cf_name, NULL);
  if (!g_file_test (test_model, G_FILE_TEST_EXISTS)) {
    goto skip_test;
  }

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (in_info, 2);

  in_dim[0] = 10;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;

  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT16);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  ml_tensors_info_set_tensor_type (in_info, 1, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 1, in_dim);

  ml_tensors_info_clone (out_info, in_info);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_CUSTOM_FILTER, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  input = output = NULL;

  /* generate input data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (input != NULL);

  for (i = 0; i < 10; i++) {
    int16_t i16 = (int16_t) (i + 1);
    float f32 = (float)(i + .1);

    status = ml_tensors_data_get_tensor_data (input, 0, &data_ptr, &data_size);
    EXPECT_EQ (status, ML_ERROR_NONE);
    ((int16_t *)data_ptr)[i] = i16;

    status = ml_tensors_data_get_tensor_data (input, 1, &data_ptr, &data_size);
    EXPECT_EQ (status, ML_ERROR_NONE);
    ((float *)data_ptr)[i] = f32;
  }

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  for (i = 0; i < 10; i++) {
    int16_t i16 = (int16_t) (i + 1);
    float f32 = (float)(i + .1);

    status = ml_tensors_data_get_tensor_data (output, 0, &data_ptr, &data_size);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (((int16_t *)data_ptr)[i], i16);

    status = ml_tensors_data_get_tensor_data (output, 1, &data_ptr, &data_size);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_FLOAT_EQ (((float *)data_ptr)[i], f32);
  }

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);

skip_test:
  g_free (lib_path);
  g_free (test_model);
}

#ifdef ENABLE_TENSORFLOW
/**
 * @brief Test NNStreamer single shot (tensorflow)
 * @detail Run pipeline with tensorflow speech command model.
 */
TEST (nnstreamer_capi_singleshot, invoke_04)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res, out_res;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim, res_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  char *name = NULL;
  int status, max_score_index;
  float score, max_score;
  void *data_ptr;
  size_t data_size;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model, *test_file;
  gchar *contents = NULL;
  gsize len = 0;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "conv_actions_frozen.pb", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  test_file = g_build_filename (root_path, "tests", "test_models", "data",
      "yes.wav", NULL);
  ASSERT_TRUE (g_file_test (test_file, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 1;
  in_dim[1] = 16022;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_name (in_info, 0, "wav_data");
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT16);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 12;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_name (out_info, 0, "labels_softmax");
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  ASSERT_TRUE (g_file_get_contents (test_file, &contents, &len, NULL));
  status = ml_tensors_info_get_tensor_size (in_info, 0, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (len == data_size);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_TENSORFLOW, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* input tensor in filter */
  status = ml_single_get_input_info (single, &in_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (in_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (in_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (name, "wav_data");
  g_free (name);

  status = ml_tensors_info_get_tensor_type (in_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_INT16);

  ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
  EXPECT_TRUE (in_dim[0] == res_dim[0]);
  EXPECT_TRUE (in_dim[1] == res_dim[1]);
  EXPECT_TRUE (in_dim[2] == res_dim[2]);
  EXPECT_TRUE (in_dim[3] == res_dim[3]);

  /* output tensor in filter */
  status = ml_single_get_output_info (single, &out_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (out_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (name, "labels_softmax");
  g_free (name);

  status = ml_tensors_info_get_tensor_type (out_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (out_res, 0, res_dim);
  EXPECT_TRUE (out_dim[0] == res_dim[0]);
  EXPECT_TRUE (out_dim[1] == res_dim[1]);
  EXPECT_TRUE (out_dim[2] == res_dim[2]);
  EXPECT_TRUE (out_dim[3] == res_dim[3]);

  input = output = NULL;

  /* generate input data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_tensors_data_set_tensor_data (input, 0, contents, len);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  /* check result (max score index is 2) */
  status = ml_tensors_data_get_tensor_data (output, 1, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_get_tensor_data (output, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  max_score = .0;
  max_score_index = 0;
  for (gint i = 0; i < 12; i++) {
    score = ((float *)data_ptr)[i];
    if (score > max_score) {
      max_score = score;
      max_score_index = i;
    }
  }

  EXPECT_EQ (max_score_index, 2);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (test_model);
  g_free (test_file);
  g_free (contents);
  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);
}
#else
/**
 * @brief Test NNStreamer single shot (tensorflow is not supported)
 */
TEST (nnstreamer_capi_singleshot, unavailable_fw_tf_n)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension in_dim, out_dim;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "conv_actions_frozen.pb", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 1;
  in_dim[1] = 16022;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_name (in_info, 0, "wav_data");
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT16);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 12;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_name (out_info, 0, "labels_softmax");
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  /* tensorflow is not supported */
  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_TENSORFLOW, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_NOT_SUPPORTED);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  g_free (test_model);
}
#endif /* ENABLE_TENSORFLOW */

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Failure case with invalid param.
 */
TEST (nnstreamer_capi_singleshot, open_fail_01_n)
{
  ml_single_h single;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  /* invalid file path */
  status = ml_single_open (&single, "wrong_file_name", NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* null file path */
  status = ml_single_open (
      &single, NULL, NULL, NULL, ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* invalid handle */
  status = ml_single_open (NULL, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* invalid file extension */
  status = ml_single_open (
      &single, test_model, NULL, NULL, ML_NNFW_TYPE_TENSORFLOW, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* invalid handle */
  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* Successfully opened unknown fw type (tf-lite) */
  status = ml_single_open (&single, test_model, NULL, NULL, ML_NNFW_TYPE_ANY, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Failure case with invalid tensor info.
 */
TEST (nnstreamer_capi_singleshot, open_fail_02_n)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension in_dim, out_dim;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  /* invalid input tensor info */
  status = ml_single_open (&single, test_model, in_info, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  /* invalid output tensor info */
  status = ml_single_open (&single, test_model, NULL, out_info,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  in_dim[0] = 3;
  in_dim[1] = 100;
  in_dim[2] = 100;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  /* invalid input dimension (model does not support dynamic dimension) */
  status = ml_single_open (&single, test_model, in_info, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_NE (status, ML_ERROR_NONE);

  in_dim[1] = in_dim[2] = 224;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT16);

  /* invalid input type */
  status = ml_single_open (&single, test_model, in_info, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);

  out_dim[0] = 1;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  /* invalid output dimension */
  status = ml_single_open (&single, test_model, NULL, out_info,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_NE (status, ML_ERROR_NONE);

  out_dim[0] = 1001;
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_UINT16);

  /* invalid output type */
  status = ml_single_open (&single, test_model, NULL, out_info,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_UINT8);

  /* Successfully opened unknown fw type (tf-lite) */
  status = ml_single_open (
      &single, test_model, in_info, out_info, ML_NNFW_TYPE_ANY, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Open model (dynamic dimension is supported)
 */
TEST (nnstreamer_capi_singleshot, open_dynamic)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension in_dim, out_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* dynamic dimension supported */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);

  in_dim[0] = 5;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  /* open with input tensor info (1:1:1:1 > 5:1:1:1) */
  status = ml_single_open (&single, test_model, in_info, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  /* validate output info */
  status = ml_single_get_output_info (single, &out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_type (out_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (out_info, 0, out_dim);
  EXPECT_EQ (out_dim[0], 5U);
  EXPECT_EQ (out_dim[1], 1U);
  EXPECT_EQ (out_dim[2], 1U);
  EXPECT_EQ (out_dim[3], 1U);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (out_info);
skip_test:
  ml_tensors_info_destroy (in_info);
  g_free (test_model);
}

/**
 * @brief Structure containing values to run single shot
 */
typedef struct {
  gchar *test_model;
  guint num_runs;
  guint timeout;
  guint min_time_to_run;
  gboolean expect;
  ml_single_h *single;
} single_shot_thread_data;

/**
 * @brief Open and run on single shot API with provided data
 */
static void *
single_shot_loop_test (void *arg)
{
  guint i;
  int status = ML_ERROR_NONE;
  ml_single_h single;
  single_shot_thread_data *ss_data = (single_shot_thread_data *)arg;
  int timeout_cond, no_error_cond;

  status = ml_single_open (&single, ss_data->test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (ss_data->expect) {
    EXPECT_EQ (status, ML_ERROR_NONE);
    if (status != ML_ERROR_NONE)
      return NULL;
  }
  ss_data->single = &single;

  /* set timeout */
  if (ss_data->timeout != 0) {
    status = ml_single_set_timeout (single, ss_data->timeout);
    if (ss_data->expect) {
      EXPECT_NE (status, ML_ERROR_INVALID_PARAMETER);
    }
    if (status == ML_ERROR_NOT_SUPPORTED)
      ss_data->timeout = 0;
  }

  ml_tensors_info_h in_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim;

  ml_tensors_info_create (&in_info);

  in_dim[0] = 3;
  in_dim[1] = 224;
  in_dim[2] = 224;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  if (ss_data->expect) {
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (input != NULL);
  }

  for (i = 0; i < ss_data->num_runs; i++) {
    status = ml_single_invoke (single, input, &output);
    if (ss_data->expect) {
      no_error_cond = status == ML_ERROR_NONE && output != NULL;
      if (ss_data->timeout < ss_data->min_time_to_run) {
        /** Default timeout can return timed out with many parallel runs */
        timeout_cond = output == NULL && (status == ML_ERROR_TIMED_OUT
                                             || status == ML_ERROR_TRY_AGAIN);
        EXPECT_TRUE (timeout_cond || no_error_cond);
      } else {
        EXPECT_TRUE (no_error_cond);
      }
    }
    output = NULL;
  }

  status = ml_single_close (single);
  if (ss_data->expect) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  }

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);

  return NULL;
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Testcase with timeout.
 */
TEST (nnstreamer_capi_singleshot, invoke_timeout)
{
  ml_single_h single;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  /* set timeout 5 ms */
  status = ml_single_set_timeout (single, 5);
  /* test timeout if supported (gstreamer ver >= 1.10) */
  if (status == ML_ERROR_NONE) {
    ml_tensors_info_h in_info;
    ml_tensors_data_h input, output;
    ml_tensor_dimension in_dim;

    ml_tensors_info_create (&in_info);

    in_dim[0] = 3;
    in_dim[1] = 224;
    in_dim[2] = 224;
    in_dim[3] = 1;
    ml_tensors_info_set_count (in_info, 1);
    ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
    ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

    input = output = NULL;

    /* generate dummy data */
    status = ml_tensors_data_create (in_info, &input);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (input != NULL);

    status = ml_single_invoke (single, input, &output);
    EXPECT_EQ (status, ML_ERROR_TIMED_OUT);
    EXPECT_TRUE (output == NULL);

    /* check the old buffer is dropped */
    status = ml_single_invoke (single, input, &output);
    /* try_again implies that previous invoke hasn't finished yet */
    EXPECT_TRUE (status == ML_ERROR_TIMED_OUT || status == ML_ERROR_TRY_AGAIN);
    EXPECT_TRUE (output == NULL);

    /* set timeout 10 s */
    status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
    /* clear out previous buffers */
    g_usleep (SINGLE_DEF_TIMEOUT_MSEC * 1000); /** 10 sec */

    status = ml_single_invoke (single, input, &output);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (output != NULL);

    ml_tensors_data_destroy (output);
    ml_tensors_data_destroy (input);
    ml_tensors_info_destroy (in_info);
  }

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Testcase with multiple runs in parallel. Some of the
 *         running instances will timeout, however others will not.
 */
TEST (nnstreamer_capi_singleshot, parallel_runs)
{
  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;
  const gint num_threads = 3;
  const gint num_cases = 3;
  pthread_t thread[num_threads * num_cases];
  single_shot_thread_data ss_data[num_cases];
  guint i, j;

  /* Skip this test if enable-tensorflow-lite is false */
  if (!is_enabled_tensorflow_lite)
    return;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  for (i = 0; i < num_cases; i++) {
    ss_data[i].test_model = test_model;
    ss_data[i].num_runs = 3;
    ss_data[i].min_time_to_run = 10; /** 10 msec */
    ss_data[i].expect = TRUE;
  }

  /** Default timeout runs */
  ss_data[0].timeout = 0;
  /** small timeout runs */
  ss_data[1].timeout = 5;
  /** large timeout runs - increases with each run as tests run in parallel */
  ss_data[2].timeout = SINGLE_DEF_TIMEOUT_MSEC * num_cases * num_threads;

  /**
   * make thread running things in background, each with different timeout,
   * some fails, some runs, all opens pipelines by themselves in parallel
   */
  for (j = 0; j < num_cases; j++) {
    for (i = 0; i < num_threads; i++) {
      pthread_create (&thread[i + j * num_threads], NULL, single_shot_loop_test,
          (void *)&ss_data[j]);
    }
  }

  for (i = 0; i < num_threads * num_cases; i++) {
    pthread_join (thread[i], NULL);
  }

  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Close the single handle while running. This test should not crash.
 *         This closes the single handle twice, while opens it once.
 */
TEST (nnstreamer_capi_singleshot, close_while_running)
{
  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;
  pthread_t thread;
  single_shot_thread_data ss_data;

  /* Skip this test if enable-tensorflow-lite is false */
  if (!is_enabled_tensorflow_lite)
    return;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ss_data.test_model = test_model;
  ss_data.num_runs = 10;
  ss_data.min_time_to_run = 10; /** 10 msec */
  ss_data.expect = FALSE;
  ss_data.timeout = SINGLE_DEF_TIMEOUT_MSEC;
  ss_data.single = NULL;

  pthread_create (&thread, NULL, single_shot_loop_test, (void *)&ss_data);

  /** Start the thread and let the code start */
  g_usleep (100000); /** 100 msec */

  /**
   * Call single API functions while its running. One run takes 100ms on
   * average.
   * So, these calls would in the middle of running and should not crash
   * However, their status can be of failure, if the thread is closed earlier
   */
  if (ss_data.single) {
    ml_single_set_timeout (*ss_data.single, ss_data.timeout);
    ml_single_close (*ss_data.single);
  }

  pthread_join (thread, NULL);

  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Try setting dimensions for input tensor.
 */
TEST (nnstreamer_capi_singleshot, set_input_info_fail_01_n)
{
  int status;
  ml_single_h single;
  ml_tensors_info_h in_info;
  ml_tensor_dimension in_dim;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_set_input_info (single, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_create (&in_info);
  in_dim[0] = 3;
  in_dim[1] = 4;
  in_dim[2] = 4;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  /** mobilenet model does not support setting different input dimension */
  status = ml_single_set_input_info (single, in_info);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_INVALID_PARAMETER);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Try setting number of input tensors and its type
 */
TEST (nnstreamer_capi_singleshot, set_input_info_fail_02_n)
{
  ml_single_h single;
  ml_tensors_info_h in_info;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /** add.tflite adds value 2 to all the values in the input */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (in_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /** changing the count of number of tensors is not allowed */
  ml_tensors_info_set_count (in_info, count + 1);
  status = ml_single_set_input_info (single, in_info);
  EXPECT_NE (status, ML_ERROR_NONE);
  ml_tensors_info_set_count (in_info, count);

  status = ml_tensors_info_get_tensor_type (in_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  /** changing the type of input tensors is not allowed */
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT32);
  status = ml_single_set_input_info (single, in_info);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Try setting dimension to the same value. This model does not allow
 *         changing the dimension to a different. However, setting the same
 *         value for dimension should be successful.
 */
TEST (nnstreamer_capi_singleshot, set_input_info_success)
{
  int status;
  ml_single_h single;
  ml_tensors_info_h in_info;
  ml_tensor_dimension in_dim;
  ml_tensors_data_h input, output;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_set_input_info (single, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  ml_tensors_info_create (&in_info);
  in_dim[0] = 3;
  in_dim[1] = 224;
  in_dim[2] = 224;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  /** set the same original input dimension */
  status = ml_single_set_input_info (single, in_info);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  /* generate dummy data */
  input = output = NULL;
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_destroy (in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Change the number of input tensors, run the model and verify output
 */
TEST (nnstreamer_capi_singleshot, set_input_info_success_01)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res = nullptr, out_res = nullptr;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim, res_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  int status, tensor_size;
  size_t data_size;
  float *data;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /** add.tflite adds value 2 to all the values in the input */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  tensor_size = 5;

  in_dim[0] = tensor_size;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = tensor_size;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_get_input_info (single, &in_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /**
   * 1. start with a model file with different input dimensions
   * 2. change the input for the model file
   * 3. run the model file with the updated input dimensions
   * 4. verify the output
   */

  ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
  EXPECT_FALSE (in_dim[0] == res_dim[0]);
  EXPECT_TRUE (in_dim[1] == res_dim[1]);
  EXPECT_TRUE (in_dim[2] == res_dim[2]);
  EXPECT_TRUE (in_dim[3] == res_dim[3]);

  /** set the same original input dimension */
  status = ml_single_set_input_info (single, in_info);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);
  if (status == ML_ERROR_NONE) {
    /* input tensor in filter */
    ml_tensors_info_destroy (in_res);
    status = ml_single_get_input_info (single, &in_res);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_tensors_info_get_count (in_res, &count);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (count, 1U);

    status = ml_tensors_info_get_tensor_type (in_res, 0, &type);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

    ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
    EXPECT_TRUE (in_dim[0] == res_dim[0]);
    EXPECT_TRUE (in_dim[1] == res_dim[1]);
    EXPECT_TRUE (in_dim[2] == res_dim[2]);
    EXPECT_TRUE (in_dim[3] == res_dim[3]);

    /* output tensor in filter */
    status = ml_single_get_output_info (single, &out_res);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_tensors_info_get_count (out_res, &count);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (count, 1U);

    status = ml_tensors_info_get_tensor_type (out_res, 0, &type);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

    ml_tensors_info_get_tensor_dimension (out_res, 0, res_dim);
    EXPECT_TRUE (out_dim[0] == res_dim[0]);
    EXPECT_TRUE (out_dim[1] == res_dim[1]);
    EXPECT_TRUE (out_dim[2] == res_dim[2]);
    EXPECT_TRUE (out_dim[3] == res_dim[3]);

    input = output = NULL;

    /* generate dummy data */
    status = ml_tensors_data_create (in_info, &input);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (input != NULL);

    status = ml_tensors_data_get_tensor_data (input, 0, (void **)&data, &data_size);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (data_size, tensor_size * sizeof (int));
    for (int idx = 0; idx < tensor_size; idx++)
      data[idx] = idx;

    status = ml_single_invoke (single, input, &output);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (output != NULL);

    status = ml_tensors_data_get_tensor_data (input, 0, (void **)&data, &data_size);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (data_size, tensor_size * sizeof (int));
    for (int idx = 0; idx < tensor_size; idx++)
      EXPECT_EQ (data[idx], idx);

    status = ml_tensors_data_get_tensor_data (output, 0, (void **)&data, &data_size);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (data_size, tensor_size * sizeof (int));
    for (int idx = 0; idx < tensor_size; idx++)
      EXPECT_EQ (data[idx], idx + 2);

    ml_tensors_data_destroy (output);
    ml_tensors_data_destroy (input);
  }

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Update property 'layout' for input tensor
 */
TEST (nnstreamer_capi_singleshot, property_01_p)
{
  ml_single_h single;
  ml_tensors_info_h in_info;
  ml_tensors_data_h input, output;
  int status;
  char *prop_value;
  void *data;
  size_t data_size;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  /* get layout */
  status = ml_single_get_property (single, "inputlayout", &prop_value);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_STREQ (prop_value, "ANY");
  g_free (prop_value);

  /* get updatable */
  status = ml_single_get_property (single, "is-updatable", &prop_value);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_STREQ (prop_value, "false");
  g_free (prop_value);

  /* get input info */
  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /* invoke */
  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_tensors_data_get_tensor_data (output, 0, (void **)&data, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (data_size, 1001U);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Failure case to set invalid property
 */
TEST (nnstreamer_capi_singleshot, property_02_n)
{
  ml_single_h single;
  int status;
  char *prop_value = NULL;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  /* get invalid property */
  status = ml_single_get_property (single, "unknown_prop", &prop_value);
  EXPECT_NE (status, ML_ERROR_NONE);
  g_free (prop_value);

  /* set invalid property */
  status = ml_single_set_property (single, "unknown_prop", "INVALID");
  EXPECT_NE (status, ML_ERROR_NONE);

  /* null params */
  status = ml_single_set_property (single, "input", NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_single_set_property (single, NULL, "INVALID");
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_single_get_property (single, "input", NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_single_get_property (single, NULL, &prop_value);
  EXPECT_NE (status, ML_ERROR_NONE);
  g_free (prop_value);

  /* dimension should be valid */
  status = ml_single_get_property (single, "input", &prop_value);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_STREQ (prop_value, "3:224:224:1");
  g_free (prop_value);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Failure case to set meta property
 */
TEST (nnstreamer_capi_singleshot, property_03_n)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension in_dim, out_dim;
  ml_tensors_data_h input, output;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  int status;
  unsigned int count = 0;
  char *name = NULL;
  char *prop_value = NULL;
  void *data;
  size_t data_size;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  /* failed to set dimension */
  status = ml_single_set_property (single, "input", "3:4:4:1");
  EXPECT_NE (status, ML_ERROR_NONE);

  status = ml_single_get_property (single, "input", &prop_value);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_STREQ (prop_value, "3:224:224:1");
  g_free (prop_value);

  /* input tensor in filter */
  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (in_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (in_info, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (in_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  ml_tensors_info_get_tensor_dimension (in_info, 0, in_dim);
  EXPECT_EQ (in_dim[0], 3U);
  EXPECT_EQ (in_dim[1], 224U);
  EXPECT_EQ (in_dim[2], 224U);
  EXPECT_EQ (in_dim[3], 1U);

  /* output tensor in filter */
  status = ml_single_get_output_info (single, &out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_info, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (out_info, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (out_info, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_UINT8);

  ml_tensors_info_get_tensor_dimension (out_info, 0, out_dim);
  EXPECT_EQ (out_dim[0], 1001U);
  EXPECT_EQ (out_dim[1], 1U);
  EXPECT_EQ (out_dim[2], 1U);
  EXPECT_EQ (out_dim[3], 1U);

  /* invoke */
  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_tensors_data_get_tensor_data (output, 0, (void **)&data, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (data_size, 1001U);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (input);
  ml_tensors_data_destroy (output);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tensorflow-lite)
 * @detail Update dimension for input tensor
 */
TEST (nnstreamer_capi_singleshot, property_04_p)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim;
  char *prop_value;
  int status;
  size_t data_size;
  float *data;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /** add.tflite adds value 2 to all the values in the input */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_set_property (single, "input", "5:1:1:1");
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_get_property (single, "input", &prop_value);
  EXPECT_EQ (status, ML_ERROR_NONE);

  EXPECT_STREQ (prop_value, "5:1:1:1");
  g_free (prop_value);

  /* validate in/out info */
  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_get_tensor_dimension (in_info, 0, in_dim);
  EXPECT_EQ (in_dim[0], 5U);
  EXPECT_EQ (in_dim[1], 1U);
  EXPECT_EQ (in_dim[2], 1U);
  EXPECT_EQ (in_dim[3], 1U);

  status = ml_single_get_output_info (single, &out_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_get_tensor_dimension (out_info, 0, out_dim);
  EXPECT_EQ (out_dim[0], 5U);
  EXPECT_EQ (out_dim[1], 1U);
  EXPECT_EQ (out_dim[2], 1U);
  EXPECT_EQ (out_dim[3], 1U);

  /* invoke */
  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_tensors_data_get_tensor_data (input, 0, (void **)&data, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (data_size, 5 * sizeof (float));
  for (int idx = 0; idx < 5; idx++)
    data[idx] = idx;

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_tensors_data_get_tensor_data (output, 0, (void **)&data, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (data_size, 5 * sizeof (float));
  for (int idx = 0; idx < 5; idx++)
    EXPECT_EQ (data[idx], idx + 2);

  ml_tensors_data_destroy (input);
  ml_tensors_data_destroy (output);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

skip_test:
  g_free (test_model);
}

#ifdef ENABLE_NNFW_RUNTIME
/**
 * @brief Test NNStreamer single shot (nnfw backend)
 */
TEST (nnstreamer_capi_singleshot, invoke_05)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res, out_res;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim, res_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  char *name = NULL;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 1;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 1;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  status = ml_single_open (
      &single, test_model, in_info, out_info, ML_NNFW_TYPE_NNFW, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* input tensor in filter */
  status = ml_single_get_input_info (single, &in_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (in_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (in_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (in_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
  EXPECT_TRUE (in_dim[0] == res_dim[0]);
  EXPECT_TRUE (in_dim[1] == res_dim[1]);
  EXPECT_TRUE (in_dim[2] == res_dim[2]);
  EXPECT_TRUE (in_dim[3] == res_dim[3]);

  /* output tensor in filter */
  status = ml_single_get_output_info (single, &out_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (out_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (out_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (out_res, 0, res_dim);
  EXPECT_TRUE (out_dim[0] == res_dim[0]);
  EXPECT_TRUE (out_dim[1] == res_dim[1]);
  EXPECT_TRUE (out_dim[2] == res_dim[2]);
  EXPECT_TRUE (out_dim[3] == res_dim[3]);

  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);

  g_free (test_model);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);
}

/**
 * @brief Test NNStreamer single shot (nnfw backend using model path)
 */
TEST (nnstreamer_capi_singleshot, open_dir)
{
  ml_single_h single;
  int status;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL, ML_NNFW_TYPE_NNFW, ML_NNFW_HW_ANY);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (test_model);
}

#endif /* ENABLE_NNFW_RUNTIME */

#ifdef ENABLE_ARMNN
/**
 * @brief Test NNStreamer single shot (caffe/armnn)
 * @detail Run pipeline with caffe lenet model.
 */
TEST (nnstreamer_capi_singleshot, invoke_06)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res, out_res;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim, res_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  char *name = NULL;
  int status, max_score_index;
  float score, max_score;
  void *data_ptr;
  size_t data_size;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model, *test_file;
  guint8 *contents_uint8 = NULL;
  gfloat *contents_float = NULL;
  gsize len = 0;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "lenet_iter_9000.caffemodel", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  test_file = g_build_filename (root_path, "tests", "test_models", "data", "9.raw", NULL);
  ASSERT_TRUE (g_file_test (test_file, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 28;
  in_dim[1] = 28;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_name (in_info, 0, "data");
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 10;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_name (out_info, 0, "prob");
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  ASSERT_TRUE (g_file_get_contents (test_file, (gchar **)&contents_uint8, &len, NULL));
  status = ml_tensors_info_get_tensor_size (in_info, 0, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ASSERT_TRUE (len == data_size / sizeof (float));

  /** Convert uint8 data with range [0, 255] to float with range [-1, 1] */
  contents_float = (gfloat *)g_malloc (data_size);
  for (unsigned int idx = 0; idx < len; idx++) {
    contents_float[idx] = static_cast<float> (contents_uint8[idx]);
    contents_float[idx] -= 127.5;
    contents_float[idx] /= 127.5;
  }

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_ARMNN, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* input tensor in filter */
  status = ml_single_get_input_info (single, &in_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (in_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (in_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (name, "data");
  g_free (name);

  status = ml_tensors_info_get_tensor_type (in_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
  EXPECT_TRUE (in_dim[0] == res_dim[0]);
  EXPECT_TRUE (in_dim[1] == res_dim[1]);
  EXPECT_TRUE (in_dim[2] == res_dim[2]);
  EXPECT_TRUE (in_dim[3] == res_dim[3]);

  /* output tensor in filter */
  status = ml_single_get_output_info (single, &out_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (out_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_STREQ (name, "prob");
  g_free (name);

  status = ml_tensors_info_get_tensor_type (out_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (out_res, 0, res_dim);
  EXPECT_TRUE (out_dim[0] == res_dim[0]);
  EXPECT_TRUE (out_dim[1] == res_dim[1]);
  EXPECT_TRUE (out_dim[2] == res_dim[2]);
  EXPECT_TRUE (out_dim[3] == res_dim[3]);

  input = output = NULL;

  /* generate input data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_tensors_data_set_tensor_data (input, 0, contents_float, data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_tensors_data_get_tensor_data (output, 1, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_tensors_data_get_tensor_data (output, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  max_score = .0;
  max_score_index = 0;
  for (gint i = 0; i < 10; i++) {
    score = ((float *)data_ptr)[i];
    if (score > max_score) {
      max_score = score;
      max_score_index = i;
    }
  }

  EXPECT_EQ (max_score_index, 9);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);

  g_free (test_model);
  g_free (test_file);
  g_free (contents_uint8);
  g_free (contents_float);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);
}

/**
 * @brief Test NNStreamer single shot (tflite/armnn)
 * @detail Run pipeline with tflite basic model.
 */
TEST (nnstreamer_capi_singleshot, invoke_07)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res, out_res;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim, res_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  char *name = NULL;
  int status;
  void *data_ptr;
  size_t data_size;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 1;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 1;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  status = ml_single_open (&single, test_model, NULL, NULL, ML_NNFW_TYPE_ARMNN, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* input tensor in filter */
  status = ml_single_get_input_info (single, &in_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (in_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (in_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (in_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
  EXPECT_TRUE (in_dim[0] == res_dim[0]);
  EXPECT_TRUE (in_dim[1] == res_dim[1]);
  EXPECT_TRUE (in_dim[2] == res_dim[2]);
  EXPECT_TRUE (in_dim[3] == res_dim[3]);

  /* output tensor in filter */
  status = ml_single_get_output_info (single, &out_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_info_get_count (out_res, &count);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (count, 1U);

  status = ml_tensors_info_get_tensor_name (out_res, 0, &name);
  EXPECT_EQ (status, ML_ERROR_NONE);
  g_free (name);

  status = ml_tensors_info_get_tensor_type (out_res, 0, &type);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

  ml_tensors_info_get_tensor_dimension (out_res, 0, res_dim);
  EXPECT_TRUE (out_dim[0] == res_dim[0]);
  EXPECT_TRUE (out_dim[1] == res_dim[1]);
  EXPECT_TRUE (out_dim[2] == res_dim[2]);
  EXPECT_TRUE (out_dim[3] == res_dim[3]);

  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_tensors_data_get_tensor_data (input, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  ((float *)data_ptr)[0] = 10.0;

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_tensors_data_get_tensor_data (output, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_EQ (((float *)data_ptr)[0], 12.0);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);

  g_free (test_model);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);
}

/**
 * @brief Test NNStreamer single shot (caffe/armnn)
 * @detail Failure open with invalid param.
 */
TEST (nnstreamer_capi_singleshot, open_fail_03_n)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensor_dimension in_dim, out_dim;
  int status;
  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "lenet_iter_9000.caffemodel", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  /** Set the correct input/output info */
  in_dim[0] = 28;
  in_dim[1] = 28;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_name (in_info, 0, "data");
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 10;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_name (out_info, 0, "prob");
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  /** Modify the input or output name to be wrong and open */
  ml_tensors_info_set_tensor_name (in_info, 0, "data1");
  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_ARMNN, ML_NNFW_HW_ANY);
  EXPECT_NE (status, ML_ERROR_NONE);
  ml_tensors_info_set_tensor_name (in_info, 0, "data");

  ml_tensors_info_set_tensor_name (out_info, 0, "prob1");
  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_ARMNN, ML_NNFW_HW_ANY);
  EXPECT_NE (status, ML_ERROR_NONE);
  ml_tensors_info_set_tensor_name (out_info, 0, "prob");

  /**
   * Modify the input dim to be wrong and open
   * output dim is not used for caffe, so wrong output dim will pass open
   * but will fail at invoke (check nnstreamer_capi_singleshot.invoke_07_n)
   */
  ml_tensors_info_set_tensor_dimension (in_info, 0, out_dim);
  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_ARMNN, ML_NNFW_HW_ANY);
  EXPECT_NE (status, ML_ERROR_NONE);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  g_free (test_model);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test NNStreamer single shot (caffe/armnn)
 * @detail Failure invoke with invalid param.
 */
TEST (nnstreamer_capi_singleshot, invoke_08_n)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim;
  int status;
  size_t data_size;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;
  gfloat *contents_float = NULL;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "lenet_iter_9000.caffemodel", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  in_dim[0] = 28;
  in_dim[1] = 28;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_name (in_info, 0, "data");
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 10;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_name (out_info, 0, "prob");
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);
  ml_tensors_info_set_tensor_dimension (out_info, 0, in_dim);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_ARMNN, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  input = output = NULL;

  /* generate input data with wrong info */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_tensors_info_get_tensor_size (in_info, 0, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  contents_float = (gfloat *)g_malloc (data_size);
  status = ml_tensors_data_set_tensor_data (input, 0, contents_float, data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_NE (status, ML_ERROR_NONE);
  EXPECT_TRUE (output == NULL);

  ml_tensors_data_destroy (input);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (test_model);
  g_free (contents_float);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
}

/**
 * @brief Test NNStreamer single shot (caffe/armnn)
 * @detail Failure invoke with invalid param.
 */
TEST (nnstreamer_capi_singleshot, invoke_09_n)
{
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res, out_res;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim;
  int status;
  size_t data_size;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;
  gfloat *contents_float = NULL;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (root_path, "tests", "test_models", "models",
      "lenet_iter_9000.caffemodel", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);
  ml_tensors_info_create (&in_res);
  ml_tensors_info_create (&out_res);

  in_dim[0] = 28;
  in_dim[1] = 28;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_name (in_info, 0, "data");
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = 10;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_name (out_info, 0, "prob");
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_ARMNN, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  input = output = NULL;

  /* generate input data with wrong info */
  status = ml_tensors_data_create (out_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_tensors_info_get_tensor_size (out_info, 0, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  contents_float = (gfloat *)g_malloc (data_size);
  status = ml_tensors_data_set_tensor_data (input, 0, contents_float, data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_set_timeout (single, SINGLE_DEF_TIMEOUT_MSEC);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);

  status = ml_single_invoke (single, input, &output);
  EXPECT_NE (status, ML_ERROR_NONE);
  EXPECT_TRUE (output == NULL);

  ml_tensors_data_destroy (input);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  g_free (test_model);
  g_free (contents_float);
  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);
}
#endif /* ENABLE_ARMNN */

/**
 * @brief Test NNStreamer single shot (custom filter)
 * @detail Run pipeline with custom filter with allocate in invoke, handle multi tensors.
 */
TEST (nnstreamer_capi_singleshot, invoke_10_p)
{
  const gchar cf_name[] = "libnnstreamer_customfilter_scaler_allocator" SO_FILE_EXTENSION;
  gchar *lib_path = NULL;
  gchar *test_model = NULL;
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim;
  int status;
  unsigned int i;
  void *data_ptr;
  size_t data_size;

  lib_path = nnsconf_get_custom_value_string ("filter", "customfilters");
  if (lib_path == NULL) {
    /* cannot get custom-filter directory */
    goto skip_test;
  }

  test_model = g_build_filename (lib_path, cf_name, NULL);
  if (!g_file_test (test_model, G_FILE_TEST_EXISTS)) {
    goto skip_test;
  }

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (in_info, 1);

  in_dim[0] = 10;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;

  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT16);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  ml_tensors_info_clone (out_info, in_info);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_CUSTOM_FILTER, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  input = output = NULL;

  /* generate input data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (input != NULL);

  status = ml_tensors_data_get_tensor_data (input, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  for (i = 0; i < 10; i++) {
    ((int16_t *)data_ptr)[i] = (int16_t) (i + 1);
  }

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  /**
   * since the output data was allocated by the tensor filter element in the
   * single API,
   * closing this single handle will also delete the data
   */
  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_data_destroy (output);

skip_test:
  g_free (lib_path);
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (custom filter)
 * @detail Run pipeline with custom filter with allocate in invoke, handle multi tensors.
 */
TEST (nnstreamer_capi_singleshot, invoke_11_p)
{
  const gchar cf_name[] = "libnnstreamer_customfilter_scaler_allocator" SO_FILE_EXTENSION;
  gchar *lib_path = NULL;
  gchar *test_model = NULL;
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim;
  int status;
  unsigned int i;
  void *data_ptr;
  size_t data_size;

  lib_path = nnsconf_get_custom_value_string ("filter", "customfilters");
  if (lib_path == NULL) {
    /* cannot get custom-filter directory */
    goto skip_test;
  }

  test_model = g_build_filename (lib_path, cf_name, NULL);
  if (!g_file_test (test_model, G_FILE_TEST_EXISTS)) {
    goto skip_test;
  }

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (in_info, 1);

  in_dim[0] = 10;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;

  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT16);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  ml_tensors_info_clone (out_info, in_info);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_CUSTOM_FILTER, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  input = output = NULL;

  /* generate input data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (input != NULL);

  status = ml_tensors_data_get_tensor_data (input, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  for (i = 0; i < 10; i++) {
    ((int16_t *)data_ptr)[i] = (int16_t) (i + 1);
  }

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /** Access data before destroy works */
  status = ml_tensors_data_get_tensor_data (output, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_destroy (output);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /** Close handle works normally */
  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);

skip_test:
  g_free (lib_path);
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (custom filter)
 * @detail Run pipeline with custom filter with allocate in invoke, handle multi tensors.
 */
TEST (nnstreamer_capi_singleshot, invoke_12_p)
{
  const gchar cf_name[] = "libnnstreamer_customfilter_scaler_allocator" SO_FILE_EXTENSION;
  gchar *lib_path = NULL;
  gchar *test_model = NULL;
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output1, output2;
  ml_tensor_dimension in_dim;
  int status;
  unsigned int i;
  void *data_ptr;
  size_t data_size;

  lib_path = nnsconf_get_custom_value_string ("filter", "customfilters");
  if (lib_path == NULL) {
    /* cannot get custom-filter directory */
    goto skip_test;
  }

  test_model = g_build_filename (lib_path, cf_name, NULL);
  if (!g_file_test (test_model, G_FILE_TEST_EXISTS)) {
    goto skip_test;
  }

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  ml_tensors_info_set_count (in_info, 1);

  in_dim[0] = 10;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;

  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_INT16);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  ml_tensors_info_clone (out_info, in_info);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_CUSTOM_FILTER, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  input = output1 = output2 = NULL;

  /* generate input data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  ASSERT_TRUE (input != NULL);

  status = ml_tensors_data_get_tensor_data (input, 0, &data_ptr, &data_size);
  EXPECT_EQ (status, ML_ERROR_NONE);
  for (i = 0; i < 10; i++) {
    ((int16_t *)data_ptr)[i] = (int16_t) (i + 1);
  }

  status = ml_single_invoke (single, input, &output1);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output1 != NULL);

  status = ml_single_invoke (single, input, &output2);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output2 != NULL);

  status = ml_tensors_data_destroy (input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_destroy (output1);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /** Destroy the other data by closing the handle */
  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_data_destroy (output2);

skip_test:
  g_free (lib_path);
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (custom filter)
 * @detail Change the number of input tensors, run the model and verify output
 */
TEST (nnstreamer_capi_singleshot, set_input_info_success_02)
{
  const gchar cf_name[] = "libnnstreamer_customfilter_passthrough_variable" SO_FILE_EXTENSION;
  gchar *lib_path = NULL;
  gchar *test_model = NULL;
  ml_single_h single;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_info_h in_res = nullptr, out_res = nullptr;
  ml_tensors_data_h input, output;
  ml_tensor_dimension in_dim, out_dim, res_dim;
  ml_tensor_type_e type = ML_TENSOR_TYPE_UNKNOWN;
  unsigned int count = 0;
  int status, tensor_size;

  lib_path = nnsconf_get_custom_value_string ("filter", "customfilters");
  if (lib_path == NULL) {
    /* cannot get custom-filter directory */
    goto skip_test;
  }

  test_model = g_build_filename (lib_path, cf_name, NULL);
  if (!g_file_test (test_model, G_FILE_TEST_EXISTS)) {
    goto skip_test;
  }

  ml_tensors_info_create (&in_info);
  ml_tensors_info_create (&out_info);

  tensor_size = 5;
  in_dim[0] = tensor_size;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);

  out_dim[0] = tensor_size;
  out_dim[1] = 1;
  out_dim[2] = 1;
  out_dim[3] = 1;
  ml_tensors_info_set_count (out_info, 1);
  ml_tensors_info_set_tensor_type (out_info, 0, ML_TENSOR_TYPE_FLOAT32);
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  status = ml_single_open (&single, test_model, in_info, out_info,
      ML_NNFW_TYPE_CUSTOM_FILTER, ML_NNFW_HW_ANY);
  ASSERT_EQ (status, ML_ERROR_NONE);

  /* Run the model once with the original input/output info */
  input = output = NULL;

  /* generate dummy data */
  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (input != NULL);

  status = ml_single_invoke (single, input, &output);
  EXPECT_EQ (status, ML_ERROR_NONE);
  EXPECT_TRUE (output != NULL);

  ml_tensors_data_destroy (output);
  ml_tensors_data_destroy (input);

  /** modify input/output info and run again */
  in_dim[0] = 10;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  out_dim[0] = 10;
  ml_tensors_info_set_tensor_dimension (out_info, 0, out_dim);

  status = ml_single_get_input_info (single, &in_res);
  EXPECT_EQ (status, ML_ERROR_NONE);

  /**
   * 1. start with a model file with different input dimensions
   * 2. change the input for the model file
   * 3. run the model file with the updated input dimensions
   * 4. verify the output
   */

  ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
  EXPECT_FALSE (in_dim[0] == res_dim[0]);
  EXPECT_TRUE (in_dim[1] == res_dim[1]);
  EXPECT_TRUE (in_dim[2] == res_dim[2]);
  EXPECT_TRUE (in_dim[3] == res_dim[3]);

  /** set the same original input dimension */
  status = ml_single_set_input_info (single, in_info);
  EXPECT_TRUE (status == ML_ERROR_NOT_SUPPORTED || status == ML_ERROR_NONE);
  if (status == ML_ERROR_NONE) {
    /* input tensor in filter */
    ml_tensors_info_destroy (in_res);
    status = ml_single_get_input_info (single, &in_res);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_tensors_info_get_count (in_res, &count);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (count, 1U);

    status = ml_tensors_info_get_tensor_type (in_res, 0, &type);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

    ml_tensors_info_get_tensor_dimension (in_res, 0, res_dim);
    EXPECT_TRUE (in_dim[0] == res_dim[0]);
    EXPECT_TRUE (in_dim[1] == res_dim[1]);
    EXPECT_TRUE (in_dim[2] == res_dim[2]);
    EXPECT_TRUE (in_dim[3] == res_dim[3]);

    /* output tensor in filter */
    status = ml_single_get_output_info (single, &out_res);
    EXPECT_EQ (status, ML_ERROR_NONE);

    status = ml_tensors_info_get_count (out_res, &count);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (count, 1U);

    status = ml_tensors_info_get_tensor_type (out_res, 0, &type);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_EQ (type, ML_TENSOR_TYPE_FLOAT32);

    ml_tensors_info_get_tensor_dimension (out_res, 0, res_dim);
    EXPECT_TRUE (out_dim[0] == res_dim[0]);
    EXPECT_TRUE (out_dim[1] == res_dim[1]);
    EXPECT_TRUE (out_dim[2] == res_dim[2]);
    EXPECT_TRUE (out_dim[3] == res_dim[3]);

    input = output = NULL;

    /* generate dummy data */
    status = ml_tensors_data_create (in_info, &input);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (input != NULL);

    status = ml_single_invoke (single, input, &output);
    EXPECT_EQ (status, ML_ERROR_NONE);
    EXPECT_TRUE (output != NULL);

    ml_tensors_data_destroy (output);
    ml_tensors_data_destroy (input);
  }

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_info_destroy (in_info);
  ml_tensors_info_destroy (out_info);
  ml_tensors_info_destroy (in_res);
  ml_tensors_info_destroy (out_res);

skip_test:
  g_free (lib_path);
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tflite)
 * @detail run the `ml_single_invoke_dynamic` api works properly.
 */
TEST (nnstreamer_capi_singleshot, invoke_dynamic_success_01_p)
{
  ml_single_h single;
  int status;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  size_t data_size;

  unsigned int tmp_count;
  ml_tensor_type_e tmp_type = ML_TENSOR_TYPE_UNKNOWN;
  ml_tensor_dimension tmp_dim;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* dynamic dimension supported */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);
  {
    float tmp_input[] = { 1.0 };
    float *output_buf;
    status = ml_tensors_data_set_tensor_data (input, 0, tmp_input, 1 * sizeof (float));

    ml_tensors_info_get_count (in_info, &tmp_count);
    ml_tensors_info_get_tensor_type (in_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (in_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 1U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    status = ml_single_invoke_dynamic (single, input, in_info, &output, &out_info);
    EXPECT_EQ (status, ML_ERROR_NONE);

    ml_tensors_data_get_tensor_data (output, 0, (void **)&output_buf, &data_size);

    EXPECT_FLOAT_EQ (output_buf[0], 3.0f);
    EXPECT_EQ (data_size, sizeof (float));

    ml_tensors_info_get_count (out_info, &tmp_count);
    ml_tensors_info_get_tensor_type (out_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (out_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 1U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    ml_tensors_data_destroy (output);
    ml_tensors_data_destroy (input);
    ml_tensors_info_destroy (in_info);
    ml_tensors_info_destroy (out_info);
  }

  status = ml_single_set_property (single, "input", "5:1:1:1");
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  {
    float tmp_input2[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    float *output_buf2;
    status = ml_tensors_data_set_tensor_data (input, 0, tmp_input2, 5 * sizeof (float));

    ml_tensors_info_get_count (in_info, &tmp_count);
    ml_tensors_info_get_tensor_type (in_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (in_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 5U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    status = ml_single_invoke_dynamic (single, input, in_info, &output, &out_info);
    EXPECT_EQ (status, ML_ERROR_NONE);

    ml_tensors_data_get_tensor_data (output, 0, (void **)&output_buf2, &data_size);

    EXPECT_FLOAT_EQ (output_buf2[0], 3.0f);
    EXPECT_FLOAT_EQ (output_buf2[1], 4.0f);
    EXPECT_FLOAT_EQ (output_buf2[2], 5.0f);
    EXPECT_FLOAT_EQ (output_buf2[3], 6.0f);
    EXPECT_FLOAT_EQ (output_buf2[4], 7.0f);
    EXPECT_EQ (data_size, 5 * sizeof (float));

    ml_tensors_info_get_count (out_info, &tmp_count);
    ml_tensors_info_get_tensor_type (out_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (out_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 5U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    status = ml_single_close (single);
    EXPECT_EQ (status, ML_ERROR_NONE);

    ml_tensors_data_destroy (output);
    ml_tensors_data_destroy (input);
    ml_tensors_info_destroy (in_info);
    ml_tensors_info_destroy (out_info);
  }
skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tflite)
 * @detail run the `ml_single_invoke_dynamic` api works properly.
 */
TEST (nnstreamer_capi_singleshot, invoke_dynamic_success_02_p)
{
  ml_single_h single;
  int status;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;
  size_t data_size;

  unsigned int tmp_count;
  ml_tensor_type_e tmp_type = ML_TENSOR_TYPE_UNKNOWN;
  ml_tensor_dimension tmp_dim, in_dim;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* dynamic dimension supported */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  {
    float tmp_input[] = { 1.0 };
    float *output_buf;

    status = ml_tensors_data_set_tensor_data (input, 0, tmp_input, 1 * sizeof (float));

    ml_tensors_info_get_count (in_info, &tmp_count);
    ml_tensors_info_get_tensor_type (in_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (in_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 1U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    status = ml_single_invoke_dynamic (single, input, in_info, &output, &out_info);
    EXPECT_EQ (status, ML_ERROR_NONE);

    ml_tensors_data_get_tensor_data (output, 0, (void **)&output_buf, &data_size);
    EXPECT_FLOAT_EQ (output_buf[0], 3.0f);
    EXPECT_EQ (data_size, sizeof (float));

    ml_tensors_info_get_count (out_info, &tmp_count);
    ml_tensors_info_get_tensor_type (out_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (out_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 1U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    ml_tensors_data_destroy (output);
    ml_tensors_data_destroy (input);
    ml_tensors_info_destroy (in_info);
    ml_tensors_info_destroy (out_info);
  }

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  in_dim[0] = 5;
  in_dim[1] = 1;
  in_dim[2] = 1;
  in_dim[3] = 1;

  status = ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  {
    float tmp_input2[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    float *output_buf2;
    status = ml_tensors_data_set_tensor_data (input, 0, tmp_input2, 5 * sizeof (float));

    ml_tensors_info_get_count (in_info, &tmp_count);
    ml_tensors_info_get_tensor_type (in_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (in_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 5U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    status = ml_single_invoke_dynamic (single, input, in_info, &output, &out_info);
    EXPECT_EQ (status, ML_ERROR_NONE);

    ml_tensors_data_get_tensor_data (output, 0, (void **)&output_buf2, &data_size);

    EXPECT_FLOAT_EQ (output_buf2[0], 3.0f);
    EXPECT_FLOAT_EQ (output_buf2[1], 4.0f);
    EXPECT_FLOAT_EQ (output_buf2[2], 5.0f);
    EXPECT_FLOAT_EQ (output_buf2[3], 6.0f);
    EXPECT_FLOAT_EQ (output_buf2[4], 7.0f);
    EXPECT_EQ (data_size, 5 * sizeof (float));

    ml_tensors_info_get_count (out_info, &tmp_count);
    ml_tensors_info_get_tensor_type (out_info, 0, &tmp_type);
    ml_tensors_info_get_tensor_dimension (out_info, 0, tmp_dim);

    EXPECT_EQ (tmp_count, 1U);
    EXPECT_EQ (tmp_type, ML_TENSOR_TYPE_FLOAT32);
    EXPECT_EQ (tmp_dim[0], 5U);
    EXPECT_EQ (tmp_dim[1], 1U);
    EXPECT_EQ (tmp_dim[2], 1U);
    EXPECT_EQ (tmp_dim[3], 1U);

    status = ml_single_close (single);
    EXPECT_EQ (status, ML_ERROR_NONE);

    ml_tensors_data_destroy (output);
    ml_tensors_data_destroy (input);
    ml_tensors_info_destroy (in_info);
    ml_tensors_info_destroy (out_info);
  }

skip_test:
  g_free (test_model);
}

/**
 * @brief Test NNStreamer single shot (tflite)
 * @detail check the `ml_single_invoke_dynamic` api handles exception cases well.
 */
TEST (nnstreamer_capi_singleshot, invoke_dynamic_fail_n)
{
  ml_single_h single;
  int status;
  ml_tensors_info_h in_info, out_info;
  ml_tensors_data_h input, output;

  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  gchar *test_model;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* dynamic dimension supported */
  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  status = ml_single_open (&single, test_model, NULL, NULL,
      ML_NNFW_TYPE_TENSORFLOW_LITE, ML_NNFW_HW_ANY);
  if (is_enabled_tensorflow_lite) {
    EXPECT_EQ (status, ML_ERROR_NONE);
  } else {
    EXPECT_NE (status, ML_ERROR_NONE);
    goto skip_test;
  }

  status = ml_single_get_input_info (single, &in_info);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_tensors_data_create (in_info, &input);
  EXPECT_EQ (status, ML_ERROR_NONE);

  status = ml_single_invoke_dynamic (NULL, input, in_info, &output, &out_info);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_invoke_dynamic (single, NULL, in_info, &output, &out_info);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_invoke_dynamic (single, input, NULL, &output, &out_info);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_invoke_dynamic (single, input, in_info, NULL, &out_info);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_invoke_dynamic (single, input, in_info, &output, NULL);
  EXPECT_EQ (status, ML_ERROR_INVALID_PARAMETER);

  status = ml_single_close (single);
  EXPECT_EQ (status, ML_ERROR_NONE);

  ml_tensors_data_destroy (input);
  ml_tensors_info_destroy (in_info);

skip_test:
  g_free (test_model);
}

/**
 * @brief Test utility functions (private)
 * @details check sub-plugin type and name
 */
TEST (nnstreamer_capi_util, nnfw_name_01_p)
{
  EXPECT_STREQ (_ml_get_nnfw_subplugin_name (ML_NNFW_TYPE_TENSORFLOW_LITE), "tensorflow-lite");
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("tensorflow-lite"), ML_NNFW_TYPE_TENSORFLOW_LITE);
  EXPECT_STREQ (_ml_get_nnfw_subplugin_name (ML_NNFW_TYPE_TENSORFLOW), "tensorflow");
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("tensorflow"), ML_NNFW_TYPE_TENSORFLOW);
  EXPECT_STREQ (_ml_get_nnfw_subplugin_name (ML_NNFW_TYPE_NNFW), "nnfw");
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("nnfw"), ML_NNFW_TYPE_NNFW);
  EXPECT_STREQ (_ml_get_nnfw_subplugin_name (ML_NNFW_TYPE_VIVANTE), "vivante");
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("vivante"), ML_NNFW_TYPE_VIVANTE);
  EXPECT_STREQ (_ml_get_nnfw_subplugin_name (ML_NNFW_TYPE_SNAP), "snap");
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("snap"), ML_NNFW_TYPE_SNAP);
  EXPECT_STREQ (_ml_get_nnfw_subplugin_name (ML_NNFW_TYPE_MXNET), "mxnet");
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("mxnet"), ML_NNFW_TYPE_MXNET);
  EXPECT_STREQ (_ml_get_nnfw_subplugin_name (ML_NNFW_TYPE_TVM), "tvm");
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("tvm"), ML_NNFW_TYPE_TVM);
}

/**
 * @brief Test utility functions (private)
 * @details check sub-plugin type and name
 */
TEST (nnstreamer_capi_util, nnfw_name_02_n)
{
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name ("invalid-fw"), ML_NNFW_TYPE_ANY);
  EXPECT_EQ (_ml_get_nnfw_type_by_subplugin_name (NULL), ML_NNFW_TYPE_ANY);
}

/**
 * @brief Test for internal function '_ml_validate_model_file'.
 * @detail Invalid params.
 */
TEST (nnstreamer_capi_internal, validate_model_file_01_n)
{
  const gchar cf_name[] = "libnnstreamer_customfilter_passthrough_variable" SO_FILE_EXTENSION;
  gchar *lib_path = NULL;
  gchar *test_model = NULL;
  int status;
  ml_nnfw_type_e nnfw = ML_NNFW_TYPE_CUSTOM_FILTER;

  lib_path = nnsconf_get_custom_value_string ("filter", "customfilters");
  if (lib_path == NULL) {
    /* cannot get custom-filter directory */
    goto skip_test;
  }

  test_model = g_build_filename (lib_path, cf_name, NULL);
  if (!g_file_test (test_model, G_FILE_TEST_EXISTS)) {
    goto skip_test;
  }

  status = _ml_validate_model_file (NULL, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = _ml_validate_model_file (&test_model, 0, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  status = _ml_validate_model_file (&test_model, 1, NULL);
  EXPECT_NE (status, ML_ERROR_NONE);

skip_test:
  g_free (lib_path);
  g_free (test_model);
}

/**
 * @brief Test for internal function '_ml_validate_model_file'.
 * @detail Invalid file extension.
 */
TEST (nnstreamer_capi_internal, validate_model_file_02_n)
{
  const gchar *sroot_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  const gchar cf_name[] = "libnnstreamer_customfilter_passthrough_variable" SO_FILE_EXTENSION;
  gchar *lib_path = NULL;
  gchar *test_model1 = NULL;
  gchar *test_model2 = NULL;
  gchar *test_models[2];
  int status;
  ml_nnfw_type_e nnfw;

  lib_path = nnsconf_get_custom_value_string ("filter", "customfilters");
  if (lib_path == NULL) {
    /* cannot get custom-filter directory */
    goto skip_test;
  }

  test_model1 = g_build_filename (lib_path, cf_name, NULL);
  if (!g_file_test (test_model1, G_FILE_TEST_EXISTS)) {
    goto skip_test;
  }

  /* supposed to run test in build directory */
  if (sroot_path == NULL)
    sroot_path = "..";

  test_model2 = g_build_filename (sroot_path, "tests", "test_models", "models",
      "mobilenet_v1_1.0_224_quant.tflite", NULL);
  ASSERT_TRUE (g_file_test (test_model2, G_FILE_TEST_EXISTS));

  test_models[0] = test_model1;
  test_models[1] = test_model2;

  nnfw = ML_NNFW_TYPE_CUSTOM_FILTER;
  status = _ml_validate_model_file (&test_model2, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_TENSORFLOW_LITE;
  status = _ml_validate_model_file (&test_model1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_TENSORFLOW;
  status = _ml_validate_model_file (&test_model2, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  /* snap only for android */
  nnfw = ML_NNFW_TYPE_SNAP;
  status = _ml_validate_model_file (&test_model1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_VIVANTE;
  status = _ml_validate_model_file (test_models, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  /** @todo currently mvnc, openvino and edgetpu always return failure */
  nnfw = ML_NNFW_TYPE_MVNC;
  status = _ml_validate_model_file (&test_model1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_OPENVINO;
  status = _ml_validate_model_file (&test_model1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_EDGE_TPU;
  status = _ml_validate_model_file (&test_model1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_ARMNN;
  status = _ml_validate_model_file (&test_model1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

skip_test:
  g_free (lib_path);
  g_free (test_model1);
  g_free (test_model2);
}

/**
 * @brief Test for internal function '_ml_validate_model_file'.
 * @detail Invalid model path.
 */
TEST (nnstreamer_capi_internal, validate_model_file_03_n)
{
  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
  int status;
  ml_nnfw_type_e nnfw;
  gchar *test_dir1, *test_dir2;

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  /* test model path */
  test_dir1 = g_build_filename (root_path, "tests", "test_models", "models", NULL);

  /* invalid dir */
  test_dir2 = g_build_filename (test_dir1, "invaliddir", NULL);

  nnfw = ML_NNFW_TYPE_TENSORFLOW_LITE;
  status = _ml_validate_model_file (&test_dir1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_TENSORFLOW;
  status = _ml_validate_model_file (&test_dir1, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

  nnfw = ML_NNFW_TYPE_NNFW;
  status = _ml_validate_model_file (&test_dir2, 1, &nnfw);
  EXPECT_NE (status, ML_ERROR_NONE);

#ifdef ENABLE_NNFW_RUNTIME
  /* only NNFW supports dir path */
  nnfw = ML_NNFW_TYPE_NNFW;
  status = _ml_validate_model_file (&test_dir1, 1, &nnfw);
  EXPECT_EQ (status, ML_ERROR_NONE);
#endif

  g_free (test_dir1);
  g_free (test_dir2);
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

  try {
    result = RUN_ALL_TESTS ();
  } catch (...) {
    g_warning ("catch `testing::internal::GoogleTestFailureException`");
  }

  set_feature_state (ML_FEATURE, NOT_CHECKED_YET);
  set_feature_state (ML_FEATURE_INFERENCE, NOT_CHECKED_YET);

  return result;
}
