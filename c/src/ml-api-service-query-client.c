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
 * @brief Structure for ml_service_query
 */
typedef struct
{
  ml_pipeline_h pipe_h;
  ml_pipeline_src_h src_h;
  ml_pipeline_sink_h sink_h;

  guint timeout; /**< in ms unit */
  GAsyncQueue *out_data_queue;
} _ml_service_query_s;

/**
 * @brief Sink callback for query_client
 */
static void
_sink_callback_for_query_client (const ml_tensors_data_h data,
    const ml_tensors_info_h info, void *user_data)
{
  _ml_service_query_s *mls = (_ml_service_query_s *) user_data;
  ml_tensors_data_h copied;
  int status;

  status = ml_tensors_data_clone (data, &copied);
  if (ML_ERROR_NONE != status) {
    _ml_error_report_continue
        ("Failed to create a new tensors data for query_client.");
    return;
  }

  g_async_queue_push (mls->out_data_queue, copied);
}

/**
 * @brief Internal function to release ml-service query data.
 */
int
_ml_service_query_release_internal (ml_service_s * mls)
{
  _ml_service_query_s *query = (_ml_service_query_s *) mls->priv;
  ml_tensors_data_h data_h;

  /* Supposed internal function call to release handle. */
  if (!query)
    return ML_ERROR_NONE;

  if (query->pipe_h) {
    if (ml_pipeline_destroy (query->pipe_h))
      _ml_error_report ("Failed to destroy pipeline");
  }

  if (query->out_data_queue) {
    while ((data_h = g_async_queue_try_pop (query->out_data_queue))) {
      ml_tensors_data_destroy (data_h);
    }

    g_async_queue_unref (query->out_data_queue);
  }

  g_free (query);
  mls->priv = NULL;

  return ML_ERROR_NONE;
}

/**
 * @brief Creates query client service handle with given ml-option handle.
 */
int
ml_service_query_create (ml_option_h option, ml_service_h * handle)
{
  int status = ML_ERROR_NONE;

  g_autofree gchar *description = NULL;
  void *value;

  GString *tensor_query_client_prop;
  g_autofree gchar *prop = NULL;

  ml_service_s *mls;

  _ml_service_query_s *query_s;
  ml_pipeline_h pipe_h;
  ml_pipeline_src_h src_h;
  ml_pipeline_sink_h sink_h;
  g_autofree gchar *caps = NULL;
  guint timeout = 1000U;        /* default 1s timeout */

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

  mls = _ml_service_create_internal (ML_SERVICE_TYPE_CLIENT_QUERY);
  if (mls == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service handle. Out of memory?");
  }

  mls->priv = query_s = g_try_new0 (_ml_service_query_s, 1);
  if (query_s == NULL) {
    _ml_service_destroy_internal (mls);
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service handle's private data. Out of memory?");
  }

  tensor_query_client_prop = g_string_new (NULL);

  if (ML_ERROR_NONE == ml_option_get (option, "host", &value))
    g_string_append_printf (tensor_query_client_prop, " host=%s ",
        (gchar *) value);

  if (ML_ERROR_NONE == ml_option_get (option, "port", &value))
    g_string_append_printf (tensor_query_client_prop, " port=%u ",
        *((guint *) value));

  if (ML_ERROR_NONE == ml_option_get (option, "dest-host", &value))
    g_string_append_printf (tensor_query_client_prop, " dest-host=%s ",
        (gchar *) value);

  if (ML_ERROR_NONE == ml_option_get (option, "dest-port", &value))
    g_string_append_printf (tensor_query_client_prop, " dest-port=%u ",
        *((guint *) value));

  if (ML_ERROR_NONE == ml_option_get (option, "connect-type", &value))
    g_string_append_printf (tensor_query_client_prop, " connect-type=%s ",
        (gchar *) value);

  if (ML_ERROR_NONE == ml_option_get (option, "topic", &value))
    g_string_append_printf (tensor_query_client_prop, " topic=%s ",
        (gchar *) value);

  if (ML_ERROR_NONE == ml_option_get (option, "timeout", &value))
    g_string_append_printf (tensor_query_client_prop, " timeout=%u ",
        *((guint *) value));

  if (ML_ERROR_NONE != ml_option_get (option, "caps", &value)) {
    g_string_free (tensor_query_client_prop, TRUE);
    _ml_service_destroy_internal (mls);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The option 'caps' must be set before call ml_service_query_create.");
  }
  caps = g_strdup ((gchar *) value);

  prop = g_string_free (tensor_query_client_prop, FALSE);
  description =
      g_strdup_printf
      ("appsrc name=srcx ! %s ! tensor_query_client %s name=qcx ! tensor_sink name=sinkx async=false sync=false",
      caps, prop);

  status = ml_pipeline_construct (description, NULL, NULL, &pipe_h);
  if (ML_ERROR_NONE != status) {
    _ml_service_destroy_internal (mls);
    _ml_error_report_return (status, "Failed to construct pipeline");
  }

  status = ml_pipeline_start (pipe_h);
  if (ML_ERROR_NONE != status) {
    ml_pipeline_destroy (pipe_h);
    _ml_service_destroy_internal (mls);
    _ml_error_report_return (status, "Failed to start pipeline");
  }

  status = ml_pipeline_src_get_handle (pipe_h, "srcx", &src_h);
  if (ML_ERROR_NONE != status) {
    ml_pipeline_destroy (pipe_h);
    _ml_service_destroy_internal (mls);
    _ml_error_report_return (status, "Failed to get src handle");
  }

  status = ml_pipeline_sink_register (pipe_h, "sinkx",
      _sink_callback_for_query_client, query_s, &sink_h);
  if (ML_ERROR_NONE != status) {
    ml_pipeline_destroy (pipe_h);
    _ml_service_destroy_internal (mls);
    _ml_error_report_return (status, "Failed to register sink handle");
  }

  query_s->timeout = timeout;
  query_s->pipe_h = pipe_h;
  query_s->src_h = src_h;
  query_s->sink_h = sink_h;
  query_s->out_data_queue = g_async_queue_new ();

  *handle = mls;

  return ML_ERROR_NONE;
}

/**
 * @brief Requests query client service an output with given input data.
 */
int
ml_service_query_request (ml_service_h handle, const ml_tensors_data_h input,
    ml_tensors_data_h * output)
{
  int status = ML_ERROR_NONE;
  ml_service_s *mls = (ml_service_s *) handle;
  _ml_service_query_s *query;

  check_feature_state (ML_FEATURE_SERVICE);
  check_feature_state (ML_FEATURE_INFERENCE);

  if (!_ml_service_handle_is_valid (mls))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  if (!input)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'input' (ml_tensors_data_h), is NULL. It should be a valid ml_tensors_data_h.");
  if (!output)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'output' (ml_tensors_data_h *), is NULL. It should be a valid pointer to an instance of ml_tensors_data_h.");

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
