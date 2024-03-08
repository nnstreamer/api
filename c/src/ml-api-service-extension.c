/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file        ml-api-service-extension.c
 * @date        1 September 2023
 * @brief       ML service extension C-API.
 * @see         https://github.com/nnstreamer/api
 * @author      Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug         No known bugs except for NYI items
 */

#include "ml-api-service-extension.h"

/**
 * @brief The time to wait for new input data in message thread, in millisecond.
 */
#define DEFAULT_TIMEOUT 200

/**
 * @brief The max number of input data in message queue (0 for no limit).
 */
#define DEFAULT_MAX_INPUT 5

/**
 * @brief Internal enumeration for ml-service extension types.
 */
typedef enum
{
  ML_EXTENSION_TYPE_UNKNOWN = 0,
  ML_EXTENSION_TYPE_SINGLE = 1,
  ML_EXTENSION_TYPE_PIPELINE = 2,

  ML_EXTENSION_TYPE_MAX
} ml_extension_type_e;

/**
 * @brief Internal enumeration for the node type in pipeline.
 */
typedef enum
{
  ML_EXTENSION_NODE_TYPE_UNKNOWN = 0,
  ML_EXTENSION_NODE_TYPE_INPUT = 1,
  ML_EXTENSION_NODE_TYPE_OUTPUT = 2,

  ML_EXTENSION_NODE_TYPE_MAX
} ml_extension_node_type_e;

/**
 * @brief Internal structure of the node info in pipeline.
 */
typedef struct
{
  gchar *name;
  ml_extension_node_type_e type;
  ml_tensors_info_h info;
  void *handle;
  void *mls;
} ml_extension_node_info_s;

/**
 * @brief Internal structure of the message in ml-service extension handle.
 */
typedef struct
{
  gchar *name;
  ml_tensors_data_h input;
  ml_tensors_data_h output;
} ml_extension_msg_s;

/**
 * @brief Internal structure for ml-service extension handle.
 */
typedef struct
{
  ml_extension_type_e type;
  gboolean running;
  guint timeout; /**< The time to wait for new input data in message thread, in millisecond (see DEFAULT_TIMEOUT). */
  guint max_input; /**< The max number of input data in message queue (see DEFAULT_MAX_INPUT). */
  GThread *msg_thread;
  GAsyncQueue *msg_queue;

  /**
   * Handles for each ml-service extension type.
   * - single : Default. Open model file and prepare invoke. The configuration should include model information.
   * - pipeline : Construct a pipeline from configuration. The configuration should include pipeline description.
   */
  ml_single_h single;

  ml_pipeline_h pipeline;
  GHashTable *node_table;
} ml_extension_s;

/**
 * @brief Internal function to create node info in pipeline.
 */
static ml_extension_node_info_s *
_ml_extension_node_info_new (ml_service_s * mls, const gchar * name,
    ml_extension_node_type_e type)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  ml_extension_node_info_s *node_info;

  if (!STR_IS_VALID (name)) {
    _ml_error_report_return (NULL,
        "Cannot add new node info, invalid node name '%s'.", name);
  }

  if (g_hash_table_lookup (ext->node_table, name)) {
    _ml_error_report_return (NULL,
        "Cannot add duplicated node '%s' in ml-service pipeline.", name);
  }

  node_info = g_try_new0 (ml_extension_node_info_s, 1);
  if (!node_info) {
    _ml_error_report_return (NULL,
        "Failed to allocate new memory for node info in ml-service pipeline. Out of memory?");
  }

  node_info->name = g_strdup (name);
  node_info->type = type;
  node_info->mls = mls;

  g_hash_table_insert (ext->node_table, g_strdup (name), node_info);

  return node_info;
}

/**
 * @brief Internal function to release pipeline node info.
 */
static void
_ml_extension_node_info_free (gpointer data)
{
  ml_extension_node_info_s *node_info = (ml_extension_node_info_s *) data;

  if (!node_info)
    return;

  if (node_info->info)
    ml_tensors_info_destroy (node_info->info);

  g_free (node_info->name);
  g_free (node_info);
}

/**
 * @brief Internal function to get the node info in ml-service extension.
 */
static ml_extension_node_info_s *
_ml_extension_node_info_get (ml_extension_s * ext, const gchar * name)
{
  if (!STR_IS_VALID (name))
    return NULL;

  return g_hash_table_lookup (ext->node_table, name);
}

/**
 * @brief Internal function to invoke ml-service event for new data.
 */
static int
_ml_extension_invoke_event_new_data (ml_service_s * mls, const char *name,
    const ml_tensors_data_h data)
{
  ml_service_event_cb_info_s cb_info = { 0 };
  ml_information_h info = NULL;
  int status = ML_ERROR_NONE;

  if (!mls || !data) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to create ml-service event data, invalid parameter.");
  }

  _ml_service_get_event_cb_info (mls, &cb_info);

  if (cb_info.cb) {
    /* Create information handle for ml-service event. */
    status = _ml_information_create (&info);
    if (status != ML_ERROR_NONE)
      goto done;

    if (name) {
      status = _ml_information_set (info, "name", (void *) name, NULL);
      if (status != ML_ERROR_NONE)
        goto done;
    }

    status = _ml_information_set (info, "data", (void *) data, NULL);
    if (status == ML_ERROR_NONE)
      cb_info.cb (ML_SERVICE_EVENT_NEW_DATA, info, cb_info.pdata);
  }

done:
  if (info)
    ml_information_destroy (info);

  if (status != ML_ERROR_NONE) {
    _ml_error_report ("Failed to invoke 'new data' event.");
  }

  return status;
}

/**
 * @brief Internal callback for sink node in pipeline description.
 */
static void
_ml_extension_pipeline_sink_cb (const ml_tensors_data_h data,
    const ml_tensors_info_h info, void *user_data)
{
  ml_extension_node_info_s *node_info = (ml_extension_node_info_s *) user_data;
  ml_service_s *mls = (ml_service_s *) node_info->mls;

  _ml_extension_invoke_event_new_data (mls, node_info->name, data);
}

/**
 * @brief Internal function to release ml-service extension message.
 */
static void
_ml_extension_msg_free (gpointer data)
{
  ml_extension_msg_s *msg = (ml_extension_msg_s *) data;

  if (!msg)
    return;

  if (msg->input)
    ml_tensors_data_destroy (msg->input);
  if (msg->output)
    ml_tensors_data_destroy (msg->output);

  g_free (msg->name);
  g_free (msg);
}

/**
 * @brief Internal function to process ml-service extension message.
 */
static gpointer
_ml_extension_msg_thread (gpointer data)
{
  ml_service_s *mls = (ml_service_s *) data;
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  int status;

  g_mutex_lock (&mls->lock);
  ext->running = TRUE;
  g_cond_signal (&mls->cond);
  g_mutex_unlock (&mls->lock);

  while (ext->running) {
    ml_extension_msg_s *msg;

    msg = g_async_queue_timeout_pop (ext->msg_queue,
        ext->timeout * G_TIME_SPAN_MILLISECOND);

    if (msg) {
      switch (ext->type) {
        case ML_EXTENSION_TYPE_SINGLE:
        {
          status = ml_single_invoke (ext->single, msg->input, &msg->output);

          if (status == ML_ERROR_NONE) {
            _ml_extension_invoke_event_new_data (mls, NULL, msg->output);
          } else {
            _ml_error_report
                ("Failed to invoke the model in ml-service extension thread.");
          }
          break;
        }
        case ML_EXTENSION_TYPE_PIPELINE:
        {
          ml_extension_node_info_s *node_info;

          node_info = _ml_extension_node_info_get (ext, msg->name);

          if (node_info && node_info->type == ML_EXTENSION_NODE_TYPE_INPUT) {
            /* The input data will be released in the pipeline. */
            status = ml_pipeline_src_input_data (node_info->handle, msg->input,
                ML_PIPELINE_BUF_POLICY_AUTO_FREE);
            msg->input = NULL;

            if (status != ML_ERROR_NONE) {
              _ml_error_report
                  ("Failed to push input data into the pipeline in ml-service extension thread.");
            }
          } else {
            _ml_error_report
                ("Failed to push input data into the pipeline, cannot find input node '%s'.",
                msg->name);
          }
          break;
        }
        default:
          /* Unknown ml-service extension type, skip this. */
          break;
      }

      _ml_extension_msg_free (msg);
    }
  }

  return NULL;
}

/**
 * @brief Wrapper to release tensors-info handle.
 */
static void
_ml_extension_destroy_tensors_info (void *data)
{
  ml_tensors_info_h info = (ml_tensors_info_h) data;

  if (info)
    ml_tensors_info_destroy (info);
}

/**
 * @brief Internal function to parse single-shot info from json.
 */
static int
_ml_extension_conf_parse_single (ml_service_s * mls, JsonObject * single)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  ml_option_h option;
  int status;

  status = ml_option_create (&option);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status,
        "Failed to parse configuration file, cannot create ml-option handle.");
  }

  /**
   * 1. "key" : load model info from ml-service agent.
   * 2. "model" : configuration file includes model path.
   */
  if (json_object_has_member (single, "key")) {
    const gchar *key = json_object_get_string_member (single, "key");

    if (STR_IS_VALID (key)) {
      ml_information_h model_info;

      status = ml_service_model_get_activated (key, &model_info);
      if (status == ML_ERROR_NONE) {
        gchar *paths = NULL;

        /** @todo parse desc and other information if necessary. */
        ml_information_get (model_info, "path", (void **) (&paths));
        ml_option_set (option, "models", g_strdup (paths), g_free);

        ml_information_destroy (model_info);
      } else {
        _ml_error_report
            ("Failed to parse configuration file, cannot get the model of '%s'.",
            key);
        goto error;
      }
    }
  } else if (json_object_has_member (single, "model")) {
    JsonNode *file_node = json_object_get_member (single, "model");
    gchar *paths = NULL;

    status = _ml_service_conf_parse_string (file_node, ",", &paths);
    if (status != ML_ERROR_NONE) {
      _ml_error_report
          ("Failed to parse configuration file, it should have valid model path.");
      goto error;
    }

    ml_option_set (option, "models", paths, g_free);
  } else {
    status = ML_ERROR_INVALID_PARAMETER;
    _ml_error_report
        ("Failed to parse configuration file, cannot get the model path.");
    goto error;
  }

  if (json_object_has_member (single, "framework")) {
    const gchar *fw = json_object_get_string_member (single, "framework");

    if (STR_IS_VALID (fw))
      ml_option_set (option, "framework_name", g_strdup (fw), g_free);
  }

  if (json_object_has_member (single, "input_info")) {
    JsonNode *info_node = json_object_get_member (single, "input_info");
    ml_tensors_info_h in_info;

    status = _ml_service_conf_parse_tensors_info (info_node, &in_info);
    if (status != ML_ERROR_NONE) {
      _ml_error_report
          ("Failed to parse configuration file, cannot parse input information.");
      goto error;
    }

    ml_option_set (option, "input_info", in_info,
        _ml_extension_destroy_tensors_info);
  }

  if (json_object_has_member (single, "output_info")) {
    JsonNode *info_node = json_object_get_member (single, "output_info");
    ml_tensors_info_h out_info;

    status = _ml_service_conf_parse_tensors_info (info_node, &out_info);
    if (status != ML_ERROR_NONE) {
      _ml_error_report
          ("Failed to parse configuration file, cannot parse output information.");
      goto error;
    }

    ml_option_set (option, "output_info", out_info,
        _ml_extension_destroy_tensors_info);
  }

  if (json_object_has_member (single, "custom")) {
    const gchar *custom = json_object_get_string_member (single, "custom");

    if (STR_IS_VALID (custom))
      ml_option_set (option, "custom", g_strdup (custom), g_free);
  }

error:
  if (status == ML_ERROR_NONE)
    status = ml_single_open_with_option (&ext->single, option);

  ml_option_destroy (option);
  return status;
}

/**
 * @brief Internal function to parse the node info in pipeline.
 */
static int
_ml_extension_conf_parse_pipeline_node (ml_service_s * mls, JsonNode * node,
    ml_extension_node_type_e type)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  JsonArray *array = NULL;
  JsonObject *object;
  guint i, n;
  int status;

  n = 1;
  if (JSON_NODE_HOLDS_ARRAY (node)) {
    array = json_node_get_array (node);
    n = json_array_get_length (array);
  }

  for (i = 0; i < n; i++) {
    const gchar *name = NULL;
    ml_extension_node_info_s *node_info;

    if (array)
      object = json_array_get_object_element (array, i);
    else
      object = json_node_get_object (node);

    if (json_object_has_member (object, "name"))
      name = json_object_get_string_member (object, "name");

    node_info = _ml_extension_node_info_new (mls, name, type);
    if (!node_info) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to parse configuration file, cannot add new node information.");
    }

    if (json_object_has_member (object, "info")) {
      JsonNode *info_node = json_object_get_member (object, "info");

      status = _ml_service_conf_parse_tensors_info (info_node,
          &node_info->info);
      if (status != ML_ERROR_NONE) {
        _ml_error_report_return (status,
            "Failed to parse configuration file, cannot parse the information.");
      }
    } else {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to parse configuration file, cannot find node information.");
    }

    switch (type) {
      case ML_EXTENSION_NODE_TYPE_INPUT:
        status = ml_pipeline_src_get_handle (ext->pipeline, name,
            &node_info->handle);
        break;
      case ML_EXTENSION_NODE_TYPE_OUTPUT:
        status = ml_pipeline_sink_register (ext->pipeline, name,
            _ml_extension_pipeline_sink_cb, node_info, &node_info->handle);
        break;
      default:
        status = ML_ERROR_INVALID_PARAMETER;
        break;
    }

    if (status != ML_ERROR_NONE) {
      _ml_error_report_return (status,
          "Failed to parse configuration file, cannot get the handle for pipeline node.");
    }
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to parse pipeline info from json.
 */
static int
_ml_extension_conf_parse_pipeline (ml_service_s * mls, JsonObject * pipe)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  g_autofree gchar *desc = NULL;
  int status;

  /**
   * 1. "key" : load pipeline from ml-service agent.
   * 2. "description" : configuration file includes pipeline description.
   */
  if (json_object_has_member (pipe, "key")) {
    const gchar *key = json_object_get_string_member (pipe, "key");

    if (STR_IS_VALID (key)) {
      status = ml_service_pipeline_get (key, &desc);
      if (status != ML_ERROR_NONE) {
        _ml_error_report_return (status,
            "Failed to parse configuration file, cannot get the pipeline of '%s'.",
            key);
      }
    }
  } else if (json_object_has_member (pipe, "description")) {
    desc = g_strdup (json_object_get_string_member (pipe, "description"));
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to parse configuration file, cannot get the pipeline description.");
  }

  status = ml_pipeline_construct (desc, NULL, NULL, &ext->pipeline);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status,
        "Failed to parse configuration file, cannot construct the pipeline.");
  }

  if (json_object_has_member (pipe, "input_node")) {
    JsonNode *node = json_object_get_member (pipe, "input_node");

    status = _ml_extension_conf_parse_pipeline_node (mls, node,
        ML_EXTENSION_NODE_TYPE_INPUT);
    if (status != ML_ERROR_NONE) {
      _ml_error_report_return (status,
          "Failed to parse configuration file, cannot get the input node.");
    }
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to parse configuration file, cannot find the input node.");
  }

  if (json_object_has_member (pipe, "output_node")) {
    JsonNode *node = json_object_get_member (pipe, "output_node");

    status = _ml_extension_conf_parse_pipeline_node (mls, node,
        ML_EXTENSION_NODE_TYPE_OUTPUT);
    if (status != ML_ERROR_NONE) {
      _ml_error_report_return (status,
          "Failed to parse configuration file, cannot get the output node.");
    }
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to parse configuration file, cannot find the output node.");
  }

  /* Start pipeline when creating ml-service handle to check pipeline description. */
  status = ml_pipeline_start (ext->pipeline);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status,
        "Failed to parse configuration file, cannot start the pipeline.");
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to parse configuration file.
 */
static int
_ml_extension_conf_parse_json (ml_service_s * mls, JsonObject * object)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  int status;

  if (json_object_has_member (object, "single")) {
    JsonObject *single = json_object_get_object_member (object, "single");

    status = _ml_extension_conf_parse_single (mls, single);
    if (status != ML_ERROR_NONE)
      return status;

    ext->type = ML_EXTENSION_TYPE_SINGLE;
  } else if (json_object_has_member (object, "pipeline")) {
    JsonObject *pipe = json_object_get_object_member (object, "pipeline");

    status = _ml_extension_conf_parse_pipeline (mls, pipe);
    if (status != ML_ERROR_NONE)
      return status;

    ext->type = ML_EXTENSION_TYPE_PIPELINE;
  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to parse configuration file, cannot get the valid type from configuration.");
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to create ml-service extension.
 */
int
ml_service_extension_create (ml_service_s * mls, JsonObject * object)
{
  ml_extension_s *ext;
  g_autofree gchar *thread_name = g_strdup_printf ("ml-ext-msg-%d", getpid ());
  int status;

  mls->priv = ext = g_try_new0 (ml_extension_s, 1);
  if (ext == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for ml-service extension. Out of memory?");
  }

  ext->type = ML_EXTENSION_TYPE_UNKNOWN;
  ext->running = FALSE;
  ext->timeout = DEFAULT_TIMEOUT;
  ext->max_input = DEFAULT_MAX_INPUT;
  ext->node_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
      _ml_extension_node_info_free);

  status = _ml_extension_conf_parse_json (mls, object);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status,
        "Failed to parse the ml-service extension configuration.");
  }

  g_mutex_lock (&mls->lock);

  ext->msg_queue = g_async_queue_new_full (_ml_extension_msg_free);
  ext->msg_thread = g_thread_new (thread_name, _ml_extension_msg_thread, mls);

  /* Wait until the message thread has been initialized. */
  g_cond_wait (&mls->cond, &mls->lock);
  g_mutex_unlock (&mls->lock);

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to release ml-service extension.
 */
int
ml_service_extension_destroy (ml_service_s * mls)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;

  /* Supposed internal function call to release handle. */
  if (!ext)
    return ML_ERROR_NONE;

  /**
   * Close message thread.
   * If model inference is running, it may wait for the result in message thread.
   * This takes time, so do not call join with extension lock.
   */
  ext->running = FALSE;
  if (ext->msg_thread) {
    g_thread_join (ext->msg_thread);
    ext->msg_thread = NULL;
  }

  if (ext->msg_queue) {
    g_async_queue_unref (ext->msg_queue);
    ext->msg_queue = NULL;
  }

  if (ext->single) {
    ml_single_close (ext->single);
    ext->single = NULL;
  }

  if (ext->pipeline) {
    ml_pipeline_stop (ext->pipeline);
    ml_pipeline_destroy (ext->pipeline);
    ext->pipeline = NULL;
  }

  if (ext->node_table) {
    g_hash_table_destroy (ext->node_table);
    ext->node_table = NULL;
  }

  g_free (ext);
  mls->priv = NULL;

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to start ml-service extension.
 */
int
ml_service_extension_start (ml_service_s * mls)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  int status = ML_ERROR_NONE;

  switch (ext->type) {
    case ML_EXTENSION_TYPE_PIPELINE:
      status = ml_pipeline_start (ext->pipeline);
      break;
    case ML_EXTENSION_TYPE_SINGLE:
      /* Do nothing. */
      break;
    default:
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  return status;
}

/**
 * @brief Internal function to stop ml-service extension.
 */
int
ml_service_extension_stop (ml_service_s * mls)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  int status = ML_ERROR_NONE;

  switch (ext->type) {
    case ML_EXTENSION_TYPE_PIPELINE:
      status = ml_pipeline_stop (ext->pipeline);
      break;
    case ML_EXTENSION_TYPE_SINGLE:
      /* Do nothing. */
      break;
    default:
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  return status;
}

/**
 * @brief Internal function to get the information of required input data.
 */
int
ml_service_extension_get_input_information (ml_service_s * mls,
    const char *name, ml_tensors_info_h * info)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  int status;

  switch (ext->type) {
    case ML_EXTENSION_TYPE_SINGLE:
      status = ml_single_get_input_info (ext->single, info);
      break;
    case ML_EXTENSION_TYPE_PIPELINE:
    {
      ml_extension_node_info_s *node_info;

      node_info = _ml_extension_node_info_get (ext, name);

      if (node_info && node_info->type == ML_EXTENSION_NODE_TYPE_INPUT) {
        status = ml_tensors_info_create (info);
        if (status != ML_ERROR_NONE)
          break;
        status = ml_tensors_info_clone (*info, node_info->info);
        if (status != ML_ERROR_NONE)
          break;
      } else {
        status = ML_ERROR_INVALID_PARAMETER;
      }
      break;
    }
    default:
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  return status;
}

/**
 * @brief Internal function to get the information of output data.
 */
int
ml_service_extension_get_output_information (ml_service_s * mls,
    const char *name, ml_tensors_info_h * info)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  int status;

  switch (ext->type) {
    case ML_EXTENSION_TYPE_SINGLE:
      status = ml_single_get_output_info (ext->single, info);
      break;
    case ML_EXTENSION_TYPE_PIPELINE:
    {
      ml_extension_node_info_s *node_info;

      node_info = _ml_extension_node_info_get (ext, name);

      if (node_info && node_info->type == ML_EXTENSION_NODE_TYPE_OUTPUT) {
        status = ml_tensors_info_create (info);
        if (status != ML_ERROR_NONE)
          break;
        status = ml_tensors_info_clone (*info, node_info->info);
        if (status != ML_ERROR_NONE)
          break;
      } else {
        status = ML_ERROR_INVALID_PARAMETER;
      }
      break;
    }
    default:
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  if (status != ML_ERROR_NONE) {
    if (*info) {
      ml_tensors_info_destroy (*info);
      *info = NULL;
    }
  }

  return status;
}

/**
 * @brief Internal function to set the information for ml-service extension.
 */
int
ml_service_extension_set_information (ml_service_s * mls, const char *name,
    const char *value)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;

  /* Check limitation of message queue and other options. */
  if (g_ascii_strcasecmp (name, "input_queue_size") == 0 ||
      g_ascii_strcasecmp (name, "max_input") == 0) {
    ext->max_input = (guint) g_ascii_strtoull (value, NULL, 10);
  } else if (g_ascii_strcasecmp (name, "timeout") == 0) {
    ext->timeout = (guint) g_ascii_strtoull (value, NULL, 10);
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to add an input data to process the model in ml-service extension handle.
 */
int
ml_service_extension_request (ml_service_s * mls, const char *name,
    const ml_tensors_data_h data)
{
  ml_extension_s *ext = (ml_extension_s *) mls->priv;
  ml_extension_msg_s *msg;
  int status, len;

  if (ext->type == ML_EXTENSION_TYPE_PIPELINE) {
    ml_extension_node_info_s *node_info;

    if (!STR_IS_VALID (name)) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The parameter, name '%s', is invalid.", name);
    }

    node_info = _ml_extension_node_info_get (ext, name);

    if (!node_info || node_info->type != ML_EXTENSION_NODE_TYPE_INPUT) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The parameter, name '%s', is invalid, cannot find the input node from pipeline.",
          name);
    }
  }

  len = g_async_queue_length (ext->msg_queue);

  if (ext->max_input > 0 && len > 0 && ext->max_input <= len) {
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Failed to push input data into the queue, the max number of input is %u.",
        ext->max_input);
  }

  msg = g_try_new0 (ml_extension_msg_s, 1);
  if (!msg) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate the ml-service extension message. Out of memory?");
  }

  msg->name = g_strdup (name);
  status = ml_tensors_data_clone (data, &msg->input);

  if (status != ML_ERROR_NONE) {
    _ml_extension_msg_free (msg);
    _ml_error_report_return (status, "Failed to clone input data.");
  }

  g_async_queue_push (ext->msg_queue, msg);

  return ML_ERROR_NONE;
}
