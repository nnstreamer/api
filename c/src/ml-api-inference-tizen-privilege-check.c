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

#include <system_info.h>
#include <restriction.h>        /* device policy manager */
#if TIZENPPM
#include <privacy_privilege_manager.h>
#endif
#if TIZEN5PLUS
#include <mm_resource_manager.h>
#endif
#include <mm_camcorder.h>


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
  mm_resource_manager_h rm_h; /**< rm handle */
  device_policy_manager_h dpm_h; /**< dpm handle */
  int dpm_cb_id; /**< dpm callback id */
  gboolean has_video_src; /**< pipeline includes video src */
  gboolean has_audio_src; /**< pipeline includes audio src */
  GHashTable *res_handles; /**< hash table of resource handles */
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
ml_tizen_check_dpm_restriction (device_policy_manager_h dpm_handle, int type)
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
ml_tizen_mm_res_get_key_string (mm_resource_manager_res_type_e type)
{
  gchar *res_key = NULL;

  switch (type) {
    case MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_DECODER:
      res_key = g_strdup ("tizen_mm_res_video_decoder");
      break;
    case MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_OVERLAY:
      res_key = g_strdup ("tizen_mm_res_video_overlay");
      break;
    case MM_RESOURCE_MANAGER_RES_TYPE_CAMERA:
      res_key = g_strdup ("tizen_mm_res_camera");
      break;
    case MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_ENCODER:
      res_key = g_strdup ("tizen_mm_res_video_encoder");
      break;
    case MM_RESOURCE_MANAGER_RES_TYPE_RADIO:
      res_key = g_strdup ("tizen_mm_res_radio");
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
static mm_resource_manager_res_type_e
ml_tizen_mm_res_get_type (const gchar * res_key)
{
  mm_resource_manager_res_type_e type = MM_RESOURCE_MANAGER_RES_TYPE_MAX;

  g_return_val_if_fail (res_key, type);

  if (g_str_equal (res_key, "tizen_mm_res_video_decoder")) {
    type = MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_DECODER;
  } else if (g_str_equal (res_key, "tizen_mm_res_video_overlay")) {
    type = MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_OVERLAY;
  } else if (g_str_equal (res_key, "tizen_mm_res_camera")) {
    type = MM_RESOURCE_MANAGER_RES_TYPE_CAMERA;
  } else if (g_str_equal (res_key, "tizen_mm_res_video_encoder")) {
    type = MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_ENCODER;
  } else if (g_str_equal (res_key, "tizen_mm_res_radio")) {
    type = MM_RESOURCE_MANAGER_RES_TYPE_RADIO;
  }

  return type;
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
 * @brief Function to get the handle of resource type.
 */
static int
ml_tizen_mm_res_get_handle (mm_resource_manager_h rm,
    mm_resource_manager_res_type_e res_type, gpointer * handle)
{
  mm_resource_manager_res_h rm_res_h;
  int err;

  /* add resource handle */
  err = mm_resource_manager_mark_for_acquire (rm, res_type,
      MM_RESOURCE_MANAGER_RES_VOLUME_FULL, &rm_res_h);
  if (err != MM_RESOURCE_MANAGER_ERROR_NONE)
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Internal error of Tizen multimedia resource manager: mm_resource_manager_mark_for_acquire () cannot acquire resources. It has returned %d.",
        err);

  err = mm_resource_manager_commit (rm);
  if (err != MM_RESOURCE_MANAGER_ERROR_NONE)
    _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
        "Internal error of Tizen multimedia resource manager: mm_resource_manager_commit has failed with error code: %d",
        err);

  *handle = rm_res_h;
  return ML_ERROR_NONE;
}

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
  if (g_hash_table_size (mm_handle->res_handles)) {
    GHashTableIter iter;
    gpointer key, value;
    gboolean marked = FALSE;

    g_hash_table_iter_init (&iter, mm_handle->res_handles);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      pipeline_resource_s *mm_res = value;

      if (mm_res->handle) {
        mm_resource_manager_mark_for_release (mm_handle->rm_h, mm_res->handle);
        mm_res->handle = NULL;
        marked = TRUE;
      }

      if (destroy)
        g_free (mm_res->type);
    }

    if (marked)
      mm_resource_manager_commit (mm_handle->rm_h);
  }

  mm_resource_manager_set_status_cb (mm_handle->rm_h, NULL, NULL);
  mm_resource_manager_destroy (mm_handle->rm_h);
  mm_handle->rm_h = NULL;

  mm_handle->invalid = FALSE;

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
    if (dpm_add_policy_changed_cb (mm_handle->dpm_h, "camera",
            ml_tizen_dpm_policy_changed_cb, pipe,
            &mm_handle->dpm_cb_id) != DPM_ERROR_NONE) {
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
 * @brief Function to acquire the resource from mm resource manager.
 */
static int
ml_tizen_mm_res_acquire (ml_pipeline_h pipe,
    mm_resource_manager_res_type_e res_type)
{
  ml_pipeline *p;
  pipeline_resource_s *res;
  tizen_mm_handle_s *mm_handle;
  gchar *res_key;
  int status = ML_ERROR_STREAMS_PIPE;
  int err;

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
  if (mm_handle->has_video_src &&
      (status =
          ml_tizen_check_dpm_restriction (mm_handle->dpm_h,
              1)) != ML_ERROR_NONE)
    _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
        "Video camera source requires permission to access the camera; you do not have the permission. Your Tizen application is required to acquire video permission (DPM) from Tizen. Refer: https://docs.tizen.org/application/native/guides/security/dpm/");

  if (mm_handle->has_audio_src &&
      (status =
          ml_tizen_check_dpm_restriction (mm_handle->dpm_h,
              2)) != ML_ERROR_NONE)
    _ml_error_report_return (ML_ERROR_PERMISSION_DENIED,
        "Audio mic source requires permission to access the mic; you do not have the permission. Your Tizen application is required to acquire audio/mic permission (DPM) from Tizen. Refer: https://docs.tizen.org/application/native/guides/security/dpm/");

  /* check invalid handle */
  if (mm_handle->invalid)
    ml_tizen_mm_res_release (mm_handle, FALSE);

  /* create rm handle */
  if (!mm_handle->rm_h) {
    mm_resource_manager_h rm_h;

    err = mm_resource_manager_create (MM_RESOURCE_MANAGER_APP_CLASS_MEDIA,
        ml_tizen_mm_res_release_cb, pipe, &rm_h);
    if (err != MM_RESOURCE_MANAGER_ERROR_NONE)
      _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
          "Cannot create multimedia resource manager handle with mm_resource_manager_create (), it has returned %d. Please check if your Tizen installation is valid; do you have all multimedia packages properly installed?",
          err);

    /* add state change callback */
    err =
        mm_resource_manager_set_status_cb (rm_h, ml_tizen_mm_res_status_cb,
        pipe);
    if (err != MM_RESOURCE_MANAGER_ERROR_NONE) {
      mm_resource_manager_destroy (rm_h);
      _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
          "Cannot configure status callback with multimedia resource manager, mm_resource_manager_set_status_cb (), it has returned %d. Please check if your Tizen installation is valid; do you have all multmedia packages properly installed?",
          err);
    }

    mm_handle->rm_h = rm_h;
  }

  /* acquire resource */
  if (res_type == MM_RESOURCE_MANAGER_RES_TYPE_MAX) {
    GHashTableIter iter;
    gpointer key, value;

    /* iterate all handle and acquire res if released */
    g_hash_table_iter_init (&iter, mm_handle->res_handles);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      pipeline_resource_s *mm_res = value;

      if (!mm_res->handle) {
        mm_resource_manager_res_type_e type;

        type = ml_tizen_mm_res_get_type (mm_res->type);
        if (type != MM_RESOURCE_MANAGER_RES_TYPE_MAX) {
          status =
              ml_tizen_mm_res_get_handle (mm_handle->rm_h, type,
              &mm_res->handle);
          if (status != ML_ERROR_NONE)
            _ml_error_report_return_continue (status,
                "Internal error: cannot get resource handle from Tizen multimedia resource manager.");
        }
      }
    }
  } else {
    res_key = ml_tizen_mm_res_get_key_string (res_type);
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
        status =
            ml_tizen_mm_res_get_handle (mm_handle->rm_h, res_type,
            &mm_res->handle);
        if (status != ML_ERROR_NONE)
          _ml_error_report_return (ML_ERROR_STREAMS_PIPE,
              "Cannot get handle from Tizen multimedia resource manager.");
      }
    }
  }

  return ML_ERROR_NONE;
}

#if !TIZENMMCONF
/**
 * @brief Gets element name from mm conf and replaces element.
 */
static int
ml_tizen_mm_replace_element (MMHandleType * handle, camera_conf * conf,
    gint category, const gchar * name, const gchar * what, gchar ** description)
{
  type_element *element = NULL;
  const gchar *src_name = NULL;
  guint changed = 0;

  _mmcamcorder_conf_get_element (handle, conf, category, name, &element);
  _mmcamcorder_conf_get_value_element_name (element, &src_name);

  if (!src_name) {
    _ml_loge ("Failed to get the name of %s.", name);
    return ML_ERROR_STREAMS_PIPE;
  }

  *description =
      _ml_replace_string (*description, what, src_name, " !", &changed);
  if (changed > 1) {
    /* allow one src in the pipeline */
    _ml_loge ("Cannot parse duplicated src node.");
    return ML_ERROR_STREAMS_PIPE;
  }

  return ML_ERROR_NONE;
}
#endif

/**
 * @brief Converts predefined mmfw element.
 */
static int
ml_tizen_mm_convert_element (ml_pipeline_h pipe, gchar ** result,
    gboolean is_internal)
{
  gchar *video_src, *audio_src;
  MMHandleType hcam = NULL;
  MMCamPreset cam_info;
#if !TIZENMMCONF                /* < 6.5 */
  camera_conf *cam_conf = NULL;
#endif
  int status = ML_ERROR_STREAMS_PIPE;
  int err;

  video_src = g_strstr_len (*result, -1, ML_TIZEN_CAM_VIDEO_SRC);
  audio_src = g_strstr_len (*result, -1, ML_TIZEN_CAM_AUDIO_SRC);

  /* replace src element */
  if (video_src || audio_src) {
    /* check privilege first */
    if (!is_internal) {
      /* ignore permission when runs as internal mode */
      if (video_src &&
          (status =
              ml_tizen_check_privilege (TIZEN_PRIVILEGE_CAMERA)) !=
          ML_ERROR_NONE) {
        goto mm_error;
      }

      if (audio_src &&
          (status =
              ml_tizen_check_privilege (TIZEN_PRIVILEGE_RECODER)) !=
          ML_ERROR_NONE) {
        goto mm_error;
      }
    }

    /* create camcoder handle (primary camera) */
    if (video_src) {
      cam_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;
    } else {
      /* no camera */
      cam_info.videodev_type = MM_VIDEO_DEVICE_NONE;
    }

    if ((err = mm_camcorder_create (&hcam, &cam_info)) != MM_ERROR_NONE) {
      _ml_loge ("Fail to call mm_camcorder_create = %x\n", err);
      goto mm_error;
    }
#if TIZENMMCONF                 /* 6.5 or higher */
    if (video_src) {
      char *src_name = NULL;    /* Do not free this! */
      int size = 0;
      guint changed = 0;
      err = mm_camcorder_get_attributes (hcam, NULL,
          MMCAM_VIDEOSRC_ELEMENT_NAME, &src_name, &size, NULL);

      if (err != MM_ERROR_NONE || !src_name || size < 1) {
        _ml_loge ("Failed to get attributes of MMCAM_VIDEOSRC_ELEMENT_NAME.");
        status = ML_ERROR_NOT_SUPPORTED;
        goto mm_error;
      }
      *result = _ml_replace_string (*result, ML_TIZEN_CAM_VIDEO_SRC, src_name,
          " !", &changed);
      if (changed > 1) {
        /* Allow one src only in a pipeline */
        _ml_loge ("Cannot parse duplicated Tizen video src nodes.");
        status = ML_ERROR_INVALID_PARAMETER;
        goto mm_error;
      }
    }
    if (audio_src) {
      char *src_name = NULL;    /* Do not free this! */
      int size = 0;
      guint changed = 0;
      err = mm_camcorder_get_attributes (hcam, NULL,
          MMCAM_AUDIOSRC_ELEMENT_NAME, &src_name, &size, NULL);

      if (err != MM_ERROR_NONE || !src_name || size < 1) {
        _ml_loge ("Failed to get attributes of MMCAM_AUDIOSRC_ELEMENT_NAME.");
        status = ML_ERROR_NOT_SUPPORTED;
        goto mm_error;
      }
      *result = _ml_replace_string (*result, ML_TIZEN_CAM_AUDIO_SRC, src_name,
          " !", &changed);
      if (changed > 1) {
        /* Allow one src only in a pipeline */
        _ml_loge ("Cannot parse duplicated Tizen audio src nodes.");
        status = ML_ERROR_INVALID_PARAMETER;
        goto mm_error;
      }
    }
#else

    /* read ini, type CONFIGURE_TYPE_MAIN */
    err =
        _mmcamcorder_conf_get_info (hcam, 0, MMFW_CONFIG_MAIN_FILE, &cam_conf);
    if (err != MM_ERROR_NONE || !cam_conf) {
      _ml_loge ("Failed to load conf %s.", MMFW_CONFIG_MAIN_FILE);
      status = ML_ERROR_NOT_SUPPORTED;
      goto mm_error;
    }

    if (video_src) {
      /* category CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT */
      status =
          ml_tizen_mm_replace_element (hcam, cam_conf, 1, "VideosrcElement",
          ML_TIZEN_CAM_VIDEO_SRC, result);
      if (status != ML_ERROR_NONE)
        goto mm_error;
    }

    if (audio_src) {
      /* category CONFIGURE_CATEGORY_MAIN_AUDIO_INPUT */
      status =
          ml_tizen_mm_replace_element (hcam, cam_conf, 2, "AudiosrcElement",
          ML_TIZEN_CAM_AUDIO_SRC, result);
      if (status != ML_ERROR_NONE)
        goto mm_error;
    }
#endif

    /* initialize rm handle */
    status =
        ml_tizen_mm_res_initialize (pipe, (video_src != NULL),
        (audio_src != NULL));
    if (status != ML_ERROR_NONE)
      goto mm_error;

    /* get the camera resource using mm resource manager */
    status =
        ml_tizen_mm_res_acquire (pipe, MM_RESOURCE_MANAGER_RES_TYPE_CAMERA);
    if (status != ML_ERROR_NONE)
      goto mm_error;
  }

  /* done */
  status = ML_ERROR_NONE;

mm_error:
#if !TIZENMMCONF
  if (cam_conf)
    _mmcamcorder_conf_release_info (hcam, &cam_conf);
#endif
  if (hcam)
    mm_camcorder_destroy (hcam);

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
ml_tizen_mm_res_acquire (ml_pipeline_h pipe,
    mm_resource_manager_res_type_e res_type)
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
    status = ml_tizen_mm_res_acquire (pipe, MM_RESOURCE_MANAGER_RES_TYPE_MAX);
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
