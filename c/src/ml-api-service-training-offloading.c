/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-training-offloading.c
 * @date 5 Apr 2024
 * @brief ML training offloading service of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/api
 * @author Hyunil Park <hyunil46.park@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include <nnstreamer-edge.h>

#include "ml-api-internal.h"
#include "ml-api-service.h"
#include "ml-api-service-training-offloading.h"

/** It(@~~@) will be replaced with the path set by the app.*/
#define APP_RW_PATH "@APP_RW_PATH@"
#define REMOTE_APP_RW_PATH "@REMOTE_APP_RW_PATH@"
/** combined with trained model file name set in conf */
#define TRAINED_MODEL_FILE "@TRAINED_MODEL_FILE@"

/** default receive time limit (second) */
#define DEFAULT_TIME_LIMIT 10

/**
 * @brief Internal enumeration for ml-service training offloading types.
 */
typedef enum
{
  ML_TRAINING_OFFLOADING_TYPE_UNKNOWN = 0,
  ML_TRAINING_OFFLOADING_TYPE_SENDER,
  ML_TRAINING_OFFLOADING_TYPE_RECEIVER,

  ML_TRAINING_OFFLOADING_TYPE_MAX,
} ml_training_offloaing_type_e;

/**
 * @brief Internal enumeration for the node type in pipeline.
 */
typedef enum
{
  ML_TRAINING_OFFLOADING_NODE_TYPE_UNKNOWN = 0,
  ML_TRAINING_OFFLOADING_NODE_TYPE_OUTPUT,
  ML_TRAINING_OFFLOADING_NODE_TYPE_TRAINING,

  ML_TRAINING_OFFLOADING_NODE_TYPE_MAX
} ml_training_offloading_node_type_e;

/**
 * @brief Internal structure of the node info in pipeline.
 */
typedef struct
{
  gchar *name;
  ml_training_offloading_node_type_e type;
  void *handle;
  void *mls;
} ml_training_offloading_node_info_s;

/**
 * @brief Internal structure for ml-service training offloading handle.
 */
typedef struct
{
  ml_training_offloaing_type_e type;
  ml_pipeline_h pipeline_h;

  gchar *receiver_pipe_json_str;   /** @TRAINED_MODEL_FILE@ and @REMOTE_APP_RW_PATH@ in the receiver pipeline is converted to model_config_path, model_path, and data_path. */
  gchar *receiver_pipe;
  gchar *sender_pipe;
  gchar *trained_model_path;    /* reply to remote sender */
  gchar *path;                  /* Readable and writable path set by the app */

  gboolean is_received;
  gint time_limit;              /* second, For receiving the data necessary for training */
  GMutex received_lock;
  GCond received_cond;
  GThread *received_thread;

  GHashTable *transfer_data_table;
  GHashTable *node_table;
} ml_training_services_s;

/**
 * @brief Internal function to invoke ml-service event for new data.
 */
static int
_invoke_event_new_data (ml_service_s * mls, const char *name,
    const ml_tensors_data_h data)
{
  int status = ML_ERROR_NONE;
  ml_service_event_cb_info_s cb_info = { 0 };
  ml_information_h info = NULL;

  g_return_val_if_fail (mls != NULL, ML_ERROR_INVALID_PARAMETER);
  g_return_val_if_fail (data != NULL, ML_ERROR_INVALID_PARAMETER);
  g_return_val_if_fail (name != NULL, ML_ERROR_INVALID_PARAMETER);

  _ml_service_get_event_cb_info (mls, &cb_info);

  if (cb_info.cb) {
    /* Create information handle for ml-service event. */
    status = _ml_information_create (&info);
    if (status != ML_ERROR_NONE)
      goto done;

    status = _ml_information_set (info, "name", (void *) name, NULL);
    if (status != ML_ERROR_NONE)
      goto done;

    status = _ml_information_set (info, "data", (void *) data, NULL);
    if (status != ML_ERROR_NONE)
      goto done;

    cb_info.cb (ML_SERVICE_EVENT_NEW_DATA, info, cb_info.pdata);
  }

done:
  if (info) {
    ml_information_destroy (info);
  }

  if (status != ML_ERROR_NONE) {
    _ml_error_report ("Failed to invoke 'new data' event.");
  }

  return status;
}

/**
 * @brief Internal callback for sink node in pipeline description.
 */
static void
_pipeline_sink_cb (const ml_tensors_data_h data,
    const ml_tensors_info_h info, void *user_data)
{
  ml_service_s *mls = NULL;
  ml_training_offloading_node_info_s *node_info = NULL;

  g_return_if_fail (data != NULL);
  g_return_if_fail (info != NULL);

  node_info = (ml_training_offloading_node_info_s *) user_data;
  g_return_if_fail (node_info != NULL);

  mls = (ml_service_s *) node_info->mls;
  g_return_if_fail (mls != NULL);

  _invoke_event_new_data (mls, node_info->name, data);
}

/**
 * @brief Internal function to release pipeline node info.
 */
static void
_training_offloading_node_info_free (gpointer data)
{
  ml_training_offloading_node_info_s *node_info =
      (ml_training_offloading_node_info_s *) data;

  if (!node_info)
    return;

  g_free (node_info->name);
  g_free (node_info);
}

/**
 * @brief Internal function to create node info in pipeline.
 */
static ml_training_offloading_node_info_s *
_training_offloading_node_info_new (ml_service_s * mls,
    const gchar * name, ml_training_offloading_node_type_e type)
{
  ml_training_offloading_node_info_s *node_info;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  g_return_val_if_fail (name != NULL, NULL);

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (NULL, "The ml service is not training mode.");
  }

  if (g_hash_table_lookup (training_s->node_table, name)) {
    _ml_error_report_return (NULL,
        "Cannot add duplicated node '%s' in ml-service pipeline.", name);
  }

  node_info = g_try_new0 (ml_training_offloading_node_info_s, 1);
  if (!node_info) {
    _ml_error_report_return (NULL,
        "Failed to allocate new memory for node info in ml-service pipeline. Out of memory?");
  }

  node_info->name = g_strdup (name);
  node_info->type = type;
  node_info->mls = mls;

  g_hash_table_insert (training_s->node_table, g_strdup (name), node_info);

  return node_info;
}

/**
 * @brief Internal function to parse configuration file.
 */
static int
_training_offloading_conf_parse_json (ml_service_s * mls, JsonObject * object)
{
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;
  JsonObject *training_obj, *data_obj;
  JsonNode *training_node, *data_node, *pipline_node;
  const gchar *key, *val;
  gchar *transfer_data = NULL;
  GList *list, *iter;

  g_return_val_if_fail (object != NULL, ML_ERROR_INVALID_PARAMETER);

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  val = json_object_get_string_member (object, "node-type");

  if (g_ascii_strcasecmp (val, "sender") == 0) {
    training_s->type = ML_TRAINING_OFFLOADING_TYPE_SENDER;
  } else if (g_ascii_strcasecmp (val, "receiver") == 0) {
    training_s->type = ML_TRAINING_OFFLOADING_TYPE_RECEIVER;
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The given param, \"node-type\" is invalid.");
  }

  training_node = json_object_get_member (object, "training");
  training_obj = json_node_get_object (training_node);

  if (json_object_has_member (training_obj, "time-limit")) {
    training_s->time_limit =
        json_object_get_int_member (training_obj, "time-limit");
  } else {
    _ml_logw
        ("The default time-limit(10 sec) is set because `time-limit` is not set.");
  }

  val = json_object_get_string_member (training_obj, "sender-pipeline");
  training_s->sender_pipe = g_strdup (val);

  if (!json_object_has_member (training_obj, "transfer-data")) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The given param, \"transfer-data\" is invalid.");
  }

  data_node = json_object_get_member (training_obj, "transfer-data");
  data_obj = json_node_get_object (data_node);
  list = json_object_get_members (data_obj);
  if (!list) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to get transfer data table");
  }

  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    key = iter->data;

    if (!STR_IS_VALID (key)) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The parameter, 'key' is invalid. It should be a valid string.");
    }

    val = json_object_get_string_member (data_obj, key);

    if (STR_IS_VALID (val)) {
      transfer_data = g_strdup (val);
    } else {
      /* pipeline is a JSON string */
      pipline_node = json_object_get_member (data_obj, key);
      transfer_data = json_to_string (pipline_node, TRUE);

      if (!g_strstr_len (transfer_data, -1, "pipeline")) {
        g_free (transfer_data);
        _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
            "The parameter, 'val' is invalid. It should be a valid string.");
      }
    }

    g_hash_table_insert (training_s->transfer_data_table, g_strdup (key),
        transfer_data);
  }
  g_list_free (list);

  /* Since we are only sending the trained model now, there is only 1 item in the list. */
  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER)
    training_s->trained_model_path = g_strdup (transfer_data);

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to parse the node info in pipeline.
 */
static int
_training_offloading_conf_parse_pipeline_node (ml_service_s * mls,
    JsonNode * node, ml_training_offloading_node_type_e type)
{
  int ret = ML_ERROR_NONE;
  guint i, n;
  const gchar *name;
  JsonArray *array = NULL;
  JsonObject *node_object;
  ml_training_offloading_node_info_s *node_info = NULL;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  g_return_val_if_fail (node != NULL, ML_ERROR_INVALID_PARAMETER);

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  n = 1;
  if (JSON_NODE_HOLDS_ARRAY (node)) {
    array = json_node_get_array (node);
    n = json_array_get_length (array);
  }

  for (i = 0; i < n; i++) {
    if (array)
      node_object = json_array_get_object_element (array, i);
    else
      node_object = json_node_get_object (node);

    if (!json_object_has_member (node_object, "name")) {
      _ml_error_report_return (ret,
          "Failed to parse configuration file, cannot get the name for pipeline node.");
    }

    name = json_object_get_string_member (node_object, "name");

    node_info = _training_offloading_node_info_new (mls, name, type);
    if (!node_info) {
      _ml_error_report_return_continue (ML_ERROR_INVALID_PARAMETER,
          "Failed to parse configuration file, cannot add new node information.");
    }

    switch (type) {
      case ML_TRAINING_OFFLOADING_NODE_TYPE_TRAINING:
        ret = ml_pipeline_element_get_handle (training_s->pipeline_h, name,
            &node_info->handle);
        break;
      case ML_TRAINING_OFFLOADING_NODE_TYPE_OUTPUT:
        ret = ml_pipeline_sink_register (training_s->pipeline_h, name,
            _pipeline_sink_cb, node_info, &node_info->handle);
        break;
      default:
        ret = ML_ERROR_INVALID_PARAMETER;
        break;
    }

    if (ret != ML_ERROR_NONE) {
      _ml_error_report_return (ret,
          "Failed to parse configuration file, cannot get the handle for pipeline node.");
    }
  }

  return ret;
}

/**
 * @brief register sink callback
 */
static int
_training_offloading_conf_parse_pipeline (ml_service_s * mls, JsonObject * pipe)
{
  int ret = ML_ERROR_NONE;
  JsonNode *node;

  g_return_val_if_fail (mls != NULL, ML_ERROR_INVALID_PARAMETER);
  g_return_val_if_fail (pipe != NULL, ML_ERROR_INVALID_PARAMETER);

  if (json_object_has_member (pipe, "output_node")) {
    node = json_object_get_member (pipe, "output_node");
    ret = _training_offloading_conf_parse_pipeline_node (mls, node,
        ML_TRAINING_OFFLOADING_NODE_TYPE_OUTPUT);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report_return (ret,
          "Failed to parse configuration file, cannot get the input node.");
    }
  }

  if (json_object_has_member (pipe, "training_node")) {
    node = json_object_get_member (pipe, "training_node");
    ret = _training_offloading_conf_parse_pipeline_node (mls, node,
        ML_TRAINING_OFFLOADING_NODE_TYPE_TRAINING);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report_return (ret,
          "Failed to parse configuration file, cannot get the training node.");
    }
  }

  return ret;
}

/**
 * @brief Internal function to create ml-service training offloading handle.
 */
static int
_training_offloading_create (ml_service_s * mls)
{
  ml_training_services_s *training_s = NULL;

  g_return_val_if_fail (mls != NULL, ML_ERROR_INVALID_PARAMETER);

  training_s = g_try_new0 (ml_training_services_s, 1);
  if (training_s == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service handle's private data. Out of memory?");
  }

  g_cond_init (&training_s->received_cond);
  g_mutex_init (&training_s->received_lock);

  training_s->type = ML_TRAINING_OFFLOADING_TYPE_UNKNOWN;
  training_s->time_limit = DEFAULT_TIME_LIMIT;

  ml_service_offloading_set_mode (mls,
      ML_SERVICE_OFFLOADING_MODE_TRAINING, training_s);

  training_s->transfer_data_table =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (!training_s->transfer_data_table) {
    ml_service_training_offloading_destroy (mls);
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the data table. Out of memory?");
  }

  training_s->node_table =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
      _training_offloading_node_info_free);
  if (!training_s->node_table) {
    ml_service_training_offloading_destroy (mls);
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the node table. Out of memory?");
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to create ml-service training offloading handle.
 */
int
ml_service_training_offloading_create (ml_service_s * mls,
    JsonObject * offloading)
{
  int ret = ML_ERROR_NONE;

  g_return_val_if_fail (mls != NULL, ML_ERROR_INVALID_PARAMETER);
  g_return_val_if_fail (offloading != NULL, ML_ERROR_INVALID_PARAMETER);

  ret = _training_offloading_create (mls);
  if (ret != ML_ERROR_NONE) {
    _ml_error_report_return_continue (ret,
        "Failed to create ml-service for training offloading.");
  }

  ret = _training_offloading_conf_parse_json (mls, offloading);
  if (ret != ML_ERROR_NONE) {
    ml_service_training_offloading_destroy (mls);
    _ml_error_report_return (ret,
        "Failed to parse the configuration file for training offloading.");
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Request training offloading
 */
static int
_training_offloading_request (ml_service_s * mls,
    const gchar * service_name, const gchar * data, gsize len)
{
  int ret = ML_ERROR_NONE;
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };

  g_return_val_if_fail (mls != NULL, ML_ERROR_INVALID_PARAMETER);
  g_return_val_if_fail (service_name != NULL, ML_ERROR_INVALID_PARAMETER);
  g_return_val_if_fail (data != NULL, ML_ERROR_INVALID_PARAMETER);
  g_return_val_if_fail (len > 0, ML_ERROR_INVALID_PARAMETER);

  ml_tensors_info_create (&in_info);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = len;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  ml_tensors_data_create (in_info, &input);
  ml_tensors_data_set_tensor_data (input, 0, data, len);

  ret = ml_service_offloading_request (mls, service_name, input);
  if (ret != ML_ERROR_NONE) {
    _ml_error_report ("Failed to request service '%s'.)", service_name);
  }

  ml_tensors_info_destroy (in_info);
  ml_tensors_data_destroy (input);

  return ret;
}

/**
* @brief Request all services to ml-service offloading.
*/
static int
_training_offloading_services_request (ml_service_s * mls)
{
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;
  int ret = ML_ERROR_NONE;
  GList *list, *iter;
  gchar *transfer_data = NULL, *service_name = NULL;
  gchar *contents = NULL, *pipeline = NULL;
  guint changed;
  gsize len;

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  _ml_logd ("path set by app:%s ", training_s->path);

  list = g_hash_table_get_keys (training_s->transfer_data_table);
  if (!list) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to get transfer data table");
  }

  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    const gchar *name = iter->data;

    transfer_data =
        g_strdup (g_hash_table_lookup (training_s->transfer_data_table, name));

    if (g_strstr_len (transfer_data, -1, APP_RW_PATH)) {
      transfer_data = _ml_replace_string (transfer_data, APP_RW_PATH,
          training_s->path, NULL, &changed);

      _ml_logd ("transfer_data:%s", transfer_data);

      if (!g_file_get_contents (transfer_data, &contents, &len, NULL)) {
        _ml_error_report ("Failed to read file:%s", transfer_data);
        goto error;
      }

      ret = _training_offloading_request (mls, name, contents, len);
      if (ret != ML_ERROR_NONE) {
        _ml_error_report ("Failed to request service '%s'.", name);
        goto error;
      }

      g_free (transfer_data);
      g_free (contents);
      transfer_data = NULL;
      contents = NULL;
    } else if (g_strstr_len (transfer_data, -1, "pipeline")) {
      service_name = g_strdup (iter->data);
      pipeline = g_strdup (transfer_data);
      transfer_data = NULL;
    }
  }

  if (pipeline) {
    /**
     * The remote sender sends the last in the pipeline.
     * When the pipeline arrives, the remote receiver determines that the sender has sent all the necessary files specified in the pipeline.
     * pipeline description must be sent last.
     */
    _ml_logd
        ("In case of pipeline, @REMOTE_APP_RW_PATH@ will be replaced at the remote receiver.\n transfer_data:pipeline(%s),",
        pipeline);
    ret = _training_offloading_request (mls, service_name, pipeline,
        strlen (pipeline) + 1);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report ("Failed to request service(%s)", service_name);
    }
  }

error:
  g_free (service_name);
  g_free (transfer_data);
  g_free (contents);
  g_list_free (list);

  return ret;
}

/**
 * @brief Thread for checking receive data.
*/
static gpointer
_check_received_data_thread (gpointer data)
{
  ml_training_services_s *training_s = (ml_training_services_s *) data;
  int usec = training_s->time_limit * 1000000;

  g_return_val_if_fail (training_s != NULL, NULL);

  while (usec > 0) {
    g_usleep (100000);
    if (training_s->receiver_pipe_json_str != NULL) {
      _ml_logd
          ("Lock to receive pipeline JSON string required for model training.");
      g_mutex_lock (&training_s->received_lock);
      training_s->is_received = TRUE;
      _ml_logd ("receive_pipe:%s", training_s->receiver_pipe_json_str);
      _ml_logd
          ("Now pipeline has arrived, The remote sender send the pipeline last, so probably received all the data."
          "If there are no files required for the pipeline, a runtime error occurs.");
      g_cond_signal (&training_s->received_cond);
      g_mutex_unlock (&training_s->received_lock);
      return NULL;
    }
    usec -= 100000;
  }

  _ml_loge ("Required data is null, receive_pipe:%s",
      training_s->receiver_pipe_json_str);
  g_mutex_lock (&training_s->received_lock);
  training_s->is_received = FALSE;
  g_cond_signal (&training_s->received_cond);
  g_mutex_unlock (&training_s->received_lock);
  return NULL;
}

/**
 * @brief Check if all necessary data is received.
*/
static gboolean
_training_offloading_check_received_data (ml_training_services_s * training_s)
{
  gboolean is_received = FALSE;

  g_return_val_if_fail (training_s != NULL, FALSE);

  training_s->received_thread = g_thread_new ("check_received_file",
      _check_received_data_thread, training_s);

  g_mutex_lock (&training_s->received_lock);

  while (!training_s->is_received) {
    _ml_logd ("Wait to receive all data needed for model training.");
    g_cond_wait (&training_s->received_cond, &training_s->received_lock);
    if (training_s->is_received == FALSE)
      break;
  }

  is_received = training_s->is_received;
  g_mutex_unlock (&training_s->received_lock);
  _ml_logd ("unlock, receive all data");

  return is_received;
}

/**
 * @brief replace path.
 */
static void
_training_offloading_replce_pipeline_data_path (ml_service_s * mls)
{
  guint changed = 0;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report ("The ml service is not training mode.");
    return;
  }

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_SENDER) {
    if (training_s->sender_pipe) {
      training_s->sender_pipe =
          _ml_replace_string (training_s->sender_pipe, APP_RW_PATH,
          training_s->path, NULL, &changed);
      _ml_logd ("@APP_RW_PATH@ is replaced, sender_pipe:%s",
          training_s->sender_pipe);
    }
  } else {
    if (training_s->receiver_pipe_json_str) {
      training_s->trained_model_path =
          _ml_replace_string (training_s->trained_model_path, APP_RW_PATH,
          training_s->path, NULL, &changed);
      training_s->receiver_pipe_json_str =
          _ml_replace_string (training_s->receiver_pipe_json_str,
          REMOTE_APP_RW_PATH, training_s->path, NULL, &changed);
      training_s->receiver_pipe_json_str =
          _ml_replace_string (training_s->receiver_pipe_json_str,
          TRAINED_MODEL_FILE, training_s->trained_model_path, NULL, &changed);
      _ml_logd
          ("@REMOTE_APP_RW_PATH@ and @TRAINED_MODEL_FILE@ are replaced, receiver_pipe JSON string: %s",
          training_s->receiver_pipe_json_str);
    }
  }
}

/**
 * @brief Set path in ml-service training offloading handle.
 */
int
ml_service_training_offloading_set_path (ml_service_s * mls, const gchar * path)
{
  int ret = ML_ERROR_NONE;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  g_return_val_if_fail (path != NULL, ML_ERROR_INVALID_PARAMETER);

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  g_free (training_s->path);
  training_s->path = g_strdup (path);

  return ret;
}

/**
 * @brief Start ml training offloading service.
 */
int
ml_service_training_offloading_start (ml_service_s * mls)
{
  int ret = ML_ERROR_NONE;
  JsonNode *pipeline_node;
  JsonObject *pipeline_obj;
  JsonObject *pipe;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_SENDER) {
    ret = _training_offloading_services_request (mls);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report_return (ret, "Failed to request service.");
    }

    _training_offloading_replce_pipeline_data_path (mls);

    ret = ml_pipeline_construct (training_s->sender_pipe, NULL, NULL,
        &training_s->pipeline_h);
    if (ML_ERROR_NONE != ret) {
      _ml_error_report_return (ret, "Failed to construct pipeline.");
    }

    ret = ml_pipeline_start (training_s->pipeline_h);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report_return (ret, "Failed to start ml pipeline.");
    }
  } else if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER) {
    /* checking if all required files are received */
    if (!_training_offloading_check_received_data (training_s)) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to receive the required data");
    }

    _training_offloading_replce_pipeline_data_path (mls);

    pipeline_node = json_from_string (training_s->receiver_pipe_json_str, NULL);
    if (!pipeline_node) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to parse the json string, %s.",
          training_s->receiver_pipe_json_str);
    }

    pipeline_obj = json_node_get_object (pipeline_node);
    if (!pipeline_obj) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to get the json object from the json node.");
    }

    if (!json_object_has_member (pipeline_obj, "pipeline")) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to parse configuration file, cannot get the pipeline JSON object.");
    }

    pipe = json_object_get_object_member (pipeline_obj, "pipeline");

    if (json_object_has_member (pipe, "description")) {
      training_s->receiver_pipe =
          g_strdup (json_object_get_string_member (pipe, "description"));
    } else {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to parse configuration file, cannot get the pipeline description.");
    }

    ret = ml_pipeline_construct (training_s->receiver_pipe, NULL, NULL,
        &training_s->pipeline_h);
    if (ML_ERROR_NONE != ret) {
      _ml_error_report_return (ret, "Failed to construct pipeline.");
    }

    ret = _training_offloading_conf_parse_pipeline (mls, pipe);
    if (ret != ML_ERROR_NONE) {
      return ret;
    }

    ret = ml_pipeline_start (training_s->pipeline_h);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report_return (ret, "Failed to start ml pipeline.");
    }
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The node type information in JSON is incorrect.");
  }

  return ret;
}

/**
 * @brief ready to complete current epoch for model training
 */
static int
_training_offloading_ready_to_complete (ml_service_s * mls)
{
  int ret = ML_ERROR_NONE;
  int loop = 120;
  GList *list, *iter;
  ml_training_services_s *training_s = NULL;
  ml_training_offloading_node_info_s *node_info = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  list = g_hash_table_get_keys (training_s->node_table);
  if (!list) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to get transfer data table");
  }

  /* For now, let's set values for all tensor_trainers */
  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    const gchar *name = iter->data;

    node_info = g_hash_table_lookup (training_s->node_table, name);
    if (node_info &&
        node_info->type == ML_TRAINING_OFFLOADING_NODE_TYPE_TRAINING) {
      _ml_logd ("Set `ready to complete` to tensor_trainer node name:%s", name);

      ml_pipeline_element_set_property_bool (node_info->handle,
          "ready-to-complete", TRUE);
    }
  }

  /** @todo FIXME : Let's make up for it later. */
  while (loop--) {
    if (g_file_test (training_s->trained_model_path, G_FILE_TEST_EXISTS))
      break;
    g_usleep (300000);
  }

  return ret;
}

/**
 * @brief Stop ml training offloading service.
 */
int
ml_service_training_offloading_stop (ml_service_s * mls)
{
  int ret = ML_ERROR_NONE;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER) {
    if (!g_file_test (training_s->trained_model_path, G_FILE_TEST_EXISTS)) {
      _training_offloading_ready_to_complete (mls);
    }
  }

  if (!training_s->pipeline_h) {
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Pipeline is not constructed.");
  }

  ret = ml_pipeline_stop (training_s->pipeline_h);
  if (ML_ERROR_NONE != ret) {
    _ml_error_report_return (ret, "Failed to stop pipeline.");
  }

  return ret;
}

/**
 * @brief Save receiver pipeline description.
 */
void
ml_service_training_offloading_process_received_data (ml_service_s * mls,
    void *data_h, const gchar * dir_path, const gchar * data, int service_type)
{
  g_autofree gchar *name = NULL;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  g_return_if_fail (data_h != NULL);
  g_return_if_fail (dir_path != NULL);
  g_return_if_fail (data != NULL);

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report ("The ml service is not training mode.");
    return;
  }

  _ml_logd ("Received data, service_type:%d", service_type);

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER) {
    if (service_type == ML_SERVICE_OFFLOADING_TYPE_PIPELINE_RAW) {
      training_s->receiver_pipe_json_str = g_strdup (data);
      _ml_logd ("Received JSON string pipeline:%s",
          training_s->receiver_pipe_json_str);
    }
  } else {
    /* receive trained model from remote */
    if (service_type == ML_SERVICE_OFFLOADING_TYPE_REPLY) {
      nns_edge_data_get_info (data_h, "name", &name);
      training_s->trained_model_path =
          g_build_path (G_DIR_SEPARATOR_S, dir_path, name, NULL);
      _ml_logd ("Reply: name:%s, received trained_model:%s", name,
          training_s->trained_model_path);
    }
  }
}

/**
 * @brief Send trained model
 */
static void
_training_offloading_send_trained_model (ml_service_s * mls)
{
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;
  GList *list, *iter;
  gchar *contents = NULL;
  gsize len;

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report ("The ml service is not training mode.");
    return;
  }

  if (training_s->trained_model_path == NULL)
    return;

  if (!g_file_get_contents (training_s->trained_model_path, &contents, &len,
          NULL)) {
    _ml_error_report ("Failed to read file:%s", training_s->trained_model_path);
    return;
  }

  list = g_hash_table_get_keys (training_s->transfer_data_table);
  if (!list) {
    _ml_error_report ("Failed to get transfer data table");
  }
  _ml_logd ("Send trained model");
  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    _training_offloading_request (mls, (gchar *) iter->data, contents, len);
  }

  g_list_free (list);
  g_free (contents);
  return;
}

/**
 * @brief Internal function to destroy ml-service training offloading data.
 */
int
ml_service_training_offloading_destroy (ml_service_s * mls)
{
  int ret = ML_ERROR_NONE;
  ml_training_services_s *training_s = NULL;
  ml_service_offloading_mode_e mode = ML_SERVICE_OFFLOADING_MODE_NONE;

  ml_service_offloading_get_mode (mls, &mode, (void **) &training_s);
  if (mode != ML_SERVICE_OFFLOADING_MODE_TRAINING || training_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The ml service is not training mode.");
  }

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER) {
    /* reply to remote sender */
    _training_offloading_send_trained_model (mls);
  }

  g_cond_clear (&training_s->received_cond);
  g_mutex_clear (&training_s->received_lock);

  if (training_s->received_thread) {
    g_thread_join (training_s->received_thread);
    training_s->received_thread = NULL;
  }

  if (training_s->transfer_data_table) {
    g_hash_table_destroy (training_s->transfer_data_table);
    training_s->transfer_data_table = NULL;
  }

  if (training_s->node_table) {
    g_hash_table_destroy (training_s->node_table);
    training_s->node_table = NULL;
  }

  if (training_s->pipeline_h) {
    ret = ml_pipeline_destroy (training_s->pipeline_h);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report ("Failed to destroy ml pipeline, clear handle anyway.");
    }

    training_s->pipeline_h = NULL;
  }

  g_free (training_s->path);
  training_s->path = NULL;

  g_free (training_s->trained_model_path);
  training_s->trained_model_path = NULL;

  g_free (training_s->receiver_pipe_json_str);
  training_s->receiver_pipe_json_str = NULL;

  g_free (training_s->receiver_pipe);
  training_s->receiver_pipe = NULL;

  g_free (training_s->sender_pipe);
  training_s->sender_pipe = NULL;

  g_free (training_s);

  ml_service_offloading_set_mode (mls, ML_SERVICE_OFFLOADING_MODE_NONE, NULL);
  return ret;
}
