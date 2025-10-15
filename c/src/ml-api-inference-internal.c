/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-inference-internal.c
 * @date 19 October 2021
 * @brief ML-API Internal Utility Functions for inference implementations
 * @see	https://github.com/nnstreamer/api
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <string.h>

#include <nnstreamer_plugin_api_util.h>
#include <tensor_typedef.h>
#include "ml-api-inference-internal.h"
#include "ml-api-internal.h"

/**
 * @brief Check tensor-info has extended rank value.
 */
static gboolean
gst_info_is_extended (const GstTensorsInfo * gst_info)
{
  GstTensorInfo *_info;
  guint i;

  for (i = 0; i < gst_info->num_tensors; i++) {
    _info = gst_tensors_info_get_nth_info ((GstTensorsInfo *) gst_info, i);
    if (!_info)
      _ml_error_report_return (FALSE,
          "The parameter, gst_info, has invalid number of tensors. The max number of tensors is "
          NNS_TENSOR_SIZE_LIMIT_STR);

    if (_info->dimension[ML_TENSOR_RANK_LIMIT_PREV] > 0)
      return TRUE;
  }

  return FALSE;
}

/**
 * @brief Allocates a tensors information handle from gst info.
 */
int
_ml_tensors_info_create_from_gst (ml_tensors_info_h * ml_info,
    const GstTensorsInfo * gst_info)
{
  gboolean is_extended;

  if (!ml_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, ml_info, is NULL. It should be a valid ml_tensors_info_h instance usually created by ml_tensors_info_create(). This could be an internal bug of ML API.");

  if (!gst_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This could be an internal bug of ML API.");

  is_extended = gst_info_is_extended (gst_info);
  if (is_extended)
    _ml_error_report_return_continue_iferr
        (ml_tensors_info_create_extended (ml_info),
        "The call to ml_tensors_info_create_extended has failed with %d.",
        _ERRNO);
  else
    _ml_error_report_return_continue_iferr (ml_tensors_info_create (ml_info),
        "The call to ml_tensors_info_create has failed with %d.", _ERRNO);

  _ml_tensors_info_copy_from_gst (*ml_info, gst_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Copies tensor meta info from gst tensors info.
 * @bug Thread safety required. Check its internal users first!
 */
int
_ml_tensors_info_copy_from_gst (ml_tensors_info_h ml_info,
    const GstTensorsInfo * gst_info)
{
  ml_tensors_info_s *_info;

  if (!ml_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, ml_info, is NULL. It should be a valid ml_tensors_info_s instance, usually created by ml_tensors_info_create(). This is probably an internal bug of ML API.");
  if (!gst_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This is probably an internal bug of ML API.");

  _info = (ml_tensors_info_s *) ml_info;

  G_LOCK_UNLESS_NOLOCK (*_info);
  _info->is_extended = gst_info_is_extended (gst_info);
  gst_tensors_info_copy (&_info->info, gst_info);
  G_UNLOCK_UNLESS_NOLOCK (*_info);

  return ML_ERROR_NONE;
}

/**
 * @brief Copies tensor meta info from gst tensors info.
 * @bug Thread safety required. Check its internal users first!
 */
int
_ml_tensors_info_copy_from_ml (GstTensorsInfo * gst_info,
    const ml_tensors_info_h ml_info)
{
  ml_tensors_info_s *_info;

  if (!ml_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, ml_info, is NULL. It should be a valid ml_tensors_info_s instance, usually created by ml_tensors_info_create(). This is probably an internal bug of ML API.");
  if (!gst_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This is probably an internal bug of ML API.");

  _info = (ml_tensors_info_s *) ml_info;

  G_LOCK_UNLESS_NOLOCK (*_info);
  gst_tensors_info_copy (gst_info, &_info->info);
  G_UNLOCK_UNLESS_NOLOCK (*_info);

  return ML_ERROR_NONE;
}
