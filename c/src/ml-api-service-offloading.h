/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file	ml-api-service-offloading.h
 * @date	04 March 2024
 * @brief	ml-service extension internal header.
 *        This file should NOT be exported to SDK or devel package.
 * @author	Gichan Jang <gichan2.jang@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#ifndef __ML_SERVICE_OFFLOADING_INTERNAL_H__
#define __ML_SERVICE_OFFLOADING_INTERNAL_H__

#include "ml-api-service-private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Enumeration for ml-offloading service type.
 */
typedef enum
{
  ML_SERVICE_OFFLOADING_TYPE_UNKNOWN = 0,
  ML_SERVICE_OFFLOADING_TYPE_MODEL_RAW = 1,
  ML_SERVICE_OFFLOADING_TYPE_MODEL_URI = 2,
  ML_SERVICE_OFFLOADING_TYPE_PIPELINE_RAW = 3,
  ML_SERVICE_OFFLOADING_TYPE_PIPELINE_URI = 4,
  ML_SERVICE_OFFLOADING_TYPE_REPLY = 5,
  ML_SERVICE_OFFLOADING_TYPE_LAUNCH = 6,

  ML_SERVICE_OFFLOADING_TYPE_MAX
} ml_service_offloading_type_e;

/**
 * @brief Enumeration for ml-service offloading mode.
 */
typedef enum
{
  ML_SERVICE_OFFLOADING_MODE_NONE = 0,
  ML_SERVICE_OFFLOADING_MODE_TRAINING = 1,

  ML_SERVICE_OFFLOADING_MODE_MAX
} ml_service_offloading_mode_e;

#if defined(ENABLE_ML_OFFLOADING)
/**
 * @brief Internal function to parse configuration file and create offloading service.
 * @param[in] handle The handle of ml-service created by ml_service_new().
 * @param[in] object The json object from config file.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int _ml_service_offloading_create (ml_service_h handle, JsonObject *object);

/**
 * @brief Internal function to start ml offloading service.
 * @param[in] handle ml-service handle created by ml_service_new().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 */
int _ml_service_offloading_start (ml_service_h handle);

/**
 * @brief Internal function to stop ml offloading service.
 * @param[in] handle ml-service handle created by ml_service_new().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 */
int _ml_service_offloading_stop (ml_service_h handle);

/**
 * @brief Internal function to request service to ml-service offloading.
 * @param[in] handle The handle of ml-service.
 * @param[in] key The key of machine learning service.
 * @param[in] input The data to be registered on the offloading server.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int _ml_service_offloading_request (ml_service_h handle, const char *key, const ml_tensors_data_h input);

/**
 * @brief Internal function to request service to ml-service offloading.
 * @param[in] handle The handle of ml-service.
 * @param[in] key The key of machine learning service.
 * @param[in] data The raw data to be registered on the offloading server.
 * @param[in] len The size of raw data.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int _ml_service_offloading_request_raw (ml_service_h handle, const char *key, void *data, size_t len);

/**
 * @brief Internal function to set a required value in ml-service offloading handle.
 * @param[in] handle The handle of ml-service.
 * @param[in] name The service key.
 * @param[in] value The value to set.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int _ml_service_offloading_set_information (ml_service_h handle, const char *name, const char *value);

/**
 * @brief Internal function to set offloading mode and private data.
 * @param[in] handle The handle of ml-service.
 * @param[in] mode The offloading mode.
 * @param[in] priv The private data for each offloading mode.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int _ml_service_offloading_set_mode (ml_service_h handle, ml_service_offloading_mode_e mode, void *priv);

/**
 * @brief Internal function to get offloading mode and private data.
 * @param[in] handle The handle of ml-service.
 * @param[out] mode The offloading mode.
 * @param[out] priv The private data for each offloading mode.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int _ml_service_offloading_get_mode (ml_service_h handle, ml_service_offloading_mode_e *mode, void **priv);

/**
 * @brief Internal function to release ml-service offloading data.
 */
int _ml_service_offloading_release_internal (ml_service_s *mls);
#else
#define _ml_service_offloading_create(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_start(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_stop(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_request(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_request_raw(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_set_information(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_release_internal(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_set_mode(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_offloading_get_mode(...) ML_ERROR_NOT_SUPPORTED
#endif /* ENABLE_ML_OFFLOADING */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_SERVICE_OFFLOADING_INTERNAL_H__ */
