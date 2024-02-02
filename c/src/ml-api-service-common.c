/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-common.c
 * @date 31 Aug 2022
 * @brief Some implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include "ml-api-service.h"
#include "ml-api-service-private.h"

#define ML_SERVICE_MAGIC 0xfeeedeed
#define ML_SERVICE_MAGIC_DEAD 0xdeaddead

/**
 * @brief Internal function to validate ml-service handle.
 */
gboolean
_ml_service_handle_is_valid (ml_service_s * mls)
{
  if (!mls)
    return FALSE;

  if (mls->magic != ML_SERVICE_MAGIC)
    return FALSE;

  return TRUE;
}

/**
 * @brief Internal function to create new ml-service handle.
 */
ml_service_s *
_ml_service_create_internal (ml_service_type_e ml_service_type)
{
  ml_service_s *mls;

  mls = g_try_new0 (ml_service_s, 1);
  if (mls) {
    mls->magic = ML_SERVICE_MAGIC;
    mls->type = ml_service_type;
  }

  return mls;
}

/**
 * @brief Internal function to release ml-service handle.
 */
int
_ml_service_destroy_internal (ml_service_s * mls)
{
  int ret = ML_ERROR_NONE;

  if (!_ml_service_handle_is_valid (mls)) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' (ml_service_h), is invalid. It should be a valid ml_service_h instance.");
  }

  switch (mls->type) {
    case ML_SERVICE_TYPE_SERVER_PIPELINE:
      ret = ml_service_pipeline_release_internal (mls);
      break;
    case ML_SERVICE_TYPE_CLIENT_QUERY:
      ret = ml_service_query_release_internal (mls);
      break;
    case ML_SERVICE_TYPE_REMOTE:
      ret = ml_service_remote_release_internal (mls);
      break;
    default:
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Invalid type of ml_service_h.");
  }

  if (ret == ML_ERROR_NONE) {
    mls->magic = ML_SERVICE_MAGIC_DEAD;
    g_free (mls);
  } else {
    _ml_error_report ("Failed to release ml-service handle, internal error?");
  }

  return ret;
}

/**
 * @brief Destroy the service handle.
 */
int
ml_service_destroy (ml_service_h handle)
{
  check_feature_state (ML_FEATURE_SERVICE);

  return _ml_service_destroy_internal ((ml_service_s *) handle);
}
