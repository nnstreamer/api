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

#include <stdio.h>
#include <glib.h>
#include <package_manager.h>
#include <json-glib/json-glib.h>

#include "pkg-mgr.h"
#include "service-db.hh"

static package_manager_h pkg_mgr = NULL;

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
    package_manager_event_type_e event_type,
    package_manager_event_state_e event_state, int progress,
    package_manager_error_e error, void *user_data)
{
  GDir *dir;
  gchar *pkg_path = NULL;
  package_info_h pkg_info = NULL;
  int ret;

  _I ("type: %s, package_name: %s, event_type: %d, event_state: %d",
      type, package_name, event_type, event_state);

  /* TODO: find out when this callback is called */
  if (event_type == PACKAGE_MANAGER_EVENT_TYPE_RES_COPY) {
    _I ("resource package copy is being started");
    return;
  }

  if (g_strcmp0 (type, "rpk") != 0)
    return;

  /* TODO handle allowed resources. Currently this only supports global resources */
  pkg_path = g_strdup_printf ("/opt/usr/globalapps/%s/res/global/", package_name);

  if (event_type == PACKAGE_MANAGER_EVENT_TYPE_INSTALL &&
      event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {
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
      return;
    }

    g_autofree gchar *res_version = NULL;
    ret = package_info_get_res_version (pkg_info, &res_version);
    if (ret != PACKAGE_MANAGER_ERROR_NONE) {
      _E ("package_info_get_res_version failed: %d", ret);
      return;
    }

    _I ("resource package %s is installed. res_type: %s, res_version: %s", package_name, res_type, res_version);

    if (pkg_info) {
      ret = package_info_destroy (pkg_info);
      if (ret != PACKAGE_MANAGER_ERROR_NONE) {
        _E ("package_info_destroy failed: %d", ret);
      }
    }

    /* TODO: find API to get this hardcoded path prefix (/opt/usr/globalapps/) */
    g_autofree gchar *json_file_path = g_strdup_printf ("/opt/usr/globalapps/%s/res/global/%s/model_description.json", package_name, res_type);

    if (!g_file_test (json_file_path, G_FILE_TEST_IS_REGULAR)) {
      _E ("Failed to find json file '%s'. RPK using ML Service APIs should provide this json file.", json_file_path);
      return;
    }

    /* parsing model_description.json */
    g_autoptr(JsonParser) parser = json_parser_new ();
    g_autoptr(GError) error = NULL;
    json_parser_load_from_file (parser, json_file_path, &error);
    if (error) {
      _E ("Failed to parse json file '%s': %s", json_file_path, error->message);
      return;
    }

    JsonArray *root_array = json_node_get_array (json_parser_get_root (parser));
    if (!root_array) {
      _E ("Failed to get root array from json file '%s'", json_file_path);
      return;
    }

    guint json_len = json_array_get_length (root_array);
    if (json_len == 0U) {
      _E ("Failed to get root array from json file '%s'", json_file_path);
      return;
    }

    /* For each model, register it into the database */
    MLServiceDB &db = MLServiceDB::getInstance ();
    try {
      db.connectDB ();
      for (guint i = 0; i < json_len; ++i) {
        JsonObjectIter iter;
        JsonObject *object;

        object = json_array_get_object_element (root_array, i);
        json_object_iter_init_ordered (&iter, object);

        const gchar *name = json_object_get_string_member (object, "name");
        const gchar *model = json_object_get_string_member (object, "model");
        const gchar *description = json_object_get_string_member (object, "description");

        if (!name || !model || !description) {
          _E ("Failed to get name, model, or description from json file '%s'", json_file_path);
          return;
        }

        /* Fill out the app_info column of DB */
        g_autoptr(JsonBuilder) builder = json_builder_new ();
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

        g_autoptr(JsonNode) root = json_builder_get_root (builder);

        g_autoptr(JsonGenerator) gen = json_generator_new ();
        json_generator_set_root (gen, root);
        json_generator_set_pretty (gen, TRUE);
        g_autofree gchar *app_info = json_generator_to_data (gen, NULL);

        guint version;
        db.set_model (name, model, true, description, app_info, &version);
        _I ("The model with app_info (%s) is registered as version %u", app_info, version);
      }
    } catch (const std::exception &e) {
      _E ("%s", e.what ());
    }

    db.disconnectDB ();

  } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL &&
      event_state == PACKAGE_MANAGER_EVENT_STATE_STARTED) {

    _I ("resource package %s is being uninstalled", package_name);
    /* TODO Need to invalid model */
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
  } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UPDATE &&
      event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {
    _I ("resource package %s is updated", package_name);

    /* TODO Need to update database */
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
  } else {
    /* Do not consider other events: do nothing */
  }

  g_free (pkg_path);
  pkg_path = NULL;
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
  ret = package_manager_set_event_status (pkg_mgr,
      PACKAGE_MANAGER_STATUS_TYPE_INSTALL |
      PACKAGE_MANAGER_STATUS_TYPE_UNINSTALL |
      PACKAGE_MANAGER_STATUS_TYPE_UPGRADE |
      PACKAGE_MANAGER_STATUS_TYPE_RES_COPY | /* TODO: Find when these RES_* status called */
      PACKAGE_MANAGER_STATUS_TYPE_RES_CREATE_DIR |
      PACKAGE_MANAGER_STATUS_TYPE_RES_REMOVE |
      PACKAGE_MANAGER_STATUS_TYPE_RES_UNINSTALL
      );
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
