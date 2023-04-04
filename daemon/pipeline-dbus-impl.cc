/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    pipeline-dbus-impl.cc
 * @date    20 Jul 2022
 * @brief   Implementation of pipeline dbus interface.
 * @see     https://github.com/nnstreamer/api
 * @author  Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug     No known bugs except for NYI items
 * @details
 *    This implements the pipeline dbus interface.
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

#include "pipeline-dbus.h"
#include "service-db.hh"
#include "dbus-interface.h"

static MachinelearningServicePipeline *g_gdbus_instance = NULL;
static GHashTable *pipeline_table = NULL;
G_LOCK_DEFINE_STATIC (pipeline_table_lock);

/**
 * @brief Structure for pipeline.
 */
typedef struct _pipeline
{
  GstElement *element;
  gint64 id;
  GMutex lock;
  gchar *service_name;
  gchar *description;
} pipeline_s;

/**
 * @brief Internal function to destroy pipeline instances.
 */
static void
_pipeline_free (gpointer data)
{
  pipeline_s *p;

  if (!data) {
    _E ("internal error, the data should not be NULL");
    return;
  }

  p = (pipeline_s *) data;

  if (p->element)
    gst_object_unref (p->element);

  g_free (p->service_name);
  g_free (p->description);
  g_mutex_clear (&p->lock);

  g_free (p);
}

/**
 * @brief Get the skeleton object of the DBus interface.
 */
static MachinelearningServicePipeline *
gdbus_get_pipeline_instance (void)
{
  return machinelearning_service_pipeline_skeleton_new ();
}

/**
 * @brief Put the obtained skeleton object and release the resource.
 */
static void
gdbus_put_pipeline_instance (MachinelearningServicePipeline **instance)
{
  g_clear_object (instance);
}

/**
 * @brief Set the service with given description. Return the call result.
 */
static gboolean
dbus_cb_core_set_pipeline (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, const gchar *service_name,
    const gchar *pipeline_desc, gpointer user_data)
{
  gint result = 0;
  MLServiceDB &db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.set_pipeline (service_name, pipeline_desc);
  } catch (const std::invalid_argument &e) {
    _E ("An exception occurred during write to the DB. Error message: %s", e.what ());
    result = -EINVAL;
  } catch (const std::exception &e) {
    _E ("An exception occurred during write to the DB. Error message: %s", e.what ());
    result = -EIO;
  }

  db.disconnectDB ();

  if (result) {
    _E ("Failed to set pipeline description of %s", service_name);
    machinelearning_service_pipeline_complete_set_pipeline (obj, invoc, result);
    return TRUE;
  }

  machinelearning_service_pipeline_complete_set_pipeline (obj, invoc, result);

  return TRUE;
}

/**
 * @brief Get the pipeline description of the given service. Return the call result and the pipeline description.
 */
static gboolean
dbus_cb_core_get_pipeline (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, const gchar *service_name,
    gpointer user_data)
{
  gint result = 0;
  std::string stored_pipeline_description;
  MLServiceDB &db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.get_pipeline (service_name, stored_pipeline_description);
  } catch (const std::invalid_argument &e) {
    _E ("An exception occurred during read the DB. Error message: %s", e.what ());
    result = -EINVAL;
  } catch (const std::exception &e) {
    _E ("An exception occurred during read the DB. Error message: %s", e.what ());
    result = -EIO;
  }

  db.disconnectDB ();

  if (result) {
    _E ("Failed to get pipeline description of %s", service_name);
    machinelearning_service_pipeline_complete_get_pipeline (obj, invoc, result, NULL);
    return TRUE;
  }

  machinelearning_service_pipeline_complete_get_pipeline (obj, invoc, result, stored_pipeline_description.c_str ());

  return TRUE;
}

/**
 * @brief Delete the pipeline description of the given service. Return the call result.
 */
static gboolean
dbus_cb_core_delete_pipeline (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, const gchar *service_name,
    gpointer user_data)
{
  gint result = 0;
  MLServiceDB &db = MLServiceDB::getInstance ();

  try {
    db.connectDB ();
    db.delete_pipeline (service_name);
  } catch (const std::invalid_argument &e) {
    _E ("An exception occurred during delete an item in the DB. Error message: %s", e.what ());
    result = -EINVAL;
  } catch (const std::exception &e) {
    _E ("An exception occurred during delete an item in the DB. Error message: %s", e.what ());
    result = -EIO;
  }

  db.disconnectDB ();

  if (result) {
    _E ("Failed to delete the pipeline description of %s", service_name);
    machinelearning_service_pipeline_complete_delete_pipeline (obj, invoc, result);
    return TRUE;
  }

  machinelearning_service_pipeline_complete_delete_pipeline (obj, invoc, result);

  return TRUE;
}

/**
 * @brief Launch the pipeline with given description. Return the call result and its id.
 */
static gboolean
dbus_cb_core_launch_pipeline (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, const gchar *service_name,
    gpointer user_data)
{
  gint result = 0;
  GError *err = NULL;
  GstStateChangeReturn sc_ret;
  GstElement *pipeline = NULL;
  pipeline_s *p;

  MLServiceDB &db = MLServiceDB::getInstance ();
  std::string stored_pipeline_description;

  /** get pipeline description from the DB */
  try {
    db.connectDB ();
    db.get_pipeline (service_name, stored_pipeline_description);
  } catch (const std::invalid_argument &e) {
    _E ("An exception occurred during read the DB. Error message: %s", e.what ());
    result = -EINVAL;
  } catch (const std::exception &e) {
    _E ("An exception occurred during read the DB. Error message: %s", e.what ());
    result = -EIO;
  }

  db.disconnectDB ();

  if (result) {
    _E ("Failed to launch pipeline of %s", service_name);
    machinelearning_service_pipeline_complete_launch_pipeline (obj, invoc, result, -1);
    return TRUE;
  }

  pipeline = gst_parse_launch (stored_pipeline_description.c_str (), &err);
  if (!pipeline || err) {
    _E ("gst_parse_launch with %s Failed. error msg: %s",
        stored_pipeline_description.c_str (),
        (err) ? err->message : "unknown reason");
    g_clear_error (&err);

    if (pipeline)
      gst_object_unref (pipeline);

    result = -ESTRPIPE;
    machinelearning_service_pipeline_complete_launch_pipeline (obj, invoc, result, -1);
    return TRUE;
  }

  /** now set pipeline as paused state */
  sc_ret = gst_element_set_state (pipeline, GST_STATE_PAUSED);
  if (sc_ret == GST_STATE_CHANGE_FAILURE) {
    _E ("Failed to set the state of the pipeline to PAUSED. For the detail, please check the GStreamer log message. The input pipeline was %s", stored_pipeline_description.c_str ());

    gst_object_unref (pipeline);
    result = -ESTRPIPE;
    machinelearning_service_pipeline_complete_launch_pipeline (obj, invoc, result, -1);
    return TRUE;
  }

  /** now fill the struct and store into hash table */
  p = g_new0 (pipeline_s, 1);
  p->element = pipeline;
  p->description = g_strdup (stored_pipeline_description.c_str ());
  p->service_name = g_strdup (service_name);
  g_mutex_init (&p->lock);

  G_LOCK (pipeline_table_lock);
  p->id = g_get_monotonic_time ();
  g_hash_table_insert (pipeline_table, GINT_TO_POINTER (p->id), p);
  G_UNLOCK (pipeline_table_lock);

  machinelearning_service_pipeline_complete_launch_pipeline (obj, invoc, result, p->id);

  return TRUE;
}

/**
 * @brief Start the pipeline with given id. Return the call result.
 */
static gboolean
dbus_cb_core_start_pipeline (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, gint64 id, gpointer user_data)
{
  gint result = 0;
  GstStateChangeReturn sc_ret;
  pipeline_s *p = NULL;

  G_LOCK (pipeline_table_lock);
  p = (pipeline_s *) g_hash_table_lookup (pipeline_table, GINT_TO_POINTER (id));

  if (!p) {
    _E ("there is no pipeline with id: %" G_GINT64_FORMAT, id);
    G_UNLOCK (pipeline_table_lock);
    result = -EINVAL;
  } else {
    g_mutex_lock (&p->lock);
    G_UNLOCK (pipeline_table_lock);
    sc_ret = gst_element_set_state (p->element, GST_STATE_PLAYING);
    g_mutex_unlock (&p->lock);

    if (sc_ret == GST_STATE_CHANGE_FAILURE) {
      _E ("Failed to set the state of the pipline to PLAYING whose service_name is %s (id: %" G_GINT64_FORMAT ")", p->service_name, id);
      result = -ESTRPIPE;
    }
  }

  machinelearning_service_pipeline_complete_start_pipeline (obj, invoc, result);

  return TRUE;
}

/**
 * @brief Stop the pipeline with given id. Return the call result.
 */
static gboolean
dbus_cb_core_stop_pipeline (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, gint64 id, gpointer user_data)
{
  gint result = 0;
  GstStateChangeReturn sc_ret;
  pipeline_s *p = NULL;

  G_LOCK (pipeline_table_lock);
  p = (pipeline_s *) g_hash_table_lookup (pipeline_table, GINT_TO_POINTER (id));

  if (!p) {
    _E ("there is no pipeline with id: %" G_GINT64_FORMAT, id);
    G_UNLOCK (pipeline_table_lock);
    result = -EINVAL;
  } else {
    g_mutex_lock (&p->lock);
    G_UNLOCK (pipeline_table_lock);
    sc_ret = gst_element_set_state (p->element, GST_STATE_PAUSED);
    g_mutex_unlock (&p->lock);

    if (sc_ret == GST_STATE_CHANGE_FAILURE) {
      _E ("Failed to set the state of the pipline to PAUSED whose service_name is %s (id: %" G_GINT64_FORMAT ")", p->service_name, id);
      result = -ESTRPIPE;
    }
  }

  machinelearning_service_pipeline_complete_stop_pipeline (obj, invoc, result);

  return TRUE;
}

/**
 * @brief Destroy the pipeline with given id. Return the call result.
 */
static gboolean
dbus_cb_core_destroy_pipeline (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, gint64 id, gpointer user_data)
{
  gint result = 0;
  pipeline_s *p = NULL;

  G_LOCK (pipeline_table_lock);
  p = (pipeline_s *) g_hash_table_lookup (pipeline_table, GINT_TO_POINTER (id));

  if (!p) {
    _E ("there is no pipeline with id: %" G_GINT64_FORMAT, id);
    result = -EINVAL;
  } else {

/**
 * @todo Fix hanging issue when trying to set GST_STATE_NULL state for pipelines containing tensor_query_*. As a workaround, just unref the pipeline instance. To fix this issue, tensor_query elements and nnstreamer-edge should well-behavior to the state change. And it should properly free socket resources. Revive following code after then.
 */
/**
 *   GstStateChangeReturn sc_ret;

 *   g_mutex_lock (&p->lock);
 *   sc_ret = gst_element_set_state (p->element, GST_STATE_NULL);
 *   g_mutex_unlock (&p->lock);

 *   if (sc_ret == GST_STATE_CHANGE_FAILURE) {
 *     _E ("Failed to set the state of the pipeline to NULL whose service_name is %s (id: %" G_GINT64_FORMAT "). Destroy it anyway.", p->service_name, id);
 *     result = -ESTRPIPE;
 *   }
 */
    g_hash_table_remove (pipeline_table, GINT_TO_POINTER (id));
  }

  G_UNLOCK (pipeline_table_lock);
  machinelearning_service_pipeline_complete_destroy_pipeline (obj, invoc, result);

  return TRUE;
}

/**
 * @brief Get the state of pipeline with given id. Return the call result and its state.
 */
static gboolean
dbus_cb_core_get_state (MachinelearningServicePipeline *obj,
    GDBusMethodInvocation *invoc, gint64 id, gpointer user_data)
{
  gint result = 0;
  GstStateChangeReturn sc_ret;
  GstState state = GST_STATE_NULL;
  pipeline_s *p = NULL;

  G_LOCK (pipeline_table_lock);
  p = (pipeline_s *) g_hash_table_lookup (pipeline_table, GINT_TO_POINTER (id));

  if (!p) {
    _E ("there is no pipeline with id: %" G_GINT64_FORMAT, id);
    result = -EINVAL;
    machinelearning_service_pipeline_complete_get_state (obj, invoc, result, (gint) state);
    G_UNLOCK (pipeline_table_lock);
    return TRUE;
  }

  g_mutex_lock (&p->lock);
  G_UNLOCK (pipeline_table_lock);
  sc_ret = gst_element_get_state (p->element, &state, NULL, GST_MSECOND);
  g_mutex_unlock (&p->lock);

  if (sc_ret == GST_STATE_CHANGE_FAILURE) {
    _E ("Failed to get the state of the pipline whose service_name is %s (id: %" G_GINT64_FORMAT ")", p->service_name, id);
    result = -ESTRPIPE;
    machinelearning_service_pipeline_complete_get_state (obj, invoc, result, (gint) state);
    return TRUE;
  }

  machinelearning_service_pipeline_complete_get_state (obj, invoc, result, (gint) state);

  return TRUE;
}

static struct gdbus_signal_info handler_infos[] = {
  {
    .signal_name = DBUS_PIPELINE_I_SET_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_set_pipeline),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_I_GET_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_get_pipeline),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_I_DELETE_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_delete_pipeline),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_I_LAUNCH_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_launch_pipeline),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_I_START_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_start_pipeline),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_I_STOP_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_stop_pipeline),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_I_DESTROY_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_destroy_pipeline),
    .cb_data = NULL,
    .handler_id = 0,
  }, {
    .signal_name = DBUS_PIPELINE_I_GET_STATE_HANDLER,
    .cb = G_CALLBACK (dbus_cb_core_get_state),
    .cb_data = NULL,
    .handler_id = 0,
  },
};

/**
 * @brief Probe the D-BUS and connect this module.
 */
static int
probe_pipeline_module (void *data)
{
  int ret = 0;

  g_gdbus_instance = gdbus_get_pipeline_instance ();
  if (g_gdbus_instance == NULL) {
    _E ("cannot get a dbus instance for the %s interface\n",
        DBUS_PIPELINE_INTERFACE);
    return -ENOSYS;
  }

  ret = gdbus_connect_signal (g_gdbus_instance,
      ARRAY_SIZE (handler_infos), handler_infos);
  if (ret < 0) {
    _E ("cannot register callbacks as the dbus method invocation handlers\n ret: %d",
        ret);
    ret = -ENOSYS;
    goto out;
  }

  ret = gdbus_export_interface (g_gdbus_instance, DBUS_PIPELINE_PATH);
  if (ret < 0) {
    _E ("cannot export the dbus interface '%s' at the object path '%s'\n",
        DBUS_PIPELINE_INTERFACE, DBUS_PIPELINE_PATH);
    ret = -ENOSYS;
    goto out_disconnect;
  }

  return 0;

out_disconnect:
  gdbus_disconnect_signal (g_gdbus_instance,
      ARRAY_SIZE (handler_infos), handler_infos);
out:
  gdbus_put_pipeline_instance (&g_gdbus_instance);

  return ret;
}

/**
 * @brief Initialize this module.
 */
static void
init_pipeline_module (void *data)
{
  GError *err = NULL;

  G_LOCK (pipeline_table_lock);
  g_assert (NULL == pipeline_table); /** Internal error */
  pipeline_table = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, _pipeline_free);
  G_UNLOCK (pipeline_table_lock);

  _I ("init gstreamer");
  if (!gst_init_check (NULL, NULL, &err)) {
    if (err) {
      _E ("Initializing gstreamer failed with err msg %s", err->message);
      g_clear_error (&err);
    } else {
      _E ("cannot initialize GStreamer with unknown reason.");
    }
  }
}

/**
 * @brief Finalize this module.
 */
static void
exit_pipeline_module (void *data)
{
  G_LOCK (pipeline_table_lock);
  g_assert (pipeline_table); /** Internal error */
  g_hash_table_destroy (pipeline_table);
  pipeline_table = NULL;
  G_UNLOCK (pipeline_table_lock);

  gdbus_disconnect_signal (g_gdbus_instance,
      ARRAY_SIZE (handler_infos), handler_infos);
  gdbus_put_pipeline_instance (&g_gdbus_instance);
}

static const struct module_ops pipeline_ops = {
  .name = "pipeline",
  .probe = probe_pipeline_module,
  .init = init_pipeline_module,
  .exit = exit_pipeline_module,
};

MODULE_OPS_REGISTER (&pipeline_ops)
