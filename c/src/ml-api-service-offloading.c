/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-offloading.c
 * @date 26 Jun 2023
 * @brief ML offloading service of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Gichan Jang <gichan2.jang@samsung.com>
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
#include "ml-api-service-offloading.h"
#include "ml-api-service-training-offloading.h"

#define MAX_PORT_NUM_LEN 6U

/**
 * @brief Data struct for options.
 */
typedef struct
{
  gchar *host;
  guint port;
  gchar *topic;
  gchar *dest_host;
  guint dest_port;
  nns_edge_connect_type_e conn_type;
  nns_edge_node_type_e node_type;
  gchar *id;
} edge_info_s;

/**
 * @brief Structure for ml_service_offloading.
 */
typedef struct
{
  nns_edge_h edge_h;
  nns_edge_node_type_e node_type;

  gchar *path; /**< A path to save the received model file */
  GHashTable *table;

  ml_service_offloading_mode_e offloading_mode;
  void *priv;
} _ml_service_offloading_s;

/**
 * @brief Get ml-service node type from ml_option.
 */
static nns_edge_node_type_e
_mlrs_get_node_type (const gchar * value)
{
  nns_edge_node_type_e node_type = NNS_EDGE_NODE_TYPE_UNKNOWN;

  if (!value)
    return node_type;

  if (g_ascii_strcasecmp (value, "sender") == 0) {
    node_type = NNS_EDGE_NODE_TYPE_QUERY_CLIENT;
  } else if (g_ascii_strcasecmp (value, "receiver") == 0) {
    node_type = NNS_EDGE_NODE_TYPE_QUERY_SERVER;
  } else {
    _ml_error_report ("Invalid node type '%s', please check node type.", value);
  }

  return node_type;
}

/**
 * @brief Get nnstreamer-edge connection type
 */
static nns_edge_connect_type_e
_mlrs_get_conn_type (const gchar * value)
{
  nns_edge_connect_type_e conn_type = NNS_EDGE_CONNECT_TYPE_UNKNOWN;

  if (!value)
    return conn_type;

  if (0 == g_ascii_strcasecmp (value, "TCP"))
    conn_type = NNS_EDGE_CONNECT_TYPE_TCP;
  else if (0 == g_ascii_strcasecmp (value, "HYBRID"))
    conn_type = NNS_EDGE_CONNECT_TYPE_HYBRID;
  else if (0 == g_ascii_strcasecmp (value, "MQTT"))
    conn_type = NNS_EDGE_CONNECT_TYPE_MQTT;
  else if (0 == g_ascii_strcasecmp (value, "AITT"))
    conn_type = NNS_EDGE_CONNECT_TYPE_AITT;
  else
    conn_type = NNS_EDGE_CONNECT_TYPE_UNKNOWN;

  return conn_type;
}

/**
 * @brief Get edge info from ml_option.
 */
static void
_mlrs_get_edge_info (ml_option_h option, edge_info_s ** edge_info)
{
  edge_info_s *_info;
  void *value;

  *edge_info = _info = g_new0 (edge_info_s, 1);

  if (ML_ERROR_NONE == ml_option_get (option, "host", &value))
    _info->host = g_strdup (value);
  else
    _info->host = g_strdup ("localhost");
  if (ML_ERROR_NONE == ml_option_get (option, "port", &value))
    _info->port = (guint) g_ascii_strtoull (value, NULL, 10);
  if (ML_ERROR_NONE == ml_option_get (option, "dest-host", &value))
    _info->dest_host = g_strdup (value);
  else
    _info->dest_host = g_strdup ("localhost");
  if (ML_ERROR_NONE == ml_option_get (option, "dest-port", &value))
    _info->dest_port = (guint) g_ascii_strtoull (value, NULL, 10);
  if (ML_ERROR_NONE == ml_option_get (option, "connect-type", &value))
    _info->conn_type = _mlrs_get_conn_type (value);
  else
    _info->conn_type = NNS_EDGE_CONNECT_TYPE_UNKNOWN;
  if (ML_ERROR_NONE == ml_option_get (option, "topic", &value))
    _info->topic = g_strdup (value);
  if (ML_ERROR_NONE == ml_option_get (option, "node-type", &value))
    _info->node_type = _mlrs_get_node_type (value);
  if (ML_ERROR_NONE == ml_option_get (option, "id", &value))
    _info->id = g_strdup (value);
}

/**
 * @brief Set nns-edge info.
 */
static void
_mlrs_set_edge_info (edge_info_s * edge_info, nns_edge_h edge_h)
{
  char port[MAX_PORT_NUM_LEN] = { 0, };

  nns_edge_set_info (edge_h, "HOST", edge_info->host);
  g_snprintf (port, MAX_PORT_NUM_LEN, "%u", edge_info->port);
  nns_edge_set_info (edge_h, "PORT", port);

  if (edge_info->topic)
    nns_edge_set_info (edge_h, "TOPIC", edge_info->topic);

  nns_edge_set_info (edge_h, "DEST_HOST", edge_info->dest_host);
  g_snprintf (port, MAX_PORT_NUM_LEN, "%u", edge_info->dest_port);
  nns_edge_set_info (edge_h, "DEST_PORT", port);
}

/**
 * @brief Release edge info.
 */
static void
_mlrs_release_edge_info (edge_info_s * edge_info)
{
  g_free (edge_info->dest_host);
  g_free (edge_info->host);
  g_free (edge_info->topic);
  g_free (edge_info->id);
  g_free (edge_info);
}

/**
 * @brief Get ml offloading service type from ml_option.
 */
static ml_service_offloading_type_e
_mlrs_get_service_type (gchar * service_str)
{
  ml_service_offloading_type_e service_type =
      ML_SERVICE_OFFLOADING_TYPE_UNKNOWN;

  if (!service_str)
    return service_type;

  if (g_ascii_strcasecmp (service_str, "model_raw") == 0) {
    service_type = ML_SERVICE_OFFLOADING_TYPE_MODEL_RAW;
  } else if (g_ascii_strcasecmp (service_str, "model_uri") == 0) {
    service_type = ML_SERVICE_OFFLOADING_TYPE_MODEL_URI;
  } else if (g_ascii_strcasecmp (service_str, "pipeline_raw") == 0) {
    service_type = ML_SERVICE_OFFLOADING_TYPE_PIPELINE_RAW;
  } else if (g_ascii_strcasecmp (service_str, "pipeline_uri") == 0) {
    service_type = ML_SERVICE_OFFLOADING_TYPE_PIPELINE_URI;
  } else if (g_ascii_strcasecmp (service_str, "reply") == 0) {
    service_type = ML_SERVICE_OFFLOADING_TYPE_REPLY;
  } else {
    _ml_error_report ("Invalid service type '%s', please check service type.",
        service_str);
  }

  return service_type;
}

/**
 * @brief Get ml offloading service activation type.
 */
static gboolean
_mlrs_parse_activate (const gchar * activate)
{
  return (activate && g_ascii_strcasecmp (activate, "true") == 0);
}

/**
 * @brief Callback function for receving data using curl.
 */
static size_t
curl_mem_write_cb (void *data, size_t size, size_t nmemb, void *clientp)
{
  size_t recv_size = size * nmemb;
  GByteArray *array = (GByteArray *) clientp;

  if (!array || !data || recv_size == 0)
    return 0;

  g_byte_array_append (array, data, recv_size);

  return recv_size;
}

/**
 * @brief Register model file given by the offloading sender.
 */
static gboolean
_mlrs_model_register (gchar * service_key, nns_edge_data_h data_h,
    void *data, nns_size_t data_len, const gchar * dir_path)
{
  guint version = 0;
  g_autofree gchar *description = NULL;
  g_autofree gchar *name = NULL;
  g_autofree gchar *activate = NULL;
  g_autofree gchar *model_path = NULL;
  gboolean active_bool = TRUE;
  GError *error = NULL;

  if (NNS_EDGE_ERROR_NONE != nns_edge_data_get_info (data_h, "description",
          &description)
      || NNS_EDGE_ERROR_NONE != nns_edge_data_get_info (data_h, "name", &name)
      || NNS_EDGE_ERROR_NONE != nns_edge_data_get_info (data_h, "activate",
          &activate)) {
    _ml_loge ("Failed to get info from data handle.");
    return FALSE;
  }

  active_bool = _mlrs_parse_activate (activate);
  model_path = g_build_path (G_DIR_SEPARATOR_S, dir_path, name, NULL);
  if (!g_file_set_contents (model_path, (char *) data, data_len, &error)) {
    _ml_loge ("Failed to write data to file: %s",
        error ? error->message : "unknown error");
    g_clear_error (&error);
    return FALSE;
  }

  /**
   * @todo Hashing the path. Where is the default path to save the model file?
   */
  if (ML_ERROR_NONE != ml_service_model_register (service_key, model_path,
          active_bool, description, &version)) {
    _ml_loge ("Failed to register model, service key is '%s'.", service_key);
    return FALSE;
  }

  return TRUE;
}

/**
 * @brief Get path to save the model given from offloading sender.
 * @note The caller is responsible for freeing the returned data using g_free().
 */
static gchar *
_mlrs_get_model_dir_path (_ml_service_offloading_s * offloading_s,
    const gchar * service_key)
{
  g_autofree gchar *dir_path = NULL;

  if (offloading_s->path) {
    dir_path = g_strdup (offloading_s->path);
  } else {
    g_autofree gchar *current_dir = g_get_current_dir ();

    dir_path = g_build_path (G_DIR_SEPARATOR_S, current_dir, service_key, NULL);
    if (g_mkdir_with_parents (dir_path, 0755) < 0) {
      _ml_loge ("Failed to create directory '%s': %s", dir_path,
          g_strerror (errno));
      return NULL;
    }
  }

  return g_steal_pointer (&dir_path);
}

/**
 * @brief Get data from gievn uri
 */
static gboolean
_mlrs_get_data_from_uri (gchar * uri, GByteArray * array)
{
  CURL *curl;
  CURLcode res;
  gboolean ret = FALSE;

  curl = curl_easy_init ();
  if (curl) {
    if (CURLE_OK != curl_easy_setopt (curl, CURLOPT_URL, (gchar *) uri) ||
        CURLE_OK != curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L) ||
        CURLE_OK != curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION,
            curl_mem_write_cb) ||
        CURLE_OK != curl_easy_setopt (curl, CURLOPT_WRITEDATA,
            (void *) array)) {
      _ml_loge ("Failed to set option for curl easy handle.");
      ret = FALSE;
      goto done;
    }

    res = curl_easy_perform (curl);

    if (res != CURLE_OK) {
      _ml_loge ("curl_easy_perform failed: %s", curl_easy_strerror (res));
      ret = FALSE;
      goto done;
    }

    ret = TRUE;
  }

done:
  if (curl)
    curl_easy_cleanup (curl);
  return ret;
}

/**
 * @brief Process ml offloading service
 */
static int
_mlrs_process_service_offloading (nns_edge_data_h data_h, void *user_data)
{
  void *data;
  nns_size_t data_len;
  g_autofree gchar *service_str = NULL;
  g_autofree gchar *service_key = NULL;
  g_autofree gchar *dir_path = NULL;
  ml_service_offloading_type_e service_type;
  int ret = NNS_EDGE_ERROR_NONE;
  ml_service_s *mls = (ml_service_s *) user_data;
  _ml_service_offloading_s *offloading_s =
      (_ml_service_offloading_s *) mls->priv;
  ml_service_event_e event_type = ML_SERVICE_EVENT_UNKNOWN;
  ml_information_h info_h = NULL;

  ret = nns_edge_data_get (data_h, 0, &data, &data_len);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report_return (ret,
        "Failed to get data while processing the ml-offloading service.");
  }

  ret = nns_edge_data_get_info (data_h, "service-type", &service_str);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report_return (ret,
        "Failed to get service type while processing the ml-offloading service.");
  }
  service_type = _mlrs_get_service_type (service_str);

  ret = nns_edge_data_get_info (data_h, "service-key", &service_key);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report_return (ret,
        "Failed to get service key while processing the ml-offloading service.");
  }

  dir_path = _mlrs_get_model_dir_path (offloading_s, service_key);

  if (offloading_s->offloading_mode == ML_SERVICE_OFFLOADING_MODE_TRAINING) {
    ml_service_training_offloading_process_received_data (mls, data_h, dir_path,
        data, service_type);
    if (service_type == ML_SERVICE_OFFLOADING_TYPE_REPLY) {
      if (!dir_path) {
        _ml_error_report_return (NNS_EDGE_ERROR_UNKNOWN,
            "Failed to get model directory path.");
      }

      if (_mlrs_model_register (service_key, data_h, data, data_len, dir_path)) {
        event_type = ML_SERVICE_EVENT_MODEL_REGISTERED;
      } else {
        _ml_error_report ("Failed to register model downloaded from: %s.",
            (gchar *) data);
      }
    }
  }

  switch (service_type) {
    case ML_SERVICE_OFFLOADING_TYPE_MODEL_URI:
    {
      GByteArray *array;

      if (!dir_path) {
        _ml_error_report_return (NNS_EDGE_ERROR_UNKNOWN,
            "Failed to get model directory path.");
      }

      array = g_byte_array_new ();

      if (!_mlrs_get_data_from_uri ((gchar *) data, array)) {
        g_byte_array_free (array, TRUE);
        _ml_error_report_return (NNS_EDGE_ERROR_IO,
            "Failed to get data from uri: %s.", (gchar *) data);
      }

      if (_mlrs_model_register (service_key, data_h, array->data, array->len,
              dir_path)) {
        event_type = ML_SERVICE_EVENT_MODEL_REGISTERED;
      } else {
        _ml_error_report ("Failed to register model downloaded from: %s.",
            (gchar *) data);
        ret = NNS_EDGE_ERROR_UNKNOWN;
      }
      g_byte_array_free (array, TRUE);
      break;
    }
    case ML_SERVICE_OFFLOADING_TYPE_MODEL_RAW:
    {
      if (!dir_path) {
        _ml_error_report_return (NNS_EDGE_ERROR_UNKNOWN,
            "Failed to get model directory path.");
      }

      if (_mlrs_model_register (service_key, data_h, data, data_len, dir_path)) {
        event_type = ML_SERVICE_EVENT_MODEL_REGISTERED;
      } else {
        _ml_error_report ("Failed to register model downloaded from: %s.",
            (gchar *) data);
        ret = NNS_EDGE_ERROR_UNKNOWN;
      }
      break;
    }
    case ML_SERVICE_OFFLOADING_TYPE_PIPELINE_URI:
    {
      GByteArray *array = g_byte_array_new ();

      ret = _mlrs_get_data_from_uri ((gchar *) data, array);
      if (!ret) {
        g_byte_array_free (array, TRUE);
        _ml_error_report_return (ret,
            "Failed to get data from uri: %s.", (gchar *) data);
      }
      ret = ml_service_pipeline_set (service_key, (gchar *) array->data);
      if (ML_ERROR_NONE == ret) {
        event_type = ML_SERVICE_EVENT_PIPELINE_REGISTERED;
      }
      g_byte_array_free (array, TRUE);
      break;
    }
    case ML_SERVICE_OFFLOADING_TYPE_PIPELINE_RAW:
      ret = ml_service_pipeline_set (service_key, (gchar *) data);
      if (ML_ERROR_NONE == ret) {
        event_type = ML_SERVICE_EVENT_PIPELINE_REGISTERED;
      }
      break;
    case ML_SERVICE_OFFLOADING_TYPE_REPLY:
    {
      ret = _ml_information_create (&info_h);
      if (ML_ERROR_NONE != ret) {
        _ml_error_report ("Failed to create information handle.");
        goto done;
      }
      ret = _ml_information_set (info_h, "data", (void *) data, NULL);
      if (ML_ERROR_NONE != ret) {
        _ml_error_report ("Failed to set data information.");
        goto done;
      }
      event_type = ML_SERVICE_EVENT_REPLY;
      break;
    }
    default:
      _ml_error_report ("Unknown service type '%d' or not supported yet.",
          service_type);
      break;
  }

  if (event_type != ML_SERVICE_EVENT_UNKNOWN) {
    ml_service_event_cb_info_s cb_info = { 0 };

    _ml_service_get_event_cb_info (mls, &cb_info);

    if (cb_info.cb) {
      cb_info.cb (event_type, info_h, cb_info.pdata);
    }
  }

done:
  if (info_h) {
    ml_information_destroy (info_h);
  }

  return ret;
}

/**
 * @brief Edge event callback.
 */
static int
_mlrs_edge_event_cb (nns_edge_event_h event_h, void *user_data)
{
  nns_edge_event_e event = NNS_EDGE_EVENT_UNKNOWN;
  nns_edge_data_h data_h = NULL;
  int ret = NNS_EDGE_ERROR_NONE;

  ret = nns_edge_event_get_type (event_h, &event);
  if (NNS_EDGE_ERROR_NONE != ret)
    return ret;

  switch (event) {
    case NNS_EDGE_EVENT_NEW_DATA_RECEIVED:
    {
      ret = nns_edge_event_parse_new_data (event_h, &data_h);
      if (NNS_EDGE_ERROR_NONE != ret)
        return ret;

      ret = _mlrs_process_service_offloading (data_h, user_data);
      break;
    }
    default:
      break;
  }

  if (data_h)
    nns_edge_data_destroy (data_h);

  return ret;
}

/**
 * @brief Create edge handle.
 */
static int
_mlrs_create_edge_handle (ml_service_s * mls, edge_info_s * edge_info)
{
  int ret = 0;
  nns_edge_h edge_h = NULL;
  _ml_service_offloading_s *offloading_s = NULL;

  ret = nns_edge_create_handle (edge_info->id, edge_info->conn_type,
      edge_info->node_type, &edge_h);

  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("nns_edge_create_handle failed.");
    return ret;
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;
  ret = nns_edge_set_event_callback (edge_h, _mlrs_edge_event_cb, mls);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("nns_edge_set_event_callback failed.");
    nns_edge_release_handle (edge_h);
    return ret;
  }

  _mlrs_set_edge_info (edge_info, edge_h);

  ret = nns_edge_start (edge_h);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("nns_edge_start failed.");
    nns_edge_release_handle (edge_h);
    return ret;
  }

  if (edge_info->node_type == NNS_EDGE_NODE_TYPE_QUERY_CLIENT) {
    ret = nns_edge_connect (edge_h, edge_info->dest_host, edge_info->dest_port);

    if (NNS_EDGE_ERROR_NONE != ret) {
      _ml_error_report ("nns_edge_connect failed.");
      nns_edge_release_handle (edge_h);
      return ret;
    }
  }
  offloading_s->edge_h = edge_h;

  return ret;
}

/**
 * @brief Set offloading mode and private data.
 */
int
ml_service_offloading_set_mode (ml_service_h handle,
    ml_service_offloading_mode_e mode, void *priv)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_offloading_s *offloading_s;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;

  offloading_s->offloading_mode = mode;
  offloading_s->priv = priv;

  return ML_ERROR_NONE;
}

/**
 * @brief Get offloading mode and private data.
 */
int
ml_service_offloading_get_mode (ml_service_h handle,
    ml_service_offloading_mode_e * mode, void **priv)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_offloading_s *offloading_s;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  if (!mode || !priv) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, mode or priv, is null. It should be a valid pointer.");
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;

  *mode = offloading_s->offloading_mode;
  *priv = offloading_s->priv;

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to release ml-service offloading data.
 */
int
ml_service_offloading_release_internal (ml_service_s * mls)
{
  _ml_service_offloading_s *offloading_s;

  /* Supposed internal function call to release handle. */
  if (!mls || !mls->priv)
    return ML_ERROR_NONE;

  offloading_s = (_ml_service_offloading_s *) mls->priv;

  if (offloading_s->offloading_mode == ML_SERVICE_OFFLOADING_MODE_TRAINING) {
    /**
     * 'ml_service_training_offloading_destroy' transfers internally trained models.
     * So keep offloading handle.
     */
    if (ML_ERROR_NONE != ml_service_training_offloading_destroy (mls)) {
      _ml_error_report
          ("Failed to release ml-service training offloading handle");
    }
  }

  if (offloading_s->edge_h) {
    nns_edge_release_handle (offloading_s->edge_h);
    offloading_s->edge_h = NULL;
  }

  if (offloading_s->table) {
    g_hash_table_destroy (offloading_s->table);
    offloading_s->table = NULL;
  }

  g_free (offloading_s->path);
  g_free (offloading_s);
  mls->priv = NULL;

  return ML_ERROR_NONE;
}

/**
 * @brief Set value in ml-service offloading handle.
 */
int
ml_service_offloading_set_information (ml_service_h handle, const gchar * name,
    const gchar * value)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_offloading_s *offloading_s;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;

  if (g_ascii_strcasecmp (name, "path") == 0) {
    if (!g_file_test (value, G_FILE_TEST_IS_DIR)) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The given param, dir path '%s' is invalid or the dir is not found or accessible.",
          value);
    }

    if (g_access (value, W_OK) != 0) {
      _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
          "Write permission to dir '%s' is denied.", value);
    }

    g_free (offloading_s->path);
    offloading_s->path = g_strdup (value);
    ml_service_training_offloading_set_path (mls, offloading_s->path);
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to parse configuration file to create offloading service.
 */
int
ml_service_offloading_create (ml_service_h handle, ml_option_h option)
{
  ml_service_s *mls;
  _ml_service_offloading_s *offloading_s = NULL;
  edge_info_s *edge_info = NULL;
  int ret = ML_ERROR_NONE;
  gchar *_path = NULL;

  if (!handle) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is NULL. It should be a valid ml_service_h.");
  }

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }
  mls = (ml_service_s *) handle;

  mls->priv = offloading_s = g_try_new0 (_ml_service_offloading_s, 1);
  if (offloading_s == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service handle's private data. Out of memory?");
  }

  offloading_s->table =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (!offloading_s->table) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the table of ml-service offloading. Out of memory?");
  }

  if (ML_ERROR_NONE == ml_option_get (option, "path", (void **) (&_path))) {
    ret = ml_service_offloading_set_information (mls, "path", _path);
    if (ML_ERROR_NONE != ret) {
      _ml_error_report_return (ret,
          "Failed to set path in ml-service offloading handle.");
    }
  }

  _mlrs_get_edge_info (option, &edge_info);

  offloading_s->node_type = edge_info->node_type;
  ret = _mlrs_create_edge_handle (mls, edge_info);
  _mlrs_release_edge_info (edge_info);

  return ret;
}

/**
 * @brief Internal function to start ml-service offloading.
 */
int
ml_service_offloading_start (ml_service_h handle)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_offloading_s *offloading_s;
  int ret = ML_ERROR_NONE;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;

  if (offloading_s->offloading_mode == ML_SERVICE_OFFLOADING_MODE_TRAINING) {
    ret = ml_service_training_offloading_start (mls);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report ("Failed to start training offloading.");
    }
  }

  return ret;
}


/**
 * @brief Internal function to stop ml-service offloading.
 */
int
ml_service_offloading_stop (ml_service_h handle)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_offloading_s *offloading_s;
  int ret = ML_ERROR_NONE;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  offloading_s = (_ml_service_offloading_s *) mls->priv;

  if (offloading_s->offloading_mode == ML_SERVICE_OFFLOADING_MODE_TRAINING) {
    ret = ml_service_training_offloading_stop (mls);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report ("Failed to stop training offloading.");
    }
  }

  return ret;
}

/**
 * @brief Register new information, such as neural network models or pipeline descriptions, on a offloading server.
 */
int
ml_service_offloading_request (ml_service_h handle, const char *key,
    const ml_tensors_data_h input)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_offloading_s *offloading_s = NULL;
  const gchar *service_key = NULL;
  nns_edge_data_h data_h = NULL;
  int ret = NNS_EDGE_ERROR_NONE;
  const gchar *service_str = NULL;
  const gchar *description = NULL;
  const gchar *name = NULL;
  const gchar *activate = NULL;
  ml_tensors_data_s *_in = NULL;
  JsonNode *service_node;
  JsonObject *service_obj;
  guint i;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  if (!STR_IS_VALID (key)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'key' is NULL. It should be a valid string.");
  }

  if (!input)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, input (ml_tensors_data_h), is NULL. It should be a valid ml_tensor_data_h instance, which is usually created by ml_tensors_data_create().");

  offloading_s = (_ml_service_offloading_s *) mls->priv;

  service_str = g_hash_table_lookup (offloading_s->table, key);
  if (!service_str) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The given service key, %s, is not registered in the ml-service offloading handle.",
        key);
  }

  service_node = json_from_string (service_str, NULL);
  if (!service_node) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to parse the json string, %s.", service_str);
  }
  service_obj = json_node_get_object (service_node);
  if (!service_obj) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to get the json object from the json node.");
  }

  service_str = json_object_get_string_member (service_obj, "service-type");
  if (!service_str) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to get service type from the json object.");
  }

  service_key = json_object_get_string_member (service_obj, "service-key");
  if (!service_key) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to get service key from the json object.");
  }

  ret = nns_edge_data_create (&data_h);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("Failed to create an edge data.");
    return ret;
  }

  ret = nns_edge_data_set_info (data_h, "service-type", service_str);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("Failed to set service type in edge data.");
    goto done;
  }
  ret = nns_edge_data_set_info (data_h, "service-key", service_key);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("Failed to set service key in edge data.");
    goto done;
  }

  description = json_object_get_string_member (service_obj, "description");
  if (description) {
    ret = nns_edge_data_set_info (data_h, "description", description);
    if (NNS_EDGE_ERROR_NONE != ret) {
      _ml_logi ("Failed to set description in edge data.");
    }
  }

  name = json_object_get_string_member (service_obj, "name");
  if (name) {
    ret = nns_edge_data_set_info (data_h, "name", name);
    if (NNS_EDGE_ERROR_NONE != ret) {
      _ml_logi ("Failed to set name in edge data.");
    }
  }

  activate = json_object_get_string_member (service_obj, "activate");
  if (activate) {
    ret = nns_edge_data_set_info (data_h, "activate", activate);
    if (NNS_EDGE_ERROR_NONE != ret) {
      _ml_logi ("Failed to set activate in edge data.");
    }
  }
  _in = (ml_tensors_data_s *) input;
  for (i = 0; i < _in->num_tensors; i++) {
    ret =
        nns_edge_data_add (data_h, _in->tensors[i].data, _in->tensors[i].size,
        NULL);
    if (NNS_EDGE_ERROR_NONE != ret) {
      _ml_error_report ("Failed to add camera data to the edge data.");
      goto done;
    }
  }

  ret = nns_edge_send (offloading_s->edge_h, data_h);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report
        ("Failed to publish the data to register the offloading service.");
  }

done:
  if (data_h)
    nns_edge_data_destroy (data_h);
  return ret;
}

/**
 * @brief Sets the services in ml-service offloading handle.
 */
int
ml_service_offloading_set_service (ml_service_h handle, const char *key,
    const char *value)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_offloading_s *offloading_s = NULL;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  if (!STR_IS_VALID (key) || !STR_IS_VALID (value)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'key' or 'value' is NULL. It should be a valid string.");
  }
  offloading_s = (_ml_service_offloading_s *) mls->priv;

  g_hash_table_insert (offloading_s->table, g_strdup (key), g_strdup (value));

  return ML_ERROR_NONE;
}
