/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-common.c
 * @date 31 Aug 2022
 * @brief Some implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include "ml-api-internal.h"
#include "ml-api-service.h"
#include "ml-api-service-private.h"

/**
 * @brief Internal function to get proxy of the pipeline d-bus interface
 */
MachinelearningServicePipeline *
_get_proxy_new_for_bus_sync (void)
{
  MachinelearningServicePipeline *mlsp;

  /** @todo deal with GError */
  mlsp = machinelearning_service_pipeline_proxy_new_for_bus_sync
      (G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
      "org.tizen.machinelearning.service",
      "/Org/Tizen/MachineLearning/Service/Pipeline", NULL, NULL);

  if (mlsp)
    return mlsp;

  /** Try with session type */
  mlsp = machinelearning_service_pipeline_proxy_new_for_bus_sync
      (G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE,
      "org.tizen.machinelearning.service",
      "/Org/Tizen/MachineLearning/Service/Pipeline", NULL, NULL);

  return mlsp;
}

/**
 * @brief Destroy the pipeline of given ml_service_h
 */
int
ml_service_destroy (ml_service_h h)
{
  int ret = ML_ERROR_NONE;
  ml_service_s *mls = (ml_service_s *) h;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!h)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h.");

  if (ML_SERVICE_TYPE_SERVER_PIPELINE == mls->type) {
    MachinelearningServicePipeline *mlsp;
    _ml_service_server_s *server = (_ml_service_server_s *) mls->priv;

    mlsp = _get_proxy_new_for_bus_sync ();
    if (!mlsp) {
      _ml_error_report ("Failed to get dbus proxy.");
      ret = ML_ERROR_INVALID_PARAMETER;
      goto exit;
    }

    machinelearning_service_pipeline_call_destroy_pipeline_sync (mlsp,
        server->id, &ret, NULL, NULL);

    g_object_unref (mlsp);

    g_free (server->service_name);
    g_free (server);
  } else if (ML_SERVICE_TYPE_CLIENT_QUERY == mls->type) {
    _ml_service_query_s *query = (_ml_service_query_s *) mls->priv;
    ml_tensors_data_h data_h;

    if (ml_pipeline_src_release_handle (query->src_h))
      _ml_error_report ("Failed to release src handle");

    if (ml_pipeline_sink_unregister (query->sink_h))
      _ml_error_report ("Failed to unregister sink handle");

    if (ml_pipeline_destroy (query->pipe_h))
      _ml_error_report ("Failed to destroy pipeline");

    while ((data_h = g_async_queue_try_pop (query->out_data_queue))) {
      ml_tensors_data_destroy (data_h);
    }

    g_async_queue_unref (query->out_data_queue);
  } else {
    _ml_error_report ("Invalid type of ml_service_h.");
    ret = ML_ERROR_INVALID_PARAMETER;
    goto exit;
  }

exit:
  g_free (mls);

  return ret;
}
