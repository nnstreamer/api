/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    pipeline-module.c
 * @date    29 June 2022
 * @brief   Implementation of pipeline dbus interface.
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This implements the pipeline dbus interface.
 */

#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <common.h>
#include <modules.h>
#include <gdbus-util.h>

#include "dbus-interface.h"
#include "log.h"

static MachinelearningServicePipeline *g_gdbus_instance = NULL;

static gboolean
dbus_cb_pipeline_set (
    MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name,
    const gchar *description,
    gpointer user_data)
{
  int ret = 0;

  _I("Name: %s, Description: %s\n", name, description);

  machinelearning_service_pipeline_complete_set(obj, invoc, ret);

  return TRUE;
}

static gboolean
dbus_cb_pipeline_get (
    MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name)
{
  int ret = 0;
  const gchar *ret_desc = "Return_Description";

  _I("Name: %s, RetDescription: %s\n", name, ret_desc);

  machinelearning_service_pipeline_complete_get(obj, invoc, ret_desc, ret);

  return TRUE;
}

static gboolean
dbus_cb_pipeline_delete (
    MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name)
{
  int ret = 0;
  _I("Name: %s\n", name);

  machinelearning_service_pipeline_complete_delete(
    obj, invoc, ret);

  return TRUE;
}

static struct gdbus_signal_info handler_infos[] = {
  {
    .signal_name = DBUS_PIPELINE_HANDLER_SET,
    .cb = G_CALLBACK(dbus_cb_pipeline_set),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_HANDLER_GET,
    .cb = G_CALLBACK(dbus_cb_pipeline_get),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_HANDLER_DELETE,
    .cb = G_CALLBACK(dbus_cb_pipeline_delete),
    .cb_data = NULL,
    .handler_id = 0,
  }
};

static void init_pipeline_module(void *data)
{
}

static void exit_pipeline_module(void *data)
{
  gdbus_disconnect_signal(g_gdbus_instance,
          ARRAY_SIZE(handler_infos), handler_infos);
  gdbus_put_instance_pipeline(&g_gdbus_instance);
}

static int probe_pipeline_module(void *data)
{
  int ret = 0;

  g_gdbus_instance = gdbus_get_instance_pipeline ();
  if (g_gdbus_instance == NULL) {
    _E ("cannot get a dbus instance for the %s interface\n",
      DBUS_PIPELINE_INTERFACE);
      return -ENOSYS;
  }

  ret = gdbus_connect_signal (g_gdbus_instance,
      ARRAY_SIZE(handler_infos), handler_infos);
  if (ret < 0) {
    _E("cannot register callbacks as the dbus method invocation "
            "handlers\n");
    ret = -ENOSYS;
    goto out;
  }

  ret = gdbus_export_interface (g_gdbus_instance, DBUS_PIPELINE_PATH);
  if (ret < 0) {
    _E("cannot export the dbus interface '%s' "
            "at the object path '%s'\n",
            DBUS_PIPELINE_INTERFACE,
            DBUS_PIPELINE_PATH);
    ret = -ENOSYS;
    goto out_disconnect;
  }

  _I("SJ: Success to probe_pipeline_module()\n");
  return 0;

out_disconnect:
  gdbus_disconnect_signal (g_gdbus_instance,
      ARRAY_SIZE(handler_infos), handler_infos);

out:
  gdbus_put_instance_pipeline (&g_gdbus_instance);

  return ret;
}

static const struct module_ops pipeline_ops = {
  .name	= "pipeline",
  .probe	= probe_pipeline_module,
  .init	= init_pipeline_module,
  .exit	= exit_pipeline_module,
};

MODULE_OPS_REGISTER(&pipeline_ops)
