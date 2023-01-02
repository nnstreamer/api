/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2021 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-inference-internal.h
 * @date 20 October 2021
 * @brief ML C-API internal header with NNStreamer deps.
 *        This file should NOT be exported to SDK or devel package.
 * @see	https://github.com/nnstreamer/api
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */

#ifndef __ML_API_INTERNAL_NNSTREAMER_H__
#define __ML_API_INTERNAL_NNSTREAMER_H__

#include <glib.h>
#include <nnstreamer_plugin_api_filter.h>

#include "ml-api-internal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Macro to check the availability of given NNFW.
 */
#define _ml_nnfw_is_available(f,h) ({bool a; (ml_check_nnfw_availability ((f), (h), &a) == ML_ERROR_NONE && a);})

/**
 * @brief Allocates a tensors information handle from gst info.
 */
int _ml_tensors_info_create_from_gst (ml_tensors_info_h *ml_info, GstTensorsInfo *gst_info);

/**
 * @brief Copies tensor metadata from gst tensors info.
 */
int _ml_tensors_info_copy_from_gst (ml_tensors_info_s *ml_info, const GstTensorsInfo *gst_info);

/**
 * @brief Copies tensor metadata from ml tensors info.
 */
int _ml_tensors_info_copy_from_ml (GstTensorsInfo *gst_info, const ml_tensors_info_s *ml_info);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_INTERNAL_NNSTREAMER_H__ */
