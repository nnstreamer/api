/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    gdbus-util.h
 * @date    25 June 2022
 * @brief   Internal GDbus utility header of Machine Learning agent daemon
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This provides the wrapper functions to use DBus easily.
 */
#ifndef __GDBUS_UTIL_H__
#define __GDBUS_UTIL_H__

#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief DBus signal handler information to connect
 */
struct gdbus_signal_info
{
  const gchar *signal_name; /**< specific signal name to handle */
  GCallback cb;         /**< Callback function to connect */
  gpointer cb_data;     /**< Data to pass to callback function */
  gulong handler_id;    /**< Connected handler ID */
};

/**
 * @brief Export the DBus interface at the Object path on the bus connection.
 * @param instance The instance of the DBus interface to export.
 * @param obj_path The path to export the interface at.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int gdbus_export_interface (gpointer instance, const char *obj_path);

/**
 * @brief Acquire the given name on the SYSTEM session of the DBus message bus.
 * @remarks If the name is acquired, 'READY=1' signal will be sent to the systemd.
 * @param name The well-known name to own.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int gdbus_get_name (const char *name);

/**
 * @brief Connects the callback functions for each signal of the particular DBus interface.
 * @param instance The instance of the DBus interface.
 * @param num_signals The number of signals to connect.
 * @param signal_infos The array of DBus signal handler.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int gdbus_connect_signal (gpointer instance, int num_signals,
    struct gdbus_signal_info *signal_infos);

/**
 * @brief Disconnects the callback functions from the particular DBus interface.
 * @param instance The instance of the DBus interface.
 * @param num_signals The number of signals to connect.
 * @param signal_infos The array of DBus signal handler.
 */
void gdbus_disconnect_signal (gpointer instance, int num_signals,
    struct gdbus_signal_info *signal_infos);

/**
 * @brief Connect to the DBus message bus
 * @param is_session True is DBus Bus type is session.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int gdbus_get_system_connection (gboolean is_session);

/**
 * @brief Disconnect the DBus message bus.
 */
void gdbus_put_system_connection (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GDBUS_UTIL_H__ */
