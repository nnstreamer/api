/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file    pkg-mgr.cc
 * @date    16 Feb 2023
 * @brief   NNStreamer/Utilities C-API Wrapper.
 * @see	    https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include <package_manager.h>
#include <stdio.h>

#include "pkg-mgr.h"
#include "service-db.hh"

/**
 * @brief Internal enumeration for data types of json.
 */
typedef enum {
  MLSVC_JSON_MODEL = 0,
  MLSVC_JSON_PIPELINE = 1,
  MLSVC_JSON_RESOURCE = 2,
  MLSVC_JSON_MAX
} mlsvc_json_type_e;

/**
 * @brief Global handle for Tizen package manager.
 */
static package_manager_h pkg_mgr = NULL;

/**
 * @brief Get app-info json string.
 */
static gchar *
_get_app_info (const gchar *package_name, const gchar *res_type, const gchar *res_version)
{
  g_autoptr (JsonBuilder) builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "is_rpk");
  json_builder_add_string_value (builder, "T");

  json_builder_set_member_name (builder, "app_id");
  json_builder_add_string_value (builder, package_name);

  json_builder_set_member_name (builder, "res_type");
  json_builder_add_string_value (builder, res_type);

  json_builder_set_member_name (builder, "res_version");
  json_builder_add_string_value (builder, res_version);

  json_builder_end_object (builder);

  g_autoptr (JsonNode) root = json_builder_get_root (builder);
  g_autoptr (JsonGenerator) gen = json_generator_new ();
  json_generator_set_root (gen, root);
  json_generator_set_pretty (gen, TRUE);

  return json_generator_to_data (gen, NULL);
}

/**
 * @brief Parse json and update ml-service database.
 */
static void
_parse_json (const gchar *json_path, mlsvc_json_type_e json_type, const gchar *app_info)
{
  g_autofree gchar *json_file = NULL;

  switch (json_type) {
    case MLSVC_JSON_MODEL:
      json_file = g_build_filename (json_path, "model_description.json", NULL);
      break;
    case MLSVC_JSON_PIPELINE:
      json_file = g_build_filename (json_path, "pipeline_description.json", NULL);
      break;
    case MLSVC_JSON_RESOURCE:
      json_file = g_build_filename (json_path, "resource_description.json", NULL);
      break;
    default:
      _E ("Unknown data type '%d', internal error?", json_type);
      return;
  }

  if (!g_file_test (json_file, (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
    _W ("Failed to find json file '%s'. RPK using ML Service API should provide this json file.",
        json_file);
    return;
  }

  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) err = NULL;

  json_parser_load_from_file (parser, json_file, &err);
  if (err) {
    _E ("Failed to parse json file '%s': %s", json_file, err->message);
    return;
  }

  JsonNode *root = json_parser_get_root (parser);
  JsonArray *array = NULL;
  JsonObject *object = NULL;
  guint json_len = 1U;

  if (JSON_NODE_HOLDS_ARRAY (root)) {
    array = json_node_get_array (root);
    if (!array) {
      _E ("Failed to get root array from json file '%s'", json_file);
      return;
    }

    json_len = json_array_get_length (array);
  }

  /* Update ML service database. */
  MLServiceDB &db = MLServiceDB::getInstance ();
  try {
    db.connectDB ();

    for (guint i = 0; i < json_len; ++i) {
      if (array)
        object = json_array_get_object_element (array, i);
      else
        object = json_node_get_object (root);

      switch (json_type) {
        case MLSVC_JSON_MODEL:
          {
            const gchar *name = json_object_get_string_member (object, "name");
            const gchar *model = json_object_get_string_member (object, "model");
            const gchar *desc = json_object_get_string_member (object, "description");
            const gchar *activate = json_object_get_string_member (object, "activate");

            if (!name || !model) {
              _E ("Failed to get name or model from json file '%s'.", json_file);
              continue;
            }

            guint version;
            bool active = (activate && g_ascii_strcasecmp (activate, "true") == 0);

            db.set_model (name, model, active, desc ? desc : "",
                app_info ? app_info : "", &version);

            _I ("The model with name '%s' is registered as version '%u'.", name, version);
          }
          break;
        case MLSVC_JSON_PIPELINE:
          {
            const gchar *name = json_object_get_string_member (object, "name");
            const gchar *desc = json_object_get_string_member (object, "description");

            if (!name || !desc) {
              _E ("Failed to get name or description from json file '%s'.", json_file);
              continue;
            }

            db.set_pipeline (name, desc);

            _I ("The pipeline description with name '%s' is registered.", name);
          }
          break;
        case MLSVC_JSON_RESOURCE:
          {
            const gchar *name = json_object_get_string_member (object, "name");
            const gchar *path = json_object_get_string_member (object, "path");
            const gchar *desc = json_object_get_string_member (object, "description");

            if (!name || !path) {
              _E ("Failed to get name or path from json file '%s'.", json_file);
              continue;
            }

            db.set_resource (name, path, desc ? desc : "", app_info ? app_info : "");

            _I ("The resource with name '%s' is registered.", name);
          }
          break;
        default:
          _E ("Unknown data type '%d', internal error?", json_type);
          break;
      }
    }
  } catch (const std::exception &e) {
    _E ("%s", e.what ());
  }

  db.disconnectDB ();
}

/**
 * @brief A simple package manager event handler for temporary use
 * @param pkg_path The path where the target package is installed
 */
static inline void
_pkg_mgr_echo_pkg_path_info (const gchar *pkg_path)
{
  GDir *dir;

  if (g_file_test (pkg_path, G_FILE_TEST_IS_DIR)) {
    _I ("package path: %s", pkg_path);

    dir = g_dir_open (pkg_path, 0, NULL);
    if (dir) {
      const gchar *file_name;

      while ((file_name = g_dir_read_name (dir))) {
        _I ("- file: %s", file_name);
      }
      g_dir_close (dir);
    }
  }
}

/**
 * @brief Callback function to be invoked for resource package.
 * @param type the package type such as rpk, tpk, wgt, etc.
 * @param package_name the name of the package
 * @param event_type the type of the request such as install, uninstall, update
 * @param event_state the current state of the requests such as completed, failed
 * @param progress the progress for the request
 * @param error the error code when the package manager is failed
 * @param user_data user data to be passed
 */
static void
_pkg_mgr_event_cb (const char *type, const char *package_name,
    package_manager_event_type_e event_type, package_manager_event_state_e event_state,
    int progress, package_manager_error_e error, void *user_data)
{
  g_autofree gchar *pkg_path = NULL;
  package_info_h pkg_info = NULL;
  int ret;

  _I ("type: %s, package_name: %s, event_type: %d, event_state: %d", type,
      package_name, event_type, event_state);

  /* TODO: find out when this callback is called */
  if (event_type == PACKAGE_MANAGER_EVENT_TYPE_RES_COPY) {
    _I ("resource package copy is being started");
    return;
  }

  if (g_ascii_strcasecmp (type, "rpk") != 0)
    return;

  /**
   * @todo package path
   * 1. Handle allowed resources. Currently this only supports global resources.
   * 2. Find API to get this hardcoded path prefix (/opt/usr/globalapps/)
   */
  pkg_path = g_strdup_printf ("/opt/usr/globalapps/%s/res/global", package_name);

  if (event_type == PACKAGE_MANAGER_EVENT_TYPE_INSTALL
      && event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {
    /* Get res information from package_info APIs */
    ret = package_info_create (package_name, &pkg_info);
    if (ret != PACKAGE_MANAGER_ERROR_NONE) {
      _E ("package_info_create failed: %d", ret);
      return;
    }

    g_autofree gchar *res_type = NULL;
    ret = package_info_get_res_type (pkg_info, &res_type);
    if (ret != PACKAGE_MANAGER_ERROR_NONE) {
      _E ("package_info_get_res_type failed: %d", ret);
      ret = package_info_destroy (pkg_info);
      if (ret != PACKAGE_MANAGER_ERROR_NONE) {
        _E ("package_info_destroy failed: %d", ret);
      }
      return;
    }

    g_autofree gchar *res_version = NULL;
    ret = package_info_get_res_version (pkg_info, &res_version);
    if (ret != PACKAGE_MANAGER_ERROR_NONE) {
      _E ("package_info_get_res_version failed: %d", ret);
      ret = package_info_destroy (pkg_info);
      if (ret != PACKAGE_MANAGER_ERROR_NONE) {
        _E ("package_info_destroy failed: %d", ret);
      }
      return;
    }

    _I ("resource package %s is installed. res_type: %s, res_version: %s",
        package_name, res_type, res_version);

    if (pkg_info) {
      ret = package_info_destroy (pkg_info);
      if (ret != PACKAGE_MANAGER_ERROR_NONE) {
        _E ("package_info_destroy failed: %d", ret);
      }
    }

    g_autofree gchar *app_info = _get_app_info (package_name, res_type, res_version);
    g_autofree gchar *json_path = g_build_filename (pkg_path, res_type, NULL);

    for (gint t = MLSVC_JSON_MODEL; t < MLSVC_JSON_MAX; t++)
      _parse_json (json_path, (mlsvc_json_type_e) t, app_info);
  } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL
             && event_state == PACKAGE_MANAGER_EVENT_STATE_STARTED) {
    _I ("resource package %s is being uninstalled", package_name);
    _pkg_mgr_echo_pkg_path_info (pkg_path);
    /* TODO: Invalidate models related to the package would be uninstalled */
  } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UPDATE
             && event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {
    _I ("resource package %s is updated", package_name);
    _pkg_mgr_echo_pkg_path_info (pkg_path);
    /* TODO: Update database */
  } else {
    /* Do not consider other events: do nothing */
  }
}

/**
 * @brief Initialize the package manager handler for the resource package.
 */
int
pkg_mgr_init (void)
{
  int ret = 0;

  ret = package_manager_create (&pkg_mgr);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    _E ("package_manager_create() failed: %d", ret);
    return -1;
  }

  /* Monitoring install, uninstall and upgrade events of the resource package. */
  /** @todo Find when these ACKAGE_MANAGER_STATUS_TYPE_RES_* status called */
  ret = package_manager_set_event_status (pkg_mgr,
      PACKAGE_MANAGER_STATUS_TYPE_INSTALL | PACKAGE_MANAGER_STATUS_TYPE_UNINSTALL
          | PACKAGE_MANAGER_STATUS_TYPE_UPGRADE | PACKAGE_MANAGER_STATUS_TYPE_RES_COPY
          | PACKAGE_MANAGER_STATUS_TYPE_RES_CREATE_DIR | PACKAGE_MANAGER_STATUS_TYPE_RES_REMOVE
          | PACKAGE_MANAGER_STATUS_TYPE_RES_UNINSTALL);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    _E ("package_manager_set_event_status() failed: %d", ret);
    return -1;
  }

  ret = package_manager_set_event_cb (pkg_mgr, _pkg_mgr_event_cb, NULL);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    _E ("package_manager_set_event_cb() failed: %d", ret);
    return -1;
  }
  return 0;
}

/**
 * @brief Finalize the package manager handler for the resource package.
 */
int
pkg_mgr_deinit (void)
{
  int ret = 0;
  ret = package_manager_destroy (pkg_mgr);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    _E ("package_manager_destroy() failed: %d", ret);
    return -1;
  }
  return 0;
}
