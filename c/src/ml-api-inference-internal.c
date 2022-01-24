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

#include <nnstreamer_log.h>
#include <nnstreamer_plugin_api.h>
#include <tensor_typedef.h>
#include "ml-api-inference-internal.h"
#include "ml-api-internal.h"

/**
 * @brief Allocates a tensors information handle from gst info.
 */
int
_ml_tensors_info_create_from_gst (ml_tensors_info_h * ml_info,
    GstTensorsInfo * gst_info)
{
  if (!ml_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, ml_info, is NULL. It should be a valid ml_tensors_info_h instance usually created by ml_tensors_info_create(). This could be an internal bug of ML API.");

  if (!gst_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This could be an internal bug of ML API.");

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
_ml_tensors_info_copy_from_gst (ml_tensors_info_s * ml_info,
    const GstTensorsInfo * gst_info)
{
  guint i, j;
  guint max_dim;

  if (!ml_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parmater, ml_info, is NULL. It should be a valid ml_tensors_info_s instance, usually created by ml_tensors_info_create(). This is probably an internal bug of ML API.");
  if (!gst_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parmater, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This is probably an internal bug of ML API.");

  _ml_tensors_info_initialize (ml_info);

  max_dim = MIN (ML_TENSOR_RANK_LIMIT, NNS_TENSOR_RANK_LIMIT);

  ml_info->num_tensors = gst_info->num_tensors;

  for (i = 0; i < gst_info->num_tensors; i++) {
    /* Copy name string */
    if (gst_info->info[i].name) {
      ml_info->info[i].name = g_strdup (gst_info->info[i].name);
    }

    /* Set tensor type */
    switch (gst_info->info[i].type) {
      case _NNS_INT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT32;
        break;
      case _NNS_UINT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT32;
        break;
      case _NNS_INT16:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT16;
        break;
      case _NNS_UINT16:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT16;
        break;
      case _NNS_INT8:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT8;
        break;
      case _NNS_UINT8:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT8;
        break;
      case _NNS_FLOAT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_FLOAT64;
        break;
      case _NNS_FLOAT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_FLOAT32;
        break;
      case _NNS_INT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT64;
        break;
      case _NNS_UINT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT64;
        break;
      default:
        ml_info->info[i].type = ML_TENSOR_TYPE_UNKNOWN;
        break;
    }

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      ml_info->info[i].dimension[j] = gst_info->info[i].dimension[j];
    }

    for (; j < ML_TENSOR_RANK_LIMIT; j++) {
      ml_info->info[i].dimension[j] = 1;
    }
  }
  return ML_ERROR_NONE;
}

/**
 * @brief Copies tensor meta info from gst tensors info.
 * @bug Thread safety required. Check its internal users first!
 */
int
_ml_tensors_info_copy_from_ml (GstTensorsInfo * gst_info,
    const ml_tensors_info_s * ml_info)
{
  guint i, j;
  guint max_dim;

  if (!ml_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parmater, ml_info, is NULL. It should be a valid ml_tensors_info_s instance, usually created by ml_tensors_info_create(). This is probably an internal bug of ML API.");
  if (!gst_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parmater, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This is probably an internal bug of ML API.");

  G_LOCK_UNLESS_NOLOCK (*ml_info);

  gst_tensors_info_init (gst_info);
  max_dim = MIN (ML_TENSOR_RANK_LIMIT, NNS_TENSOR_RANK_LIMIT);

  gst_info->num_tensors = ml_info->num_tensors;

  for (i = 0; i < ml_info->num_tensors; i++) {
    /* Copy name string */
    if (ml_info->info[i].name) {
      gst_info->info[i].name = g_strdup (ml_info->info[i].name);
    }

    /* Set tensor type */
    switch (ml_info->info[i].type) {
      case ML_TENSOR_TYPE_INT32:
        gst_info->info[i].type = _NNS_INT32;
        break;
      case ML_TENSOR_TYPE_UINT32:
        gst_info->info[i].type = _NNS_UINT32;
        break;
      case ML_TENSOR_TYPE_INT16:
        gst_info->info[i].type = _NNS_INT16;
        break;
      case ML_TENSOR_TYPE_UINT16:
        gst_info->info[i].type = _NNS_UINT16;
        break;
      case ML_TENSOR_TYPE_INT8:
        gst_info->info[i].type = _NNS_INT8;
        break;
      case ML_TENSOR_TYPE_UINT8:
        gst_info->info[i].type = _NNS_UINT8;
        break;
      case ML_TENSOR_TYPE_FLOAT64:
        gst_info->info[i].type = _NNS_FLOAT64;
        break;
      case ML_TENSOR_TYPE_FLOAT32:
        gst_info->info[i].type = _NNS_FLOAT32;
        break;
      case ML_TENSOR_TYPE_INT64:
        gst_info->info[i].type = _NNS_INT64;
        break;
      case ML_TENSOR_TYPE_UINT64:
        gst_info->info[i].type = _NNS_UINT64;
        break;
      default:
        gst_info->info[i].type = _NNS_END;
        break;
    }

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      gst_info->info[i].dimension[j] = ml_info->info[i].dimension[j];
    }

    for (; j < NNS_TENSOR_RANK_LIMIT; j++) {
      gst_info->info[i].dimension[j] = 1;
    }
  }
  G_UNLOCK_UNLESS_NOLOCK (*ml_info);

  return ML_ERROR_NONE;
}
