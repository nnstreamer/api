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
#include "nnstreamer-tizen-internal.h"
#include "ml-api-experimental.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_PRIVATE_DATA_H__ */
