/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-agent-client.c
 * @date 20 Jul 2022
 * @brief agent (dbus) implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <gio/gio.h>

#include "ml-api-internal.h"
#include "ml-api-service.h"
#include "ml-api-service-private.h"

/**
 * @brief Set the pipeline description with a given name.
 */
int
ml_service_set_pipeline (const char *name, const char *pipeline_desc)
{
  int ret = ML_ERROR_NONE;
  MachinelearningServicePipeline *mlsp;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string.");
  }

  if (!pipeline_desc) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'pipeline_desc' is NULL. It should be a valid string.");
  }

  mlsp = _get_mlsp_proxy_new_for_bus_sync ();
  if (!mlsp) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  result = machinelearning_service_pipeline_call_set_pipeline_sync (mlsp, name,
      pipeline_desc, &ret, NULL, &err);

  g_object_unref (mlsp);

  if (!result) {
    _ml_error_report ("Failed to invoke the method set_pipeline (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Get the pipeline description with a given name.
 */
int
ml_service_get_pipeline (const char *name, char **pipeline_desc)
{
  int ret = ML_ERROR_NONE;
  MachinelearningServicePipeline *mlsp;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL, It should be a valid string");
  }

  if (!pipeline_desc) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter 'pipeline_desc'. It should be a valid char**");
  }

  mlsp = _get_mlsp_proxy_new_for_bus_sync ();
  if (!mlsp) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  result = machinelearning_service_pipeline_call_get_pipeline_sync (mlsp, name,
      &ret, pipeline_desc, NULL, &err);

  g_object_unref (mlsp);

  if (!result) {
    _ml_error_report ("Failed to invoke the method get_pipeline (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Delete the pipeline description with a given name.
 */
int
ml_service_delete_pipeline (const char *name)
{
  int ret = ML_ERROR_NONE;
  MachinelearningServicePipeline *mlsp;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL, It should be a valid string");
  }

  mlsp = _get_mlsp_proxy_new_for_bus_sync ();
  if (!mlsp) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  result = machinelearning_service_pipeline_call_delete_pipeline_sync (mlsp,
      name, &ret, NULL, &err);

  g_object_unref (mlsp);

  if (!result) {
    _ml_error_report ("Failed to invoke the method delete_pipeline (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Launch the pipeline of given service.
 */
int
ml_service_launch_pipeline (const char *name, ml_service_h * h)
{
  int ret = ML_ERROR_NONE;
  ml_service_s *mls;
  _ml_service_server_s *server;
  gint64 out_id;
  MachinelearningServicePipeline *mlsp;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!h)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");

  mlsp = _get_mlsp_proxy_new_for_bus_sync ();
  if (!mlsp) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  result = machinelearning_service_pipeline_call_launch_pipeline_sync (mlsp,
      name, &ret, &out_id, NULL, &err);

  g_object_unref (mlsp);

  if (!result) {
    _ml_error_report ("Failed to invoke the method launch_pipeline (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  if (ML_ERROR_NONE != ret) {
    _ml_error_report_return (ret,
        "Failed to launch pipeline, please check its integrity.");
  }

  mls = g_new0 (ml_service_s, 1);
  server = g_new0 (_ml_service_server_s, 1);
  if (server == NULL || mls == NULL) {
    g_free (mls);
    g_free (server);
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service_server. Out of memory?");
  }

  server->id = out_id;
  server->service_name = g_strdup (name);

  mls->type = ML_SERVICE_TYPE_SERVER_PIPELINE;
  mls->priv = server;
  *h = mls;

  return ML_ERROR_NONE;
}

/**
 * @brief Start the pipeline of given ml_service_h
 */
int
ml_service_start_pipeline (ml_service_h h)
{
  int ret = ML_ERROR_NONE;
  ml_service_s *mls = (ml_service_s *) h;
  _ml_service_server_s *server;
  MachinelearningServicePipeline *mlsp;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!h)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");

  mlsp = _get_mlsp_proxy_new_for_bus_sync ();
  if (!mlsp) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  server = (_ml_service_server_s *) mls->priv;
  result = machinelearning_service_pipeline_call_start_pipeline_sync (mlsp,
      server->id, &ret, NULL, &err);

  g_object_unref (mlsp);

  if (!result) {
    _ml_error_report ("Failed to invoke the method start_pipeline (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Stop the pipeline of given ml_service_h
 */
int
ml_service_stop_pipeline (ml_service_h h)
{
  int ret = ML_ERROR_NONE;
  ml_service_s *mls = (ml_service_s *) h;
  _ml_service_server_s *server;
  MachinelearningServicePipeline *mlsp;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!h)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");

  mlsp = _get_mlsp_proxy_new_for_bus_sync ();
  if (!mlsp) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  server = (_ml_service_server_s *) mls->priv;
  result = machinelearning_service_pipeline_call_stop_pipeline_sync (mlsp,
      server->id, &ret, NULL, &err);

  g_object_unref (mlsp);

  if (!result) {
    _ml_error_report ("Failed to invoke the method stop_pipeline (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Return state of given ml_service_h
 */
int
ml_service_get_pipeline_state (ml_service_h h, ml_pipeline_state_e * state)
{
  int ret = ML_ERROR_NONE;
  gint _state = ML_PIPELINE_STATE_UNKNOWN;
  ml_service_s *mls = (ml_service_s *) h;
  _ml_service_server_s *server;
  MachinelearningServicePipeline *mlsp;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!h)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");

  if (!state)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'state' is NULL. It should be a valid ml_pipeline_state_e pointer");

  mlsp = _get_mlsp_proxy_new_for_bus_sync ();
  if (!mlsp) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  server = (_ml_service_server_s *) mls->priv;
  result = machinelearning_service_pipeline_call_get_state_sync (mlsp,
      server->id, &ret, &_state, NULL, &err);

  *state = (ml_pipeline_state_e) _state;

  g_object_unref (mlsp);

  if (!result) {
    _ml_error_report ("Failed to invoke the method get_state (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief TBU
 */
int
ml_service_model_register (const char *key, const char *model_path,
    unsigned int *version)
{
  int ret = ML_ERROR_NONE;
  MachinelearningServiceModel *mlsm;
  GError *err = NULL;
  gboolean result;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!key)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'key' is NULL. It should be a valid string");

  if (!model_path)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'model_path' is NULL. It should be a valid string");

  if (!version)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'version' is NULL. It should be a valid unsigned int pointer");

  mlsm = _get_mlsm_proxy_new_for_bus_sync ();
  if (!mlsm) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Failed to get dbus proxy.");
  }

  result = machinelearning_service_model_call_register_sync (mlsm,
      key, model_path, version, &ret, NULL, &err);

  g_object_unref (mlsm);

  if (!result) {
    _ml_error_report ("Failed to invoke the method register (%s).",
        err ? err->message : "Unknown error");
    ret = ML_ERROR_IO_ERROR;
  }
  g_clear_error (&err);

  return ret;
}
