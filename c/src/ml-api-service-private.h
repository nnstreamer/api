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

#include <glib.h>
#include <json-glib/json-glib.h>

#include <ml-api-service.h>
#include <ml-api-inference-internal.h>
#include <mlops-agent-interface.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Macro for the event types of machine learning service.
 * @todo TBU, need ACR later (update enum for ml-service event, see ml_service_event_cb)
 */
#define ML_SERVICE_EVENT_MODEL_REGISTERED 2
#define ML_SERVICE_EVENT_PIPELINE_REGISTERED 3
#define ML_SERVICE_EVENT_REPLY 4
#define ML_SERVICE_EVENT_LAUNCH 5

/**
 * @brief Enumeration for ml-service type.
 */
typedef enum
{
  ML_SERVICE_TYPE_UNKNOWN = 0,
  ML_SERVICE_TYPE_SERVER_PIPELINE,
  ML_SERVICE_TYPE_CLIENT_QUERY,
  ML_SERVICE_TYPE_OFFLOADING,
  ML_SERVICE_TYPE_EXTENSION,

  ML_SERVICE_TYPE_MAX
} ml_service_type_e;

/**
 * @brief Structure for ml-service event callback.
 */
typedef struct
{
  ml_service_event_cb cb;
  void *pdata;
} ml_service_event_cb_info_s;

/**
 * @brief Structure for ml_service_h
 */
typedef struct
{
  uint32_t magic;
  ml_service_type_e type;
  GMutex lock;
  GCond cond;
  ml_option_h information;
  ml_service_event_cb_info_s cb_info;
  void *priv;
} ml_service_s;

/**
 * @brief Structure for ml_service_server
 */
typedef struct
{
  int64_t id;
  gchar *service_name;
} _ml_service_server_s;

/**
 * @brief Internal function to validate ml-service handle.
 */
gboolean _ml_service_handle_is_valid (ml_service_s *mls);

/**
 * @brief Internal function to create new ml-service handle.
 */
ml_service_s * _ml_service_create_internal (ml_service_type_e ml_service_type);

/**
 * @brief Internal function to release ml-service handle.
 */
int _ml_service_destroy_internal (ml_service_s *mls);

/**
 * @brief Internal function to get ml-service event callback.
 */
void _ml_service_get_event_cb_info (ml_service_s *mls, ml_service_event_cb_info_s *cb_info);

/**
 * @brief Internal function to parse string value from json.
 */
int _ml_service_conf_parse_string (JsonNode *str_node, const gchar *delimiter, gchar **str);

/**
 * @brief Internal function to parse tensors-info from json.
 */
int _ml_service_conf_parse_tensors_info (JsonNode *info_node, ml_tensors_info_h *info_h);

/**
 * @brief Internal function to release ml-service pipeline data.
 */
int _ml_service_pipeline_release_internal (ml_service_s *mls);

/**
 * @brief Internal function to release ml-service query data.
 */
int _ml_service_query_release_internal (ml_service_s *mls);

/**
 * @brief Internal function to get json string member.
 */
const gchar * _ml_service_get_json_string_member (JsonObject *object, const gchar *member_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_PRIVATE_DATA_H__ */
