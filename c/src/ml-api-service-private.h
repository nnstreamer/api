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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  ML_SERVICE_TYPE = 0,
  ML_SERVICE_TYPE_SERVER_PIPELINE,

  ML_SERVICE_TYPE_MAX
} ml_service_type_e;

/**
 * @brief Structure for ml_service_h
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
 * @brief Internal function to get proxy of the pipeline d-bus interface
 */
MachinelearningServicePipeline * _get_proxy_new_for_bus_sync (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_PRIVATE_DATA_H__ */
