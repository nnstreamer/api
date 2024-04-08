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

#if defined(ENABLE_SERVICE_OFFLOADING)
/**
 * @brief Parse configuration file and create offloading service.
 * @param[in] handle The handle of ml-service created by ml_service_new().
 * @param[in] option The option used for creating query service.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_offloading_create (ml_service_h handle, ml_option_h option);

/**
 * @brief Request service to ml-service offloading.
 * @param[in] handle The handle of ml-service.
 * @param[in] key The key of machine learning service.
 * @param[in] input The Data to be registered on the offloading server.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_service_offloading_request (ml_service_h handle, const char *key, const ml_tensors_data_h input);

/**
 * @brief Sets the services in ml-service offloading handle.
 * @param[in] handle The handle of ml-service.
 * @param[in] name The service key.
 * @param[in] value The value to be set (json string).
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 */
int ml_service_offloading_set_service (ml_service_h handle, const char *name, const char *value);

/**
 * @brief Set a required value in ml-service offloading handle.
 * @param[in] handle The handle of ml-service.
 * @param[in] name The service key.
 * @param[in] value The value to set.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_service_offloading_set_information (ml_service_h handle, const char *name, const char *value);

/**
 * @brief Internal function to release ml-service offloading data.
 */
int ml_service_offloading_release_internal (ml_service_s *mls);
#else
#define ml_service_offloading_create(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_offloading_request(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_offloading_set_service(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_offloading_set_information(...) ML_ERROR_NOT_SUPPORTED
#define ml_service_offloading_release_internal(...) ML_ERROR_NOT_SUPPORTED
#endif /* ENABLE_SERVICE_OFFLOADING */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_SERVICE_OFFLOADING_INTERNAL_H__ */
