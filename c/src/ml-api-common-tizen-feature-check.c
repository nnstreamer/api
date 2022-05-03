/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-common-tizen-feature-check.c
 * @date 21 July 2020
 * @brief NNStreamer/C-API Tizen dependent functions, common for ML APIs, common for ML APIs.
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */

#if !defined (__TIZEN__) || !defined (__FEATURE_CHECK_SUPPORT__)
#error "This file can be included only in Tizen."
#endif

#include <glib.h>
#include <system_info.h>

#include "ml-api-internal.h"

/**
 * @brief Tizen ML feature.
 */
static const gchar *ML_FEATURES[] = {
  [ML_FEATURE] = "tizen.org/feature/machine_learning",
  [ML_FEATURE_INFERENCE] = "tizen.org/feature/machine_learning.inference",
  [ML_FEATURE_TRAINING] = "tizen.org/feature/machine_learning.training",
  [ML_FEATURE_SERVICE] = "tizen.org/feature/machine_learning.service",
  [ML_FEATURE_MAX] = NULL
};

/**
 * @brief Internal struct to control tizen feature support (machine_learning.inference).
 * -1: Not checked yet, 0: Not supported, 1: Supported
 */
typedef struct
{
  GMutex mutex;
  feature_state_t feature_state[ML_FEATURE_MAX];
} feature_info_s;

static feature_info_s *feature_info = NULL;

/**
 * @brief Internal function to initialize feature state.
 */
static void
ml_tizen_initialize_feature_state (void)
{
  int i;

  if (feature_info == NULL) {
    feature_info = g_new0 (feature_info_s, 1);
    g_assert (feature_info);

    g_mutex_init (&feature_info->mutex);
    for (i = 0; i < ML_FEATURE_MAX; i++)
      feature_info->feature_state[i] = NOT_CHECKED_YET;
  }
}

/**
 * @brief Set the feature status of machine_learning.inference.
 */
int
_ml_tizen_set_feature_state (ml_feature_e ml_feature, int state)
{
  ml_tizen_initialize_feature_state ();
  g_mutex_lock (&feature_info->mutex);

  /**
   * Update feature status
   * -1: Not checked yet, 0: Not supported, 1: Supported
   */
  feature_info->feature_state[ml_feature] = state;

  g_mutex_unlock (&feature_info->mutex);
  return ML_ERROR_NONE;
}

/**
 * @brief Get machine learning feature path.
 */
const char *
_ml_tizen_get_feature_path (ml_feature_e ml_feature)
{
  return ML_FEATURES[ml_feature];
}

/**
 * @brief Checks whether machine_learning.inference feature is enabled or not.
 */
int
_ml_tizen_get_feature_enabled (ml_feature_e ml_feature)
{
  int ret;
  int feature_enabled;

  ml_tizen_initialize_feature_state ();

  g_mutex_lock (&feature_info->mutex);
  feature_enabled = feature_info->feature_state[ml_feature];
  g_mutex_unlock (&feature_info->mutex);

  if (NOT_SUPPORTED == feature_enabled) {
    _ml_loge
        ("Tizen machine learning feature (%s) is NOT supported. Please check if your application has properly requested the feature and the device has the feature installed.",
        ML_FEATURES[ml_feature]);
    return ML_ERROR_NOT_SUPPORTED;
  } else if (NOT_CHECKED_YET == feature_enabled) {
    bool ml_feature_supported = false;
    ret = system_info_get_platform_bool (ML_FEATURES[ml_feature],
        &ml_feature_supported);
    if (0 == ret) {
      if (false == ml_feature_supported) {
        _ml_loge
            ("Tizen machine learning feature (%s) is NOT supported! Enable the feature before calling ML APIs.",
            ML_FEATURES[ml_feature]);
        _ml_tizen_set_feature_state (ml_feature, NOT_SUPPORTED);
        return ML_ERROR_NOT_SUPPORTED;
      }

      _ml_tizen_set_feature_state (ml_feature, SUPPORTED);
    } else {
      switch (ret) {
        case SYSTEM_INFO_ERROR_INVALID_PARAMETER:
          _ml_loge
              ("Failed to get feature value because feature key %s is not valid.",
              ML_FEATURES[ml_feature]);
          ret = ML_ERROR_NOT_SUPPORTED;
          break;

        case SYSTEM_INFO_ERROR_IO_ERROR:
          _ml_loge
              ("Failed to get feature value because of input/output error.");
          ret = ML_ERROR_NOT_SUPPORTED;
          break;

        case SYSTEM_INFO_ERROR_PERMISSION_DENIED:
          _ml_loge
              ("Failed to get feature value because of permission denied.");
          ret = ML_ERROR_PERMISSION_DENIED;
          break;

        default:
          _ml_loge ("Failed to get feature value because of unknown error.");
          ret = ML_ERROR_NOT_SUPPORTED;
          break;
      }
      return ret;
    }
  }

  return ML_ERROR_NONE;
}
