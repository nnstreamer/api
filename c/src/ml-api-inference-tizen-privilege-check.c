/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-inference-tizen-privilege-check.c
 * @date 22 July 2020
 * @brief NNStreamer/C-API Tizen dependent functions for inference APIs.
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */

#if !defined (__TIZEN__) || !defined (__PRIVILEGE_CHECK_SUPPORT__)
#error "This file can be included only in Tizen."
#endif

#include <glib.h>
#include <nnstreamer.h>
#include "ml-api-internal.h"
#include "ml-api-inference-internal.h"
#include "ml-api-inference-pipeline-internal.h"

#include <restriction.h>        /* device policy manager */
#if TIZENPPM
#include <privacy_privilege_manager.h>
#endif
#if TIZEN9PLUS
#include <unistd.h>
#include <rm_api.h>
#include <resource_center.h>
#elif TIZEN5PLUS
#include <mm_resource_manager.h>
#endif
#include <mm_camcorder.h>

/**
 * @brief Internal enumeration for tizen-mm resource type.
 * @todo we should check the changes of tizen resource manager.
 */
typedef enum
{
  TIZEN_MM_RES_TYPE_VIDEO_DECODER = 0, /**< ID of video decoder resource type */
  TIZEN_MM_RES_TYPE_VIDEO_OVERLAY,     /**< ID of video overlay resource type */
  TIZEN_MM_RES_TYPE_CAMERA,            /**< ID of camera resource type */
  TIZEN_MM_RES_TYPE_VIDEO_ENCODER,     /**< ID of video encoder resource type */
  TIZEN_MM_RES_TYPE_RADIO,             /**< ID of radio resource type */
  TIZEN_MM_RES_TYPE_AUDIO_OFFLOAD,     /**< ID of audio offload resource type */

  TIZEN_MM_RES_TYPE_MAX                /**< Used to iterate on resource types only */
} tizen_mm_res_type_e;

#if TIZENMMCONF
/* We can use "MMCAM_VIDEOSRC_ELEMENT_NAME and MMCAM_AUDIOSRC_ELEMENT_NAME */
#else /* TIZENMMCONF */
/* Tizen multimedia framework */
/* Defined in mm_camcorder_configure.h */

/**
 * @brief Structure to parse ini file for mmfw elements.
 */
typedef struct _type_int
{
  const char *name;
  int value;
} type_int;

/**
 * @brief Structure to parse ini file for mmfw elements.
 */
typedef struct _type_string
{
  const char *name;
  const char *value;
} type_string;

/**
 * @brief Structure to parse ini file for mmfw elements.
 */
typedef struct _type_element
{
  const char *name;
  const char *element_name;
  type_int **value_int;
  int count_int;
  type_string **value_string;
  int count_string;
} type_element;

/**
 * @brief Structure to parse ini file for mmfw elements.
 */
typedef struct _conf_detail
{
  int count;
  void **detail_info;
} conf_detail;

/**
 * @brief Structure to parse ini file for mmfw elements.
 */
typedef struct _camera_conf
{
  int type;
  conf_detail **info;
} camera_conf;

#define MMFW_CONFIG_MAIN_FILE "mmfw_camcorder.ini"

extern int
_mmcamcorder_conf_get_info (MMHandleType handle, int type, const char *ConfFile,
    camera_conf ** configure_info);

extern void
_mmcamcorder_conf_release_info (MMHandleType handle,
    camera_conf ** configure_info);

extern int
_mmcamcorder_conf_get_element (MMHandleType handle,
    camera_conf * configure_info, int category, const char *name,
    type_element ** element);

extern int
_mmcamcorder_conf_get_value_element_name (type_element * element,
    const char **value);
#endif /* TIZENMMCONF */

/**
 * @brief Internal structure for tizen mm framework.
 */
typedef struct
{
  gboolean invalid; /**< flag to indicate rm handle is valid */
  gpointer rm_h; /**< rm handle */
  device_policy_manager_h dpm_h; /**< dpm handle */
  int dpm_cb_id; /**< dpm callback id */
  gboolean has_video_src; /**< pipeline includes video src */
  gboolean has_audio_src; /**< pipeline includes audio src */
  GHashTable *res_handles; /**< hash table of resource handles */
  gpointer priv; /**< private data for rm */
} tizen_mm_handle_s;

/**
 * @brief Tizen resource type for multimedia.
 */
#define TIZEN_RES_MM "tizen_res_mm"

/**
 * @brief Tizen Privilege Camera (See https://www.tizen.org/privilege)
 */
#define TIZEN_PRIVILEGE_CAMERA "http://tizen.org/privilege/camera"

/**
 * @brief Tizen Privilege Recoder (See https://www.tizen.org/privilege)
 */
#define TIZEN_PRIVILEGE_RECODER "http://tizen.org/privilege/recorder"

/** The following functions are either not used or supported in Tizen 4 */
#if TIZEN5PLUS
#if TIZENPPM
/**
 * @brief Function to check tizen privilege.
 */
static int
ml_tizen_check_privilege (const gchar * privilege)
{
  int status = ML_ERROR_NONE;
  ppm_check_result_e priv_result;
  int err;

  /* check privilege */
  err = ppm_check_permission (privilege, &priv_result);
  if (err == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE &&
      priv_result == PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW) {
    /* privilege allowed */
  } else {
    _ml_loge ("Failed to check the privilege %s.", privilege);
    status = ML_ERROR_PERMISSION_DENIED;
  }

  return status;
}
#else
#define ml_tizen_check_privilege(...) (ML_ERROR_NONE)
#endif /* TIZENPPM */

/**
 * @brief Function to check device policy.
 */
static int
ml_tizen_dpm_check_restriction (device_policy_manager_h dpm_handle, int type)
{
  int err = DPM_ERROR_NOT_PERMITTED;
  int dpm_is_allowed = 0;

  switch (type) {
    case 1:                    /* camera */
      err = dpm_restriction_get_camera_state (dpm_handle, &dpm_is_allowed);
      break;
    case 2:                    /* mic */
      err = dpm_restriction_get_microphone_state (dpm_handle, &dpm_is_allowed);
      break;
    default:
      /* unknown type */
      break;
  }

  if (err != DPM_ERROR_NONE || dpm_is_allowed != 1) {
    _ml_loge ("Failed, device policy is not allowed.");
    return ML_ERROR_PERMISSION_DENIED;
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Callback to be called when device policy is changed.
 */
static void
ml_tizen_dpm_policy_changed_cb (const char *name, const char *state,
    void *user_data)
{
  ml_pipeline *p;

  g_return_if_fail (state);
  g_return_if_fail (user_data);

  p = (ml_pipeline *) user_data;

  if (g_ascii_strcasecmp (state, "disallowed") == 0) {
    g_mutex_lock (&p->lock);

    /* pause the pipeline */
    gst_element_set_state (p->element, GST_STATE_PAUSED);

    g_mutex_unlock (&p->lock);
  }

  return;
}

/**
 * @brief Function to get key string of resource type to handle hash table.
 */
static gchar *
ml_tizen_mm_res_get_key_string (tizen_mm_res_type_e type)
{
  gchar *res_key = NULL;

  switch (type) {
    case TIZEN_MM_RES_TYPE_VIDEO_DECODER:
      res_key = g_strdup ("tizen_mm_res_video_decoder");
      break;
    case TIZEN_MM_RES_TYPE_VIDEO_OVERLAY:
      res_key = g_strdup ("tizen_mm_res_video_overlay");
      break;
    case TIZEN_MM_RES_TYPE_CAMERA:
      res_key = g_strdup ("tizen_mm_res_camera");
      break;
    case TIZEN_MM_RES_TYPE_VIDEO_ENCODER:
      res_key = g_strdup ("tizen_mm_res_video_encoder");
      break;
    case TIZEN_MM_RES_TYPE_RADIO:
      res_key = g_strdup ("tizen_mm_res_radio");
      break;
    case TIZEN_MM_RES_TYPE_AUDIO_OFFLOAD:
      res_key = g_strdup ("tizen_mm_res_audio_offload");
      break;
    default:
      _ml_logw ("The resource type %d is invalid.", type);
      break;
  }

  return res_key;
}

/**
 * @brief Function to get resource type from key string to handle hash table.
 */
static tizen_mm_res_type_e
ml_tizen_mm_res_get_type (const gchar * res_key)
{
  tizen_mm_res_type_e type = TIZEN_MM_RES_TYPE_MAX;

  g_return_val_if_fail (res_key, type);

  if (g_str_equal (res_key, "tizen_mm_res_video_decoder")) {
    type = TIZEN_MM_RES_TYPE_VIDEO_DECODER;
  } else if (g_str_equal (res_key, "tizen_mm_res_video_overlay")) {
    type = TIZEN_MM_RES_TYPE_VIDEO_OVERLAY;
  } else if (g_str_equal (res_key, "tizen_mm_res_camera")) {
    type = TIZEN_MM_RES_TYPE_CAMERA;
  } else if (g_str_equal (res_key, "tizen_mm_res_video_encoder")) {
    type = TIZEN_MM_RES_TYPE_VIDEO_ENCODER;
  } else if (g_str_equal (res_key, "tizen_mm_res_radio")) {
    type = TIZEN_MM_RES_TYPE_RADIO;
  } else if (g_str_equal (res_key, "tizen_mm_res_audio_offload")) {
    type = TIZEN_MM_RES_TYPE_AUDIO_OFFLOAD;
  }

  return type;
}

#if TIZEN9PLUS
/**
 * @brief Function to get resource manager appid using pid
 */
static int
ml_tizen_mm_rm_get_appid (rm_consumer_info ** rci)
{
  g_autofree gchar *cmdline = NULL;
  g_autofree gchar *contents = NULL;
  g_autofree gchar *base = NULL;
  rm_consumer_info *consumer_info = NULL;
  size_t size;

  consumer_info = g_new0 (rm_consumer_info, 1);
  if (!consumer_info) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate new memory for resource info.");
  }

  consumer_info->app_pid = (int) getpid ();
  size = sizeof (consumer_info->app_id);
  cmdline = g_strdup_printf ("/proc/%d/cmdline", consumer_info->app_pid);
  if (!g_file_get_contents (cmdline, &contents, NULL, NULL)) {
    g_free (consumer_info);
    _ml_error_report_return (ML_ERROR_UNKNOWN,
        "Failed to get appid, cannot read proc.");
  }

  base = g_path_get_basename (contents);
  if (g_strlcpy (consumer_info->app_id, base, size) >= size) {
    g_free (consumer_info);
    _ml_error_report_return (ML_ERROR_UNKNOWN,
        "Failed to get appid, string truncated.");
  }

  *rci = consumer_info;
  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to release resource manager.
 */
static void
ml_tizen_mm_res_release_rm (tizen_mm_handle_s * mm_handle)
{
  rm_device_return_s *device;
  int rm_h;
  int i, ret;

  rm_h = GPOINTER_TO_INT (mm_handle->rm_h);

  if (g_hash_table_size (mm_handle->res_handles)) {
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, mm_handle->res_handles);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      pipeline_resource_s *mm_res = value;

      device = (rm_device_return_s *) mm_res->handle;
      mm_res->handle = NULL;
      if (device) {
        if (device->allocated_num > 0) {
          rm_device_request_s requested = { 0 };

          requested.request_num = device->allocated_num;
          for (i = 0; i < requested.request_num; i++) {
            requested.device_id[i] = device->device_id[i];
          }

          ret = rm_deallocate_resources (rm_h, &requested);
          if (ret != RM_OK) {
            _ml_loge
                ("Failed to deallocate resource (%d), allocated num is %d.",
                ret, requested.request_num);
          }

          for (i = 0; i < device->allocated_num; i++) {
            g_free (device->device_node[i]);
            g_free (device->omx_comp_name[i]);
          }
        }

        g_free (device);
      }
    }
  }

  ret = rm_unregister (rm_h);
  if (ret != RM_OK) {
    _ml_loge ("Failed to unregister resource manager (%d).", ret);
  }

  g_free (mm_handle->priv);

  mm_handle->rm_h = NULL;
  mm_handle->invalid = FALSE;
  mm_handle->priv = NULL;
}

/**
 * @brief Callback function for tizen-mm resource manager.
 */
static rm_cb_result
ml_tizen_mm_rm_resource_cb (int handle, rm_callback_type event,
    rm_device_request_s * info, void *data)
{
  ml_pipeline_h pipe = (ml_pipeline_h) data;
  ml_pipeline *p = (ml_pipeline *) pipe;
  pipeline_resource_s *res;
  tizen_mm_handle_s *mm_handle;

  g_mutex_lock (&p->lock);

  res =
      (pipeline_resource_s *) g_hash_table_lookup (p->resources, TIZEN_RES_MM);
  if (!res) {
    _ml_error_report
        ("Internal function error: cannot find the resource, '%s', from the resource table.",
        TIZEN_RES_MM);
    goto done;
  }

  mm_handle = (tizen_mm_handle_s *) res->handle;
  if (!mm_handle) {
    _ml_error_report
        ("Internal function error: the resource '%s' does not have a valid mm handle (NULL).",
        TIZEN_RES_MM);
    goto done;
  }

  switch (event) {
    case RM_CALLBACK_TYPE_RESOURCE_CONFLICT:
    case RM_CALLBACK_TYPE_RESOURCE_CONFLICT_UD:
      /* release resource and pause pipeline */
      mm_handle->invalid = TRUE;
      ml_tizen_mm_res_release_rm (mm_handle);
      gst_element_set_state (p->element, GST_STATE_PAUSED);
      break;
    default:
      break;
  }

done:
  g_mutex_unlock (&p->lock);
  return RM_CB_RESULT_OK;
}

/**
 * @brief Internal function to create resource manager.
 */
static int
ml_tizen_mm_res_create_rm (ml_pipeline_h pipe, tizen_mm_handle_s * mm_handle)
{
  rm_consumer_info *rci = NULL;
  int rm_h;
  int ret;

  /* Get app id */
  ret = ml_tizen_mm_rm_get_appid (&rci);
  if (ret != ML_ERROR_NONE) {
    _ml_error_report_return (ret, "Failed to get appid using pid.");
  }

  ret = rm_register (ml_tizen_mm_rm_resource_cb, pipe, &rm_h, rci);
  if (ret != RM_OK) {
    g_free (rci);
    _ml_error_report_return (ML_ERROR_UNKNOWN,
        "Failed to register resource manager (%d).", ret);
  }

  mm_handle->rm_h = GINT_TO_POINTER (rm_h);
  mm_handle->priv = rci;
  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to get the handle of resource type.
 */
static int
ml_tizen_mm_res_get_handle (tizen_mm_handle_s * mm_handle,
    tizen_mm_res_type_e res_type, gpointer * handle)
{
  rm_category_request_s request = { 0 };
  rm_device_return_s *device;
  rm_consumer_info *rci;
  rm_rsc_category_e category_id = RM_CATEGORY_NONE;
  int rm_h;
  int category_option;
  int ret;

  switch (res_type) {
    case TIZEN_MM_RES_TYPE_VIDEO_DECODER:
      category_id = RM_CATEGORY_VIDEO_DECODER;
      break;
    case TIZEN_MM_RES_TYPE_VIDEO_OVERLAY:
      category_id = RM_CATEGORY_SCALER;
      break;
    case TIZEN_MM_RES_TYPE_CAMERA:
      category_id = RM_CATEGORY_CAMERA;
      break;
    case TIZEN_MM_RES_TYPE_VIDEO_ENCODER:
      category_id = RM_CATEGORY_VIDEO_ENCODER;
      break;
    case TIZEN_MM_RES_TYPE_RADIO:
      category_id = RM_CATEGORY_RADIO;
      break;
    case TIZEN_MM_RES_TYPE_AUDIO_OFFLOAD:
      category_id = RM_CATEGORY_AUDIO_OFFLOAD;
      break;
    default:
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Unknown resource type.");
  }

  rm_h = GPOINTER_TO_INT (mm_handle->rm_h);
  rci = (rm_consumer_info *) mm_handle->priv;

  device = (rm_device_return_s *) g_new0 (rm_device_return_s, 1);
  if (device == NULL) {
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate new memory for resource device.");
  }

  category_option = rc_get_capable_category_id (rm_h, rci->app_id, category_id);

  request.request_num = 1;
  request.state[0] = RM_STATE_EXCLUSIVE;
  request.category_id[0] = category_id;
  request.category_option[0] = category_option;

  ret = rm_allocate_resources (rm_h, &request, device);
  if (ret != RM_OK) {
    _ml_loge ("Failed to allocate resource for type %d (%d).", res_type, ret);
    g_free (device);
    return ML_ERROR_UNKNOWN;
  }

  *handle = device;
  return ML_ERROR_NONE;
}
#else
/**
 * @brief Internal function to release resource manager.
 */
static void
ml_tizen_mm_res_release_rm (tizen_mm_handle_s * mm_handle)
{
  mm_resource_manager_h rm_h = (mm_resource_manager_h) mm_handle->rm_h;

  if (g_hash_table_size (mm_handle->res_handles)) {
    GHashTableIter iter;
    gpointer key, value;
    gboolean marked = FALSE;

    g_hash_table_iter_init (&iter, mm_handle->res_handles);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      pipeline_resource_s *mm_res = value;

      if (mm_res->handle) {
        mm_resource_manager_mark_for_release (rm_h, mm_res->handle);
        mm_res->handle = NULL;
        marked = TRUE;
      }
    }

    if (marked)
      mm_resource_manager_commit (rm_h);
  }

  mm_resource_manager_set_status_cb (rm_h, NULL, NULL);
  mm_resource_manager_destroy (rm_h);

  mm_handle->rm_h = NULL;
  mm_handle->invalid = FALSE;
}

/**
 * @brief Callback to be called from mm resource manager.
 */
static int
ml_tizen_mm_res_release_cb (mm_resource_manager_h rm,
    mm_resource_manager_res_h resource_h, void *user_data)
{
  ml_pipeline *p;
  pipeline_resource_s *res;
  tizen_mm_handle_s *mm_handle;

  g_return_val_if_fail (user_data, FALSE);
  p = (ml_pipeline *) user_data;
  g_mutex_lock (&p->lock);

  res =
      (pipeline_resource_s *) g_hash_table_lookup (p->resources, TIZEN_RES_MM);
  if (!res) {
    /* rm handle is not registered or removed */
    goto done;
  }

  mm_handle = (tizen_mm_handle_s *) res->handle;
  if (!mm_handle) {
    /* supposed the rm handle is already released */
    goto done;
  }

  /* pause pipeline */
  gst_element_set_state (p->element, GST_STATE_PAUSED);
  mm_handle->invalid = TRUE;

done:
  g_mutex_unlock (&p->lock);
  return FALSE;
}

/**
 * @brief Callback to be called from mm resource manager.
 */
static void
ml_tizen_mm_res_status_cb (mm_resource_manager_h rm,
    mm_resource_manager_status_e status, void *user_data)
{
  ml_pipeline *p;
  pipeline_resource_s *res;
  tizen_mm_handle_s *mm_handle;

  g_return_if_fail (user_data);

  p = (ml_pipeline *) user_data;
  g_mutex_lock (&p->lock);

  res =
      (pipeline_resource_s *) g_hash_table_lookup (p->resources, TIZEN_RES_MM);
  if (!res) {
    /* rm handle is not registered or removed */
    goto done;
  }

  mm_handle = (tizen_mm_handle_s *) res->handle;
  if (!mm_handle) {
    /* supposed the rm handle is already released */
    goto done;
  }

  switch (status) {
    case MM_RESOURCE_MANAGER_STATUS_DISCONNECTED:
      /* pause pipeline, rm handle should be released */
      gst_element_set_state (p->element, GST_STATE_PAUSED);
      mm_handle->invalid = TRUE;
      break;
    default:
      break;
  }

done:
  g_mutex_unlock (&p->lock);
}

/**
 * @brief Internal function to create resource manager.
 */
static int
ml_tizen_mm_res_create_rm (ml_pipeline_h pipe, tizen_mm_handle_s * mm_handle)
{
  mm_resource_manager_h rm_h = (mm_resource_manager_h) mm_handle->rm_h;
  int err;

  /* Already created */
  if (rm_h)
    return ML_ERROR_NONE;

  err = mm_resource_manager_create (MM_RESOURCE_MANAGER_APP_CLASS_MEDIA,
      ml_tizen_mm_res_release_cb, pipe, &rm_h);
  if (err != MM_RESOURCE_MANAGER_ERROR_NONE)
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Cannot create multimedia resource manager handle with mm_resource_manager_create (), it has returned %d. Please check if your Tizen installation is valid; do you have all multimedia packages properly installed?",
        err);

  /* add state change callback */
  err = mm_resource_manager_set_status_cb (rm_h, ml_tizen_mm_res_status_cb,
      pipe);
  if (err != MM_RESOURCE_MANAGER_ERROR_NONE) {
    mm_resource_manager_destroy (rm_h);
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Cannot configure status callback with multimedia resource manager, mm_resource_manager_set_status_cb (), it has returned %d. Please check if your Tizen installation is valid; do you have all multmedia packages properly installed?",
        err);
  }

  mm_handle->rm_h = rm_h;
  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to get the handle of resource type.
 */
static int
ml_tizen_mm_res_get_handle (tizen_mm_handle_s * mm_handle,
    tizen_mm_res_type_e res_type, gpointer * handle)
{
  mm_resource_manager_h rm_h = (mm_resource_manager_h) mm_handle->rm_h;
  mm_resource_manager_res_h rm_res_h;
  mm_resource_manager_res_type_e rm_res_type;
  int err;

  switch (res_type) {
    case TIZEN_MM_RES_TYPE_VIDEO_DECODER:
      rm_res_type = MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_DECODER;
      break;
    case TIZEN_MM_RES_TYPE_VIDEO_OVERLAY:
      rm_res_type = MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_OVERLAY;
      break;
    case TIZEN_MM_RES_TYPE_CAMERA:
      rm_res_type = MM_RESOURCE_MANAGER_RES_TYPE_CAMERA;
      break;
    case TIZEN_MM_RES_TYPE_VIDEO_ENCODER:
      rm_res_type = MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_ENCODER;
      break;
    case TIZEN_MM_RES_TYPE_RADIO:
      rm_res_type = MM_RESOURCE_MANAGER_RES_TYPE_RADIO;
      break;
    case TIZEN_MM_RES_TYPE_AUDIO_OFFLOAD:
      rm_res_type = MM_RESOURCE_MANAGER_RES_TYPE_AUDIO_OFFLOAD;
      break;
    default:
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Unknown resource type.");
  }

  /* add resource handle */
  err = mm_resource_manager_mark_for_acquire (rm_h, rm_res_type,
      MM_RESOURCE_MANAGER_RES_VOLUME_FULL, &rm_res_h);
  if (err != MM_RESOURCE_MANAGER_ERROR_NONE)
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Internal error of Tizen multimedia resource manager: mm_resource_manager_mark_for_acquire () cannot acquire resources. It has returned %d.",
        err);

  err = mm_resource_manager_commit (rm_h);
  if (err != MM_RESOURCE_MANAGER_ERROR_NONE)
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Internal error of Tizen multimedia resource manager: mm_resource_manager_commit has failed with error code: %d",
        err);

  *handle = rm_res_h;
  return ML_ERROR_NONE;
}
#endif /* TIZEN9PLUS */

/**
 * @brief Function to release the resource handle of tizen mm resource manager.
 */
static void
ml_tizen_mm_res_release (gpointer handle, gboolean destroy)
{
  tizen_mm_handle_s *mm_handle;

  g_return_if_fail (handle);

  mm_handle = (tizen_mm_handle_s *) handle;

  /* release res handles */
  ml_tizen_mm_res_release_rm (mm_handle);

  if (destroy) {
    if (mm_handle->dpm_h) {
      if (mm_handle->dpm_cb_id > 0) {
        dpm_remove_policy_changed_cb (mm_handle->dpm_h, mm_handle->dpm_cb_id);
        mm_handle->dpm_cb_id = 0;
      }

      dpm_manager_destroy (mm_handle->dpm_h);
      mm_handle->dpm_h = NULL;
    }

    g_hash_table_remove_all (mm_handle->res_handles);
    g_free (mm_handle);
  }
}

/**
 * @brief Function to initialize mm resource manager.
 */
static int
ml_tizen_mm_res_initialize (ml_pipeline_h pipe, gboolean has_video_src,
    gboolean has_audio_src)
{
  ml_pipeline *p;
  pipeline_resource_s *res;
  tizen_mm_handle_s *mm_handle = NULL;
  int status = ML_ERROR_STREAMS_PIPE;
  int err;

  p = (ml_pipeline *) pipe;

  res =
      (pipeline_resource_s *) g_hash_table_lookup (p->resources, TIZEN_RES_MM);

  /* register new resource handle of tizen mmfw */
  if (!res) {
    res = g_new0 (pipeline_resource_s, 1);
    if (!res) {
      _ml_loge ("Failed to allocate pipeline resource handle.");
      status = ML_ERROR_OUT_OF_MEMORY;
      goto rm_error;
    }

    res->type = g_strdup (TIZEN_RES_MM);
    g_hash_table_insert (p->resources, g_strdup (TIZEN_RES_MM), res);
  }

  mm_handle = (tizen_mm_handle_s *) res->handle;
  if (!mm_handle) {
    mm_handle = g_new0 (tizen_mm_handle_s, 1);
    if (!mm_handle) {
      _ml_loge ("Failed to allocate media resource handle.");
      status = ML_ERROR_OUT_OF_MEMORY;
      goto rm_error;
    }

    mm_handle->res_handles =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    /* device policy manager */
    mm_handle->dpm_h = dpm_manager_create ();
    err = dpm_add_policy_changed_cb (mm_handle->dpm_h, "camera",
        ml_tizen_dpm_policy_changed_cb, pipe, &mm_handle->dpm_cb_id);
    if (err != DPM_ERROR_NONE) {
      _ml_loge ("Failed to add device policy callback.");
      status = ML_ERROR_PERMISSION_DENIED;
      goto rm_error;
    }

    /* set mm handle */
    res->handle = mm_handle;
  }

  mm_handle->has_video_src = has_video_src;
  mm_handle->has_audio_src = has_audio_src;
  status = ML_ERROR_NONE;

rm_error:
  if (status != ML_ERROR_NONE) {
    /* failed to initialize mm handle */
    if (mm_handle)
      ml_tizen_mm_res_release (mm_handle, TRUE);
  }

  return status;
}

/**
 * @brief Internal function to acquire the handle from resource manager.
 */
static int
ml_tizen_mm_res_acquire_handle (tizen_mm_handle_s * mm_handle,
    tizen_mm_res_type_e res_type)
{
  int ret;

  if (res_type == TIZEN_MM_RES_TYPE_MAX) {
    GHashTableIter iter;
    gpointer key, value;

    /* iterate all handle and acquire res if released */
    g_hash_table_iter_init (&iter, mm_handle->res_handles);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      pipeline_resource_s *mm_res = value;

      if (!mm_res->handle) {
        tizen_mm_res_type_e type;

        type = ml_tizen_mm_res_get_type (mm_res->type);
        if (type != TIZEN_MM_RES_TYPE_MAX) {
          ret = ml_tizen_mm_res_get_handle (mm_handle, type, &mm_res->handle);
          if (ret != ML_ERROR_NONE)
            _ml_error_report_return_continue (ret,
                "Internal error: cannot get resource handle from Tizen multimedia resource manager.");
        }
      }
    }
  } else {
    gchar *res_key = ml_tizen_mm_res_get_key_string (res_type);
    if (res_key) {
      pipeline_resource_s *mm_res;

      mm_res =
          (pipeline_resource_s *) g_hash_table_lookup (mm_handle->res_handles,
          res_key);
      if (!mm_res) {
        mm_res = g_new0 (pipeline_resource_s, 1);
        if (mm_res == NULL) {
          _ml_loge ("Failed to allocate media resource data.");
          g_free (res_key);
          _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
              "Cannot allocate memory. Out of memory?");
        }

        mm_res->type = g_strdup (res_key);
        g_hash_table_insert (mm_handle->res_handles, g_strdup (res_key),
            mm_res);
      }

      g_free (res_key);

      if (!mm_res->handle) {
        ret = ml_tizen_mm_res_get_handle (mm_handle, res_type, &mm_res->handle);
        if (ret != ML_ERROR_NONE)
          _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
              "Cannot get handle from Tizen multimedia resource manager.");
      }
    }
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Function to acquire the resource from mm resource manager.
 */
static int
ml_tizen_mm_res_acquire (ml_pipeline_h pipe, tizen_mm_res_type_e res_type)
{
  ml_pipeline *p;
  pipeline_resource_s *res;
  tizen_mm_handle_s *mm_handle;
  int status = ML_ERROR_STREAMS_PIPE;

  p = (ml_pipeline *) pipe;

  res =
      (pipeline_resource_s *) g_hash_table_lookup (p->resources, TIZEN_RES_MM);
  if (!res)
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Internal function error: cannot find the resource, '%s', from the resource table",
        TIZEN_RES_MM);

  mm_handle = (tizen_mm_handle_s *) res->handle;
  if (!mm_handle)
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Internal function error: the resource '%s' does not have a valid mm handle (NULL).",
        TIZEN_RES_MM);

  /* check dpm state */
  if (mm_handle->has_video_src) {
    status = ml_tizen_dpm_check_restriction (mm_handle->dpm_h, 1);
    if (status != ML_ERROR_NONE)
      _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
          "Video camera source requires permission to access the camera; you do not have the permission. Your Tizen application is required to acquire video permission (DPM) from Tizen. Refer: https://docs.tizen.org/application/native/guides/security/dpm/");
  }

  if (mm_handle->has_audio_src) {
    status = ml_tizen_dpm_check_restriction (mm_handle->dpm_h, 2);
    if (status != ML_ERROR_NONE)
      _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
          "Audio mic source requires permission to access the mic; you do not have the permission. Your Tizen application is required to acquire audio/mic permission (DPM) from Tizen. Refer: https://docs.tizen.org/application/native/guides/security/dpm/");
  }

  /* check invalid handle */
  if (mm_handle->invalid)
    ml_tizen_mm_res_release (mm_handle, FALSE);

  /* create rm handle */
  status = ml_tizen_mm_res_create_rm (pipe, mm_handle);
  if (status != ML_ERROR_NONE)
    return status;

  /* acquire resource */
  status = ml_tizen_mm_res_acquire_handle (mm_handle, res_type);
  if (status != ML_ERROR_NONE)
    return status;

  return ML_ERROR_NONE;
}

/**
 * @brief Gets element name from mm and replaces element.
 */
static int
ml_tizen_mm_replace_element (gboolean has_video, gboolean has_audio,
    gchar ** description)
{
  MMHandleType hcam = NULL;
  MMCamPreset cam_info = { 0 };
  gchar *_video = NULL;         /* Do not free this! */
  gchar *_audio = NULL;         /* Do not free this! */
  guint changed = 0;
  int err;
#if !TIZENMMCONF
  camera_conf *conf = NULL;
  type_element *elem = NULL;
#endif

  /* create camcoder handle (primary camera) */
  if (has_video) {
    cam_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;
    /**
     * @note Now network camera is disabled. (cam_info.reserved[0] = 0)
     * If we need to set net-camera later, discuss with MM team.
     */
  } else {
    /* no camera */
    cam_info.videodev_type = MM_VIDEO_DEVICE_NONE;
  }

  err = mm_camcorder_create (&hcam, &cam_info);
  if (err != MM_ERROR_NONE) {
    _ml_loge ("Fail to call mm_camcorder_create = %x\n", err);
    err = ML_ERROR_STREAMS_PIPE;
    goto error;
  }

#if TIZENMMCONF                 /* 6.5 or higher */
  if (has_video) {
    int size = 0;
    err = mm_camcorder_get_attributes (hcam, NULL, MMCAM_VIDEOSRC_ELEMENT_NAME,
        &_video, &size, NULL);
    if (err != MM_ERROR_NONE || !_video || size < 1) {
      _ml_loge ("Failed to get attributes of MMCAM_VIDEOSRC_ELEMENT_NAME.");
      err = ML_ERROR_NOT_SUPPORTED;
      goto error;
    }
  }

  if (has_audio) {
    int size = 0;
    err = mm_camcorder_get_attributes (hcam, NULL, MMCAM_AUDIOSRC_ELEMENT_NAME,
        &_audio, &size, NULL);
    if (err != MM_ERROR_NONE || !_audio || size < 1) {
      _ml_loge ("Failed to get attributes of MMCAM_AUDIOSRC_ELEMENT_NAME.");
      err = ML_ERROR_NOT_SUPPORTED;
      goto error;
    }
  }
#else
  /* read ini, type CONFIGURE_TYPE_MAIN */
  err = _mmcamcorder_conf_get_info (hcam, 0, MMFW_CONFIG_MAIN_FILE, &conf);
  if (err != MM_ERROR_NONE || !conf) {
    _ml_loge ("Failed to load conf %s.", MMFW_CONFIG_MAIN_FILE);
    err = ML_ERROR_NOT_SUPPORTED;
    goto error;
  }

  if (has_video) {
    /* category CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT */
    _mmcamcorder_conf_get_element (hcam, conf, 1, "VideosrcElement", &elem);
    _mmcamcorder_conf_get_value_element_name (elem, (const char **) &_video);
    if (!_video) {
      _ml_loge ("Failed to get the name of videosrc element.");
      err = ML_ERROR_NOT_SUPPORTED;
      goto error;
    }
  }

  if (has_audio) {
    /* category CONFIGURE_CATEGORY_MAIN_AUDIO_INPUT */
    _mmcamcorder_conf_get_element (hcam, conf, 2, "AudiosrcElement", &elem);
    _mmcamcorder_conf_get_value_element_name (elem, (const char **) &_audio);
    if (!_audio) {
      _ml_loge ("Failed to get the name of audiosrc element.");
      err = ML_ERROR_NOT_SUPPORTED;
      goto error;
    }
  }
#endif

  /**
   * @todo handle the properties of video and audio src elements.
   * e.g., tizencamerasrc hal-name="" ! ...
   */
  if (has_video) {
    *description = _ml_replace_string (*description, ML_TIZEN_CAM_VIDEO_SRC,
        _video, " !", &changed);
    if (changed > 1) {
      /* Allow one src only in a pipeline */
      _ml_loge ("Cannot parse duplicated Tizen video src nodes.");
      err = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }
  }

  if (has_audio) {
    *description = _ml_replace_string (*description, ML_TIZEN_CAM_AUDIO_SRC,
        _audio, " !", &changed);
    if (changed > 1) {
      /* Allow one src only in a pipeline */
      _ml_loge ("Cannot parse duplicated Tizen audio src nodes.");
      err = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }
  }

  /* successfully done */
  err = ML_ERROR_NONE;

error:
#if !TIZENMMCONF
  if (conf)
    _mmcamcorder_conf_release_info (hcam, &conf);
#endif
  if (hcam)
    mm_camcorder_destroy (hcam);

  return err;
}

/**
 * @brief Converts predefined mmfw element.
 */
static int
ml_tizen_mm_convert_element (ml_pipeline_h pipe, gchar ** result,
    gboolean is_internal)
{
  gchar *video_src, *audio_src;
  gchar *desc = NULL;
  int status;

  video_src = g_strstr_len (*result, -1, ML_TIZEN_CAM_VIDEO_SRC);
  audio_src = g_strstr_len (*result, -1, ML_TIZEN_CAM_AUDIO_SRC);

  /* replace src element */
  if (video_src || audio_src) {
    gboolean has_video = (video_src != NULL);
    gboolean has_audio = (audio_src != NULL);

    /* copy pipeline description so that original description string is not changed if failed to replace element. */
    desc = g_strdup (*result);

    /* check privilege first */
    if (!is_internal) {
      /* ignore permission when runs as internal mode */
      if (has_video) {
        status = ml_tizen_check_privilege (TIZEN_PRIVILEGE_CAMERA);
        if (status != ML_ERROR_NONE)
          goto mm_error;
      }

      if (has_audio) {
        status = ml_tizen_check_privilege (TIZEN_PRIVILEGE_RECODER);
        if (status != ML_ERROR_NONE)
          goto mm_error;
      }
    }

    status = ml_tizen_mm_replace_element (has_video, has_audio, &desc);
    if (status != ML_ERROR_NONE)
      goto mm_error;

    /* initialize rm handle */
    status = ml_tizen_mm_res_initialize (pipe, has_video, has_audio);
    if (status != ML_ERROR_NONE)
      goto mm_error;

    /* get the camera resource using mm resource manager */
    status = ml_tizen_mm_res_acquire (pipe, TIZEN_MM_RES_TYPE_CAMERA);
    if (status != ML_ERROR_NONE)
      goto mm_error;

    g_free (*result);
    *result = desc;
  }

  /* done */
  status = ML_ERROR_NONE;

mm_error:
  if (status != ML_ERROR_NONE)
    g_free (desc);

  return status;
}
#else
/**
 * @brief A dummy function for Tizen 4.0
 */
static void
ml_tizen_mm_res_release (gpointer handle, gboolean destroy)
{
}

/**
 * @brief A dummy function for Tizen 4.0
 */
static int
ml_tizen_mm_res_acquire (ml_pipeline_h pipe, tizen_mm_res_type_e res_type)
{
  return ML_ERROR_NOT_SUPPORTED;
}

/**
 * @brief A dummy function for Tizen 4.0
 */
static int
ml_tizen_mm_convert_element (ml_pipeline_h pipe, gchar ** result,
    gboolean is_internal)
{
  return ML_ERROR_NOT_SUPPORTED;
}
#endif /* TIZEN5PLUS */

/**
 * @brief Releases the resource handle of Tizen.
 */
void
_ml_tizen_release_resource (gpointer handle, const gchar * res_type)
{
  if (g_str_equal (res_type, TIZEN_RES_MM)) {
    ml_tizen_mm_res_release (handle, TRUE);
  }
}

/**
 * @brief Gets the resource handle of Tizen.
 */
int
_ml_tizen_get_resource (ml_pipeline_h pipe, const gchar * res_type)
{
  int status = ML_ERROR_NONE;

  if (g_str_equal (res_type, TIZEN_RES_MM)) {
    /* iterate all handle and acquire res if released */
    status = ml_tizen_mm_res_acquire (pipe, TIZEN_MM_RES_TYPE_MAX);
  }

  return status;
}

/**
 * @brief Converts predefined element for Tizen.
 */
int
_ml_tizen_convert_element (ml_pipeline_h pipe, gchar ** result,
    gboolean is_internal)
{
  int status;

  /* convert predefined element of multimedia fw */
  status = ml_tizen_mm_convert_element (pipe, result, is_internal);

  return status;
}
