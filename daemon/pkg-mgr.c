/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file    pkg-mgr.c
 * @date    16 Feb 2023
 * @brief   NNStreamer/Utilities C-API Wrapper.
 * @see	    https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 */

#include "pkg-mgr.h"

static package_manager_h pkg_mgr = NULL;

/**
 * @brief Callback function to be invoked for resource package.
 * @param type the package type such as rpk, tpk, wgt, etc.
 * @param package the name of the package
 * @param event_type the type of the request such as install, uninstall, update
 * @param event_state the current state of the requests such as completed, failed
 * @param progress the progress for the request
 * @param error the error code when the package manager is failed
 * @param user_data user data to be passed
 */
static void
_pkg_mgr_event_cb (const char *type, const char *package,
    package_manager_event_type_e event_type,
    package_manager_event_state_e event_state, int progress,
    package_manager_error_e error, void *user_data)
{
  GDir *dir;
  gchar *pkg_path = NULL;
  _I ("type: %s, package: %s, event_type: %d, event_state: %d",
      type, package, event_type, event_state);

  if (g_strcmp0 (type, "rpk") != 0)
    return;

  /* TODO Define the path of the model & xml files */
  pkg_path = g_strdup_printf ("/opt/usr/globalapps/%s/shared/res", package);

  if (event_type == PACKAGE_MANAGER_EVENT_TYPE_INSTALL &&
      event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {
    /* TODO Need to register the model into database */
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
  } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL &&
      event_state == PACKAGE_MANAGER_EVENT_STATE_STARTED) {
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
      PACKAGE_MANAGER_STATUS_TYPE_UPGRADE);
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
