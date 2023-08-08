/**
 * @file    ml-agent-interface.h
 * @date    5 April 2023
 * @brief   A set of exported ml-agent interfaces for managing pipelines, models, and other service.
 * @see     https://github.com/nnstreamer/api
 * @author  Wook Song <wook16.song@samsung.com>
 * @bug     No known bugs except for NYI items
 */

#ifndef __ML_AGENT_INTERFACE_H__
#define __ML_AGENT_INTERFACE_H__
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib.h>

/**
 * @brief An interface exported for setting the description of a pipeline
 * @param[in] name A name indicating the pipeline whose description would be set
 * @param[in] pipeline_desc A stringified description of the pipeline
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_set_description (const gchar *name, const gchar *pipeline_desc, GError **err);

/**
 * @brief An interface exported for getting the pipeline's description corresponding to the given @name
 * @param[in] name A given name of the pipeline to get the description
 * @param[out] pipeline_desc A stringified description of the pipeline
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_get_description (const gchar *name, gchar **pipeline_desc, GError **err);

/**
 * @brief An interface exported for deletion of the pipeline's description corresponding to the given @name
 * @param[in] name A given name of the pipeline to remove the description
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_delete (const gchar *name, GError **err);

/**
 * @brief An interface exported for launching the pipeline's description corresponding to the given @name
 * @param[in] name A given name of the pipeline to launch
 * @param[out] id Return an integer identifier for the launched pipeline
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_launch (const gchar *name, gint64 *id, GError **err);

/**
 * @brief An interface exported for changing the pipeline's state of the given @id to start
 * @param[in] id An identifier of the launched pipeline whose state would be changed to start
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_start (gint64 id, GError **err);

/**
 * @brief An interface exported for changing the pipeline's state of the given @id to stop
 * @param[in] id An identifier of the launched pipeline whose state would be changed to stop
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_stop (gint64 id, GError **err);

/**
 * @brief An interface exported for destroying a launched pipeline corresponding to the given @id
 * @param[in] id An identifier of the launched pipeline that would be destroyed
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_destroy (gint64 id, GError **err);

/**
 * @brief An interface exported for getting the pipeline's state of the given @id
 * @param[in] id An identifier of the launched pipeline that would be destroyed
 * @param[out] state Return location for the pipeline's state
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_pipeline_get_state (gint64 id, gint *state, GError **err);

/**
 * @brief An interface exported for registering a model
 * @param[in] name A name indicating the model that would be registered
 * @param[in] path A path that specifies the location of the model file
 * @param[in] activate An initial activation state
 * @param[in] description A stringified description of the given model
 * @param[in] app_info Application-specific information from Tizen's RPK
 * @param[out] version Return location for the version of the given model registered
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_model_register(const gchar *name, const gchar *path, const gboolean activate, const gchar *description, const gchar *app_info, guint *version, GError **err);

/**
 * @brief An interface exported for updating the description of the given model, @name
 * @param[in] name A name indicating the model whose description would be updated
 * @param[in] version A version for identifying the model whose description would be updated
 * @param[in] description A new description to update the existing one
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_model_update_description (const gchar *name, const guint version, const gchar *description, GError **err);

/**
 * @brief An interface exported for activating the model of @name and @version
 * @param[in] name A name indicating a registered model
 * @param[in] version A version of the given model, @name
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_model_activate(const gchar *name, const guint version, GError **err);

/**
 * @brief An interface exported for getting the description of the given model of @name and @version
 * @param[in] name A name indicating the model whose description would be get
 * @param[in] version A version of the given model, @name
 * @param[out] description Return location for the description of the given model of @name and @version
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_model_get(const gchar *name, const guint version, gchar **description, GError **err);

/**
 * @brief An interface exported for getting the description of the activated model of the given @name
 * @param[in] name A name indicating the model whose description would be get
 * @param[out] description Return location for the description of an activated model of the given @name
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_model_get_activated(const gchar *name, gchar **description, GError **err);

/**
 * @brief An interface exported for getting the description of all the models corresponding to the given @name
 * @param[in] name A name indicating the models whose description would be get
 * @param[out] description Return location for the description of all the models corresponding to the given @name
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_model_get_all(const gchar *name, gchar **description, GError **err);

/**
 * @brief An interface exported for removing the model of @name and @version
 * @param[in] name A name indicating the model that would be removed
 * @param[in] version A version for identifying a specific model whose name is @name
 * @param[in] description A new description to update the existing one
 * @param[out] err Return location for error of NULL
 * @return 0 on success, a negative error value if @err is set
 */
gint ml_agent_model_delete(const gchar *name, const guint version, GError **err);

/**
 * @brief An interface exported for adding the resource
 * @param[in] name A name indicating the resource
 * @param[in] path A path that specifies the location of the resource
 * @param[in] description A stringified description of the resource
 * @param[in] app_info Application-specific information from Tizen's RPK
 * @param[out] err Return location for error
 * @return 0 on success, a negative error value if failed
 */
gint ml_agent_resource_add (const gchar *name, const gchar *path, const gchar *description, const gchar *app_info, GError **err);

/**
 * @brief An interface exported for removing the resource with @name
 * @param[in] name A name indicating the resource
 * @param[out] err Return location for error
 * @return 0 on success, a negative error value if failed
 */
gint ml_agent_resource_delete (const gchar *name, GError **err);

/**
 * @brief An interface exported for getting the description of the resource with @name
 * @param[in] name A name indicating the resource
 * @param[out] res_info Return location for the information of the resource
 * @param[out] err Return location for error
 * @return 0 on success, a negative error value if failed
 */
gint ml_agent_resource_get (const gchar *name, gchar **res_info, GError **err);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_AGENT_INTERFACE_H__ */
