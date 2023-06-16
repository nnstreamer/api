/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-query-client.c
 * @date 30 Aug 2022
 * @brief Query client implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/app/app.h>
#include <string.h>

#include "ml-api-internal.h"
#include "ml-api-service.h"
#include "ml-api-service-private.h"

/**
 * @brief Sink callback for query_client
 */
static void
_sink_callback_for_query_client (const ml_tensors_data_h data,
    const ml_tensors_info_h info, void *user_data)
{
  _ml_service_query_s *mls = (_ml_service_query_s *) user_data;
  ml_tensors_data_s *data_s = (ml_tensors_data_s *) data;
  ml_tensors_data_h copied_data = NULL;
  ml_tensors_data_s *_copied_data_s;

  guint i, count = 0U;
  int status;

  status = ml_tensors_data_create (info, &copied_data);
  if (ML_ERROR_NONE != status) {
    _ml_error_report_continue
        ("Failed to create a new tensors data for query_client.");
    return;
  }
  _copied_data_s = (ml_tensors_data_s *) copied_data;

  status = ml_tensors_info_get_count (info, &count);
  if (ML_ERROR_NONE != status) {
    _ml_error_report_continue
        ("Failed to get count of tensors info from tensor_sink.");
    return;
  }

  for (i = 0; i < count; ++i) {
    memcpy (_copied_data_s->tensors[i].tensor, data_s->tensors[i].tensor,
        data_s->tensors[i].size);
  }

  g_async_queue_push (mls->out_data_queue, copied_data);
}

/**
 * @brief Creates query client service handle with given ml-option handle.
 */
int
ml_service_query_create (ml_option_h option, ml_service_h * h)
{
  int status = ML_ERROR_NONE;

  gchar *description = NULL;

  ml_option_s *_option;
  GHashTableIter iter;
  gchar *key;
  ml_option_value_s *_option_value;

  GString *tensor_query_client_prop;
  gchar *prop = NULL;

  ml_service_s *mls;

  _ml_service_query_s *query_s;
  ml_pipeline_h pipe_h;
  ml_pipeline_src_h src_h;
  ml_pipeline_sink_h sink_h;
  gchar *caps = NULL;
  guint timeout = 1000U;        /* default 1s timeout */

  check_feature_state (ML_FEATURE_SERVICE);
  check_feature_state (ML_FEATURE_INFERENCE);

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }

  if (!h) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' (ml_service_h), is NULL. It should be a valid ml_service_h.");
  }

  _option = (ml_option_s *) option;
  g_hash_table_iter_init (&iter, _option->option_table);
  tensor_query_client_prop = g_string_new (NULL);
  while (g_hash_table_iter_next (&iter, (gpointer *) & key,
          (gpointer *) & _option_value)) {
    if (0 == g_ascii_strcasecmp (key, "host")) {
      g_string_append_printf (tensor_query_client_prop, " host=%s ",
          (gchar *) _option_value->value);
    } else if (0 == g_ascii_strcasecmp (key, "port")) {
      g_string_append_printf (tensor_query_client_prop, " port=%u ",
          *((guint *) _option_value->value));
    } else if (0 == g_ascii_strcasecmp (key, "dest-host")) {
      g_string_append_printf (tensor_query_client_prop, " dest-host=%s ",
          (gchar *) _option_value->value);
    } else if (0 == g_ascii_strcasecmp (key, "dest-port")) {
      g_string_append_printf (tensor_query_client_prop, " dest-port=%u ",
          *((guint *) _option_value->value));
    } else if (0 == g_ascii_strcasecmp (key, "connect-type")) {
      g_string_append_printf (tensor_query_client_prop, " connect-type=%s ",
          (gchar *) _option_value->value);
    } else if (0 == g_ascii_strcasecmp (key, "topic")) {
      g_string_append_printf (tensor_query_client_prop, " topic=%s ",
          (gchar *) _option_value->value);
    } else if (0 == g_ascii_strcasecmp (key, "timeout")) {
      g_string_append_printf (tensor_query_client_prop, " timeout=%u ",
          *((guint *) _option_value->value));
      timeout = *((guint *) _option_value->value);
    } else if (0 == g_ascii_strcasecmp (key, "caps")) {
      caps = g_strdup (_option_value->value);
    } else {
      _ml_logw ("Ignore unknown key for ml_option: %s", key);
    }
  }

  if (!caps) {
    g_string_free (tensor_query_client_prop, TRUE);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The option 'caps' must be set before call ml_service_query_create.");
  }

  prop = g_string_free (tensor_query_client_prop, FALSE);
  description =
      g_strdup_printf
      ("appsrc name=srcx ! %s ! tensor_query_client %s name=qcx ! tensor_sink name=sinkx async=false sync=false",
      caps, prop);

  g_free (caps);
  g_free (prop);

  status = ml_pipeline_construct (description, NULL, NULL, &pipe_h);
  g_free (description);
  if (ML_ERROR_NONE != status) {
    _ml_error_report_return (status, "Failed to construct pipeline");
  }

  status = ml_pipeline_start (pipe_h);
  if (ML_ERROR_NONE != status) {
    _ml_error_report ("Failed to start pipeline");
    ml_pipeline_destroy (pipe_h);
    return status;
  }

  status = ml_pipeline_src_get_handle (pipe_h, "srcx", &src_h);
  if (ML_ERROR_NONE != status) {
    ml_pipeline_destroy (pipe_h);
    _ml_error_report_return (status, "Failed to get src handle");
  }

  query_s = g_new0 (_ml_service_query_s, 1);
  status = ml_pipeline_sink_register (pipe_h, "sinkx",
      _sink_callback_for_query_client, query_s, &sink_h);
  if (ML_ERROR_NONE != status) {
    ml_pipeline_destroy (pipe_h);
    g_free (query_s);
    _ml_error_report_return (status, "Failed to register sink handle");
  }

  query_s->timeout = timeout;
  query_s->pipe_h = pipe_h;
  query_s->src_h = src_h;
  query_s->sink_h = sink_h;
  query_s->out_data_queue = g_async_queue_new ();

  mls = g_new0 (ml_service_s, 1);
  mls->type = ML_SERVICE_TYPE_CLIENT_QUERY;
  mls->priv = query_s;

  *h = mls;

  return ML_ERROR_NONE;
}

/**
 * @brief Requests query client service an output with given input data.
 */
int
ml_service_query_request (ml_service_h h, const ml_tensors_data_h input,
    ml_tensors_data_h * output)
{
  int status = ML_ERROR_NONE;
  ml_service_s *mls = (ml_service_s *) h;
  _ml_service_query_s *query;

  check_feature_state (ML_FEATURE_SERVICE);
  check_feature_state (ML_FEATURE_INFERENCE);

  if (!h)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");

  query = (_ml_service_query_s *) mls->priv;

  status = ml_pipeline_src_input_data (query->src_h, input,
      ML_PIPELINE_BUF_POLICY_DO_NOT_FREE);
  if (ML_ERROR_NONE != status) {
    _ml_error_report_return (status, "Failed to input data");
  }

  *output = g_async_queue_timeout_pop (query->out_data_queue,
      query->timeout * G_TIME_SPAN_MILLISECOND);
  if (NULL == *output) {
    _ml_error_report_return (ML_ERROR_TIMED_OUT, "timeout!");
  }

  return ML_ERROR_NONE;
}
