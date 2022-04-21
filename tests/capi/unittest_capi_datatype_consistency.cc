/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file        unittest_capi_datatype_consistency.cc
 * @date        06 Aug 2021
 * @brief       Unit test for data type consistency with frameworks
 * @see         https://github.com/nnstreamer/api
 * @author      MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug         No known bugs
 */

/* NNStreamer Side */
#include <tensor_typedef.h>

/* ML API Side */
#include <nnstreamer.h>
#include <ml-api-internal.h>

/* GStreamer Side */
#include <gst/gst.h>

#include <gtest/gtest.h>

/**
 * @brief Check the consistency of nnstreamer data types.
 */
TEST (nnstreamer_datatypes, test_all_1)
{
  EXPECT_EQ ((int) NNS_TENSOR_RANK_LIMIT, (int) ML_TENSOR_RANK_LIMIT);
  EXPECT_EQ ((int) NNS_TENSOR_SIZE_LIMIT, (int) ML_TENSOR_SIZE_LIMIT);
  EXPECT_EQ (sizeof (tensor_dim), sizeof (ml_tensor_dimension));
  EXPECT_EQ (sizeof (tensor_dim[0]), sizeof (ml_tensor_dimension[0]));
  EXPECT_EQ ((int) _NNS_INT32, (int) ML_TENSOR_TYPE_INT32);
  EXPECT_EQ ((int) _NNS_UINT32, (int) ML_TENSOR_TYPE_UINT32);
  EXPECT_EQ ((int) _NNS_INT16, (int) ML_TENSOR_TYPE_INT16);
  EXPECT_EQ ((int) _NNS_UINT16, (int) ML_TENSOR_TYPE_UINT16);
  EXPECT_EQ ((int) _NNS_INT8, (int) ML_TENSOR_TYPE_INT8);
  EXPECT_EQ ((int) _NNS_UINT8, (int) ML_TENSOR_TYPE_UINT8);
  EXPECT_EQ ((int) _NNS_INT64, (int) ML_TENSOR_TYPE_INT64);
  EXPECT_EQ ((int) _NNS_UINT64, (int) ML_TENSOR_TYPE_UINT64);
  EXPECT_EQ ((int) _NNS_FLOAT64, (int) ML_TENSOR_TYPE_FLOAT64);
  EXPECT_EQ ((int) _NNS_FLOAT32, (int) ML_TENSOR_TYPE_FLOAT32);
  EXPECT_EQ ((int) _NNS_END, (int) ML_TENSOR_TYPE_UNKNOWN);
}

/**
 * @brief Check the consistency of nnstreamer data types. (negative cases)
 */
TEST (nnstreamer_datatypes, test_all_2_n)
{
  ml_tensors_info_h info;
  int ret;

  ret = ml_tensors_info_create (&info);
  EXPECT_EQ (ret, 0);

  ret = ml_tensors_info_set_count (info, NNS_TENSOR_SIZE_LIMIT + 1);
  EXPECT_EQ (ret, ML_ERROR_INVALID_PARAMETER);

  ret = ml_tensors_info_destroy (info);
  EXPECT_EQ (ret, 0);
}

/**
 * @brief Check the consistency of nnstreamer data types. (negative cases)
 */
TEST (nnstreamer_datatypes, test_all_3_n)
{
  ml_tensors_info_h info;
  int ret;

  ret = ml_tensors_info_create (&info);
  EXPECT_EQ (ret, 0);

  ret = ml_tensors_info_set_count (info, 1);
  EXPECT_EQ (ret, 0);

  ret = ml_tensors_info_set_tensor_type (info, 0, ML_TENSOR_TYPE_UNKNOWN);
  EXPECT_EQ (ret, ML_ERROR_INVALID_PARAMETER);

  ret = ml_tensors_info_destroy (info);
  EXPECT_EQ (ret, 0);
}

/**
 * @brief Check the consistency of gstreamer data types.
 */
TEST (gstreamer_datatypes, test_all_1)
{
  EXPECT_EQ ((int) GST_STATE_VOID_PENDING, (int) ML_PIPELINE_STATE_UNKNOWN);
  EXPECT_EQ ((int) GST_STATE_NULL, (int) ML_PIPELINE_STATE_NULL);
  EXPECT_EQ ((int) GST_STATE_READY, (int) ML_PIPELINE_STATE_READY);
  EXPECT_EQ ((int) GST_STATE_PAUSED, (int) ML_PIPELINE_STATE_PAUSED);
  EXPECT_EQ ((int) GST_STATE_PLAYING, (int) ML_PIPELINE_STATE_PLAYING);
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

  try {
    result = RUN_ALL_TESTS ();
  } catch (...) {
    g_warning ("catch `testing::internal::GoogleTestFailureException`");
  }

  set_feature_state (ML_FEATURE, NOT_CHECKED_YET);

  return result;
}
