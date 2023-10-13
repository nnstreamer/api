/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-remote.c
 * @date 26 Jun 2023
 * @brief ML remote service of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Gichan Jang <gichan2.jang@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/app/app.h>
#include <string.h>
#include <curl/curl.h>
#include <nnstreamer-edge.h>

#include "ml-api-internal.h"
#include "ml-api-service.h"
#include "ml-api-service-private.h"

#define MAX_PORT_NUM_LEN 6U

/**
 * @brief Enumeration for ml-remote service type.
 */
typedef enum
{
  ML_REMOTE_SERVICE_TYPE_UNKNOWN = 0,
  ML_REMOTE_SERVICE_TYPE_MODEL_RAW,
  ML_REMOTE_SERVICE_TYPE_MODEL_URI,
  ML_REMOTE_SERVICE_TYPE_PIPELINE_RAW,
  ML_REMOTE_SERVICE_TYPE_PIPELINE_URI,

  ML_REMOTE_SERVICE_TYPE_MAX
} ml_remote_service_type_e;

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
} edge_info_s;

/**
 * @brief Structure for ml_remote_service
 */
typedef struct
{
  nns_edge_h edge_h;
  nns_edge_node_type_e node_type;

  ml_service_event_cb event_cb;
  void *user_data;
} _ml_remote_service_s;

/**
 * @brief Get ml-service node type from ml_option.
 */
static nns_edge_node_type_e
_mlrs_get_node_type (const gchar * value)
{
  nns_edge_node_type_e node_type = NNS_EDGE_NODE_TYPE_UNKNOWN;

  if (!value)
    return node_type;

  if (g_ascii_strcasecmp (value, "remote_sender") == 0) {
    node_type = NNS_EDGE_NODE_TYPE_PUB;
  } else if (g_ascii_strcasecmp (value, "remote_receiver") == 0) {
    node_type = NNS_EDGE_NODE_TYPE_SUB;
  } else {
    _ml_error_report ("Invalid node type: %s, Please check ml_option.", value);
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
_mlrs_get_edge_info (ml_option_h option, edge_info_s * edge_info)
{
  void *value;

  if (ML_ERROR_NONE == ml_option_get (option, "host", &value)) {
    g_free (edge_info->host);
    edge_info->host = g_strdup (value);
  }
  if (ML_ERROR_NONE == ml_option_get (option, "port", &value))
    edge_info->port = *((guint *) value);
  if (ML_ERROR_NONE == ml_option_get (option, "dest-host", &value)) {
    g_free (edge_info->dest_host);
    edge_info->dest_host = g_strdup (value);
  }
  if (ML_ERROR_NONE == ml_option_get (option, "dest-port", &value))
    edge_info->dest_port = *((guint *) value);
  if (ML_ERROR_NONE == ml_option_get (option, "connect-type", &value))
    edge_info->conn_type = _mlrs_get_conn_type (value);
  if (ML_ERROR_NONE == ml_option_get (option, "topic", &value))
    edge_info->topic = g_strdup (value);
  if (ML_ERROR_NONE == ml_option_get (option, "node-type", &value))
    edge_info->node_type = _mlrs_get_node_type (value);
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
}

/**
 * @brief Get ml remote service type from ml_option.
 */
static ml_remote_service_type_e
_mlrs_get_service_type (gchar * service_str)
{
  ml_remote_service_type_e service_type = ML_REMOTE_SERVICE_TYPE_UNKNOWN;

  if (!service_str)
    return service_type;

  if (g_ascii_strcasecmp (service_str, "model_raw") == 0) {
    service_type = ML_REMOTE_SERVICE_TYPE_MODEL_RAW;
  } else if (g_ascii_strcasecmp (service_str, "model_uri") == 0) {
    service_type = ML_REMOTE_SERVICE_TYPE_MODEL_URI;
  } else if (g_ascii_strcasecmp (service_str, "pipeline_raw") == 0) {
    service_type = ML_REMOTE_SERVICE_TYPE_PIPELINE_RAW;
  } else if (g_ascii_strcasecmp (service_str, "pipeline_uri") == 0) {
    service_type = ML_REMOTE_SERVICE_TYPE_PIPELINE_URI;
  } else {
    _ml_error_report ("Invalid service type: %s, Please check service type.",
        service_str);
  }
  return service_type;
}

/**
 * @brief Get ml remote service activation type.
 */
static gboolean
_mlrs_parse_activate (gchar * activate)
{
  gboolean ret = TRUE;

  if (g_ascii_strcasecmp (activate, "false") == 0) {
    ret = FALSE;
  }

  return ret;
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
 * @brief Register model file given by the remote sender.
 */
static gboolean
_mlrs_model_register (gchar * service_key, nns_edge_data_h data_h,
    void *data, nns_size_t data_len)
{
  guint version = -1;
  gchar *description = NULL;
  gchar *name = NULL;
  gchar *current_dir = g_get_current_dir ();
  gchar *dir_path = NULL;
  gchar *model_path = NULL;
  gchar *activate = NULL;
  gboolean active_bool = TRUE;
  GError *error = NULL;
  gboolean ret = TRUE;

  if (NNS_EDGE_ERROR_NONE != nns_edge_data_get_info (data_h, "description",
          &description)
      || NNS_EDGE_ERROR_NONE != nns_edge_data_get_info (data_h, "name", &name)
      || NNS_EDGE_ERROR_NONE != nns_edge_data_get_info (data_h, "activate",
          &activate)) {
    _ml_loge ("Failed to get info from data handle.");
    ret = FALSE;
    goto error;
  }

  active_bool = _mlrs_parse_activate (activate);

  dir_path = g_build_path ("/", current_dir, service_key, NULL);
  if (g_mkdir_with_parents (dir_path, 0755) < 0) {
    _ml_loge ("Failed to create directory %s., error: %s", dir_path,
        g_strerror (errno));
    ret = FALSE;
    goto error;
  }

  model_path = g_build_path ("/", dir_path, name, NULL);

  if (!g_file_set_contents (model_path, (char *) data, data_len, &error)) {
    _ml_loge ("Failed to write data to file: %s",
        error ? error->message : "unknown error");
    g_clear_error (&error);
    ret = FALSE;
    goto error;
  }

  /**
   * @todo Hashing the path. Where is the default path to save the model file?
   */
  if (ML_ERROR_NONE != ml_service_model_register (service_key, model_path,
          active_bool, description, &version)) {
    _ml_loge ("Failed to register model, service ket:%s", service_key);
    ret = FALSE;
  }
error:
  g_free (current_dir);
  g_free (dir_path);
  g_free (activate);
  g_free (model_path);
  g_free (description);
  g_free (name);

  return ret;
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
      _ml_loge ("curl_easy_perform failed: %s\n", curl_easy_strerror (res));
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
 * @brief Process ml remote service
 */
static int
_mlrs_process_remote_service (nns_edge_data_h data_h, void *user_data)
{
  void *data;
  nns_size_t data_len;
  g_autofree gchar *service_str = NULL;
  g_autofree gchar *service_key = NULL;
  ml_remote_service_type_e service_type;
  int ret = NNS_EDGE_ERROR_NONE;
  _ml_remote_service_s *remote_s = (_ml_remote_service_s *) user_data;
  ml_service_event_e event_type = NNS_EDGE_EVENT_TYPE_UNKNOWN;

  ret = nns_edge_data_get (data_h, 0, &data, &data_len);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report_return (ret,
        "Failed to get data while processing the ml-remote service.");
  }

  ret = nns_edge_data_get_info (data_h, "service-type", &service_str);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report_return (ret,
        "Failed to get service type while processing the ml-remote service.");
  }
  service_type = _mlrs_get_service_type (service_str);
  ret = nns_edge_data_get_info (data_h, "service-key", &service_key);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report_return (ret,
        "Failed to get service key while processing the ml-remote service.");
  }

  switch (service_type) {
    case ML_REMOTE_SERVICE_TYPE_MODEL_URI:
    {
      GByteArray *array = g_byte_array_new ();

      if (!_mlrs_get_data_from_uri ((gchar *) data, array)) {
        g_byte_array_free (array, TRUE);
        _ml_error_report_return (NNS_EDGE_ERROR_IO,
            "Failed to get data from uri: %s.", (gchar *) data);
      }
      if (_mlrs_model_register (service_key, data_h, array->data, array->len)) {
        event_type = ML_SERVICE_EVENT_MODEL_REGISTERED;
      } else {
        _ml_error_report ("Failed to register model downloaded from: %s.",
            (gchar *) data);
        ret = NNS_EDGE_ERROR_UNKNOWN;
      }
      g_byte_array_free (array, TRUE);
      break;
    }
    case ML_REMOTE_SERVICE_TYPE_MODEL_RAW:
    {
      if (_mlrs_model_register (service_key, data_h, data, data_len)) {
        event_type = ML_SERVICE_EVENT_MODEL_REGISTERED;
      } else {
        _ml_error_report ("Failed to register model downloaded from: %s.",
            (gchar *) data);
        ret = NNS_EDGE_ERROR_UNKNOWN;
      }
      break;
    }
    case ML_REMOTE_SERVICE_TYPE_PIPELINE_URI:
    {
      GByteArray *array = g_byte_array_new ();

      ret = _mlrs_get_data_from_uri ((gchar *) data, array);
      if (!ret) {
        g_byte_array_free (array, TRUE);
        _ml_error_report_return (ret,
            "Failed to get data from uri: %s.", (gchar *) data);
      }
      ret = ml_service_set_pipeline (service_key, (gchar *) array->data);
      if (ML_ERROR_NONE == ret) {
        event_type = ML_SERVICE_EVENT_PIPELINE_REGISTERED;
      }
      g_byte_array_free (array, TRUE);
      break;
    }
    case ML_REMOTE_SERVICE_TYPE_PIPELINE_RAW:
      ret = ml_service_set_pipeline (service_key, (gchar *) data);
      if (ML_ERROR_NONE == ret) {
        event_type = ML_SERVICE_EVENT_PIPELINE_REGISTERED;
      }
      break;
    default:
      _ml_error_report ("Unknown service type or not supported yet. "
          "Service num: %d", service_type);
      break;
  }

  if (remote_s && event_type != NNS_EDGE_EVENT_TYPE_UNKNOWN) {
    if (remote_s->event_cb) {
      remote_s->event_cb (event_type, remote_s->user_data);
    }
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
    case NNS_EDGE_EVENT_NEW_DATA_RECEIVED:{
      ret = nns_edge_event_parse_new_data (event_h, &data_h);
      if (NNS_EDGE_ERROR_NONE != ret)
        return ret;

      ret = _mlrs_process_remote_service (data_h, user_data);
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
_mlrs_create_edge_handle (_ml_remote_service_s * remote_s,
    edge_info_s * edge_info)
{
  int ret = 0;
  nns_edge_h edge_h = NULL;

  ret = nns_edge_create_handle (edge_info->topic, edge_info->conn_type,
      edge_info->node_type, &edge_h);

  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("nns_edge_create_handle failed.");
    return ret;
  }

  ret = nns_edge_set_event_callback (edge_h, _mlrs_edge_event_cb, remote_s);
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

  if (edge_info->node_type == NNS_EDGE_NODE_TYPE_SUB) {
    ret = nns_edge_connect (edge_h, edge_info->dest_host, edge_info->dest_port);

    if (NNS_EDGE_ERROR_NONE != ret) {
      _ml_error_report ("nns_edge_connect failed.");
      nns_edge_release_handle (edge_h);
      return ret;
    }
  }
  remote_s->edge_h = edge_h;

  return ret;
}

/**
 * @brief Internal function to release ml-service remote data.
 */
int
ml_service_remote_release_internal (void *priv)
{
  _ml_remote_service_s *mlrs = (_ml_remote_service_s *) priv;

  if (!mlrs)
    return ML_ERROR_INVALID_PARAMETER;

  nns_edge_release_handle (mlrs->edge_h);

  /** Wait some time until release the edge handle. */
  g_usleep (1000000);
  g_free (mlrs);

  return ML_ERROR_NONE;
}

/**
 * @brief Creates ml-service handle with given ml-option handle.
 */
int
ml_service_remote_create (ml_option_h option, ml_service_event_cb cb,
    void *user_data, ml_service_h * handle)
{
  ml_service_s *mls;
  _ml_remote_service_s *remote_s;
  edge_info_s *edge_info = NULL;
  int ret = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE_SERVICE);
  check_feature_state (ML_FEATURE_INFERENCE);

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }

  if (!handle) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is NULL. It should be a valid ml_service_h.");
  }

  edge_info = g_new0 (edge_info_s, 1);
  edge_info->topic = NULL;
  edge_info->host = g_strdup ("localhost");
  edge_info->port = 0;
  edge_info->dest_host = g_strdup ("localhost");
  edge_info->dest_port = 0;
  edge_info->conn_type = NNS_EDGE_CONNECT_TYPE_UNKNOWN;

  _mlrs_get_edge_info (option, edge_info);

  remote_s = g_new0 (_ml_remote_service_s, 1);
  remote_s->node_type = edge_info->node_type;
  remote_s->event_cb = cb;
  remote_s->user_data = user_data;

  ret = _mlrs_create_edge_handle (remote_s, edge_info);
  if (ML_ERROR_NONE != ret) {
    g_free (edge_info);
    g_free (remote_s);
    return ret;
  }

  mls = g_new0 (ml_service_s, 1);
  mls->type = ML_SERVICE_TYPE_REMOTE;
  mls->priv = remote_s;

  *handle = mls;

  _mlrs_release_edge_info (edge_info);
  g_free (edge_info);

  return ret;
}

/**
 *  @brief Register new information, such as neural network models or pipeline descriptions, on a remote server.
*/
int
ml_service_remote_register (ml_service_h handle, ml_option_h option, void *data,
    size_t data_len)
{
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_remote_service_s *remote_s = NULL;
  gchar *service_key = NULL;
  nns_edge_data_h data_h = NULL;
  int ret = NNS_EDGE_ERROR_NONE;
  gchar *service_str = NULL;
  gchar *description = NULL;
  gchar *name = NULL;
  gchar *activate = NULL;

  check_feature_state (ML_FEATURE_SERVICE);
  check_feature_state (ML_FEATURE_INFERENCE);

  if (!handle) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' is NULL. It should be a valid ml_service_h.");
  }

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }

  if (!data) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'data' is NULL. It should be a valid pointer.");
  }

  if (data_len <= 0) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'data_len' should be greater than 0.");
  }

  ret = ml_option_get (option, "service-type", (void **) &service_str);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report
        ("Failed to get ml-remote service type. It should be set by ml_option_set().");
    return ret;
  }
  ret = ml_option_get (option, "service-key", (void **) &service_key);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report
        ("Failed to get ml-remote service key. It should be set by ml_option_set().");
    return ret;
  }

  remote_s = (_ml_remote_service_s *) mls->priv;

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
  ret = ml_option_get (option, "description", (void **) &description);
  if (ML_ERROR_NONE != ret) {
    _ml_logi ("Failed to get option description.");
  }
  ret = nns_edge_data_set_info (data_h, "description", description);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_logi ("Failed to set description in edge data.");
  }
  ret = ml_option_get (option, "name", (void **) &name);
  if (ML_ERROR_NONE != ret) {
    _ml_logi ("Failed to get option name.");
  }
  ret = nns_edge_data_set_info (data_h, "name", name);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_logi ("Failed to set name in edge data.");
  }
  ret = ml_option_get (option, "activate", (void **) &activate);
  if (ML_ERROR_NONE != ret) {
    _ml_logi ("Failed to get option activate.");
  }
  ret = nns_edge_data_set_info (data_h, "activate", activate);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_logi ("Failed to set activate in edge data.");
  }

  ret = nns_edge_data_add (data_h, data, data_len, NULL);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report ("Failed to add camera data to the edge data.");
    goto done;
  }

  ret = nns_edge_send (remote_s->edge_h, data_h);
  if (NNS_EDGE_ERROR_NONE != ret) {
    _ml_error_report
        ("Failed to publish the data to register the remote service.");
  }

done:
  if (data_h)
    nns_edge_data_destroy (data_h);
  return ret;
}
