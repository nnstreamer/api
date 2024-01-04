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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Enumeration for ml-service type.
 */
typedef enum
{
  ML_SERVICE_TYPE_UNKNOWN = 0,
  ML_SERVICE_TYPE_SERVER_PIPELINE,
  ML_SERVICE_TYPE_CLIENT_QUERY,
  ML_SERVICE_TYPE_REMOTE,

  ML_SERVICE_TYPE_MAX
} ml_service_type_e;

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
 * @brief Internal function to release ml-service pipeline data.
 */
int ml_service_pipeline_release_internal (void *priv);

/**
 * @brief Internal function to release ml-service query data.
 */
int ml_service_query_release_internal (void *priv);

#if defined(ENABLE_REMOTE_SERVICE)
/**
 * @brief Internal function to release ml-service remote data.
 */
int ml_service_remote_release_internal (void *priv);
#else
#define ml_service_remote_release_internal(...) ML_ERROR_NOT_SUPPORTED
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_PRIVATE_DATA_H__ */
