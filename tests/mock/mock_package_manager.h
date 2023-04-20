/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (C) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file    mock_package_manager.h
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
#ifndef __MOCK_PACKAGE_MANAGER_H__
#define __MOCK_PACKAGE_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum mock_package_manager_error_e
{
  PACKAGE_MANAGER_ERROR_NONE = TIZEN_ERROR_NONE,
  PACKAGE_MANAGER_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER,
  PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY,
  PACKAGE_MANAGER_ERROR_IO_ERROR = TIZEN_ERROR_IO_ERROR,
  PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE = TIZEN_ERROR_PACKAGE_MANAGER | 0x71,
  PACKAGE_MANAGER_ERROR_SYSTEM_ERROR = TIZEN_ERROR_PACKAGE_MANAGER | 0x72,
  PACKAGE_MANAGER_ERROR_PERMISSION_DENIED = TIZEN_ERROR_PERMISSION_DENIED,
} package_manager_error_e;

typedef enum mock_package_manager_event_type_e
{
  PACKAGE_MANAGER_EVENT_TYPE_INSTALL = 0,
  PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL,
  PACKAGE_MANAGER_EVENT_TYPE_UPDATE,
  PACKAGE_MANAGER_EVENT_TYPE_MOVE,
  PACKAGE_MANAGER_EVENT_TYPE_CLEAR,
  PACKAGE_MANAGER_EVENT_TYPE_RES_COPY,
  PACKAGE_MANAGER_EVENT_TYPE_RES_CREATE_DIR,
  PACKAGE_MANAGER_EVENT_TYPE_RES_REMOVE,
  PACKAGE_MANAGER_EVENT_TYPE_RES_UNINSTALL,
} package_manager_event_type_e;

typedef enum mock_package_manager_event_state_e
{
  PACKAGE_MANAGER_EVENT_STATE_STARTED = 0,
  PACKAGE_MANAGER_EVENT_STATE_PROCESSING,
  PACKAGE_MANAGER_EVENT_STATE_COMPLETED,
  PACKAGE_MANAGER_EVENT_STATE_FAILED,
} package_manager_event_state_e;

typedef enum mock_package_manager_status_type_e
{
  PACKAGE_MANAGER_STATUS_TYPE_ALL = 0x00,
  PACKAGE_MANAGER_STATUS_TYPE_INSTALL = 0x01,
  PACKAGE_MANAGER_STATUS_TYPE_UNINSTALL = 0x02,
  PACKAGE_MANAGER_STATUS_TYPE_UPGRADE = 0x04,
  PACKAGE_MANAGER_STATUS_TYPE_MOVE = 0x08,
  PACKAGE_MANAGER_STATUS_TYPE_CLEAR_DATA = 0x10,
  PACKAGE_MANAGER_STATUS_TYPE_INSTALL_PROGRESS = 0x20,
  PACKAGE_MANAGER_STATUS_TYPE_GET_SIZE = 0x40,
  PACKAGE_MANAGER_STATUS_TYPE_RES_COPY = 0x80,
  PACKAGE_MANAGER_STATUS_TYPE_RES_CREATE_DIR = 0x100,
  PACKAGE_MANAGER_STATUS_TYPE_RES_REMOVE = 0x200,
  PACKAGE_MANAGER_STATUS_TYPE_RES_UNINSTALL = 0x400,
} package_manager_status_type_e;

typedef void (*mock_event_cb) (const char *,
    const char *,
    package_manager_event_type_e,
    package_manager_event_state_e, int, package_manager_error_e, void *);

/**
 * @brief A fake package manager handle for the mock Package Manager APIs
 */
struct mock_package_manager
{
  guint events;
  mock_event_cb event_cb;
};
typedef mock_event_cb package_manager_event_cb;
typedef struct mock_package_manager *package_manager_h;

int package_manager_create (package_manager_h * manager);
int package_manager_destroy (package_manager_h manager);
int package_manager_set_event_status (package_manager_h manager,
    int status_type);
int package_manager_set_event_cb (package_manager_h manager,
    package_manager_event_cb callback, void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MOCK_PACKAGE_MANAGER_H__ */
