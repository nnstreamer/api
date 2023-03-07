/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file model-dbus-impl.cc
 * @date 29 Jul 2022
 * @brief DBus implementation for Model Interface
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>
#include <errno.h>

#include "common.h"
#include "modules.h"
#include "gdbus-util.h"
#include "dbus-interface.h"
#include "model-dbus.h"
#include "service-db.hh"

static MachinelearningServiceModel *g_gdbus_instance = NULL;

/**
 * @brief Utility function to get the DBus proxy of Model interface.
 */
static MachinelearningServiceModel *
gdbus_get_model_instance (void)
{
  return machinelearning_service_model_skeleton_new ();
}

/**
 * @brief Utility function to release DBus proxy of Model interface.
 */
static void
gdbus_put_model_instance (MachinelearningServiceModel ** instance)
{
  g_clear_object (instance);
}

/**
 * @brief The callback function of Register method
 *
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target model.
 * @param path The file path of target.
 * @param is_active The active status of target model.
 * @param description The description of target model.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_model_register (MachinelearningServiceModel *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name,
    const gchar *path,
    const bool is_active,
    const gchar *description)
{
  int ret = 0;
  guint version = 0U;
  MLServiceDB & db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.set_model (name, path, is_active, description, &version);
  } catch (...) {
    g_critical ("DB error occurred. Failed to register the model: %s", name);
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_model_complete_register (obj, invoc, version, ret);

  return TRUE;
}

/**
 * @brief The callback function of update description method
 *
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target model.
 * @param version The version of target model.
 * @param description The description of target model.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_model_update_description (MachinelearningServiceModel *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name,
    const guint version,
    const gchar *description)
{
  int ret = 0;
  MLServiceDB & db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.update_model_description (name, version, description);
  } catch (const std::invalid_argument &e) {
    g_critical ("There is no such model");
    ret = -EINVAL;
  } catch (...) {
    g_critical ("DB error occurred. Failed to update the model description: %s", name);
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_model_complete_update_description (obj, invoc, ret);

  return TRUE;
}

/**
 * @brief The callback function of activate method
 *
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target model.
 * @param version The version of target model.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_model_activate (MachinelearningServiceModel *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name,
    const guint version)
{
  int ret = 0;
  MLServiceDB & db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.activate_model (name, version);
  } catch (const std::invalid_argument &e) {
    g_critical ("There is no such model");
    ret = -EINVAL;
  } catch (...) {
    g_critical ("DB error occurred. Failed to activate the model: %s", name);
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_model_complete_activate (obj, invoc, ret);

  return TRUE;
}

/**
 * @brief The callback function of get method
 *
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target model.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_model_get (MachinelearningServiceModel *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name,
    const guint version)
{
  int ret = 0;
  std::string model_info;
  MLServiceDB & db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.get_model (name, model_info, version);
  } catch (const std::invalid_argument &e) {
    g_critical ("There is no active model");
    ret = -EINVAL;
  } catch (...) {
    g_critical ("DB error occurred. Failed to get the active model: %s", name);
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_model_complete_get_activated (obj, invoc, model_info.c_str (), ret);

  return TRUE;
}

/**
 * @brief The callback function of get activated method
 *
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target model.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_model_get_activated (MachinelearningServiceModel *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name)
{
  int ret = 0;
  std::string model_info;
  MLServiceDB & db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.get_model (name, model_info, -1);
  } catch (const std::invalid_argument &e) {
    g_critical ("There is no active model");
    ret = -EINVAL;
  } catch (...) {
    g_critical ("DB error occurred. Failed to get the active model: %s", name);
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_model_complete_get_activated (obj, invoc, model_info.c_str (), ret);

  return TRUE;
}

/**
 * @brief The callback function of get all method
 *
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target model.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_model_get_all (MachinelearningServiceModel *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name
    )
{
  int ret = 0;
  MLServiceDB & db = MLServiceDB::getInstance ();
  std::string all_model_list;

  try {
    db.connectDB ();
    db.get_model (name, all_model_list, 0);
  } catch (const std::invalid_argument &e) {
    g_critical ("There is no such model");
    ret = -EINVAL;
  } catch (...) {
    g_critical ("DB error occurred. Failed to get all the models");
    ret = -EIO;
  }

  db.disconnectDB ();

  machinelearning_service_model_complete_get (obj, invoc, all_model_list.c_str (), ret);

  return TRUE;
}

/**
 * @brief The callback function of delete method
 *
 * @param obj Proxy instance.
 * @param invoc Method invocation handle.
 * @param name The name of target model.
 * @param version The version of target model.
 * @return @c TRUE if the request is handled. FALSE if the service is not available.
 */
static gboolean
gdbus_cb_model_delete (MachinelearningServiceModel *obj,
    GDBusMethodInvocation *invoc,
    const gchar *name,
    const guint version)
{
  int ret = 0;
  MLServiceDB & db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.delete_model (name, version);
  } catch (const std::invalid_argument &e) {
    g_critical ("There is no such model");
    ret = -EINVAL;
  } catch (...) {
    g_critical ("DB error occurred. Failed to delete the model: %s", name);
    ret = -EIO;
  }

  db.disconnectDB ();
  machinelearning_service_model_complete_delete (obj, invoc, ret);

  return TRUE;
}

/**
 * @brief Event handler list of Model interface
 */
static struct gdbus_signal_info handler_infos[] = {
  {
    .signal_name = DBUS_MODEL_I_HANDLER_REGISTER,
    .cb = G_CALLBACK (gdbus_cb_model_register),
    .cb_data = NULL,
    .handler_id = 1U,
  }, {
    .signal_name = DBUS_MODEL_I_HANDLER_UPDATE_DESCRIPTION,
    .cb = G_CALLBACK (gdbus_cb_model_update_description),
    .cb_data = NULL,
    .handler_id = 2U,
  }, {
    .signal_name = DBUS_MODEL_I_HANDLER_ACTIVATE,
    .cb = G_CALLBACK (gdbus_cb_model_activate),
    .cb_data = NULL,
    .handler_id = 3U,
  }, {
    .signal_name = DBUS_MODEL_I_HANDLER_GET,
    .cb = G_CALLBACK (gdbus_cb_model_get),
    .cb_data = NULL,
    .handler_id = 4U,
  }, {
    .signal_name = DBUS_MODEL_I_HANDLER_GET_ACTIVATED,
    .cb = G_CALLBACK (gdbus_cb_model_get_activated),
    .cb_data = NULL,
    .handler_id = 5U,
  }, {
    .signal_name = DBUS_MODEL_I_HANDLER_GET_ALL,
    .cb = G_CALLBACK (gdbus_cb_model_get_all),
    .cb_data = NULL,
    .handler_id = 6U,
  }, {
    .signal_name = DBUS_MODEL_I_HANDLER_DELETE,
    .cb = G_CALLBACK (gdbus_cb_model_delete),
    .cb_data = NULL,
    .handler_id = 7U,
  }
};

/**
 * @brief The callback function for probing Model Interface module.
 */
static int
probe_model_module (void *data)
{
  int ret = 0;
  g_debug ("probe_model_module");

  g_gdbus_instance = gdbus_get_model_instance ();
  if (NULL == g_gdbus_instance) {
    g_critical ("cannot get a dbus instance for the %s interface\n",
        DBUS_MODEL_INTERFACE);
    return -ENOSYS;
  }

  ret = gdbus_connect_signal (g_gdbus_instance,
      ARRAY_SIZE(handler_infos), handler_infos);
  if (ret < 0) {
    g_critical ("cannot register callbacks as the dbus method invocation "
        "handlers\n ret: %d", ret);
    ret = -ENOSYS;
    goto out;
  }

  ret = gdbus_export_interface (g_gdbus_instance, DBUS_MODEL_PATH);
  if (ret < 0) {
    g_critical ("cannot export the dbus interface '%s' "
        "at the object path '%s'\n", DBUS_MODEL_INTERFACE, DBUS_MODEL_PATH);
    ret = -ENOSYS;
    goto out_disconnect;
  }

  return 0;

out_disconnect:
  gdbus_disconnect_signal (g_gdbus_instance,
    ARRAY_SIZE (handler_infos), handler_infos);

out:
  gdbus_put_model_instance (&g_gdbus_instance);

  return ret;
}

/**
 * @brief The callback function for initializing Model Interface module.
 */
static void
init_model_module (void *data) { }

/**
 * @brief The callback function for exiting Model Interface module.
 */
static void
exit_model_module (void *data)
{
  gdbus_disconnect_signal (g_gdbus_instance,
    ARRAY_SIZE (handler_infos), handler_infos);
  gdbus_put_model_instance (&g_gdbus_instance);
}

static const struct module_ops model_ops = {
  .name = "model-interface",
  .probe = probe_model_module,
  .init = init_model_module,
  .exit = exit_model_module,
};

MODULE_OPS_REGISTER (&model_ops)
