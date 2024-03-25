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
#include <json-glib/json-glib.h>
#include <nnstreamer-edge.h>

#include "ml-api-internal.h"
#include "ml-api-service.h"
#include "ml-api-service-private.h"
#include "ml-api-service-training-offloading.h"

/**
 * @brief Internal function to parse configuration file.
 */
static int
_ml_service_training_offloadin_conf_parse_json (_ml_service_offloading_s *
    offloading_s, JsonObject * object)
{
  ml_training_services_s *training_s = NULL;
  JsonObject *training_obj, *data_obj;
  JsonNode *training_node, *data_node;
  const gchar *key;
  const gchar *val;
  GList *list = NULL, *iter;
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

  training_node = json_object_get_member (object, "training");
  training_obj = json_node_get_object (training_node);

  val = json_object_get_string_member (training_obj, "sender-pipeline");
  training_s->sender_pipe = g_strdup (val);

  if (!json_object_has_member (training_obj, "transfer-data"))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The given param, \"transfer-data\" is invalid.");
  data_node = json_object_get_member (training_obj, "transfer-data");
  data_obj = json_node_get_object (data_node);

  list = json_object_get_members (data_obj);
  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    val = json_object_get_string_member (data_obj, iter->data);
    key = iter->data;
    if (!STR_IS_VALID (key) || !STR_IS_VALID (val)) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The parameter, 'key' or 'value' is NULL. It should be a valid string.");
    }
    g_critical ("######## %s : %s: %d key:%s, val:%s", __FILE__, __FUNCTION__,
        __LINE__, key, val);
    g_hash_table_insert (training_s->transfer_data_table, g_strdup (key),
        g_strdup (val));
  }
  g_list_free (list);

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

  training_s->transfer_data_table =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (!training_s->transfer_data_table) {
    _ml_error_report ("Failed to allocate memory for the hash table.");
  }

  g_cond_init (&training_s->received_cond);
  g_mutex_init (&training_s->received_lock);

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
  int ret;

  if (!offloading_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'remote_s' (_ml_remote_service_s), is NULL.");
  }

  if (!object) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'object' is NULL.");
  }

  ret = _ml_service_create_training_offloading (offloading_s);
  if (ret != ML_ERROR_NONE) {
    _ml_error_report_return (ret, "Failed to create the ml-service extension.");
  }

  ret = _ml_service_training_offloadin_conf_parse_json (offloading_s, object);
  if (ret != ML_ERROR_NONE) {
    _ml_error_report_return (ret, "Failed to parse the configuration file.");
  }

  return ret;
}

// /**
//  * @brief Set path in ml-service training offloading handle.
//  */
// int
// ml_service_training_offloading_set_path (_ml_service_offloading_s *
//     offloading_s, const gchar * name, const gchar * path)
// {
//   ml_training_services_s *training_s = NULL;
//   g_critical ("######## %s : %s: %d ", __FILE__, __FUNCTION__, __LINE__);
//   if (!offloading_s) {
//     _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
//         "The parameter, 'remote_s' (_ml_service_offloading_s), is NULL.");
//   }

//   training_s = (ml_training_services_s *) offloading_s->priv;

//   if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
//     _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
//         "The given param, dir path = \"%s\" is invalid or the dir is not found or accessible.",
//         path);
//   }
//   if (g_access (path, W_OK) != 0) {
//     _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
//         "Write permission denied, path: %s", path);
//   }

//   if (g_ascii_strcasecmp (name, "model-path") == 0) {
//     g_free (training_s->model_path);
//     training_s->model_path = g_strdup (path);
//     g_critical ("######## %s : %s: %d model_path %s", __FILE__, __FUNCTION__, __LINE__, training_s->model_path);
//   } else if (g_ascii_strcasecmp (name, "model-config-path") == 0) {
//     g_free (training_s->model_config_path);
//     training_s->model_config_path = g_strdup (path);
//     g_critical ("######## %s : %s: %d model_config_path %s", __FILE__, __FUNCTION__, __LINE__, training_s->model_config_path);

//   } else if (g_ascii_strcasecmp (name, "data-path") == 0) {
//     g_free (training_s->data_path);
//     training_s->data_path = g_strdup (path);
//     g_critical ("######## %s : %s: %d data_path %s", __FILE__, __FUNCTION__, __LINE__, training_s->data_path);

//   } else {
//     _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
//         "The given param, \"%s\" is invalid.", name);
//   }

//   return ML_ERROR_NONE;
// }

/**
 * @brief A handle of input or output frames. #ml_tensors_info_h is the handle for tensors metadata.
 * @since_tizen 5.5
 */
//typedef void *ml_tensors_data_h;


static int
_ml_service_training_offloading_request (ml_service_s * mls,
    const gchar * service_name, const gchar * data, gsize len)
{
  int ret = ML_ERROR_NONE;
  ml_tensors_data_h input = NULL;
  ml_tensors_info_h in_info = NULL;
  ml_tensor_dimension in_dim = { 0 };

  ret = ml_tensors_info_create (&in_info);
  ml_tensors_info_set_count (in_info, 1);
  ml_tensors_info_set_tensor_type (in_info, 0, ML_TENSOR_TYPE_UINT8);
  in_dim[0] = len;
  ml_tensors_info_set_tensor_dimension (in_info, 0, in_dim);
  ret = ml_tensors_data_create (in_info, &input);
  ret = ml_tensors_data_set_tensor_data (input, 0, data, len);

  ret = ml_service_offloading_request (mls, service_name, input);
  if (ret != ML_ERROR_NONE)
    g_critical ("######## %s : %s: %d, Failed to request service:%s", __FILE__,
        __FUNCTION__, __LINE__, service_name);
  ret = ml_tensors_info_destroy (in_info);
  ret = ml_tensors_data_destroy (input);

  return ret;
}


/**
* @brief Request all services to ml-service offloading.
*/
static int
_ml_service_training_offloading_services_request (ml_service_s * mls)
{
  ml_training_services_s *training_s = NULL;
  _ml_service_offloading_s *offloading_s = NULL;
  int ret = ML_ERROR_NONE;
  GList *list = NULL, *iter;
  gchar *transfer_data = NULL, *service_name = NULL, *contents =
      NULL, *pipeline = NULL;
  guint changed;
  gsize len;

  offloading_s = (_ml_service_offloading_s *) mls->priv;
  training_s = (ml_training_services_s *) offloading_s->priv;
  g_critical ("######## %s : %s: %d path=%s ", __FILE__, __FUNCTION__, __LINE__,
      offloading_s->path);
  list = g_hash_table_get_keys (training_s->transfer_data_table);

  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    transfer_data =
        g_strdup (g_hash_table_lookup (training_s->transfer_data_table,
            (gchar *) iter->data));

    if (g_strstr_len (transfer_data, -1, "@APP_RW_PATH@")) {
      transfer_data =
          _ml_replace_string (transfer_data, "@APP_RW_PATH@",
          offloading_s->path, NULL, &changed);

      g_critical ("######## %s : %s: %d transfer_data=%s", __FILE__,
          __FUNCTION__, __LINE__, transfer_data);

      if (!g_file_get_contents (transfer_data, &contents, &len, NULL)) {
        _ml_error_report ("Failed to read file:%s", transfer_data);
        goto error;
      }
      ret =
          _ml_service_training_offloading_request (mls, (gchar *) iter->data,
          contents, len);
      if (ret != ML_ERROR_NONE) {
        _ml_error_report ("Failed to request service(%s)",
            (gchar *) iter->data);
        goto error;
      }
      g_free (transfer_data);
      g_free (contents);
      transfer_data = NULL;
      contents = NULL;

    } else if (g_strstr_len (transfer_data, -1, "@REMOTE_APP_RW_PATH@")) {
      service_name = g_strdup (iter->data);
      pipeline = g_strdup (transfer_data);
      transfer_data = NULL;
    }
  }
  /** The remote sender sends the last in the pipeline.
     When the pipeline arrives, the remote receiver determines that the sender has sent all the necessary files specified in the pipeline.
     pipeline description must be sent last. */
  g_critical ("######## %s : %s: %d pipeline=%s", __FILE__, __FUNCTION__,
      __LINE__, pipeline);

  ret =
      _ml_service_training_offloading_request (mls, service_name, pipeline,
      strlen (pipeline) + 1);
  if (ret != ML_ERROR_NONE)
    _ml_error_report ("Failed to request service(%s)", service_name);

error:
  g_free (service_name);
  g_free (transfer_data);
  g_free (contents);
  g_list_free (list);

  return ret;
}

/**
 * @brief Thread for checking receive data
*/
static void
check_received_data (ml_training_services_s * training_s)
{
  int loop = 100;
  while (loop--) {
    g_usleep (100000);
    /** The remote sender sends the receiver_pipeline last. When the it arrives, all files sent by the remote sender have been received.
       If the remote sender does not send the file defined in the receiver_pipeline, a runtime error occurs. */
    if (training_s->receiver_pipe != NULL) {
      g_critical ("######## %s : %s: %d locked", __FILE__, __FUNCTION__,
          __LINE__);
      g_mutex_lock (&training_s->received_lock);
      training_s->is_received = TRUE;
      g_critical
          ("######## %s : %s: %d Receive all required data, receive_pipe =%s",
          __FILE__, __FUNCTION__, __LINE__, training_s->receiver_pipe);
      g_cond_signal (&training_s->received_cond);
      g_mutex_unlock (&training_s->received_lock);
      return;
    }
  }
  g_critical ("######## %s : %s: %d Data is null, receive_pipe =%s", __FILE__,
      __FUNCTION__, __LINE__, training_s->receiver_pipe);
  training_s->is_received = FALSE;
  g_cond_signal (&training_s->received_cond);
  g_mutex_unlock (&training_s->received_lock);
}

/**
 * @brief Check if all necessary data is received
*/
static int
_ml_service_training_offloading_check_received_data (ml_training_services_s *
    training_s)
{
  training_s->received_thread =
      g_thread_new ("check_received_file", (GThreadFunc) check_received_data,
      training_s);
  g_mutex_lock (&training_s->received_lock);
  while (!training_s->is_received) {
    g_critical ("######## %s : %s: %d g_cond_wait", __FILE__, __FUNCTION__,
        __LINE__);
    g_cond_wait (&training_s->received_cond, &training_s->received_lock);
  }

  g_mutex_unlock (&training_s->received_lock);
  g_critical ("######## %s : %s: %d unlocked", __FILE__, __FUNCTION__,
      __LINE__);

  return training_s->is_received;
}

/**
 * @brief replace path.
 */
static void
    _ml_service_training_offloading_replce_pipeline_data_path
    (_ml_service_offloading_s * offloading_s)
{
  guint changed = 0;
  ml_training_services_s *training_s =
      (ml_training_services_s *) offloading_s->priv;

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_SENDER) {
    if (training_s->sender_pipe) {
      training_s->sender_pipe =
          _ml_replace_string (training_s->sender_pipe, "@APP_RW_PATH@",
          offloading_s->path, NULL, &changed);
      g_critical ("######## %s : %s: %d sender_pipe = %s", __FILE__,
          __FUNCTION__, __LINE__, training_s->sender_pipe);
    }
  } else {
    if (training_s->receiver_pipe) {
      training_s->receiver_pipe =
          _ml_replace_string (training_s->receiver_pipe, "@REMOTE_APP_RW_PATH@",
          offloading_s->path, NULL, &changed);
      g_critical ("######## %s : %s: %d receiver_pipe = %s", __FILE__,
          __FUNCTION__, __LINE__, training_s->receiver_pipe);
    }
  }
}

/**
 * @brief Start ml training offloading service.
 */
int
ml_service_training_offloading_start (ml_service_s * mls)
{
  int ret = ML_ERROR_NONE;
  ml_training_services_s *training_s = NULL;
  _ml_service_offloading_s *offloading_s = NULL;

  if (!mls) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'mls' (ml_service_s), is NULL.");
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;
  if (!offloading_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'offloading_s' (_ml_service_offloading_s), is NULL.");
  }

  training_s = (ml_training_services_s *) offloading_s->priv;
  if (!training_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'training_s' (training_s), is NULL.");
  }

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_SENDER) {
    g_critical ("SENDER ######## %s : %s: %d ", __FILE__, __FUNCTION__,
        __LINE__);

    ret = _ml_service_training_offloading_services_request (mls);
    if (ret != ML_ERROR_NONE)
      _ml_error_report_return (ret, "Failed to request service");
    _ml_service_training_offloading_replce_pipeline_data_path (offloading_s);
    g_critical ("SENDER ######## %s : %s: %d ", __FILE__, __FUNCTION__,
        __LINE__);

    ret =
        ml_pipeline_construct (training_s->sender_pipe, NULL, NULL,
        &training_s->pipeline_h);
    if (ML_ERROR_NONE != ret) {
      g_critical ("SENDER ######## %s : %s: %d Failed to construct pipeline",
          __FILE__, __FUNCTION__, __LINE__);
      _ml_error_report_return (ret, "Failed to construct pipeline");
    }

    ret = ml_pipeline_start (training_s->pipeline_h);
    if (ret != ML_ERROR_NONE) {
      g_critical ("SENDER ######## %s : %s: %d Failed to start pipeline",
          __FILE__, __FUNCTION__, __LINE__);
      _ml_error_report_return (ret, "Failed to start ml pipeline ret", ret);
    }

  } else if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER) {
    g_critical ("RECEIVER ######## %s : %s: %d ", __FILE__, __FUNCTION__,
        __LINE__);

    /* checking if all required files are received */
    if (FALSE ==
        _ml_service_training_offloading_check_received_data (training_s))
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Failed to receive the required data");

    _ml_service_training_offloading_replce_pipeline_data_path (offloading_s);

    g_critical ("RECEIVER ######## %s : %s: %d ", __FILE__, __FUNCTION__,
        __LINE__);

    ret =
        ml_pipeline_construct (training_s->receiver_pipe, NULL, NULL,
        &training_s->pipeline_h);
    if (ML_ERROR_NONE != ret) {
      g_critical ("RECEIVER ######## %s : %s: %d Failed to construct pipeline",
          __FILE__, __FUNCTION__, __LINE__);
      _ml_error_report_return (ret, "Failed to construct pipeline");
    }

    ret = ml_pipeline_start (training_s->pipeline_h);
    if (ret != ML_ERROR_NONE) {
      g_critical ("RECEIVER ######## %s : %s: %d Failed to start pipeline",
          __FILE__, __FUNCTION__, __LINE__);
      _ml_error_report_return (ret, "Failed to start ml pipeline ret", ret);
    }

  } else {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The node type information in JSON is incorrect.");
  }

  g_critical ("######## %s : %s: %d !!! START !!! ", __FILE__, __FUNCTION__,
      __LINE__);

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
  _ml_service_offloading_s *offloading_s = NULL;

  if (!mls) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'mls' (ml_service_s), is NULL.");
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;
  if (!offloading_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'offloading_s' (_ml_service_offloading_s), is NULL.");
  }

  training_s = (ml_training_services_s *) offloading_s->priv;
  if (!training_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'training_s' (training_s), is NULL.");
  }

  g_critical ("######## %s : %s: %d stop", __FILE__, __FUNCTION__, __LINE__);

  /* need to set property "ready-to-complete" */

  /* check is_model_complete */

  ret = ml_pipeline_stop (training_s->pipeline_h);
  if (ML_ERROR_NONE != ret) {
    g_critical ("SENDER ######## %s : %s: %d Failed to construct pipeline",
        __FILE__, __FUNCTION__, __LINE__);
    _ml_error_report_return (ret, "Failed to construct pipeline");
  }

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER) {
    g_critical ("SENDER ######## %s : %s: %d ", __FILE__, __FUNCTION__,
        __LINE__);

    // ret = _ml_service_training_offloading_services_request (mls);
    // if (ret != ML_ERROR_NONE)
    //   _ml_error_report_return (ret, "Failed to request service");
  }

  return ret;
}

// /**
//  * @brief get file extension
//  */
// static gchar *
// _get_file_extension (char *filename)
// {
//   char *extension = NULL;
//   int i = 0;
//   int len = strlen (filename);

//   filename += len;
//   for (i = 0; i < len; i++) {
//     if (*filename == '.') {
//       extension = filename + 1;
//       break;
//     }
//     filename--;
//   }

//   return extension;
// }

/**
 * @brief Save received file path.
 */
void
ml_service_training_offloading_save_received_file_path (_ml_service_offloading_s
    * offloading_s, nns_edge_data_h data_h, const gchar * dir_path,
    const gchar * data, int service_type)
{
  ml_training_services_s *training_s = NULL;
  g_autofree gchar *name = NULL;
  g_autofree gchar *model_path = NULL;

  training_s = (ml_training_services_s *) offloading_s->priv;

  g_critical ("######## %s : %s: %d cb!!! ", __FILE__, __FUNCTION__, __LINE__);

  if (training_s->type == ML_TRAINING_OFFLOADING_TYPE_RECEIVER) {
    g_critical ("######## %s : %s: %d cb!!!, service_type=%d ", __FILE__,
        __FUNCTION__, __LINE__, service_type);

    if (service_type == 3) {
      training_s->receiver_pipe = g_strdup (data);
      g_critical ("######## %s : %s: %d cb!!! pipeline = %s", __FILE__,
          __FUNCTION__, __LINE__, training_s->receiver_pipe);
    }
  }
}

/**
 * @brief Internal function to destroy ml-service training offloading data.
 */
int
ml_service_training_offloading_destroy (_ml_service_offloading_s * offloading_s)
{
  int ret = ML_ERROR_NONE;
  ml_training_services_s *training_s = NULL;

  if (!offloading_s) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'offloading_s' is NULL.");
  }

  training_s = (ml_training_services_s *) offloading_s->priv;

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

  if (training_s->pipeline_h) {
    g_critical ("######## %s : %s: %d destroy", __FILE__, __FUNCTION__,
        __LINE__);
    ret = ml_pipeline_destroy (training_s->pipeline_h);
    if (ret != ML_ERROR_NONE) {
      g_critical ("Failed to destroy ml pipeline ret(%d)", ret);
      return ret;
    }
  }

  training_s->pipeline_h = NULL;

  g_free (training_s->receiver_pipe);
  training_s->receiver_pipe = NULL;

  g_free (training_s->sender_pipe);
  training_s->sender_pipe = NULL;

  g_free (training_s);
  offloading_s->priv = NULL;

  return ret;
}
