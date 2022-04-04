/**
 * @file        unittest_capi_service.cc
 * @date        08 Mar 2022
 * @brief       Unit test for ML Service C-API.
 * @see         https://github.com/nnstreamer/api
 * @author      Sangjung Woo <sangjung.woo@samsung.com>
 * @bug         No known bugs
 */

#include <gtest/gtest.h>
#include <ml-api-internal.h>
#include <ml-api-service.h>

/**
 * @brief Test base class for Database of ML Service API.
 */
class MLServiceDBTest : public::testing::Test
{
protected:
  const gchar *root_path;
  const gchar *key;
  gchar *test_model;
  gchar *pipeline;

public:
  /**
   * @brief Setup method for each test case.
   */
  void SetUp() override
  {
    /* supposed to run test in build directory */
    root_path = g_getenv ("NNSTREAMER_SOURCE_ROOT_PATH");
    if (root_path == NULL)
      root_path = "..";

    key = "ServiceName";
    test_model =
        g_build_filename (root_path, "tests", "test_models", "models",
        "add.tflite", NULL);
    pipeline = g_strdup_printf ("appsrc name=appsrc ! "
        "other/tensor,dimension=(string)1:1:1:1,type=(string)float32,framerate=(fraction)0/1 ! "
        "tensor_filter name=filter_h framework=tensorflow-lite model=%s ! tensor_sink name=tensor_sink",
        test_model);
  }

  /**
   * @brief Teardown method for each test case.
   */
  void TearDown() override
  {
    g_free (pipeline);
    g_free (test_model);
  }
};

/**
 * @brief Set the pipeline description with given name.
 */
TEST_F (MLServiceDBTest, set_pipeline_description_0_p)
{
  int ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);
}

/**
 * @brief Set the pipeline description with wrong parameters.
 */
TEST_F (MLServiceDBTest, set_pipeline_description_1_n)
{
  int ret = 0;

  /* Test */
  ret = ml_service_set_pipeline (NULL, pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  ret = ml_service_set_pipeline (key, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);
}

/**
 * @brief Update the pipeline description with the same name.
 */
TEST_F (MLServiceDBTest, set_pipeline_description_2_p)
{
  int ret = 0;
  g_autofree gchar *ret_pipeline = NULL;
  g_autofree gchar *pipeline_new = g_strdup_printf (
      "v4l2src ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=640,height=480,framerate=5/1 ! "
      "mqttsink pub-topic=example/objectDetection");

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_set_pipeline (key, pipeline_new);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  ret = ml_service_get_pipeline (key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);
  EXPECT_STREQ (pipeline_new, ret_pipeline);
}

/**
 * @brief Update the pipeline description with the same name.
 */
TEST_F (MLServiceDBTest, set_pipeline_description_3_n)
{
  int ret = 0;
  g_autofree gchar *ret_pipeline;

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_set_pipeline (key, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  ret = ml_service_get_pipeline (key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);
  EXPECT_STREQ (pipeline, ret_pipeline);
}

/**
 * @brief Get the pipeline description with given name.
 */
TEST_F (MLServiceDBTest, get_pipeline_description_0_p)
{
  int ret = 0;
  g_autofree gchar *ret_pipeline;

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_get_pipeline (key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);
  EXPECT_STREQ (pipeline, ret_pipeline);
}

/**
 * @brief Get the pipeline description with wrong parameters.
 */
TEST_F (MLServiceDBTest, get_pipeline_description_1_n)
{
  int ret = 0;
  g_autofree gchar *ret_pipeline = NULL;

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_get_pipeline (NULL, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  ret = ml_service_get_pipeline (key, NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);
}

/**
 * @brief Delete the pipeline description with given name.
 */
TEST_F (MLServiceDBTest, del_pipeline_description_0_p)
{
  int ret = 0;
  gchar *ret_pipeline = NULL;

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  ret = ml_service_get_pipeline (key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);
  g_free (ret_pipeline);
  ret_pipeline = NULL;

  /* Test */
  ret = ml_service_delete_pipeline (key);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  ret = ml_service_get_pipeline (key, &ret_pipeline);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);
}

/**
 * @brief Delete the pipeline description with wrong parameters.
 */
TEST_F (MLServiceDBTest, del_pipeline_description_1_n)
{
  int ret = 0;
  const gchar *key_invalid = "InvalidName";

  ret = ml_service_set_pipeline (key, pipeline);
  EXPECT_EQ (ML_ERROR_NONE, ret);

  /* Test */
  ret = ml_service_delete_pipeline (NULL);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);

  ret = ml_service_delete_pipeline (key_invalid);
  EXPECT_EQ (ML_ERROR_INVALID_PARAMETER, ret);
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
