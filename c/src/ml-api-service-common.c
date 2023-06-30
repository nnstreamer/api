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
#include "ml-agent-interface.h"

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
    _ml_service_server_s *server = (_ml_service_server_s *) mls->priv;
    GError *err = NULL;

    ret = ml_agent_pipeline_destroy (server->id, &err);
    if (ret < 0) {
      _ml_error_report ("Failed to invoke the method destroy_pipeline (%s).",
          err ? err->message : "Unknown error");
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
  } else if (ML_SERVICE_TYPE_REMOTE == mls->type) {
    _ml_remote_service_s *mlrs = (_ml_remote_service_s *) mls->priv;
    nns_edge_release_handle (mlrs->edge_h);
    /** Wait some time until release the edge handle. */
    g_usleep (1000000);
    g_free (mlrs);
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Invalid type of ml_service_h.");
  }

  g_free (mls);
  return ML_ERROR_NONE;
}
