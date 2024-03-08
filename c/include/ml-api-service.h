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
#include <nnstreamer-single.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/**
 * @addtogroup CAPI_ML_NNSTREAMER_SERVICE_MODULE
 * @{
 */

/**
 * @brief A handle for ml-service instance.
 * @since_tizen 7.0
 */
typedef void *ml_service_h;

/**
 * @brief Enumeration for the event types of machine learning service.
 * @since_tizen 9.0
 */
typedef enum {
  ML_SERVICE_EVENT_UNKNOWN = 0,             /**< Unknown or invalid event type. */
  ML_SERVICE_EVENT_NEW_DATA = 1,            /**< New data is processed from machine learning service. */
} ml_service_event_e;

/**
 * @brief Callback for the event from machine learning service.
 * @details Note that the handle of event data may be deallocated after the return and this is synchronously called.
 *           Thus, if you need the event data, copy the data and return fast. Do not spend too much time in the callback.
 * @since_tizen 9.0
 * @remarks The @a event_data should not be released.
 * @param[in] event The event from machine learning service.
 * @param[in] event_data The handle of event data. If it is null, the event does not include data field.
 * @param[in] user_data Private data for the callback.
 */
typedef void (*ml_service_event_cb) (ml_service_event_e event, ml_information_h event_data, void *user_data);

/**
 * @brief Creates a handle for machine learning service using a configuration file.
 * @since_tizen 9.0
 * @remarks %http://tizen.org/privilege/mediastorage is needed if the configuration is relevant to media storage.
 * @remarks %http://tizen.org/privilege/externalstorage is needed if the configuration is relevant to external storage.
 * @remarks The @a handle should be released using ml_service_destroy().
 * @param[in] config The absolute path to configuration file.
 * @param[out] handle The handle of ml-service.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the media storage or external storage.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR Failed to parse the configuration file.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to open the model.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 *
 * Here is an example of the usage:
 * @code
 *
 * // Callback function for the event from machine learning service.
 * // Note that the handle of event data will be deallocated after the return and this is synchronously called.
 * // Thus, if you need the event data, copy the data and return fast.
 * // Do not spend too much time in the callback.
 * static void
 * _ml_service_event_cb (ml_service_event_e event, ml_information_h event_data, void *user_data)
 * {
 *   ml_tensors_data_h data;
 *   void *_data;
 *   size_t _size;
 *
 *   switch (event) {
 *     case ML_SERVICE_EVENT_NEW_DATA:
 *       // For the case of new data event, handle output data.
 *       ml_information_get (event_data, "data", &data);
 *       ml_tensors_data_get_tensor_data (data, 0, &_data, &_size);
 *       break;
 *     default:
 *       break;
 *   }
 * }
 *
 * // The path to the configuration file.
 * const char config_path[] = "/path/to/application/configuration/my_application_config.conf";
 *
 * // Create ml-service for model inference from configuration.
 * ml_service_h handle;
 *
 * ml_service_new (config_path, &handle);
 * ml_service_set_event_cb (handle, _ml_service_event_cb, NULL);
 *
 * // Get input information and allocate input buffer.
 * ml_tensors_info_h input_info;
 * void *input_buffer;
 * size_t input_size;

 * ml_service_get_input_information (handle, NULL, &input_info);
 *
 * ml_tensors_info_get_tensor_size (input_info, 0, &input_size);
 * input_buffer = malloc (input_size);
 *
 * // Create input data handle.
 * ml_tensors_data_h input;
 *
 * ml_tensors_data_create (input_info, &input);
 * ml_tensors_data_set_tensor_data (input, 0, input_buffer, input_size);
 *
 * // Push input data into ml-service and process the output in the callback.
 * ml_service_request (handle, NULL, input);
 *
 * // Finally, release all handles and allocated memories.
 * ml_tensors_info_destroy (input_info);
 * ml_tensors_data_destroy (input);
 * ml_service_destroy (handle);
 * free (input_buffer);
 *
 * @endcode
 */
int ml_service_new (const char *config, ml_service_h *handle);

/**
 * @brief Sets the callback which will be invoked when a new event occurs from machine learning service.
 * @since_tizen 9.0
 * @param[in] handle The handle of ml-service.
 * @param[in] cb The callback to handle the event from ml-service.
 * @param[in] user_data Private data for the callback. This value is passed to the callback when it's invoked.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_service_set_event_cb (ml_service_h handle, ml_service_event_cb cb, void *user_data);

/**
 * @brief Starts the process of machine learning service.
 * @since_tizen 9.0
 * @param[in] handle The handle of ml-service.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to start the process.
 */
int ml_service_start (ml_service_h handle);

/**
 * @brief Stops the process of machine learning service.
 * @since_tizen 9.0
 * @param[in] handle The handle of ml-service.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to stop the process.
 */
int ml_service_stop (ml_service_h handle);

/**
 * @brief Gets the information of required input data.
 * @details Note that a model may not have such information if its input type is not determined statically.
 * @since_tizen 9.0
 * @remarks The @a info should be released using ml_tensors_info_destroy().
 * @param[in] handle The handle of ml-service.
 * @param[in] name The name of input node in the pipeline. You can set NULL if ml-service is constructed from model configuration.
 * @param[out] info The handle of input tensors information.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 */
int ml_service_get_input_information (ml_service_h handle, const char *name, ml_tensors_info_h *info);

/**
 * @brief Gets the information of output data.
 * @details Note that a model may not have such information if its output is not determined statically.
 * @since_tizen 9.0
 * @remarks The @a info should be released using ml_tensors_info_destroy().
 * @param[in] handle The handle of ml-service.
 * @param[in] name The name of output node in the pipeline. You can set NULL if ml-service is constructed from model configuration.
 * @param[out] info The handle of output tensors information.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 */
int ml_service_get_output_information (ml_service_h handle, const char *name, ml_tensors_info_h *info);

/**
 * @brief Sets the information for machine learning service.
 * @since_tizen 9.0
 * @param[in] handle The handle of ml-service.
 * @param[in] name The name to set the corresponding value.
 * @param[in] value The value of the name.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 */
int ml_service_set_information (ml_service_h handle, const char *name, const char *value);

/**
 * @brief Gets the information from machine learning service.
 * @details Note that a configuration file may not have such information field.
 * @since_tizen 9.0
 * @remarks The @a value should be released using free().
 * @param[in] handle The handle of ml-service.
 * @param[in] name The name to get the corresponding value.
 * @param[out] value The value of the name.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 */
int ml_service_get_information (ml_service_h handle, const char *name, char **value);

/**
 * @brief Adds an input data to process the model in machine learning service.
 * @since_tizen 9.0
 * @param[in] handle The handle of ml-service.
 * @param[in] name The name of input node in the pipeline. You can set NULL if ml-service is constructed from model configuration.
 * @param[in] data The handle of tensors data to be processed.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to process the input data.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_request (ml_service_h handle, const char *name, const ml_tensors_data_h data);

/**
 * @brief Destroys the handle for machine learning service.
 * @details If given service handle is created by ml_service_pipeline_launch(), this requests machine learning agent to destroy the pipeline.
 * @since_tizen 7.0
 * @param[in] handle The handle of ml-service.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to stop the process.
 */
int ml_service_destroy (ml_service_h handle);

/****************************************************
 ** API for AI pipeline                            **
 ****************************************************/
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

/** @todo remove below macros after updating tct. */
#define ml_service_set_pipeline ml_service_pipeline_set
#define ml_service_get_pipeline ml_service_pipeline_get
#define ml_service_delete_pipeline ml_service_pipeline_delete
#define ml_service_launch_pipeline ml_service_pipeline_launch
#define ml_service_start_pipeline ml_service_start
#define ml_service_stop_pipeline ml_service_stop
#define ml_service_get_pipeline_state ml_service_pipeline_get_state

/****************************************************
 ** API for among-device AI service                **
 ****************************************************/
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
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE The input is incompatible with the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_TIMED_OUT Failed to get output from the query service.
 */
int ml_service_query_request (ml_service_h handle, const ml_tensors_data_h input, ml_tensors_data_h *output);

/****************************************************
 ** API for managing AI models                     **
 ****************************************************/
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
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 *
 * Here is an example of the usage:
 * @code
 * // The machine-learning service API for model provides a method to share model files those can be used for ML application.
 *
 * /// Model Provider APP
 * const gchar *key = "imgcls-mobilenet"; // The name shared among ML applications.
 * gchar *model_path = g_strdup_printf ("%s/%s", app_get_shared_resource_path (), "mobilenet_v2.tflite"); // Provide the absolute file path.
 * const bool is_active = true; // Parameter deciding whether to activate this model or not.
 * const gchar *description = "This is the description of mobilenet_v2 model ..."; // Model description parameter.
 * unsigned int version; // Out parameter for the version of registered model.
 *
 * // Register the model via ML Service API.
 * int status;
 * status = ml_service_model_register (key, model_path, is_active, description, &version);
 * if (status != ML_ERROR_NONE) {
 *   // Handle error case.
 * }
 *
 * /// Model Consumer APP
 * const gchar *key = "imgcls-mobilenet"; // The name shared among ML applications.
 * gchar *model_path; // Out parameter for the path of registered model.
 * ml_information_h activated_model_info; // The ml_information handle for the activated model.
 *
 * // Get the model which is registered and activated by ML Service API.
 * int status;
 * status = ml_service_model_get_activated (key, &activated_model_info);
 * if (status == ML_ERROR_NONE) {
 *   // Get the path of the model.
 *   gchar *activated_model_path;
 *   status = ml_information_get (activated_model_info, "path", (void **) &activated_model_path);
 *   model_path = g_strdup (activated_model_path);
 * } else {
 *   // Handle error case.
 * }
 *
 * ml_information_destroy (activated_model_info); // Release the information handle.
 *
 * // Do ML things with the variable `model_path`.
 * @endcode
 */
int ml_service_model_register (const char *name, const char *path, const bool activate, const char *description, unsigned int *version);

/**
 * @brief Updates the description of neural network model with given @a name and @a version.
 * @since_tizen 8.0
 * @param[in] name The unique name to indicate the model.
 * @param[in] version The version of registered model.
 * @param[in] description The description for neural network model.
 * @return @c 0 on success. Otherwise a negative error value.
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
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_model_activate (const char *name, const unsigned int version);

/**
 * @brief Gets the information of neural network model with given @a name and @a version.
 * @since_tizen 8.0
 * @remarks If the function succeeds, the @a info should be released using ml_information_destroy().
 * @param[in] name The unique name to indicate the model.
 * @param[in] version The version of registered model.
 * @param[out] info The handle of model information.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_model_get (const char *name, const unsigned int version, ml_information_h *info);

/**
 * @brief Gets the information of activated neural network model with given @a name.
 * @since_tizen 8.0
 * @remarks If the function succeeds, the @a info should be released using ml_information_destroy().
 * @param[in] name The unique name to indicate the model.
 * @param[out] info The handle of activated model.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_model_get_activated (const char *name, ml_information_h *info);

/**
 * @brief Gets the list of neural network model with given @a name.
 * @since_tizen 8.0
 * @remarks If the function succeeds, the @a info_list should be released using ml_information_list_destroy().
 * @param[in] name The unique name to indicate the model.
 * @param[out] info_list The handle of list of registered models.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_model_get_all (const char *name, ml_information_list_h *info_list);

/**
 * @brief Deletes a model information with given @a name and @a version from machine learning service.
 * @since_tizen 8.0
 * @remarks This does not remove the model file from file system. If @a version is 0, machine learning service will delete all information with given @a name.
 * @param[in] name The unique name to indicate the model.
 * @param[in] version The version of registered model.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_model_delete (const char *name, const unsigned int version);

/****************************************************
 ** API for managing AI data files                 **
 ****************************************************/
/**
 * @brief Adds new information of machine learning resources those contain images, audio samples, binary files, and so on.
 * @since_tizen 8.0
 * @remarks If same name is already registered in machine learning service, this returns no error and the list of resource files will be updated.
 * @remarks %http://tizen.org/privilege/mediastorage is needed if model file is relevant to media storage.
 * @remarks %http://tizen.org/privilege/externalstorage is needed if model file is relevant to external storage.
 * @param[in] name The unique name to indicate the resources.
 * @param[in] path The path to machine learning resources.
 * @param[in] description Nullable, description for machine learning resources.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 *
 * Here is an example of the usage:
 * @code
 * // The machine-learning resource API provides a method to share the data files those can be used for training or inferencing AI model.
 * // Users may generate preprocessed data file, and add it into machine-learning service.
 * // Then an application can fetch the data set for retraining an AI model.
 *
 * const char *my_resources[3] = {
 *   "/path/to/resources/my_res1.dat",
 *   "/path/to/resources/my_res2.dat"
 *   "/path/to/resources/my_res3.dat"
 * };
 *
 * int status;
 * unsigned int i, length;
 * ml_information_list_h resources;
 * ml_information_h res;
 * char *path_to_resource;
 *
 * // Add resource files with name "my_resource".
 * for (i = 0; i < 3; i++) {
 *   status = ml_service_resource_add ("my_resource", my_resources[i], "This is my resource data file.");
 *   if (status != ML_ERROR_NONE) {
 *     // Handle error case.
 *   }
 * }
 *
 * // Get the resources with specific name.
 * status = ml_service_resource_get ("my_resource", &resources);
 * if (status != ML_ERROR_NONE) {
 *   // Handle error case.
 * }
 *
 * status = ml_information_list_length (resources, &length);
 * for (i = 0; i < length; i++) {
 *   status = ml_information_list_get (resources, i, &res);
 *   // Get the path of added resources.
 *   status = ml_information_get (res, "path", (void **) &path_to_resource);
 * }
 *
 * // Release the information handle of resources.
 * status = ml_information_list_destroy (resources);
 * @endcode
 */
int ml_service_resource_add (const char *name, const char *path, const char *description);

/**
 * @brief Deletes the information of the resources from machine learning service.
 * @since_tizen 8.0
 * @remarks This does not remove the resource files from file system.
 * @param[in] name The unique name to indicate the resources.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 */
int ml_service_resource_delete (const char *name);

/**
 * @brief Gets the information of the resources from machine learning service.
 * @since_tizen 8.0
 * @remarks If the function succeeds, the @a res should be released using ml_information_list_destroy().
 * @param[in] name The unique name to indicate the resources.
 * @param[out] res The handle of the machine learning resources.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR The operation of DB or filesystem has failed.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_service_resource_get (const char *name, ml_information_list_h *res);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_H__ */
