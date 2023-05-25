/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer / Tizen Machine-Learning "Service API"'s private data structures
 * Copyright (C) 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
 */
/**
 * @file    ml-api-service-private.h
 * @date    03 Nov 2021
 * @brief   ML-API Private Data Structure Header
 * @see     https://github.com/nnstreamer/api
 * @author  MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug     No known bugs except for NYI items
 */
#ifndef __ML_API_SERVICE_PRIVATE_DATA_H__
#define __ML_API_SERVICE_PRIVATE_DATA_H__

#include <ml-api-service.h>
#include <ml-api-inference-internal.h>

#include "pipeline-dbus.h"
#include "model-dbus.h"
#include "resource-dbus.h"
#include "nnstreamer-edge.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
  ML_REMOTE_SERVICE_TYPE_MODEL_URL,
  ML_REMOTE_SERVICE_TYPE_PIPELINE_RAW,
  ML_REMOTE_SERVICE_TYPE_PIPELINE_URL,

  ML_REMOTE_SERVICE_TYPE_MAX
} ml_remote_service_type_e;

/**
 * @brief Structure for ml_remote_service_h
 */
typedef struct
{
  ml_service_type_e type;

  void *priv;
} ml_service_s;

/**
 * @brief Structure for ml_service_server
 */
typedef struct
{
  gint64 id;
  gchar *service_name;
} _ml_service_server_s;

/**
 * @brief Structure for ml_service_query
 */
typedef struct
{
  ml_pipeline_h pipe_h;
  ml_pipeline_src_h src_h;
  ml_pipeline_sink_h sink_h;

  gchar *caps;
  guint timeout; /**< in ms unit */
  GAsyncQueue *out_data_queue;
} _ml_service_query_s;

/**
 * @brief Structure for ml_remote_service
 */
typedef struct
{
  nns_edge_h edge_h;
  nns_edge_node_type_e node_type;
} _ml_remote_service_s;

/**
 * @brief Creates ml remote service handle with given ml-option handle.
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
#endif /* __ML_API_SERVICE_PRIVATE_DATA_H__ */
