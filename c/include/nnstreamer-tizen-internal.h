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
#include <ml-api-service.h>
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

typedef void *ml_service_event_h;

/**
 * @brief Enumeration for the event types of ml-service.
 */
typedef enum {
  ML_SERVICE_EVENT_MODEL_REGISTERED = 0,
  ML_SERVICE_EVENT_PIPELINE_REGISTERED,

  NNS_EDGE_EVENT_TYPE_UNKNOWN
} ml_service_event_e;

/**
 * @brief Callback for the ml-service event.
 * @return User should return ML_ERROR_NONE if an event is successfully handled.
 */
typedef int (*ml_service_event_cb) (ml_service_event_e event_type, void *user_data);

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

/**
 * @brief Creates ml remote service handle with given ml-option handle.
 * @since_tizen 8.0
 * @note This is not official and public API but experimental API.
 * @details The caller should set one of "remote_sender" and "remote_receiver" as a service type in @a ml_option.
 * @remarks The @a handle should be destroyed using ml_service_destroy().
 * @param[in] option The option used for creating query service.
 * @param[in] cb The ml-service callbacks for event handling.
 * @param[in] user_data Private data for the callback. This value is passed to the callback when service is received.
 * @param[out] handle Newly created query service handle is returned.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 */
int ml_service_remote_create (ml_option_h option, ml_service_event_cb cb, void *user_data, ml_service_h *handle);


/**
 * @brief Creates ml remote service handle with json string.
 * @since_tizen 8.5
 * @note This is not official and public API but experimental API.
 * @remarks The @a handle should be destroyed using ml_service_destroy().
 * @param[in] json_string json string describing ml-option. This is not the path of json file.
 * @param[in] cb The ml-service callbacks for event handling.
 * @param[in] user_data Private data for the callback. This value is passed to the callback when service is received.
 * @param[out] handle Newly created query service handle is returned.
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 *
 * Here is an example of the json string:
 * @code
 * // Update this field later.
 * @endcode
 */
int ml_service_remote_create_from_json (const char *json_string, ml_service_event_cb cb, void *user_data, ml_service_h * handle);

/**
 * @todo DRAFT. API name should be determined later.
 * @brief Register new information, such as neural network models or pipeline descriptions, on a remote server.
 * @since_tizen 8.0
 * @note This is not official and public API but experimental API.
 * @param[in] handle The remote service handle created by ml_service_remote_create().
 * @param[in] option The option used for registering machine learning service.
 * @param[in] data The Data to be registered on the remote server.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 *
 * Here is an example of the usage:
 * @code
 * // ================== Client side ==================
 * // Example of saving a model file received from a client.
 *
 *
 *   status = ml_option_create (&client_option_h);
 *
 * gchar *client_node_type = g_strdup ("remote_sender");
 * status = ml_option_set (client_option_h, "node-type", client_node_type, g_free);
 *
 * gchar *client_connect_type = g_strdup ("TCP");
 * ml_option_set (client_option_h, "connect-type", client_connect_type, g_free);
 *
 * status = ml_service_remote_create (client_option_h, NULL, NULL, &client_h);
 *
 * // ================== Server side ==================
 * ml_service_h server_h;
 * ml_option_h server_option_h = NULL;
 *
 * // Set option to handle
 * ml_option_create (&server_option_h);
 *
 *  gchar *server_node_type = g_strdup ("remote_receiver");
 *  ml_option_set (server_option_h, "node-type", server_node_type, g_free);
 *
 *  gchar *server_connect_type = g_strdup ("TCP");
 *  ml_option_set (server_option_h, "connect-type", server_connect_type, g_free);
 *
 * // Create ml-remote service.
 * ml_service_remote_create (server_option_h, NULL, NULL, &server_h)
 *
 * // ================== Client side ==================
 * // Send neural network model url to the query server.
 * ml_service_h client_h;
 * ml_option_h client_option_h = NULL;
 *
 * // Set option to handle
 * ml_option_create (&client_option_h);
 *
 * gchar *node_type = g_strdup ("remote_launch_client");
 * ml_option_set (client_option_h, "node_type", node_type, g_free);
 *
 * // Set the options required for connection, such as server address, port, protocol, etc.
 * gchar *dest_host = g_strdup ("localhost");
 * ml_option_set (client_option_h, "dest_host", dest_host, g_free);
 *
 * // Create query service.
 * ml_service_remote_create (client_option_h, NULL, NULL, &client_h);
 *
 * ml_option_h query_option_h = NULL;
 * ml_option_create (&query_option_h);
 *
 * // ================== Create service option ==================
 * // Load model files.
 * ml_option_h remote_service_option_h = NULL;
 * ml_option_create (&remote_service_option_h);
 *
 * gchar *service_key = g_strdup ("model_registration_test_key");
 * ml_option_set (remote_service_option_h, "service-key", service_key, g_free);
 *
 * gchar *service_type = g_strdup ("model_raw");
 * ml_option_set (remote_service_option_h, "service-type", service_type, g_free);
 *
 * gchar *activate = g_strdup ("true");
 * ml_option_set (remote_service_option_h, "activate", activate, g_free);
 *
 * gchar *description = g_strdup ("temp description for remote model register test");
 * ml_option_set (remote_service_option_h, "description", description, g_free);
 *
 * gchar *name = g_strdup ("model_name.nnfw");
 * ml_option_set (remote_service_option_h, "name", name, g_free);
 *
 * ml_service_remote_register (client_h, remote_service_option_h, contents, len);
 *
 * ml_service_destroy (client_h);
 * ml_service_destroy (server_h);
 * ml_option_destroy (server_option_h);
 * ml_option_destroy (client_option_h);
 * ml_option_destroy (remote_service_option_h);
 *
 * @endcode
 * @todo Let's use ml_service_remote_request() instead of ml_service_remote_register(). I will replace ml_service_remote_register() with ml_service_remote_request() when ml_service_remote_request() is determined to use.
 */
int ml_service_remote_register (ml_service_h handle, ml_option_h option, void *data, size_t data_len);

/**
 * @todo DRAFT. API name should be determined later.
 * @brief Request remote servers to register Neural network models or launch GStreaner pipelines, etc. with ml-option handle.
 * @since_tizen 8.5
 * @note This is not official and public API but experimental API.
 * @param[in] handle The remote service handle created by ml_service_remote_create() or ml_service_remote_create_from_json().
 * @param[in] option The option used for registering machine learning service.
 * @param[in] data The Data to be registered on the remote server.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 *
 * Here is an example of the usage:
 * @code
 * // Update this field later.
 * @endcode
 */
int ml_service_remote_request (ml_service_h handle, ml_option_h option, void *data, size_t data_len);

/**
 * @todo DRAFT. API name should be determined later.
 * @brief Request remote servers to register Neural network models or launch GStreaner pipelines, etc. with json string.
 * @since_tizen 8.5
 * @note This is not official and public API but experimental API.
 * @param[in] handle The remote service handle created by ml_service_remote_create() or ml_service_remote_create_from_json().
 * @param[in] json_string json string describing ml-option. This is not the path of json file.
 * @param[in] data The Data to be registered on the remote server.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 *
 * Here is an example of the usage:
 * @code
 * // Update this field later.
 * @endcode
 */
int ml_service_remote_request_from_json (ml_service_h handle, const char *json_string, void *data, size_t data_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __TIZEN_MACHINELEARNING_NNSTREAMER_INTERNAL_H__ */
