/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-training-offloading.h
 * @date 5 Apr 2024
 * @brief ml-service training offloading internal header.
 *        This file should NOT be exported to SDK or devel package.
 * @brief ML training offloading service of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/api
 * @author Hyunil Park <hyunil46.park@samsung.com>
 * @bug No known bugs except for NYI items
 */

#ifndef __ML_SERVICE_TRAINING_OFFLOADING_H__
#define __ML_SERVICE_TRAINING_OFFLOADING_H__

#include <ml-api-service.h>
#include "ml-api-service-offloading.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(ENABLE_TRAINING_OFFLOADING)
/**
 * @brief Creates a training offloading handle for ml-service training offloading service.
 * @param[in] mls ml-service handle created by ml_service_new().
 * @param[in] offloading The Json object containing the service option.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the media storage or external storage.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR Failed to parse the configuration file.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to open the model.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_training_offloading_create (ml_service_s *mls, JsonObject *offloading);

/**
 * @brief Set path in ml-service training offloading handle.
 * @param[in] mls ml-service handle created by ml_service_new().
 * @param[in] path Readable and writable path set by the app.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_service_training_offloading_set_path (ml_service_s *mls, const gchar *path);

/**
 * @brief Start ml training offloading service.
 * @param[in] mls ml-service handle created by ml_service_new().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 */
int ml_service_training_offloading_start (ml_service_s *mls);

/**
 * @brief Stop ml training offloading service.
 * @param[in] mls ml-service handle created by ml_service_new().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 */
int ml_service_training_offloading_stop (ml_service_s *mls);

/**
 * @brief Process received data
 * @param[in] mls ml-service handle created by ml_service_new().
 * @param[in] data_h handle nns_edge_data_h
 * @param[in] data data of received file
 * @param[in] dir_path dir path
 * @param[in] service_type received service type from remote edge
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 */
int ml_service_training_offloading_process_received_data (ml_service_s *mls, void *data_h, const gchar *dir_path, const gchar *data, int service_type);

/**
 * @brief Internal function to destroy ml-service training offloading data.
 * @param[in] mls ml-service handle created by ml_service_new().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_service_training_offloading_destroy (ml_service_s *mls);
#else
#define ml_service_training_offloading_create(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_training_offloading_set_path(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_training_offloading_start(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_training_offloading_stop(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_training_offloading_process_received_data(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_training_offloading_destroy(...) ML_ERROR_NOT_SUPPORTED
#endif /* ENABLE_TRAINING_OFFLOADING */
#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __ML_SERVICE_TRAINING_OFFLOADING_H__ */
