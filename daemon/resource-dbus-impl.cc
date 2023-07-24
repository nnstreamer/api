/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file resource-dbus-impl.cc
 * @date 17 July 2023
 * @brief DBus implementation for Resource Interface
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <errno.h>
#include <glib.h>

#include "common.h"
#include "dbus-interface.h"
#include "gdbus-util.h"
#include "log.h"
#include "modules.h"
#include "resource-dbus.h"
#include "service-db.hh"

static MachinelearningServiceResource *g_gdbus_res_instance = NULL;

/**
 * @brief Utility function to get the DBus proxy.
 */
static MachinelearningServiceResource *
gdbus_get_resource_instance (void)
{
  return machinelearning_service_resource_skeleton_new ();
}

/**
 * @brief Utility function to release DBus proxy.
 */
static void
gdbus_put_resource_instance (MachinelearningServiceResource **instance)
{
  g_clear_object (instance);
}

/**
 * @brief The callback function of Add method
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target resource.
 * @param path The file path of target.
 * @param description The description of the resource.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_resource_add (MachinelearningServiceResource *obj, GDBusMethodInvocation *invoc,
    const gchar *name, const gchar *path, const gchar *description)
{
  int ret = 0;
  MLServiceDB &db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.set_resource (name, path, description);
  } catch (const std::invalid_argument &e) {
    _E ("%s", e.what ());
    ret = -EINVAL;
  } catch (const std::exception &e) {
    _E ("%s", e.what ());
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_resource_complete_add (obj, invoc, ret);

  return TRUE;
}

/**
 * @brief The callback function of get method
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target resource.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_resource_get (MachinelearningServiceResource *obj,
    GDBusMethodInvocation *invoc, const gchar *name)
{
  int ret = 0;
  std::string res_info;
  MLServiceDB &db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.get_resource (name, res_info);
  } catch (const std::invalid_argument &e) {
    _E ("%s", e.what ());
    ret = -EINVAL;
  } catch (const std::exception &e) {
    _E ("%s", e.what ());
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_resource_complete_get (obj, invoc, res_info.c_str (), ret);

  return TRUE;
}

/**
 * @brief The callback function of delete method
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target resource.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_resource_delete (MachinelearningServiceResource *obj,
    GDBusMethodInvocation *invoc, const gchar *name)
{
  int ret = 0;
  MLServiceDB &db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.delete_resource (name);
  } catch (const std::invalid_argument &e) {
    _E ("%s", e.what ());
    ret = -EINVAL;
  } catch (const std::exception &e) {
    _E ("%s", e.what ());
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_resource_complete_delete (obj, invoc, ret);

  return TRUE;
}

/**
 * @brief Event handler list of resource interface
 */
static struct gdbus_signal_info res_handler_infos[] = {
  {
      .signal_name = DBUS_RESOURCE_I_HANDLER_ADD,
      .cb = G_CALLBACK (gdbus_cb_resource_add),
      .cb_data = NULL,
      .handler_id = 0,
  },
  {
      .signal_name = DBUS_RESOURCE_I_HANDLER_GET,
      .cb = G_CALLBACK (gdbus_cb_resource_get),
      .cb_data = NULL,
      .handler_id = 0,
  },
  {
      .signal_name = DBUS_RESOURCE_I_HANDLER_DELETE,
      .cb = G_CALLBACK (gdbus_cb_resource_delete),
      .cb_data = NULL,
      .handler_id = 0,
  },
};

/**
 * @brief The callback function for probing resource Interface module.
 */
static int
probe_resource_module (void *data)
{
  int ret = 0;
  _D ("probe_resource_module");

  g_gdbus_res_instance = gdbus_get_resource_instance ();
  if (NULL == g_gdbus_res_instance) {
    _E ("cannot get a dbus instance for the %s interface\n", DBUS_RESOURCE_INTERFACE);
    return -ENOSYS;
  }

  ret = gdbus_connect_signal (
      g_gdbus_res_instance, ARRAY_SIZE (res_handler_infos), res_handler_infos);
  if (ret < 0) {
    _E ("cannot register callbacks as the dbus method invocation handlers\n ret: %d", ret);
    ret = -ENOSYS;
    goto out;
  }

  ret = gdbus_export_interface (g_gdbus_res_instance, DBUS_RESOURCE_PATH);
  if (ret < 0) {
    _E ("cannot export the dbus interface '%s' at the object path '%s'\n",
        DBUS_RESOURCE_INTERFACE, DBUS_RESOURCE_PATH);
    ret = -ENOSYS;
    goto out_disconnect;
  }

  return 0;

out_disconnect:
  gdbus_disconnect_signal (
      g_gdbus_res_instance, ARRAY_SIZE (res_handler_infos), res_handler_infos);

out:
  gdbus_put_resource_instance (&g_gdbus_res_instance);

  return ret;
}

/**
 * @brief The callback function for initializing resource interface module.
 */
static void
init_resource_module (void *data)
{
  gdbus_initialize ();
}

/**
 * @brief The callback function for exiting resource interface module.
 */
static void
exit_resource_module (void *data)
{
  gdbus_disconnect_signal (
      g_gdbus_res_instance, ARRAY_SIZE (res_handler_infos), res_handler_infos);
  gdbus_put_resource_instance (&g_gdbus_res_instance);
}

static const struct module_ops resource_ops = {
  .name = "resource-interface",
  .probe = probe_resource_module,
  .init = init_resource_module,
  .exit = exit_resource_module,
};

MODULE_OPS_REGISTER (&resource_ops)
