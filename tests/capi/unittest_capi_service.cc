/**
 * @file        unittest_capi_service.cc
 * @date        08 Mar 2022
 * @brief       Unit test for ML Service C-API.
 * @see         https://github.com/nnstreamer/api
 * @author      Sangjung Woo <sangjung.woo@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <glib.h>
#include <ml-api-internal.h>
#include <ml-api-service.h>

/**
 * @brief Test NNStreamer Service API for adding and getting the pipeline description with name
 */
TEST (nnstreamer_capi_service, basic_test_0_p)
{
  int ret = 0;
  gchar *pipeline, *test_model;
  gchar *ret_pipeline;
  const gchar *key = "ServiceName";
  const gchar *root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");

  /* supposed to run test in build directory */
  if (root_path == NULL)
    root_path = "..";

  test_model = g_build_filename (
      root_path, "tests", "test_models", "models", "add.tflite", NULL);
  EXPECT_TRUE (g_file_test (test_model, G_FILE_TEST_EXISTS));

  pipeline = g_strdup_printf (
      "appsrc name=appsrc ! "
      "other/tensor,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
      "tensor_filter name=filter_h framework=tensorflow-lite model=%s ! tensor_sink name=tensor_sink",
      test_model);

  ret = ml_service_pipeline_add(key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  ret = ml_service_pipeline_get(key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);
  EXPECT_STREQ (pipeline, ret_pipeline);
  
  g_free (pipeline);
  g_free (ret_pipeline);
  g_free (test_model);
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
  set_feature_state (SUPPORTED);

  try {
    result = RUN_ALL_TESTS ();
  } catch (...) {
    g_warning ("catch `testing::internal::GoogleTestFailureException`");
  }

  set_feature_state (NOT_CHECKED_YET);

  return result;
}
