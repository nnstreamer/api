/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file gdbus-util.c
 * @date 25 June 2022
 * @brief Internal GDbus utility wrapper of Machine Learning agent daemon
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <errno.h>
#include <stdbool.h>
#include <systemd/sd-daemon.h>

#include <gdbus-util.h>
#include <log.h>

static GDBusConnection *g_dbus_sys_conn = NULL;

/**
 * @brief Export the DBus interface at the Object path on the bus connection.
 */
int
gdbus_export_interface (gpointer instance, const char *obj_path)
{
  if (g_dbus_sys_conn == NULL) {
    _E ("cannot get the dbus connection to the system message bus\n");
    return -ENOSYS;
  }

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (instance),
          g_dbus_sys_conn, obj_path, NULL)) {
    return -EBUSY;
  }

  return 0;
}

/**
 * @brief Callback function for acquireing the bus name.
 * @remarks If the daemon is launched by systemd service,
 * it should notify to the systemd about its status when it is ready.
 */
static void
name_acquired_cb (GDBusConnection * connection,
    const gchar * name, gpointer user_data)
{
  sd_notify (0, "READY=1");
}

/**
 * @brief Acquire the given name on the SYSTEM session of the DBus message bus.
 */
int
gdbus_get_name (const char *name)
{
  guint id;

  id = g_bus_own_name_on_connection (g_dbus_sys_conn, name,
      G_BUS_NAME_OWNER_FLAGS_NONE, name_acquired_cb, NULL, NULL, NULL);
  if (id == 0)
    return -ENOSYS;

  return 0;
}

/**
 * @brief Connects the callback functions for each signal of the particular DBus interface.
 */
int
gdbus_connect_signal (gpointer instance, int num_signals,
    struct gdbus_signal_info *signal_infos)
{
  int i;
  unsigned long handler_id;

  for (i = 0; i < num_signals; i++) {
    handler_id = g_signal_connect (instance,
        signal_infos[i].signal_name,
        signal_infos[i].cb, signal_infos[i].cb_data);
    if (handler_id <= 0)
      goto out_err;
    signal_infos[i].handler_id = handler_id;
  }
  return 0;

out_err:
  for (i = 0; i < num_signals; i++) {
    if (signal_infos[i].handler_id > 0) {
      g_signal_handler_disconnect (instance, signal_infos[i].handler_id);
      signal_infos[i].handler_id = 0;
    }
  }

  return -EINVAL;
}

/**
 * @brief Disconnects the callback functions from the particular DBus interface.
 */
void
gdbus_disconnect_signal (gpointer instance, int num_signals,
    struct gdbus_signal_info *signal_infos)
{
  int i;

  for (i = 0; i < num_signals; i++) {
    if (signal_infos[i].handler_id > 0) {
      g_signal_handler_disconnect (instance, signal_infos[i].handler_id);
      signal_infos[i].handler_id = 0;
    }
  }
}

/**
 * @brief Cleanup the instance of the DBus interface.
 */
static void
put_instance (gpointer * instance)
{
  g_object_unref (*instance);
  *instance = NULL;
}

/**
 * @brief Get the skeleton object of the DBus interface.
 */
MachinelearningServicePipeline *
gdbus_get_instance_pipeline (void)
{
  return machinelearning_service_pipeline_skeleton_new ();
}

/**
 * @brief Put the obtained skeleton object and release the resource.
 */
void
gdbus_put_instance_pipeline (MachinelearningServicePipeline ** instance)
{
  put_instance ((gpointer *) instance);
}

/**
 * @brief Connect to the DBus message bus, which type is SYSTEM.
 */
int
gdbus_get_system_connection (gboolean is_session)
{
  GError *error = NULL;
  GBusType bus_type = is_session ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM;

  g_dbus_sys_conn = g_bus_get_sync (bus_type, NULL, &error);
  if (g_dbus_sys_conn == NULL) {
    _E ("cannot connect to the system message bus: %s\n", error->message);
    g_clear_error(&error);
    return -ENOSYS;
  }

  return 0;
}

/**
 * @brief Disconnect the DBus message bus.
 */
void
gdbus_put_system_connection (void)
{
  g_clear_object (&g_dbus_sys_conn);
}
