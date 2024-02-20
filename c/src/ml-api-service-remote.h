/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file	ml-api-service-remote.h
 * @date	04 March 2024
 * @brief	ml-service extension internal header.
 *        This file should NOT be exported to SDK or devel package.
 * @author	Gichan Jang <gichan2.jang@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#ifndef __ML_SERVICE_REMOTE_INTERNAL_H__
#define __ML_SERVICE_REMOTE_INTERNAL_H__

#include <ml-api-service.h>
#include "nnstreamer-tizen-internal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
typedef void * ml_service_remote_h;

/**
 * @brief Creates ml remote service handle with given ml-option handle.
 * @remarks The @a handle should be destroyed using ml_service_destroy().
 * @param[in] handle ml-service handle created by ml_service_new().
 * @param[in] option The option used for creating query service.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 */
int ml_service_remote_create (ml_service_remote_h handle, ml_option_h option);

/**
 * @brief Request service to ml-service remote.
 * @param[in] handle The query service handle created by ml_service_query_create().
 * @param[in] key The key of machine learning service.
 * @param[in] input The Data to be registered on the remote server.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_service_remote_request (ml_service_remote_h handle,const char * key, const ml_tensors_data_h input);

/**
 * @brief Sets the services in ml-service remote handle.
 * @param[in] handle The handle of ml-service.
 * @param[in] name The service key.
 * @param[in] value The value to be set (json string).
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 */
int ml_service_remote_set_service (ml_service_h handle, const char *name, const char *value);

/**
 * @brief Set path in ml-service remote handle.
 * @note This is not official and public API but experimental API.
 * @param[in] handle The query service handle created by ml_service_query_create().
 * @param[in] name The service key.
 * @param[in] path The value to set.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_service_remote_set_information (ml_service_h handle, const char *name, const char *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_SERVICE_REMOTE_INTERNAL_H__ */
