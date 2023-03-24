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
_get_mlsp_proxy_new_for_bus_sync (void)
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
 * @brief Internal function to get proxy of the model d-bus interface
 */
MachinelearningServiceModel *
_get_mlsm_proxy_new_for_bus_sync (void)
{
  MachinelearningServiceModel *mlsm;

  /** @todo deal with GError */
  mlsm = machinelearning_service_model_proxy_new_for_bus_sync
      (G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
      "org.tizen.machinelearning.service",
      "/Org/Tizen/MachineLearning/Service/Model", NULL, NULL);

  if (mlsm)
    return mlsm;

  /** Try with session type */
  mlsm = machinelearning_service_model_proxy_new_for_bus_sync
      (G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE,
      "org.tizen.machinelearning.service",
      "/Org/Tizen/MachineLearning/Service/Model", NULL, NULL);

  return mlsm;
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
    GError *err = NULL;
    gboolean result;

    mlsp = _get_mlsp_proxy_new_for_bus_sync ();
    if (!mlsp) {
      _ml_error_report_return (ML_ERROR_IO_ERROR, "Failed to get dbus proxy.");
    }

    result = machinelearning_service_pipeline_call_destroy_pipeline_sync (mlsp,
        server->id, &ret, NULL, &err);

    g_object_unref (mlsp);

    if (!result) {
      _ml_error_report ("Failed to invoke the method destroy_pipeline (%s).",
          err ? err->message : "Unknown error");
      ret = ML_ERROR_IO_ERROR;
    }
    g_clear_error (&err);

    if (ML_ERROR_NONE != ret)
      _ml_error_report_return (ret,
          "The data of given handle is corrupted. Please check it.");

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
    g_free (query);
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Invalid type of ml_service_h.");
  }

  g_free (mls);
  return ML_ERROR_NONE;
}
