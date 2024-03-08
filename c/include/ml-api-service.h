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
 * @remarks If the name already exists, the pipeline description is overwritten. Overwriting an existing description is restricted to APP/service that set it. However, users should keep their @a name unexposed to prevent unexpected overwriting.
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
 * status = ml_service_pipeline_set ("my_pipeline", my_pipeline);
 * if (status != ML_ERROR_NONE) {
 *   // handle error case
 *   goto error;
 * }
 *
 * // Example to construct a pipeline with stored pipeline description.
 * // Users may register intelligence pipelines for other processes and fetch such registered pipelines.
 * // For example, a developer adds a pipeline which includes preprocessing and invoking a neural network model,
 * // then an application can fetch and construct this for intelligence service.
 * status = ml_service_pipeline_get ("my_pipeline", &pipeline);
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
int ml_service_pipeline_set (const char *name, const char *pipeline_desc);

/**
 * @brief Gets the pipeline description with a given name.
 * @since_tizen 7.0
 * @remarks If the function succeeds, @a pipeline_desc must be released using free().
 * @param[in] name The unique name to retrieve.
 * @param[out] pipeline_desc The pipeline corresponding with the given name.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_pipeline_get (const char *name, char **pipeline_desc);

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
int ml_service_pipeline_delete (const char *name);

/**
 * @brief A handle for ml-service instance.
 * @since_tizen 7.0
 */
typedef void *ml_service_h;

/**
 * @brief Launches the pipeline of given service and gets the service handle.
 * @details This requests machine learning agent daemon to launch a new pipeline of given service. The pipeline of service @a name should be set.
 * @since_tizen 7.0
 * @remarks The @a handle should be destroyed using ml_service_destroy().
 * @param[in] name The service name.
 * @param[out] handle Newly created service handle is returned.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 */
int ml_service_pipeline_launch (const char *name, ml_service_h *handle);

/**
 * @brief Starts the pipeline of given service handle.
 * @details This requests machine learning agent daemon to start the pipeline.
 * @since_tizen 7.0
 * @param[in] handle The service handle.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to start the pipeline.
 */
int ml_service_start (ml_service_h handle);

/**
 * @brief Stops the pipeline of given service handle.
 * @details This requests machine learning agent daemon to stop the pipeline.
 * @since_tizen 7.0
 * @param[in] handle The service handle.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to stop the pipeline.
 */
int ml_service_stop (ml_service_h handle);

/**
 * @brief Destroys the given service handle.
 * @details If given service handle is created by ml_service_pipeline_launch(), this requests machine learning agent daemon to destroy the pipeline.
 * @since_tizen 7.0
 * @param[in] handle The service handle.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to access the pipeline state.
 */
int ml_service_destroy (ml_service_h handle);

/**
 * @brief Gets the state of given handle's pipeline.
 * @since_tizen 7.0
 * @param[in] handle The service handle.
 * @param[out] state The pipeline state.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to access the pipeline state.
 */
int ml_service_pipeline_get_state (ml_service_h handle, ml_pipeline_state_e *state);

/**
 * @brief Creates query service handle with given ml-option handle.
 * @since_tizen 7.0
 * @remarks The @a handle should be destroyed using ml_service_destroy().
 * @param[in] option The option used for creating query service.
 * @param[out] handle Newly created query service handle is returned.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 */
int ml_service_query_create (ml_option_h option, ml_service_h *handle);

/**
 * @brief Requests the query service to process the @a input and produce an @a output.
 * @since_tizen 7.0
 * @remarks If the function succeeds, the @a output should be released using ml_tensors_data_destroy().
 * @param[in] handle The query service handle created by ml_service_query_create().
 * @param[in] input The handle of input tensors.
 * @param[out] output The handle of output tensors.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE The input is incompatible with the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_TIMED_OUT Failed to get output from the query service.
 */
int ml_service_query_request (ml_service_h handle, const ml_tensors_data_h input, ml_tensors_data_h *output);

/**
 * @brief Registers new information of a neural network model.
 * @since_tizen 8.0
 * @remarks Only one model can be activated with given @a name. If same name is already registered in machine learning service, this returns no error and old model will be deactivated when the flag @a activate is true.
 * @remarks %http://tizen.org/privilege/mediastorage is needed if model file is relevant to media storage.
 * @remarks %http://tizen.org/privilege/externalstorage is needed if model file is relevant to external storage.
 * @param[in] name The unique name to indicate the model.
 * @param[in] path The path to neural network model.
 * @param[in] activate The flag to set the model to be activated.
 * @param[in] description Nullable, description for neural network model.
 * @param[out] version The version of registered model.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_model_register (const char *name, const char *path, const bool activate, const char *description, unsigned int *version);

/**
 * @brief Updates the description of neural network model with given @a name and @a version.
 * @since_tizen 8.0
 * @param[in] name The unique name to indicate the model.
 * @param[in] version The version of registered model.
 * @param[in] description The description for neural network model.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_model_update_description (const char *name, const unsigned int version, const char *description);

/**
 * @brief Activates a neural network model with given @a name and @a version.
 * @since_tizen 8.0
 * @param[in] name The unique name to indicate the model.
 * @param[in] version The version of registered model.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_model_activate (const char *name, const unsigned int version);

/**
 * @brief Gets the information of neural network model with given @a name and @a version.
 * @since_tizen 8.0
 * @remarks If the function succeeds, the @a info should be released using ml_option_destroy().
 * @param[in] name The unique name to indicate the model.
 * @param[in] version The version of registered model.
 * @param[out] info The handle of model.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_model_get (const char *name, const unsigned int version, ml_option_h *info);

/**
 * @brief Gets the information of activated neural network model with given @a name.
 * @since_tizen 8.0
 * @remarks If the function succeeds, the @a info should be released using ml_option_destroy().
 * @param[in] name The unique name to indicate the model.
 * @param[out] info The handle of activated model.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_model_get_activated (const char *name, ml_option_h *info);

/**
 * @brief Gets the list of neural network model with given @a name.
 * @since_tizen 8.0
 * @remarks If the function succeeds, each handle in @a info_list should be released using ml_option_destroy().
 * @param[in] name The unique name to indicate the model.
 * @param[out] info_list The handles of registered model.
 * @param[out] num Total number of registered model.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_model_get_all (const char *name, ml_option_h *info_list[], unsigned int *num);

/**
 * @brief Deletes a model information with given @a name and @a version from machine learning service.
 * @since_tizen 8.0
 * @remarks This does not remove the model file from file system. If @a version is 0, machine learning service will delete all information with given @a name.
 * @param[in] name The unique name to indicate the model.
 * @param[in] version The version of registered model.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_model_delete (const char *name, const unsigned int version);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_H__ */
