/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file test-dbus-impl.c
 * @date 16 Jul 2022
 * @brief DBus implementation for Test Interface
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

#include <common.h>
#include <modules.h>
#include <gdbus-util.h>
#include <log.h>

#include "test-dbus-interface.h"
#include "test-dbus.h"

static MachinelearningServiceTest *g_gdbus_instance = NULL;

/**
 * @brief DBus handler for 'get_state' method of Test interface.
 */
static gboolean
dbus_cb_service_get_status (MachinelearningServiceTest * obj,
    GDBusMethodInvocation * invoc)
{
  int ret = 0;
  int status = 1;

  machinelearning_service_test_complete_get_state (obj, invoc, status, ret);
  return TRUE;
}

static struct gdbus_signal_info g_gdbus_signal_infos[] = {
  {
    .signal_name = DBUS_TEST_I_GET_STATE_HANDLER,
    .cb = G_CALLBACK (dbus_cb_service_get_status),
    .cb_data = NULL,
    .handler_id = 0,
  },
};

/**
 * @brief Utility function to get the DBus proxy of Test interface.
 */
static MachinelearningServiceTest *
gdbus_get_instance_test (void)
{
  return machinelearning_service_test_skeleton_new ();
}

/**
 * @brief Utility function to release DBus proxy.
 */
static void
gdbus_put_instance_test (MachinelearningServiceTest ** instance)
{
  g_clear_object (instance);
}

/**
 * @brief The callback function for initializing Test module.
 */
static void
init_test (void *data)
{
  GError *err = NULL;
  g_debug ("init_test module");

  if (!gst_init_check (NULL, NULL, &err)) {
    if (err) {
      g_critical ("Initializing gstreamer failed with err msg %s",
          err->message);
      g_clear_error (&err);
    } else {
      g_critical ("cannot initalize GStreamer with unknown reason.");
    }
  }
}

/**
 * @brief The callback function for exiting Test modeule.
 */
static void
exit_test (void *data)
{
  gdbus_disconnect_signal (g_gdbus_instance,
      ARRAY_SIZE (g_gdbus_signal_infos), g_gdbus_signal_infos);
  gdbus_put_instance_test (&g_gdbus_instance);
}

/**
 * @brief The callback function for proving Test modeule.
 */
static int
probe_test (void *data)
{
  int ret = 0;
  g_debug ("probe_test");
  g_gdbus_instance = gdbus_get_instance_test ();
  if (g_gdbus_instance == NULL) {
    g_critical ("cannot get a dbus instance for the %s interface\n",
        DBUS_TEST_INTERFACE);
    return -ENOSYS;
  }

  ret = gdbus_connect_signal (g_gdbus_instance,
      ARRAY_SIZE (g_gdbus_signal_infos), g_gdbus_signal_infos);
  if (ret < 0) {
    g_critical ("cannot register callbacks as the dbus method invocation "
        "handlers\n ret: %d", ret);
    ret = -ENOSYS;
    goto out;
  }

  ret = gdbus_export_interface (g_gdbus_instance, DBUS_TEST_PATH);
  if (ret < 0) {
    g_critical ("cannot export the dbus interface '%s' "
        "at the object path '%s'\n", DBUS_TEST_INTERFACE, DBUS_TEST_PATH);
    ret = -ENOSYS;
    goto out_disconnect;
  }
  return 0;

out_disconnect:
  gdbus_disconnect_signal (g_gdbus_instance,
      ARRAY_SIZE (g_gdbus_signal_infos), g_gdbus_signal_infos);
out:
  gdbus_put_instance_test (&g_gdbus_instance);

  return ret;
}

static const struct module_ops test_ops = {
  .name = "ml-agent-test",
  .probe = probe_test,
  .init = init_test,
  .exit = exit_test,
};

MODULE_OPS_REGISTER (&test_ops)
