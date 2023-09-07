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


typedef enum {
  ML_SERVICE_TYPE_UNKNOWN = 0,
  ML_SERVICE_TYPE_SERVER_PIPELINE,
  ML_SERVICE_TYPE_CLIENT_QUERY,
  ML_SERVICE_TYPE_REMOTE,

  ML_SERVICE_TYPE_MAX
} ml_service_type_e;

typedef enum {
  ML_REMOTE_SERVICE_TYPE_UNKNOWN = 0,
  ML_REMOTE_SERVICE_TYPE_MODEL_RAW,
  ML_REMOTE_SERVICE_TYPE_MODEL_URI,
  ML_REMOTE_SERVICE_TYPE_PIPELINE_RAW,
  ML_REMOTE_SERVICE_TYPE_PIPELINE_URI,

  ML_REMOTE_SERVICE_TYPE_MAX
} ml_remote_service_type_e;


/**
 * @brief Creates ml remote service handle with given ml-option handle.
 * @since_tizen 8.0
 * @note This is not official and public API but experimental API.
 * @details The caller should set one of "remote_sender" and "remote_receiver" as a service type in @a ml_option.
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
int ml_remote_service_create (ml_option_h option, ml_service_h *handle);

/**
 * @todo DRAFT. API name should be determined later.
 * @brief Register new information, such as neural network models or pipeline descriptions, on a remote server.
 * @since_tizen 8.0
 * @note This is not official and public API but experimental API.
 * @param[in] handle The query service handle created by ml_service_query_create().
 * @param[in] option The option used for registering machine learning service.
 * @param[in] data The Data to be registered on the remote server.
 * @return 0 on success. Otherwise a negative error value.
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
 * status = ml_remote_service_create (client_option_h, &client_h);
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
 * ml_remote_service_create (server_option_h, &server_h)
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
 * ml_remote_service_create (client_option_h, &client_h);
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
 * ml_remote_service_register (client_h, remote_service_option_h, contents, len);
 *
 * ml_service_destroy (client_h);
 * ml_service_destroy (server_h);
 * ml_option_destroy (server_option_h);
 * ml_option_destroy (client_option_h);
 * ml_option_destroy (remote_service_option_h);
 *
 * @endcode
 */
int ml_remote_service_register (ml_service_h handle, ml_option_h option, void *data, size_t data_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __TIZEN_MACHINELEARNING_NNSTREAMER_INTERNAL_H__ */
