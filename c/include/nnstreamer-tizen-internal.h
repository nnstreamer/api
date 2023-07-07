/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file	nnstreamer-tizen-internal.h
 * @date	02 October 2019
 * @brief	NNStreamer/Pipeline(main) C-API Header for Tizen Internal API. This header should be used only in Tizen.
 * @author	Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#ifndef __TIZEN_MACHINELEARNING_NNSTREAMER_INTERNAL_H__
#define __TIZEN_MACHINELEARNING_NNSTREAMER_INTERNAL_H__

#include <nnstreamer.h>
#include <nnstreamer-single.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Constructs the pipeline (GStreamer + NNStreamer).
 * @details This function is to construct the pipeline without checking the permission in platform internally. See ml_pipeline_construct() for the details.
 * @since_tizen 5.5
 */
int ml_pipeline_construct_internal (const char *pipeline_description, ml_pipeline_state_cb cb, void *user_data, ml_pipeline_h *pipe);

/**
 * @brief An information to create single-shot instance.
 */
typedef struct {
  ml_tensors_info_h input_info;  /**< The input tensors information. */
  ml_tensors_info_h output_info; /**< The output tensors information. */
  ml_nnfw_type_e nnfw;           /**< The neural network framework. */
  ml_nnfw_hw_e hw;               /**< The type of hardware resource. */
  char *models;                  /**< Comma separated neural network model files. */
  char *custom_option;           /**< Custom option string for neural network framework. */
  char *fw_name;                 /**< The explicit framework name given by user */
} ml_single_preset;

/**
 * @brief Opens an ML model with the custom options and returns the instance as a handle.
 * This is internal function to handle various options in public APIs.
 */
int ml_single_open_custom (ml_single_h *single, ml_single_preset *info);

/**
 * @brief Gets the version number of machine-learning API. (major.minor.micro)
 * @since_tizen 8.0
 * @param[out] major The pointer to store the major version number. Set null if won't fetch the version.
 * @param[out] minor The pointer to store the minor version number. Set null if won't fetch the version.
 * @param[out] micro The pointer to store the micro version number. Set null if won't fetch the version.
 */
void ml_api_get_version (unsigned int *major, unsigned int *minor, unsigned int *micro);

/**
 * @brief Gets the version string of machine-learning API.
 * @since_tizen 8.0
 * @return Newly allocated string. The returned string should be freed with free().
 */
char * ml_api_get_version_string (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __TIZEN_MACHINELEARNING_NNSTREAMER_INTERNAL_H__ */
