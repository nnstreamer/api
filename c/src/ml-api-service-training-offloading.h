/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-training-offloading.h
 * @date 30 Apr 2024
 * @brief ml-service extension internal header.
 *        This file should NOT be exported to SDK or devel package.
 * @brief ML training offloading service of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Hyunil Park <hyunil46.park@samsung.com>
 * @bug No known bugs except for NYI items
 */



#ifndef __ML_SERVICE_TRAINING_OFFLOADING_H__
#define __ML_SERVICE_TRAINING_OFFLOADING_H__

#include <json-glib/json-glib.h>
#include <ml-api-service.h>
#include "ml-api-service-offloading.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * @brief Creates a handle for ml-service training offloading handle.
 * @param[in] offloading_s The handle of ml-service offloading.
 * @param[in] object The Json object containing the service option.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the media storage or external storage.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR Failed to parse the configuration file.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to open the model.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
  int ml_service_training_offloading_create (_ml_service_offloading_s *
      offloading_s, JsonObject * object);

/**
 * @brief Set path in ml-service training offloading handle.
 * @note This is not official and public API but experimental API.
 * @param[in] offloading_s The handle of ml-service offloading.
 * @param[in] name The name of path (path: received model file, model-path: to send pretrained model, model-config-path: to send model-config, data-path: to send training data)
 * @param[in] path The path to save file.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
  int ml_service_training_offloading_set_path (_ml_service_offloading_s *
      offloading_s, const char *name, const gchar * path);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_SERVICE_TRAINING_OFFLOADING_H__ */
