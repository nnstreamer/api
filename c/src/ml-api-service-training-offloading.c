/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-training-offloading.c
 * @date 30 Apr 2024
 * @brief ML training offloading service of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Hyunil Park <hyunil46.park@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/app/app.h>
#include <string.h>
#include <curl/curl.h>
#include <nnstreamer-edge.h>

#include "ml-api-internal.h"
//#include "ml-api-service.h"
#include "ml-api-service-private.h"
#include "ml-api-service-training-offloading.h"

/**
 * @brief Internal enumeration for ml-service training offloading types.
 */
typedef enum
{
  ML_TRAINING_OFFLOADING_TYPE_UNKNOWN = 0,
  ML_TRAINING_OFFLOADING_TYPE_SENDER,
  ML_TRAINING_OFFLOADING_TYPE_RECEIVER,

  ML_TRAINING_OFFLOADING_TYPE_MAX
} ml_training_offloaing_type_e;

/**
 * @brief Internal structure for ml-service training offloading handle.
 */
typedef struct
{
  ml_training_offloaing_type_e type;
  ml_pipeline_h pipeline;
  gchar *model_config;
  gchar *pretrained_model;
  gchar *trained_model;
  gchar *sender_pipe;
  gchar *receiver_pipe;
  gchar *model_config_path;
  gchar *model_path;
  gchar *data_path;

  JsonObject *json_object;
} ml_training_services_s;


/**
 * @brief Internal function to parse configuration file.
 */
static int
_ml_service_training_offloadin_conf_parse_json (_ml_service_offloading_s *
    offloading_s, JsonObject * object)
{
  ml_training_services_s *training_s = NULL;
  JsonObject *training_obj, *obj;
  JsonNode *training_node, *node;
  const gchar *val;

  training_s = (ml_training_services_s *) offloading_s->priv;

  val = json_object_get_string_member (object, "node-type");
  if (g_ascii_strcasecmp (val, "sender") == 0) {
    training_s->type = ML_TRAINING_OFFLOADING_TYPE_SENDER;
  } else if (g_ascii_strcasecmp (val, "receiver") == 0) {
    training_s->type = ML_TRAINING_OFFLOADING_TYPE_RECEIVER;
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The given param, \"node-type\" is invalid.");
  }
  g_critical ("######## %s : %s: %d  type = %d", __FILE__, __FUNCTION__,
      __LINE__, training_s->type);

  training_node = json_object_get_member (object, "training");
  training_obj = json_node_get_object (training_node);
  g_critical ("######## %s : %s: %d ", __FILE__, __FUNCTION__, __LINE__);

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_SENDER) {
    val = json_object_get_string_member (training_obj, "sender-pipeline");
    training_s->sender_pipe = g_strdup (val);
    g_critical ("######## %s : %s: %d, data_name =%s", __FILE__, __FUNCTION__,
        __LINE__, training_s->sender_pipe);

    if (!json_object_has_member (training_obj, "transfer-data"))
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The given param, \"transfer-data\" is invalid.");
    node = json_object_get_member (training_obj, "transfer-data");
    obj = json_node_get_object (node);
    if (json_object_has_member (obj, "model-config")) {
      val = json_object_get_string_member (obj, "model-config");
      training_s->model_config = g_strdup (val);
      g_critical ("######## %s : %s: %d, data_name =%s", __FILE__, __FUNCTION__,
          __LINE__, training_s->model_config);
    }
    if (json_object_has_member (obj, "pretrained-model")) {
      val = json_object_get_string_member (obj, "pretrained-model");
      training_s->pretrained_model = g_strdup (val);
      g_critical ("######## %s : %s: %d, data_name =%s", __FILE__, __FUNCTION__,
          __LINE__, training_s->pretrained_model);
    }
    if (json_object_has_member (obj, "receiver-pipeline")) {
      val = json_object_get_string_member (obj, "receiver-pipeline");
      training_s->receiver_pipe = g_strdup (val);
      g_critical ("######## %s : %s: %d, data_name =%s", __FILE__, __FUNCTION__,
          __LINE__, training_s->receiver_pipe);
    }
  }

  return ML_ERROR_NONE;
}


/**
 * @brief Internal function to create ml-service training offloading handle.
 */
static int
_ml_service_create_training_offloading (_ml_service_offloading_s * offloading_s)
{
  ml_training_services_s *training_s = NULL;

  training_s = g_try_new0 (ml_training_services_s, 1);
  if (training_s == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service handle's private data. Out of memory?");
  }

  training_s->type = ML_TRAINING_OFFLOADING_TYPE_UNKNOWN;
  offloading_s->priv = training_s;

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to create ml-service training offloading handle.
 */
int
ml_service_training_offloading_create (_ml_service_offloading_s * offloading_s,
    JsonObject * object)
{
  int status;

  if (!offloading_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'remote_s' (_ml_remote_service_s), is NULL.");
  }

  if (!object) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'object' is NULL.");
  }

  status = _ml_service_create_training_offloading (offloading_s);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status,
        "Failed to create the ml-service extension.");
  }
  status =
      _ml_service_training_offloadin_conf_parse_json (offloading_s, object);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status, "Failed to parse the configuration file.");
  }

  return status;
}

/**
 * @brief Set path in ml-service training offloading handle.
 */
int
ml_service_training_offloading_set_path (_ml_service_offloading_s *
    offloading_s, const gchar * name, const gchar * path)
{
  ml_training_services_s *training_s = NULL;

  if (!offloading_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'remote_s' (_ml_service_offloading_s), is NULL.");
  }

  training_s = (ml_training_services_s *) offloading_s->priv;

  g_critical ("######## %s : %s: %d", __FILE__, __FUNCTION__, __LINE__);
  if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The given param, dir path = \"%s\" is invalid or the dir is not found or accessible.",
        path);
  }
  if (g_access (path, W_OK) != 0) {
    _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
        "Write permission denied, path: %s", path);
  }

  if (g_ascii_strcasecmp (name, "model-path") == 0) {
    g_free (training_s->model_path);
    training_s->model_path = g_strdup (path);
    g_critical ("######## %s : %s: %d, model_path =%s", __FILE__, __FUNCTION__,
        __LINE__, training_s->model_path);
  } else if (g_ascii_strcasecmp (name, "config-path") == 0) {
    g_free (training_s->model_config_path);
    training_s->model_config_path = g_strdup (path);
    g_critical ("######## %s : %s: %d, model_config_path =%s", __FILE__,
        __FUNCTION__, __LINE__, training_s->model_config_path);
  } else if (g_ascii_strcasecmp (name, "data-path") == 0) {
    g_free (training_s->data_path);
    training_s->data_path = g_strdup (path);
    g_critical ("######## %s : %s: %d, data_path =%s", __FILE__, __FUNCTION__,
        __LINE__, training_s->data_path);
  } else if (g_ascii_strcasecmp (name, "path") == 0) {
    //status = ml_service_remote_set_path (mls, value);
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The given param, \"%s\" is invalid.", name);
  }

  return ML_ERROR_NONE;
}
