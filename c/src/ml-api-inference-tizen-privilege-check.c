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
#if TIZEN5PLUS
#include <sys/types.h>
#include <unistd.h>
#include <rm_type.h>
#include <rm_api.h>
#include <resource_center.h>
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
 * @brief Enumeration for resource type
 */
typedef enum {
  RES_TYPE_VIDEO_DECODER,     /**< ID of video decoder resource type */
  RES_TYPE_VIDEO_OVERLAY,     /**< ID of video overlay resource type */
  RES_TYPE_CAMERA,            /**< ID of camera resource type */
  RES_TYPE_VIDEO_ENCODER,     /**< ID of video encoder resource type */
  RES_TYPE_RADIO,             /**< ID of radio resource type */
  RES_TYPE_MAX,               /**< Used to iterate on resource types only */
} res_type_e;

/**
 * @brief Internal structure for tizen mm framework.
 */
typedef struct
{
  int rm_h; /**< rm handle */
  rm_device_return_s devices[RES_TYPE_MAX];
  rm_consumer_info rci;
  gboolean need_to_acquire[RES_TYPE_MAX];
  GMutex lock;
  device_policy_manager_h dpm_h; /**< dpm handle */
  int dpm_cb_id; /**< dpm callback id */
  gboolean has_video_src; /**< pipeline includes video src */
  gboolean has_audio_src; /**< pipeline includes audio src */
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
 * @brief Function to get resource manager appid using pid
 */
static int
ml_tizen_mm_get_appid_by_pid (int pid, char *appid, size_t size)
{
	g_autofree gchar *cmdline = NULL;
	g_autofree gchar *contents = NULL;
	g_autofree gchar *base = NULL;
	g_autoptr(GError) error = NULL;

	g_return_val_if_fail(appid, ML_ERROR_INVALID_PARAMETER);
	g_return_val_if_fail(size != 0, ML_ERROR_INVALID_PARAMETER);

	cmdline = g_strdup_printf("/proc/%d/cmdline", (int)pid);

	if (!g_file_get_contents(cmdline, &contents, NULL, &error)) {
		LOGE("error : %s", error->message);
		return ML_ERROR_UNKNOWN;
	}

	base = g_path_get_basename(contents);

	if (g_strlcpy(appid, base, size) >= size)
		LOGE("string truncated");

	return ML_ERROR_NONE;
}

/**
 * @brief Function to allocate resource by type
 */
static int
ml_tizen_mm_res_allocate (tizen_mm_handle_s *resources, res_type_e type)
{
	int ret = RM_OK;
	int idx = 0;
	int category_option = 0;
	rm_rsc_category_e category_id = RM_CATEGORY_NONE;
	rm_requests_resource_state_e state;
	rm_category_request_s request_resources;
	rm_device_return_s *device;
	g_autoptr(GMutexLocker) locker = NULL;


	g_return_val_if_fail(resources, ML_ERROR_INVALID_PARAMETER);

	locker = g_mutex_locker_new(&resources->lock);

	_ml_logi("app id : %s type %d", resources->rci.app_id, type);
	memset(&request_resources, 0x0, sizeof(rm_category_request_s));

	device = &resources->devices[type];
	memset(device, 0x0, sizeof(rm_device_return_s));

	switch (type) {
	case RES_TYPE_VIDEO_DECODER:
		state = RM_STATE_EXCLUSIVE;
		category_id = RM_CATEGORY_VIDEO_DECODER;
		break;
	case RES_TYPE_VIDEO_OVERLAY:
		state = RM_STATE_EXCLUSIVE;
		category_id = RM_CATEGORY_SCALER;
		break;
	case RES_TYPE_CAMERA:
		state = RM_STATE_EXCLUSIVE;
		category_id = RM_CATEGORY_CAMERA;
		break;
	case RES_TYPE_VIDEO_ENCODER:
		state = RM_STATE_EXCLUSIVE;
		category_id = RM_CATEGORY_VIDEO_ENCODER;
		break;
	case RES_TYPE_RADIO:
		state = RM_STATE_EXCLUSIVE;
		category_id = RM_CATEGORY_RADIO;
		break;
	default:
		LOGE("category id can't set");
		return ML_ERROR_INVALID_PARAMETER;
	}

	category_option = rc_get_capable_category_id(resources->rm_h, resources->rci.app_id, category_id);

	request_resources.request_num = 1;
	request_resources.state[0] = state;
	request_resources.category_id[0] = category_id;
	request_resources.category_option[0] = category_option;
	_ml_logi("state %d category id 0x%x category option %d", state, category_id, category_option);

	ret = rm_allocate_resources(resources->rm_h, &request_resources, device);
	if (ret != RM_OK) {
		LOGW("Resource allocation request failed ret %d [error type %d]", ret, device->error_type);
		return ML_ERROR_UNKNOWN;
	}

	for (idx = 0; idx < device->allocated_num; idx++)
		_ml_logi("#%d / %d [%p] device %d %s %s", idx, device->allocated_num, device,
			device->device_id[idx], device->device_node[idx],
			device->device_name[idx]);

	return ML_ERROR_NONE;
}

/**
 * @brief Function to deallocate the resource by type.
 */
static int
ml_tizen_mm_res_deallocate(tizen_mm_handle_s *resources, res_type_e type)
{
	int rm_ret = RM_OK;
	int idx = 0;
	rm_device_request_s requested;
	rm_device_return_s *devices;
	g_autoptr(GMutexLocker) locker = NULL;

	g_return_val_if_fail(resources, ML_ERROR_INVALID_PARAMETER);
	g_return_val_if_fail(resources->rm_h, ML_ERROR_INVALID_PARAMETER);

	locker = g_mutex_locker_new(&resources->lock);

	devices = &resources->devices[type];

	LOGI("[%p] #%d (type %d) alloc num %d", devices, idx, type, devices->allocated_num);

	if (devices->allocated_num > 0) {
		memset(&requested, 0x0, sizeof(rm_device_request_s));
		requested.request_num = devices->allocated_num;
		for (idx = 0; idx < requested.request_num; idx++) {
			requested.device_id[idx] = devices->device_id[idx];
			LOGI("[device id %d] [device name %s]", devices->device_id[idx], devices->device_name[idx]);
		}

		rm_ret = rm_deallocate_resources(resources->rm_h, &requested);
		if (rm_ret != RM_OK) {
			LOGE("Resource deallocation request failed [%d] [request num %d]", rm_ret, requested.request_num);
			return ML_ERROR_UNKNOWN;
		}
	}

	for (idx = 0; idx < devices->allocated_num; idx++) {
		if (devices->device_node[idx]) {
			free(devices->device_node[idx]);
			devices->device_node[idx] = NULL;
		}

		if (devices->omx_comp_name[idx]) {
			free(devices->omx_comp_name[idx]);
			devices->omx_comp_name[idx] = NULL;
		}
	}

	return ML_ERROR_NONE;
}

/**
 * @brief Callback to be called from resource manager.
 */
static rm_cb_result
ml_tizen_mm_res_release_cb (int handle, rm_callback_type event_src,
     rm_device_request_s *info, void *cb_data)
{
  tizen_mm_handle_s *mm_handle = (tizen_mm_handle_s *)cb_data;
  g_autoptr(GMutexLocker) locker = g_mutex_locker_new(&mm_handle->lock);

  g_return_val_if_fail (cb_data, RM_CB_RESULT_ERROR);

  switch (event_src) {
  case RM_CALLBACK_TYPE_RESOURCE_CONFLICT:
  case RM_CALLBACK_TYPE_RESOURCE_CONFLICT_UD:
    /* pause pipeline */
    //gst_element_set_state (p->element, GST_STATE_PAUSED);
    break;
  default:
    break;
  }

  return RM_CB_RESULT_OK;
}

/**
 * @brief Function to release the resource handle of tizen mm resource manager.
 */
static void
ml_tizen_mm_res_release (gpointer handle, gboolean destroy)
{
  tizen_mm_handle_s *mm_handle;
  res_type_e type;
  int ret;

  g_return_if_fail (handle);

  mm_handle = (tizen_mm_handle_s *) handle;

  /* deallocate resource */
  for (type = RES_TYPE_VIDEO_DECODER; type < RES_TYPE_MAX; type++) {
    ret = ml_tizen_mm_res_deallocate(mm_handle, type);
    if (ret != ML_ERROR_NONE) {
      _ml_loge("Fail to deallocate resoure [type %d].", type);
    }
  }

  /* release res handles */
  ret = rm_unregister(mm_handle->rm_h);
  if (ret != RM_OK)
    LOGE("rm_unregister fail %d", ret);

  g_mutex_clear(&mm_handle->lock);

  if (destroy) {
    if (mm_handle->dpm_h) {
      if (mm_handle->dpm_cb_id > 0) {
        dpm_remove_policy_changed_cb (mm_handle->dpm_h, mm_handle->dpm_cb_id);
        mm_handle->dpm_cb_id = 0;
      }

      dpm_manager_destroy (mm_handle->dpm_h);
      mm_handle->dpm_h = NULL;
    }

    g_free (mm_handle);
  }
}

/**
 * @brief Function to initialize the resource manager.
 */
static int
ml_tizen_mm_res_initialize (ml_pipeline_h pipe, gboolean has_video_src,
    gboolean has_audio_src)
{
  ml_pipeline *p;
  tizen_mm_handle_s *mm_handle = NULL;
  int status = ML_ERROR_STREAMS_PIPE;
  int err = ML_ERROR_NONE;

  p = (ml_pipeline *) pipe;

  /* create rm handle */
  mm_handle = g_new0 (tizen_mm_handle_s, 1);
  if (!mm_handle) {
    _ml_loge ("Failed to allocate media resource handle.");
    status = ML_ERROR_OUT_OF_MEMORY;
    goto rm_error;
  }

  g_mutex_init(&mm_handle->lock);

  mm_handle->rci.app_pid = (int)getpid();

  if (ml_tizen_mm_get_appid_by_pid(mm_handle->rci.app_pid, mm_handle->rci.app_id, sizeof(mm_handle->rci.app_id)) != ML_ERROR_NONE) {
    _ml_loge("Failed to get appid using pid.");
    status = ML_ERROR_UNKNOWN;
    goto rm_error;
  }

  _ml_logi("app pid %d app id %s", mm_handle->rci.app_pid, mm_handle->rci.app_id);

  err = rm_register((rm_resource_cb)ml_tizen_mm_res_release_cb,
      (tizen_mm_handle_s *)mm_handle,
      &(mm_handle->rm_h),
      (mm_handle->rci.app_id[0] != '\0') ? &mm_handle->rci : NULL);
  if (err != RM_OK) {
    _ml_loge("rm_register fail %d", err);
    return ML_ERROR_UNKNOWN;
  }

  /* device policy manager */
  mm_handle->dpm_h = dpm_manager_create ();
  if (dpm_add_policy_changed_cb (mm_handle->dpm_h, "camera",
          ml_tizen_dpm_policy_changed_cb, pipe,
          &mm_handle->dpm_cb_id) != DPM_ERROR_NONE) {
    _ml_loge ("Failed to add device policy callback.");
    status = ML_ERROR_PERMISSION_DENIED;
    goto rm_error;
  }

  mm_handle->has_video_src = has_video_src;
  mm_handle->has_audio_src = has_audio_src;

  /* set rm handle to pipeline's resources */
  p->resources = mm_handle;

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
    res_type_e res_type)
{
  ml_pipeline *p;
  tizen_mm_handle_s *mm_handle;
  int status = ML_ERROR_STREAMS_PIPE;

  p = (ml_pipeline *) pipe;

  mm_handle =  p->resources;
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

  /* acquire resource */
  status = ml_tizen_mm_res_allocate(p->resources, res_type);
  if (status != ML_ERROR_NONE) {
    _ml_loge("Faied to allocate resource.");
    return ML_ERROR_UNKNOWN;
  }

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
  MMCamPreset cam_info;
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
    /* copy pipeline description so that original description string is not changed if failed to replace element. */
    desc = g_strdup (*result);

    /* check privilege first */
    if (!is_internal) {
      /* ignore permission when runs as internal mode */
      if (video_src &&
          (status = ml_tizen_check_privilege (TIZEN_PRIVILEGE_CAMERA)) !=
          ML_ERROR_NONE) {
        goto mm_error;
      }

      if (audio_src &&
          (status = ml_tizen_check_privilege (TIZEN_PRIVILEGE_RECODER)) !=
          ML_ERROR_NONE) {
        goto mm_error;
      }
    }

    status = ml_tizen_mm_replace_element (video_src != NULL, audio_src != NULL,
        &desc);
    if (status != ML_ERROR_NONE)
      goto mm_error;

    /* initialize rm handle */
    status =
        ml_tizen_mm_res_initialize (pipe, (video_src != NULL),
        (audio_src != NULL));
    if (status != ML_ERROR_NONE)
      goto mm_error;

    /* get the camera resource using mm resource manager */
    status =
        ml_tizen_mm_res_acquire (pipe, RES_TYPE_CAMERA);
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
ml_tizen_mm_res_acquire (ml_pipeline_h pipe,
    res_type_e res_type)
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
    status = ml_tizen_mm_res_acquire (pipe, RES_TYPE_MAX);
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
