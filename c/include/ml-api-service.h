/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer / Tizen Machine-Learning "Service API" Header
 * Copyright (C) 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
 */
/**
 * @file    ml-api-service.h
 * @date    03 Nov 2021
 * @brief   ML-API Service Header
 * @see     https://github.com/nnstreamer/api
 * @author  MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    Phase 1. With ML Service APIs, users may register intelligence pipelines
 *      for other processes and fetch such registered pipelines.
 *    Phase 2. With ML Service APIs, users may register/start services of
 *      such pipelines so that other processes may share the instances of
 *      pipelines.
 *    Note that this requires a shared pipeline repository (Phase 1) and
 *    a shared service/process (Phase 2), which may heavily depend on the
 *    given software platforms or operating systems.
 *
 *    Because registered (added) models and pipelines should be provided to
 *      other processes, ml-api-service implementation should be a daemon
 *      or have a shared repository. Note that Phase 2 requires a daemon
 *      anyway.
 */
#ifndef __ML_API_SERVICE_H__
#define __ML_API_SERVICE_H__

#include <nnstreamer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Sets the pipeline description with a given name.
 * @since_tizen 7.0
 * @param[in] name Unique name to retrieve the associated pipeline description.
 * @param[in] pipeline_desc The pipeline description to be stored
 * @return @c 0 on success. Otherwise a negative error value.
 * @note If the name already exists, the pipeline description is overwritten.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem is failed.
 */
int ml_service_set_pipeline (const char *name, const char * pipeline_desc);

/**
 * @brief Gets the pipeline description with a given name.
 * @since_tizen 7.0
 * @param[in] name The unique name to retrieve.
 * @param[out] pipeline_desc The pipeline corresponding with the given name.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem is failed.
 */
int ml_service_get_pipeline (const char *name, char **pipeline_desc);

/**
 * @brief Deletes the pipeline description with a given name.
 * @since_tizen 7.0
 * @param[in] name The unique name to delete.
 * @return @c 0 on success. Otherwise a negative error value.
 * @note If the name did not exist in the database, this function returns ML_ERROR_NONE without any errors.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem is failed.
 */
int ml_service_delete_pipeline (const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_H__ */
