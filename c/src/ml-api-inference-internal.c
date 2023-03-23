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
 * @brief Convert the type from ml_tensor_type_e to tensor_type.
 * @note This code is based on the same order between NNS type and ML type.
 * The index should be the same in case of adding a new type.
 */
static tensor_type
convert_tensor_type_from (ml_tensor_type_e type)
{
  if (type < ML_TENSOR_TYPE_INT32 || type >= ML_TENSOR_TYPE_UNKNOWN) {
    _ml_error_report
        ("Failed to convert the type. Input ml_tensor_type_e %d is invalid.",
        type);
    return _NNS_END;
  }

  return (tensor_type) type;
}

/**
 * @brief Convert the type from tensor_type to ml_tensor_type_e.
 * @note This code is based on the same order between NNS type and ML type.
 * The index should be the same in case of adding a new type.
 */
static ml_tensor_type_e
convert_ml_tensor_type_from (tensor_type type)
{
  if (type < _NNS_INT32 || type >= _NNS_END) {
    _ml_error_report
        ("Failed to convert the type. Input tensor_type %d is invalid.", type);
    return ML_TENSOR_TYPE_UNKNOWN;
  }

  return (ml_tensor_type_e) type;
}

static gboolean
gst_info_is_extended (const GstTensorsInfo * gst_info)
{
  int i, j;
  for (i = 0; i < gst_info->num_tensors; i++) {
    for (j = ML_TENSOR_RANK_LIMIT_PREV; j < NNS_TENSOR_RANK_LIMIT; j++) {
      if (gst_info->info[i].dimension[j] != 1)
        return TRUE;
    }
  }
  return FALSE;
}

/**
 * @brief Allocates a tensors information handle from gst info.
 */
int
_ml_tensors_info_create_from_gst (ml_tensors_info_h * ml_info,
    GstTensorsInfo * gst_info)
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
    _ml_error_report_return_continue_iferr (ml_tensors_info_create_extended
        (ml_info),
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
  ml_info->is_extended = gst_info_is_extended (gst_info);

  for (i = 0; i < gst_info->num_tensors; i++) {
    /* Copy name string */
    if (gst_info->info[i].name) {
      ml_info->info[i].name = g_strdup (gst_info->info[i].name);
    }

    ml_info->info[i].type =
        convert_ml_tensor_type_from (gst_info->info[i].type);

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      ml_info->info[i].dimension[j] = gst_info->info[i].dimension[j];
    }

    for (; j < ML_TENSOR_RANK_LIMIT; j++) {
      ml_info->info[i].dimension[j] = 1;
    }

    if (!ml_info->is_extended) {
      for (j = ML_TENSOR_RANK_LIMIT_PREV; j < ML_TENSOR_RANK_LIMIT; j++) {
        ml_info->info[i].dimension[j] = 1;
      }
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

    gst_info->info[i].type = convert_tensor_type_from (ml_info->info[i].type);

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      gst_info->info[i].dimension[j] = ml_info->info[i].dimension[j];
    }

    for (; j < NNS_TENSOR_RANK_LIMIT; j++) {
      gst_info->info[i].dimension[j] = 1;
    }

    if (!ml_info->is_extended) {
      for (j = ML_TENSOR_RANK_LIMIT_PREV; j < NNS_TENSOR_RANK_LIMIT; j++) {
        gst_info->info[i].dimension[j] = 1;
      }
    }
  }
  G_UNLOCK_UNLESS_NOLOCK (*ml_info);

  return ML_ERROR_NONE;
}
