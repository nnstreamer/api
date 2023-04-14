/**
 * @file    ml-agent-dbus-interface.c
 * @date    5 April 2023
 * @brief   A set of exported DBus interfaces for managing pipelines and models
 * @see     https://github.com/nnstreamer/api
 * @author  Wook Song <wook16.song@samsung.com>
 * @bug     No known bugs except for NYI items
 */

#include "includes/ml-agent-dbus-interface.h"
#include "includes/dbus-interface.h"
#include "model-dbus.h"
#include "pipeline-dbus.h"

#include <glib.h>

typedef enum
{
  ML_AGENT_DBUS_SERVICE_PIPELINE = 0,
  ML_AGENT_DBUS_SERVICE_MODEL,
  ML_AGENT_DBUS_SERVICE_END,
} ml_agent_dbus_service_type_e;

typedef gpointer ml_agent_dbus_proxy_h;

/**
 * @brief An internal helper to get the dbus proxy
 */
static ml_agent_dbus_proxy_h
_get_proxy_new_for_bus_sync (ml_agent_dbus_service_type_e type, GError ** err)
{
  static const GBusType bus_types[] = { G_BUS_TYPE_SYSTEM, G_BUS_TYPE_SESSION };
  static const size_t num_bus_types =
      sizeof (bus_types) / sizeof (bus_types[0]);
  ml_agent_dbus_proxy_h *proxy = NULL;
  GError *_err = NULL;
  size_t i;

  switch (type) {
    case ML_AGENT_DBUS_SERVICE_PIPELINE:
    {
      MachinelearningServicePipeline *mlsp;

      for (i = 0; i < num_bus_types; ++i) {
        g_clear_error (&_err);
        mlsp =
            machinelearning_service_pipeline_proxy_new_for_bus_sync (bus_types
            [i], G_DBUS_PROXY_FLAGS_NONE, DBUS_ML_BUS_NAME, DBUS_PIPELINE_PATH,
            NULL, &_err);
        if (mlsp) {
          break;
        }
      }
      proxy = (ml_agent_dbus_proxy_h *) mlsp;
      break;
    }
    case ML_AGENT_DBUS_SERVICE_MODEL:
    {
      MachinelearningServiceModel *mlsm;

      for (i = 0; i < num_bus_types; ++i) {
        g_clear_error (&_err);

        mlsm =
            machinelearning_service_model_proxy_new_for_bus_sync (bus_types[i],
            G_DBUS_PROXY_FLAGS_NONE, DBUS_ML_BUS_NAME, DBUS_MODEL_PATH, NULL,
            &_err);
        if (mlsm)
          break;
      }
      proxy = (ml_agent_dbus_proxy_h *) mlsm;
      break;
    }
    default:
      break;
  }

  if (_err) {
    *err = g_error_copy (_err);
    g_clear_error (&_err);
  }

  return proxy;
}

/**
 * @brief A dbus interface exported for setting the description of a pipeline
 */
gint
ml_agent_dbus_interface_pipeline_set_description (const gchar * name,
    const gchar * pipeline_desc, GError ** err)
{
  MachinelearningServicePipeline *mlsp;
  gboolean result;

  if (!name || !pipeline_desc) {
    g_return_val_if_reached (-EINVAL);
  }

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_set_pipeline_sync
      (MACHINELEARNING_SERVICE_PIPELINE (mlsp), name, pipeline_desc, NULL, NULL,
      err);
  g_object_unref (mlsp);
  g_return_val_if_fail (result, -EIO);

  return 0;
}

/**
 * @brief A dbus interface exported for getting the pipeline's description corresponding to the given @name
 */
gint
ml_agent_dbus_interface_pipeline_get_description (const gchar * name,
    gchar ** pipeline_desc, GError ** err)
{
  MachinelearningServicePipeline *mlsp;
  gboolean result;
  gint ret;

  if (!name || !pipeline_desc) {
    g_return_val_if_reached (-EINVAL);
  }

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_get_pipeline_sync
      (MACHINELEARNING_SERVICE_PIPELINE (mlsp), name, &ret, pipeline_desc, NULL,
      err);
  g_object_unref (mlsp);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for deletion of the pipeline's description corresponding to the given @name
 */
gint
ml_agent_dbus_interface_pipeline_delete (const gchar * name, GError ** err)
{
  MachinelearningServicePipeline *mlsp;
  gboolean result;
  gint ret;

  if (!name) {
    g_return_val_if_reached (-EINVAL);
  }

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_delete_pipeline_sync
      (MACHINELEARNING_SERVICE_PIPELINE (mlsp), name, &ret, NULL, err);
  g_object_unref (mlsp);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for launching the pipeline's description corresponding to the given @name
 */
gint
ml_agent_dbus_interface_pipeline_launch (const gchar * name, gint64 * id,
    GError ** err)
{
  MachinelearningServicePipeline *mlsp;
  gboolean result;
  gint ret;

  if (!name) {
    g_return_val_if_reached (-EINVAL);
  }

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_launch_pipeline_sync (mlsp, name,
      &ret, id, NULL, err);
  g_object_unref (mlsp);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for changing the pipeline's state of the given @id to start
 */
gint
ml_agent_dbus_interface_pipeline_start (gint64 id, GError ** err)
{
  MachinelearningServicePipeline *mlsp;
  gboolean result;
  gint ret;

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_start_pipeline_sync (mlsp, id, &ret,
      NULL, err);
  g_object_unref (mlsp);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for changing the pipeline's state of the given @id to stop
 */
gint
ml_agent_dbus_interface_pipeline_stop (gint64 id, GError ** err)
{
  MachinelearningServicePipeline *mlsp;
  gboolean result;
  gint ret;

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_stop_pipeline_sync (mlsp, id, &ret,
      NULL, err);
  g_object_unref (mlsp);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for destroying a launched pipeline corresponding to the given @id
 */
gint
ml_agent_dbus_interface_pipeline_destroy (gint64 id, GError ** err)
{
  MachinelearningServicePipeline *mlsp;

  gboolean result;
  gint ret;

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_destroy_pipeline_sync (mlsp, id,
      &ret, NULL, err);
  g_object_unref (mlsp);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for getting the pipeline's state of the given @id
 */
gint
ml_agent_dbus_interface_pipeline_get_state (gint64 id, gint * state,
    GError ** err)
{
  MachinelearningServicePipeline *mlsp;

  gboolean result;
  gint ret;

  mlsp = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_PIPELINE, err);
  if (!mlsp) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_pipeline_call_get_state_sync (mlsp, id, &ret,
      state, NULL, err);
  g_object_unref (mlsp);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}


/**
 * @brief A dbus interface exported for registering a model
 */
gint
ml_agent_dbus_interface_model_register (const gchar * name, const gchar * path,
    const gboolean activate, const gchar * description, const gchar *app_info,
    guint * version, GError ** err)
{
  MachinelearningServiceModel *mlsm;

  gboolean result;
  gint ret;

  mlsm = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_MODEL, err);
  if (!mlsm) {
    g_return_val_if_reached (-EIO);
  }

  result = machinelearning_service_model_call_register_sync (mlsm, name, path,
      activate, description ? description : "", app_info ? app_info : "",
      version, &ret, NULL, err);
  g_object_unref (mlsm);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for updating the description of the given model, @name
 */
gint
ml_agent_dbus_interface_model_update_description (const gchar * name,
    const guint version, const gchar * description, GError ** err)
{
  MachinelearningServiceModel *mlsm;

  gboolean result;
  gint ret;

  mlsm = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_MODEL, err);
  if (!mlsm) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_model_call_update_description_sync (mlsm, name,
      version, description, &ret, NULL, err);
  g_object_unref (mlsm);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for activating the model of @name and @version
 */
gint
ml_agent_dbus_interface_model_activate (const gchar * name, const guint version,
    GError ** err)
{
  MachinelearningServiceModel *mlsm;

  gboolean result;
  gint ret;

  mlsm = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_MODEL, err);
  if (!mlsm) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_model_call_activate_sync (mlsm, name, version,
      &ret, NULL, err);
  g_object_unref (mlsm);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for getting the description of the given model of @name and @version
 */
gint
ml_agent_dbus_interface_model_get (const gchar * name, const guint version,
    gchar ** description, GError ** err)
{
  MachinelearningServiceModel *mlsm;

  gboolean result;
  gint ret;

  mlsm = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_MODEL, err);
  if (!mlsm) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_model_call_get_sync (mlsm, name, version,
      description, &ret, NULL, err);
  g_object_unref (mlsm);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for getting the description of the activated model of the given @name
 */
gint
ml_agent_dbus_interface_model_get_activated (const gchar * name,
    gchar ** description, GError ** err)
{
  MachinelearningServiceModel *mlsm;
  gboolean result;
  gint ret;

  mlsm = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_MODEL, err);
  if (!mlsm) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_model_call_get_activated_sync (mlsm, name,
      description, &ret, NULL, err);
  g_object_unref (mlsm);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief  A dbus interface exported for getting the description of all the models corresponding to the given @name
 */
gint
ml_agent_dbus_interface_model_get_all (const gchar * name, gchar ** description,
    GError ** err)
{
  MachinelearningServiceModel *mlsm;
  gboolean result;
  gint ret;

  mlsm = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_MODEL, err);
  if (!mlsm) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_model_call_get_all_sync (mlsm, name, description,
      &ret, NULL, err);
  g_object_unref (mlsm);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}

/**
 * @brief A dbus interface exported for removing the model of @name and @version
 */
gint
ml_agent_dbus_interface_model_delete (const gchar * name, const guint version,
    GError ** err)
{
  MachinelearningServiceModel *mlsm;
  gboolean result;
  gint ret;

  mlsm = _get_proxy_new_for_bus_sync (ML_AGENT_DBUS_SERVICE_MODEL, err);
  if (!mlsm) {
    g_return_val_if_reached (-EIO);
  }

  result =
      machinelearning_service_model_call_delete_sync (mlsm, name, version, &ret,
      NULL, err);
  g_object_unref (mlsm);

  g_return_val_if_fail (ret == 0 && result, ret);

  return 0;
}
