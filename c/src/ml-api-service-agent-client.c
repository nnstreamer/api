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

#include <glib/gstdio.h>
#include <json-glib/json-glib.h>

#include "ml-agent-dbus-interface.h"
#include "ml-api-internal.h"
#include "ml-api-service-private.h"
#include "ml-api-service.h"

#define WARN_MSG_DPTR_SET_OVER "The memory blocks pointed by pipeline_desc will be set over with a new one.\n" \
        "It is highly suggested that `%s` before it is set."

#if defined(__TIZEN__)
#include <app_common.h>

/**
 * @brief Parse app_info and update path (for model from rpk). Only for Tizen Applications.
 */
static int
_parse_app_info_and_update_path (ml_option_h ml_info)
{
  int ret = ML_ERROR_NONE;

  gchar *app_info = NULL;
  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (GError) err = NULL;

  JsonObject *j_object;

  /* parsing app_info and fill path (for rpk) */
  ret = ml_option_get (ml_info, "app_info", (void **) &app_info);
  if (ret != ML_ERROR_NONE) {
    _ml_error_report ("Failed to get app_info from the model info.");
    return ret;
  }

  _ml_logi ("Parsing app_info: %s", app_info);

  /* parsing the app_info json string. If the model is from rpk, path should be updated. */
  parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, app_info, -1, &err)) {
    _ml_logi ("Failed to parse app_info (%s). Skip it.", err->message);
    return 0;
  }

  j_object = json_node_get_object (json_parser_get_root (parser));
  if (!j_object) {
    _ml_error_report ("Failed to get json object from the app_info. Skip it.");
    return 0;
  }

  if (g_strcmp0 (json_object_get_string_member (j_object, "is_rpk"), "T") == 0) {
    gchar *ori_path, *new_path;
    g_autofree gchar *global_resource_path;
    const gchar *res_type =
        json_object_get_string_member (j_object, "res_type");

    ret = ml_option_get (ml_info, "path", (void **) &ori_path);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report ("Failed to get path from the model info.");
      return ret;
    }

    ret =
        app_get_res_control_global_resource_path (res_type,
        &global_resource_path);
    if (ret != APP_ERROR_NONE) {
      _ml_error_report ("Failed to get global resource path.");
      ret = ML_ERROR_INVALID_PARAMETER;
      return ret;
    }

    new_path = g_strdup_printf ("%s/%s", global_resource_path, ori_path);
    ret = ml_option_set (ml_info, "path", new_path, g_free);
    if (ret != ML_ERROR_NONE) {
      _ml_error_report ("Failed to set path to the model info.");
      return ret;
    }
  } else {
    _ml_logi ("The model is not from rpk. Skip it.");
  }

  return 0;
}
#else
#define _parse_app_info_and_update_path(...) ((int) 0)
#endif

/**
 * @brief Set the pipeline description with a given name.
 */
int
ml_service_set_pipeline (const char *name, const char *pipeline_desc)
{
  int ret = ML_ERROR_NONE;
  GError *err = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string.");
  }

  if (!pipeline_desc) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'pipeline_desc' is NULL. It should be a valid string.");
  }

  ret =
      ml_agent_dbus_interface_pipeline_set_description (name, pipeline_desc,
      &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method set_pipeline (%s).",
        err ? err->message : "Unknown error");
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
  GError *err = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL, It should be a valid string");
  }

  if (pipeline_desc == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The argument for 'pipeline_desc' should not be NULL");
  }

  if (*pipeline_desc != NULL) {
    _ml_logw (WARN_MSG_DPTR_SET_OVER, "char *pipeline_desc = NULL");
    *pipeline_desc = NULL;
  }

  ret =
      ml_agent_dbus_interface_pipeline_get_description (name, pipeline_desc,
      &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method get_pipeline (%s).",
        err ? err->message : "Unknown error");
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
  GError *err = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL, It should be a valid string");
  }

  ret = ml_agent_dbus_interface_pipeline_delete (name, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method delete_pipeline (%s).",
        err ? err->message : "Unknown error");
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
  GError *err = NULL;
  ml_service_s *mls;
  _ml_service_server_s *server;

  check_feature_state (ML_FEATURE_SERVICE);

  if (h == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The argument for 'h' should not be NULL");
  }

  if (*h != NULL) {
    _ml_logw (WARN_MSG_DPTR_SET_OVER, "ml_service_h *h = NULL");
  }
  *h = NULL;

  mls = g_try_new0 (ml_service_s, 1);
  if (mls == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service handle. Out of memory?");
  }

  server = g_try_new0 (_ml_service_server_s, 1);
  if (server == NULL) {
    g_free (mls);
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the service handle's private data. "
        "Out of memory?");
  }

  ret = ml_agent_dbus_interface_pipeline_launch (name, &(server->id), &err);
  if (ret < 0) {
    g_free (server);
    g_free (mls);
    _ml_error_report ("Failed to invoke the method launch_pipeline (%s).",
        (err ? err->message : "Unknown error"));
    g_clear_error (&err);
    return ret;
  }

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
  ml_service_s *mls;
  _ml_service_server_s *server;
  GError *err = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!h) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");
  }

  mls = (ml_service_s *) h;
  server = (_ml_service_server_s *) mls->priv;
  ret = ml_agent_dbus_interface_pipeline_start (server->id, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method start_pipeline (%s).",
        err ? err->message : "Unknown error");
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
  GError *err = NULL;
  ml_service_s *mls;
  _ml_service_server_s *server;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!h) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");
  }

  mls = (ml_service_s *) h;
  server = (_ml_service_server_s *) mls->priv;
  ret = ml_agent_dbus_interface_pipeline_stop (server->id, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method stop_pipeline (%s).",
        err ? err->message : "Unknown error");
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
  GError *err = NULL;
  gint _state = ML_PIPELINE_STATE_UNKNOWN;
  ml_service_s *mls;
  _ml_service_server_s *server;

  check_feature_state (ML_FEATURE_SERVICE);

  if (NULL == state) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter 'state' should not be NULL");
  }
  *state = ML_PIPELINE_STATE_UNKNOWN;

  if (!h) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'h' is NULL. It should be a valid ml_service_h");
  }
  mls = (ml_service_s *) h;
  server = (_ml_service_server_s *) mls->priv;
  ret = ml_agent_dbus_interface_pipeline_get_state (server->id, &_state, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method get_state (%s).",
        err ? err->message : "Unknown error");
  }
  g_clear_error (&err);

  *state = (ml_pipeline_state_e) _state;
  return ret;
}

/**
 * @brief Registers new information of a neural network model.
 */
int
ml_service_model_register (const char *name, const char *path,
    const bool activate, const char *description, unsigned int *version)
{
  int ret = ML_ERROR_NONE;
  GError *err = NULL;
  g_autofree gchar *dir_name = NULL;
  g_autofree gchar *app_info = NULL;
  g_autofree gchar *app_id = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonGenerator) gen = NULL;
  GStatBuf statbuf;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string");
  }

  if (!path) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'path' is NULL. It should be a valid string");
  }

  if (NULL == version) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter 'version' should not be NULL");
  }
  *version = 0U;

  dir_name = g_path_get_dirname (path);
  ret = g_stat (dir_name, &statbuf);
  if (ret != 0) {
    _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
        "Failed to get the information of the model file '%s'.", path);
  }

  if (!g_path_is_absolute (path)
      || !g_file_test (path, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
      || g_file_test (path, G_FILE_TEST_IS_SYMLINK)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The model file '%s' is not a regular file.", path);
  }
#if defined(__TIZEN__)
  /* Tizen application info of caller should be provided as a json string */

  ret = app_get_id (&app_id);
  if (ret == APP_ERROR_INVALID_CONTEXT) {
    /* Not a Tizen APP context, e.g. gbs build test */
    _ml_logi ("Not an APP context, skip creating app_info");
    goto app_info_exit;
  }

  /**
   * @todo Check whether the given path is in the app's resource directory.
   * Below is sample code for this (unfortunately, TCT get error with it):
   * g_autofree gchar *app_resource_path = NULL;
   * g_autofree gchar *app_shared_resource_path = NULL;
   * app_resource_path = app_get_resource_path ();
   * app_shared_resource_path = app_get_shared_resource_path ();
   * if (!app_resource_path || !app_shared_resource_path) {
   *   _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
   *      "Failed to get the app resource path of the caller.");
   * }
   * if (!g_str_has_prefix (path, app_resource_path) &&
   *     !g_str_has_prefix (path, app_shared_resource_path)) {
   *   _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
   *      "The model file '%s' is not in the app's resource directory.", path);
   * }
   */

  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "is_rpk");
  json_builder_add_string_value (builder, "F");

  json_builder_set_member_name (builder, "app_id");
  json_builder_add_string_value (builder, app_id);

  json_builder_end_object (builder);

  gen = json_generator_new ();
  json_generator_set_root (gen, json_builder_get_root (builder));
  json_generator_set_pretty (gen, TRUE);

  app_info = json_generator_to_data (gen, NULL);
app_info_exit:
#endif
  ret = ml_agent_dbus_interface_model_register (name, path, activate,
      description ? description : "", app_info ? app_info : "", version, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method register (%s).",
        err ? err->message : "Unknown error");
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Updates the description of neural network model with given @a name and @a version.
 */
int
ml_service_model_update_description (const char *name,
    const unsigned int version, const char *description)
{
  int ret = ML_ERROR_NONE;
  GError *err = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string");
  }

  if (version == 0U) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'version' is 0. It should be a valid unsigned int");
  }

  if (!description) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'description' is NULL. It should be a valid string");
  }

  ret =
      ml_agent_dbus_interface_model_update_description (name, version,
      description, &err);

  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method update_description (%s).",
        err ? err->message : "Unknown error");
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Activates a neural network model with given @a name and @a version.
 */
int
ml_service_model_activate (const char *name, const unsigned int version)
{
  int ret = ML_ERROR_NONE;
  GError *err = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string");
  }

  if (version == 0U) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'version' is 0. It should be a valid unsigned int");
  }

  ret = ml_agent_dbus_interface_model_activate (name, version, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method activate (%s).",
        err ? err->message : "Unknown error");
  }
  g_clear_error (&err);

  return ret;
}

/**
 * @brief Gets the information of neural network model with given @a name and @a version.
 */
int
ml_service_model_get (const char *name, const unsigned int version,
    ml_option_h * info)
{
  int ret = ML_ERROR_NONE;
  ml_option_h _info = NULL;
  GError *err = NULL;
  g_autofree gchar *description = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string");
  }

  if (info == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The argument for 'info' should not be NULL");
  }

  if (*info != NULL) {
    _ml_logw (WARN_MSG_DPTR_SET_OVER, "ml_option_h info = NULL");
  }
  *info = NULL;

  ret = ml_agent_dbus_interface_model_get (name, version, &description, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method get_activated (%s).",
        err ? err->message : "Unknown error");
    g_clear_error (&err);
    goto error;
  }

  if (ML_ERROR_NONE != ret || !description) {
    goto error;
  }

  ret = ml_option_create (&_info);
  if (ML_ERROR_NONE != ret) {
    goto error;
  }

  /* fill ml_info */
  {
    const gchar *member_name;
    g_autoptr (JsonParser) parser = NULL;
    JsonNode *root_node;
    JsonNode *member_node;
    JsonObject *j_object;
    JsonObjectIter iter;

    parser = json_parser_new ();
    if (!parser) {
      _ml_error_report
          ("Failed to allocate memory for JsonParser. Out of memory?");
      ret = ML_ERROR_OUT_OF_MEMORY;
      goto error;
    }

    if (!json_parser_load_from_data (parser, description, -1, &err)) {
      _ml_error_report ("Failed to parse the json string. %s", err->message);
      g_clear_error (&err);
      ret = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }

    root_node = json_parser_get_root (parser);
    if (!root_node) {
      _ml_error_report ("Failed to get the root node of json string.");
      ret = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }

    j_object = json_node_get_object (root_node);
    json_object_iter_init_ordered (&iter, j_object);
    while (json_object_iter_next_ordered (&iter, &member_name, &member_node)) {
      const gchar *value =
          json_object_get_string_member (j_object, member_name);
      ml_option_set (_info, member_name, g_strdup (value), g_free);
    }
  }

  if (_parse_app_info_and_update_path (_info) != 0) {
    _ml_error_report ("Failed to parse app_info and update path.");
    ret = ML_ERROR_INVALID_PARAMETER;
    goto error;
  }

  *info = _info;

error:
  if (ret != ML_ERROR_NONE && _info) {
    ml_option_destroy (_info);
  }

  return ret;
}

/**
 * @brief Gets the information of activated neural network model with given @a name.
 */
int
ml_service_model_get_activated (const char *name, ml_option_h * info)
{
  int ret = ML_ERROR_NONE;

  ml_option_h _info = NULL;
  GError *err = NULL;
  g_autofree gchar *description = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string");
  }

  if (info == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The argument for 'info' should not be NULL");
  }

  if (*info != NULL) {
    _ml_logw (WARN_MSG_DPTR_SET_OVER, "ml_option_h info = NULL");
  }
  *info = NULL;

  ret = ml_agent_dbus_interface_model_get_activated (name, &description, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method get_activated (%s).",
        err ? err->message : "Unknown error");
    g_clear_error (&err);
    goto error;
  }

  if (ML_ERROR_NONE != ret || !description) {
    goto error;
  }

  ret = ml_option_create (&_info);
  if (ML_ERROR_NONE != ret) {
    goto error;
  }

  /* fill ml_info */
  {
    g_autoptr (JsonParser) parser = NULL;
    JsonObjectIter iter;
    JsonNode *root_node;
    JsonNode *member_node;
    const gchar *member_name;
    JsonObject *j_object;

    parser = json_parser_new ();
    if (!parser) {
      _ml_error_report
          ("Failed to allocate memory for JsonParser. Out of memory?");
      ret = ML_ERROR_OUT_OF_MEMORY;
      goto error;
    }

    if (!json_parser_load_from_data (parser, description, -1, &err)) {
      _ml_error_report ("Failed to parse the json string. %s", err->message);
      g_clear_error (&err);
      ret = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }

    root_node = json_parser_get_root (parser);
    if (!root_node) {
      _ml_error_report ("Failed to get the root node of json string.");
      ret = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }

    j_object = json_node_get_object (root_node);
    json_object_iter_init_ordered (&iter, j_object);
    while (json_object_iter_next_ordered (&iter, &member_name, &member_node)) {
      const gchar *value =
          json_object_get_string_member (j_object, member_name);

      ml_option_set (_info, member_name, g_strdup (value), g_free);
    }
  }

  if (_parse_app_info_and_update_path (_info) != 0) {
    _ml_error_report ("Failed to parse app_info and update path.");
    ret = ML_ERROR_INVALID_PARAMETER;
    goto error;
  }

  *info = _info;

error:
  if (ret != ML_ERROR_NONE && _info) {
    ml_option_destroy (_info);
  }

  return ret;
}

/**
 * @brief Gets the list of neural network model with given @a name.
 */
int
ml_service_model_get_all (const char *name, ml_option_h * info_list[],
    unsigned int *num)
{
  int ret = ML_ERROR_NONE;
  ml_option_h *_info_list = NULL;
  GError *err = NULL;
  g_autofree gchar *description = NULL;
  guint i, n;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string");
  }

  if (NULL == info_list) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter 'info_list' should not be NULL");
  }
  *info_list = NULL;

  if (NULL == num) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter 'num' should not be NULL");
  }
  *num = 0;

  ret = ml_agent_dbus_interface_model_get_all (name, &description, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method get_activated (%s).",
        err ? err->message : "Unknown error");
    g_clear_error (&err);
    goto error;
  }

  if (ML_ERROR_NONE != ret || !description) {
    goto error;
  }

  {
    g_autoptr (JsonParser) parser = NULL;
    JsonArray *array;

    parser = json_parser_new ();
    if (!parser) {
      _ml_error_report
          ("Failed to allocate memory for JsonParser. Out of memory?");
      ret = ML_ERROR_OUT_OF_MEMORY;
      goto error;
    }

    if (!json_parser_load_from_data (parser, description, -1, &err)) {
      _ml_error_report ("Failed to parse the json string. %s", err->message);
      g_clear_error (&err);
      ret = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }

    array = json_node_get_array (json_parser_get_root (parser));
    if (!array) {
      _ml_error_report ("Failed to get array from json string.");
      ret = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }

    n = json_array_get_length (array);
    if (n == 0U) {
      _ml_error_report ("Failed to get array from json string.");
      ret = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }

    _info_list = g_try_new0 (ml_option_h, n);
    if (!_info_list) {
      _ml_error_report
          ("Failed to allocate memory for list of ml_info_h. Out of memory?");
      ret = ML_ERROR_OUT_OF_MEMORY;
      goto error;
    }

    for (i = 0; i < n; i++) {
      const gchar *member_name;
      JsonObjectIter iter;
      JsonNode *member_node;
      JsonObject *object;

      if (ml_option_create (&_info_list[i]) != ML_ERROR_NONE) {
        _ml_error_report
            ("Failed to allocate memory for ml_option_h. Out of memory?");
        n = i;
        ret = ML_ERROR_OUT_OF_MEMORY;
        goto error;
      }

      object = json_array_get_object_element (array, i);
      json_object_iter_init_ordered (&iter, object);
      while (json_object_iter_next_ordered (&iter, &member_name, &member_node)) {
        const gchar *value =
            json_object_get_string_member (object, member_name);
        ml_option_set (_info_list[i], member_name, g_strdup (value), g_free);
      }

      if (_parse_app_info_and_update_path (_info_list[i]) != 0) {
        _ml_error_report ("Failed to parse app_info and update path.");
        ret = ML_ERROR_INVALID_PARAMETER;
        goto error;
      }
    }
  }

  *info_list = _info_list;
  *num = n;

error:
  if (ret != ML_ERROR_NONE) {
    if (_info_list) {
      for (i = 0; i < n; i++) {
        if (_info_list[i]) {
          ml_option_destroy (_info_list[i]);
        }
      }

      g_free (_info_list);
    }
  }

  return ret;
}

/**
 * @brief Deletes a model information with given @a name and @a version from machine learning service.
 */
int
ml_service_model_delete (const char *name, const unsigned int version)
{
  int ret = ML_ERROR_NONE;
  GError *err = NULL;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!name) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'name' is NULL. It should be a valid string");
  }

  ret = ml_agent_dbus_interface_model_delete (name, version, &err);
  if (ret < 0) {
    _ml_error_report ("Failed to invoke the method delete (%s).",
        err ? err->message : "Unknown error");
  }
  g_clear_error (&err);

  return ret;
}
