/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (C) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file    mock_package_manager.c
 * @date    18 Apr 2023
 * @brief   A set of helper functions for emulation of Tizen's Package Manager APIs
 * @see     https://github.com/nnstreamer/api
 * @author  Wook Song <wook16.song@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *  This provides helper functions for the emulation of Tizen's Package Manager APIs
 *  and utilities for unit test cases of ML Agent Service APIs.
 */

#include <glib-2.0/glib.h>

#include "mock_package_manager.h"

/**
 * @brief A mock implementation of the package_manager_create function
 * @see https://docs.tizen.org/application/native/guides/app-management/package-manager/
 */
int
package_manager_create (package_manager_h * manager)
{
  struct mock_package_manager *_manager;

  if (manager == NULL) {
    return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;
  }

  _manager = g_try_new0 (struct mock_package_manager, 1);
  if (_manager == NULL) {
    return PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY;
  }

  _manager->events = -1;
  _manager->event_cb = NULL;
  *manager = _manager;

  return PACKAGE_MANAGER_ERROR_NONE;
}

/**
 * @brief A mock implementation of the package_manager_destroy function
 * @see https://docs.tizen.org/application/native/guides/app-management/package-manager/
 */
int
package_manager_destroy (package_manager_h manager)
{
  if (manager == NULL)
    return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

  g_free(manager);
  return PACKAGE_MANAGER_ERROR_NONE;
}

/**
 * @brief A mock implementation of the package_manager_set_event_status function
 * @see https://docs.tizen.org/application/native/guides/app-management/package-manager/
 */
int
package_manager_set_event_status (package_manager_h manager, int status_type)
{
  if (!manager || status_type > 0x7FF || status_type < 0) {
    return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;
  }

  manager->events = status_type;
  return PACKAGE_MANAGER_ERROR_NONE;
}

/**
 * @brief A mock implementation of the package_manager_set_event_cb function
 * @see https://docs.tizen.org/application/native/guides/app-management/package-manager/
 */
int
package_manager_set_event_cb (package_manager_h manager,
    package_manager_event_cb callback, void *user_data)
{
  if (!callback)
    return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

  manager->event_cb = callback;

  return PACKAGE_MANAGER_ERROR_NONE;
}
