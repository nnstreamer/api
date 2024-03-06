/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service.c
 * @date 31 Aug 2022
 * @brief Some implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <nnstreamer_plugin_api_util.h>

#include "ml-api-service.h"
#include "ml-api-service-extension.h"
#include "ml-api-service-offloading.h"
#include "ml-api-service-training-offloading.h"

#define ML_SERVICE_MAGIC 0xfeeedeed
#define ML_SERVICE_MAGIC_DEAD 0xdeaddead

/**
 * @brief Internal function to validate ml-service handle.
 */
gboolean
_ml_service_handle_is_valid (ml_service_s * mls)
{
  if (!mls)
    return FALSE;

  if (mls->magic != ML_SERVICE_MAGIC)
    return FALSE;

  switch (mls->type) {
    case ML_SERVICE_TYPE_SERVER_PIPELINE:
    case ML_SERVICE_TYPE_CLIENT_QUERY:
    case ML_SERVICE_TYPE_OFFLOADING:
    case ML_SERVICE_TYPE_EXTENSION:
      if (mls->priv == NULL)
        return FALSE;
      break;
    default:
      /* Invalid handle type. */
      return FALSE;
  }

  return TRUE;
}

/**
 * @brief Internal function to set information.
 */
static int
_ml_service_set_information_internal (ml_service_s * mls, const char *name,
    const char *value)
{
  int status = ML_ERROR_NONE;

  /* Prevent empty string case. */
  if (!STR_IS_VALID (name) || !STR_IS_VALID (value))
    return ML_ERROR_INVALID_PARAMETER;

  status = ml_option_set (mls->information, name, g_strdup (value), g_free);
  if (status != ML_ERROR_NONE)
    return status;

  switch (mls->type) {
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_set_information (mls, name, value);
      break;
    case ML_SERVICE_TYPE_OFFLOADING:
      status = ml_service_offloading_set_information (mls, name, value);
      break;
    default:
      break;
  }

  return status;
}

/**
 * @brief Internal function to create new ml-service handle.
 */
ml_service_s *
_ml_service_create_internal (ml_service_type_e ml_service_type)
{
  ml_service_s *mls;
  int status;

  mls = g_try_new0 (ml_service_s, 1);
  if (mls) {
    status = ml_option_create (&mls->information);
    if (status != ML_ERROR_NONE) {
      g_free (mls);
      _ml_error_report_return (NULL,
          "Failed to create ml-option handle in ml-service.");
    }

    mls->magic = ML_SERVICE_MAGIC;
    mls->type = ml_service_type;
    g_mutex_init (&mls->lock);
    g_cond_init (&mls->cond);
  }

  return mls;
}

/**
 * @brief Internal function to release ml-service handle.
 */
int
_ml_service_destroy_internal (ml_service_s * mls)
{
  ml_service_event_cb_info_s old_cb;
  int status = ML_ERROR_NONE;

  if (!mls) {
    /* Internal error? */
    return ML_ERROR_INVALID_PARAMETER;
  }

  /* Clear callback before closing internal handles. */
  g_mutex_lock (&mls->lock);
  old_cb = mls->cb_info;
  memset (&mls->cb_info, 0, sizeof (ml_service_event_cb_info_s));
  g_mutex_unlock (&mls->lock);

  switch (mls->type) {
    case ML_SERVICE_TYPE_SERVER_PIPELINE:
      status = ml_service_pipeline_release_internal (mls);
      break;
    case ML_SERVICE_TYPE_CLIENT_QUERY:
      status = ml_service_query_release_internal (mls);
      break;
    case ML_SERVICE_TYPE_OFFLOADING:
      status = ml_service_offloading_release_internal (mls);
      break;
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_destroy (mls);
      break;
    default:
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Invalid type of ml_service_h.");
  }

  if (status == ML_ERROR_NONE) {
    mls->magic = ML_SERVICE_MAGIC_DEAD;
    ml_option_destroy (mls->information);

    g_cond_clear (&mls->cond);
    g_mutex_clear (&mls->lock);
    g_free (mls);
  } else {
    _ml_error_report ("Failed to release ml-service handle, internal error?");

    g_mutex_lock (&mls->lock);
    mls->cb_info = old_cb;
    g_mutex_unlock (&mls->lock);
  }

  return status;
}

/**
 * @brief Internal function to get ml-service event callback.
 */
void
_ml_service_get_event_cb_info (ml_service_s * mls,
    ml_service_event_cb_info_s * cb_info)
{
  if (!mls || !cb_info)
    return;

  g_mutex_lock (&mls->lock);
  *cb_info = mls->cb_info;
  g_mutex_unlock (&mls->lock);
}

/**
 * @brief Internal function to parse string value from json.
 */
int
_ml_service_conf_parse_string (JsonNode * str_node, const gchar * delimiter,
    gchar ** str)
{
  guint i, n;

  if (!str_node || !delimiter || !str)
    return ML_ERROR_INVALID_PARAMETER;

  *str = NULL;

  if (JSON_NODE_HOLDS_ARRAY (str_node)) {
    JsonArray *array = json_node_get_array (str_node);
    GString *val = g_string_new (NULL);

    n = (array) ? json_array_get_length (array) : 0U;
    for (i = 0; i < n; i++) {
      const gchar *p = json_array_get_string_element (array, i);

      g_string_append (val, p);
      if (i < n - 1)
        g_string_append (val, delimiter);
    }

    *str = g_string_free (val, FALSE);
  } else {
    *str = g_strdup (json_node_get_string (str_node));
  }

  return (*str != NULL) ? ML_ERROR_NONE : ML_ERROR_INVALID_PARAMETER;
}

/**
 * @brief Internal function to parse tensors-info from json.
 */
int
_ml_service_conf_parse_tensors_info (JsonNode * info_node,
    ml_tensors_info_h * info_h)
{
  JsonArray *array = NULL;
  JsonObject *object;
  GstTensorsInfo info;
  GstTensorInfo *_info;
  const gchar *_str;
  guint i;
  int status;

  if (!info_node || !info_h)
    return ML_ERROR_INVALID_PARAMETER;

  gst_tensors_info_init (&info);

  info.num_tensors = 1;
  if (JSON_NODE_HOLDS_ARRAY (info_node)) {
    array = json_node_get_array (info_node);
    info.num_tensors = json_array_get_length (array);
  }

  for (i = 0; i < info.num_tensors; i++) {
    _info = gst_tensors_info_get_nth_info (&info, i);

    if (array)
      object = json_array_get_object_element (array, i);
    else
      object = json_node_get_object (info_node);

    if (json_object_has_member (object, "type")) {
      _str = json_object_get_string_member (object, "type");

      if (STR_IS_VALID (_str))
        _info->type = gst_tensor_get_type (_str);
    }

    if (json_object_has_member (object, "dimension")) {
      _str = json_object_get_string_member (object, "dimension");

      if (STR_IS_VALID (_str))
        gst_tensor_parse_dimension (_str, _info->dimension);
    }

    if (json_object_has_member (object, "name")) {
      _str = json_object_get_string_member (object, "name");

      if (STR_IS_VALID (_str))
        _info->name = g_strdup (_str);
    }
  }

  if (gst_tensors_info_validate (&info))
    status = _ml_tensors_info_create_from_gst (info_h, &info);
  else
    status = ML_ERROR_INVALID_PARAMETER;

  gst_tensors_info_free (&info);
  return status;
}

/**
 * @brief Internal function to parse service info from config file.
 */
static int
_ml_service_offloading_conf_to_opt (ml_service_s * mls, JsonObject * object,
    const gchar * name, ml_option_h option)
{
  int status;
  JsonObject *offloading_object;
  const gchar *val = NULL;
  const gchar *key = NULL;
  GList *list = NULL, *iter;

  offloading_object = json_object_get_object_member (object, name);
  if (!offloading_object) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to get %s member from the config file", name);
  }

  list = json_object_get_members (offloading_object);
  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    key = iter->data;
    if (g_ascii_strcasecmp (key, "training") == 0) {
      /* It is not a value to set for option. */
      continue;
    }
    val = json_object_get_string_member (offloading_object, key);
    status = ml_option_set (option, key, g_strdup (val), g_free);
    if (status != ML_ERROR_NONE) {
      _ml_error_report ("Failed to set %s option: %s.", key, val);
      break;
    }
  }
  g_list_free (list);

  return status;
}

/**
 * @brief Internal function to parse service info from config file.
 */
static int
_ml_service_offloading_parse_services (ml_service_s * mls, JsonObject * object)
{
  GList *list, *iter;
  JsonNode *json_node = NULL;
  int status = ML_ERROR_NONE;

  list = json_object_get_members (object);
  for (iter = list; iter != NULL; iter = g_list_next (iter)) {
    const gchar *key = iter->data;
    gchar *val = NULL;

    json_node = json_object_get_member (object, key);
    val = json_to_string (json_node, TRUE);
    if (val) {
      status = ml_service_offloading_set_service (mls, key, val);
      g_free (val);

      if (status != ML_ERROR_NONE) {
        _ml_error_report ("Failed to set service key : %s", key);
        break;
      }
    }
  }
  g_list_free (list);

  return status;
}

/**
 * @brief Internal function to parse configuration file to create offloading service.
 */
static int
_ml_service_offloading_create_json (ml_service_s * mls, JsonObject * object)
{
  int status;
  ml_option_h option;

  status = ml_option_create (&option);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status, "Failed to create ml-option.");
  }

  status =
      _ml_service_offloading_conf_to_opt (mls, object, "offloading", option);
  if (status != ML_ERROR_NONE) {
    _ml_error_report ("Failed to set ml-option from config file.");
    goto done;
  }

  status = ml_service_offloading_create (mls, option);
  if (status != ML_ERROR_NONE) {
    _ml_error_report ("Failed to create ml-service-offloading.");
    goto done;
  }
  if (json_object_has_member (object, "services")) {
    JsonObject *svc_object;
    svc_object = json_object_get_object_member (object, "services");
    status = _ml_service_offloading_parse_services (mls, svc_object);
    if (status != ML_ERROR_NONE) {
      _ml_logw ("Failed to parse services from config file.");
    }
  }

  status = ml_service_training_offloading_create (mls, object);
  if (status != ML_ERROR_NONE) {
    _ml_logw ("Failed to parse training from config file.");
  }

done:
  ml_option_destroy (option);
  return status;
}

/**
 * @brief Internal function to get ml-service type.
 */
static ml_service_type_e
_ml_service_get_type (JsonObject * object)
{
  ml_service_type_e type = ML_SERVICE_TYPE_UNKNOWN;

  /** @todo add more services such as training offloading, offloading service */
  if (json_object_has_member (object, "single") ||
      json_object_has_member (object, "pipeline")) {
    type = ML_SERVICE_TYPE_EXTENSION;
  } else if (json_object_has_member (object, "offloading")) {
    type = ML_SERVICE_TYPE_OFFLOADING;
  }

  return type;
}

/**
 * @brief Creates a handle for machine learning service with configuration.
 */
int
ml_service_new (const char *config, ml_service_h * handle)
{
  ml_service_s *mls;
  ml_service_type_e service_type = ML_SERVICE_TYPE_UNKNOWN;
  g_autofree gchar *json_string = NULL;
  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (GError) err = NULL;
  JsonNode *root;
  JsonObject *object;
  int status;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!handle) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is NULL. It should be a valid pointer to create new instance.");
  }

  /* Init null. */
  *handle = NULL;

  if (!STR_IS_VALID (config) ||
      !g_file_test (config, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, config, is invalid. It should be a valid path.");
  }

  if (!g_file_get_contents (config, &json_string, NULL, NULL)) {
    _ml_error_report_return (ML_ERROR_IO_ERROR,
        "Failed to read configuration file '%s'.", config);
  }

  parser = json_parser_new ();
  if (!parser) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to parse configuration file, cannot allocate memory for JsonParser. Out of memory?");
  }

  if (!json_parser_load_from_data (parser, json_string, -1, &err)) {
    _ml_error_report_return (ML_ERROR_IO_ERROR,
        "Failed to parse configuration file, cannot load json string (%s).",
        err ? err->message : "Unknown error");
  }

  root = json_parser_get_root (parser);
  if (!root) {
    _ml_error_report_return (ML_ERROR_IO_ERROR,
        "Failed to parse configuration file, cannot get the top node from json string.");
  }

  object = json_node_get_object (root);

  service_type = _ml_service_get_type (object);
  if (ML_SERVICE_TYPE_UNKNOWN == service_type) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to parse configuration file, cannot get the valid type from configuration.");
  }

  /* Parse each service type. */
  mls = _ml_service_create_internal (service_type);
  if (mls == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the ml-service handle. Out of memory?");
  }

  switch (service_type) {
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_create (mls, object);
      break;
    case ML_SERVICE_TYPE_OFFLOADING:
      status = _ml_service_offloading_create_json (mls, object);
      break;
    default:
      /* Invalid handle type. */
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  if (status != ML_ERROR_NONE)
    goto error;

  /* Parse information. */
  if (json_object_has_member (object, "information")) {
    JsonObject *info = json_object_get_object_member (object, "information");
    g_autoptr (GList) members = json_object_get_members (info);
    GList *iter;

    for (iter = members; iter; iter = g_list_next (iter)) {
      const gchar *name = iter->data;
      const gchar *value = json_object_get_string_member (info, name);

      status = _ml_service_set_information_internal (mls, name, value);
      if (status != ML_ERROR_NONE)
        goto error;
    }
  }

error:
  if (status == ML_ERROR_NONE) {
    *handle = mls;
  } else {
    _ml_error_report ("Failed to open the ml-service configuration.");
    _ml_service_destroy_internal (mls);
  }

  return status;
}

/**
 * @brief Sets the callbacks which will be invoked when a new event occurs from ml-service.
 */
int
ml_service_set_event_cb (ml_service_h handle, ml_service_event_cb cb,
    void *user_data)
{
  ml_service_s *mls = (ml_service_s *) handle;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  g_mutex_lock (&mls->lock);

  mls->cb_info.cb = cb;
  mls->cb_info.pdata = user_data;

  g_mutex_unlock (&mls->lock);

  return ML_ERROR_NONE;
}

/**
 * @brief Starts the process of ml-service.
 */
int
ml_service_start (ml_service_h handle)
{
  ml_service_s *mls = (ml_service_s *) handle;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  switch (mls->type) {
    case ML_SERVICE_TYPE_SERVER_PIPELINE:
    {
      _ml_service_server_s *server = (_ml_service_server_s *) mls->priv;

      status = ml_agent_pipeline_start (server->id);
      if (status < 0)
        _ml_error_report ("Failed to invoke the method start_pipeline.");

      break;
    }
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_start (mls);
      break;
    case ML_SERVICE_TYPE_OFFLOADING:
      status = ml_service_offloading_start (mls);
      break;
    default:
      /* Invalid handle type. */
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  return status;
}

/**
 * @brief Stops the process of ml-service.
 */
int
ml_service_stop (ml_service_h handle)
{
  ml_service_s *mls = (ml_service_s *) handle;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  switch (mls->type) {
    case ML_SERVICE_TYPE_SERVER_PIPELINE:
    {
      _ml_service_server_s *server = (_ml_service_server_s *) mls->priv;

      status = ml_agent_pipeline_stop (server->id);
      if (status < 0)
        _ml_error_report ("Failed to invoke the method stop_pipeline.");

      break;
    }
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_stop (mls);
      break;
    case ML_SERVICE_TYPE_OFFLOADING:
      status = ml_service_offloading_stop (mls);
      break;
    default:
      /* Invalid handle type. */
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  return status;
}

/**
 * @brief Gets the information of required input data.
 */
int
ml_service_get_input_information (ml_service_h handle, const char *name,
    ml_tensors_info_h * info)
{
  ml_service_s *mls = (ml_service_s *) handle;
  int status;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  if (!info) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info (ml_tensors_info_h), is NULL. It should be a valid pointer to create new instance.");
  }

  /* Init null. */
  *info = NULL;

  switch (mls->type) {
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_get_input_information (mls, name, info);
      break;
    default:
      /* Invalid handle type. */
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
 * @brief Gets the information of output data.
 */
int
ml_service_get_output_information (ml_service_h handle, const char *name,
    ml_tensors_info_h * info)
{
  ml_service_s *mls = (ml_service_s *) handle;
  int status;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  if (!info) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info (ml_tensors_info_h), is NULL. It should be a valid pointer to create new instance.");
  }

  /* Init null. */
  *info = NULL;

  switch (mls->type) {
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_get_output_information (mls, name, info);
      break;
    default:
      /* Invalid handle type. */
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
 * @brief Sets the information for ml-service.
 */
int
ml_service_set_information (ml_service_h handle, const char *name,
    const char *value)
{
  ml_service_s *mls = (ml_service_s *) handle;
  int status;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  if (!STR_IS_VALID (name)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, name '%s', is invalid.", name);
  }

  if (!STR_IS_VALID (value)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, value '%s', is invalid.", value);
  }

  g_mutex_lock (&mls->lock);
  status = _ml_service_set_information_internal (mls, name, value);
  g_mutex_unlock (&mls->lock);

  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status,
        "Failed to set the information '%s'.", name);
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Gets the information from ml-service.
 */
int
ml_service_get_information (ml_service_h handle, const char *name, char **value)
{
  ml_service_s *mls = (ml_service_s *) handle;
  gchar *val = NULL;
  int status;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  if (!STR_IS_VALID (name)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, name '%s', is invalid.", name);
  }

  if (!value) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, value, is NULL. It should be a valid pointer.");
  }

  g_mutex_lock (&mls->lock);
  status = ml_option_get (mls->information, name, (void **) (&val));
  g_mutex_unlock (&mls->lock);

  if (status != ML_ERROR_NONE) {
    _ml_error_report_return (status,
        "The ml-service handle does not include the information '%s'.", name);
  }

  *value = g_strdup (val);
  return ML_ERROR_NONE;
}

/**
 * @brief Adds an input data to process the model in ml-service extension handle.
 */
int
ml_service_request (ml_service_h handle, const char *name,
    const ml_tensors_data_h data)
{
  ml_service_s *mls = (ml_service_s *) handle;
  int status;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  if (!data) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data (ml_tensors_data_h), is NULL. It should be a valid ml_tensor_data_h instance, which is usually created by ml_tensors_data_create().");
  }

  switch (mls->type) {
    case ML_SERVICE_TYPE_EXTENSION:
      status = ml_service_extension_request (mls, name, data);
      break;
    case ML_SERVICE_TYPE_OFFLOADING:
      status = ml_service_offloading_request (mls, name, data);
      break;
    default:
      /* Invalid handle type. */
      status = ML_ERROR_NOT_SUPPORTED;
      break;
  }

  return status;
}

/**
 * @brief Destroys the handle for machine learning service.
 */
int
ml_service_destroy (ml_service_h handle)
{
  ml_service_s *mls = (ml_service_s *) handle;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance, which is usually created by ml_service_new().");
  }

  return _ml_service_destroy_internal (mls);
}
