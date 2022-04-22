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
 * @addtogroup CAPI_ML_NNSTREAMER_SERVICE_MODULE
 * @{
 */

/**
 * @brief Sets the pipeline description with a given name.
 * @since_tizen 7.0
 * @remarks If the name already exists, the pipeline description is overwritten.
 * @param[in] name Unique name to retrieve the associated pipeline description.
 * @param[in] pipeline_desc The pipeline description to be stored.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 *
 * Here is an example of the usage:
 * @code
 * const gchar my_pipeline[] = "videotestsrc is-live=true ! videoconvert ! tensor_converter ! tensor_sink async=false";
 * gchar *pipeline;
 * int status;
 * ml_pipeline_h handle;
 *
 * // Set pipeline description.
 * status = ml_service_set_pipeline ("my_pipeline", my_pipeline);
 * if (status != ML_ERROR_NONE) {
 *   // handle error case
 *   goto error;
 * }
 *
 * // Example to construct a pipeline with stored pipeline description.
 * // Users may register intelligence pipelines for other processes and fetch such registered pipelines.
 * // For example, a developer adds a pipeline which includes preprocessing and invoking a neural network model,
 * // then an application can fetch and construct this for intelligence service.
 * status = ml_service_get_pipeline ("my_pipeline", &pipeline);
 * if (status != ML_ERROR_NONE) {
 *   // handle error case
 *   goto error;
 * }
 *
 * status = ml_pipeline_construct (pipeline, NULL, NULL, &handle);
 * if (status != ML_ERROR_NONE) {
 *   // handle error case
 *   goto error;
 * }
 *
 * error:
 * ml_pipeline_destroy (handle);
 * g_free (pipeline);
 * @endcode
 */
int ml_service_set_pipeline (const char *name, const char *pipeline_desc);

/**
 * @brief Gets the pipeline description with a given name.
 * @since_tizen 7.0
 * @remarks If the function succeeds, @a pipeline_desc must be released using g_free().
 * @param[in] name The unique name to retrieve.
 * @param[out] pipeline_desc The pipeline corresponding with the given name.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_get_pipeline (const char *name, char **pipeline_desc);

/**
 * @brief Deletes the pipeline description with a given name.
 * @since_tizen 7.0
 * @param[in] name The unique name to delete.
 * @return @c 0 on success. Otherwise a negative error value.
 * @note If the name does not exist in the database, this function returns ML_ERROR_NONE without any errors.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_delete_pipeline (const char *name);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_H__ */
