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

/**
 * @brief Destroy the service handle.
 */
int
ml_service_destroy (ml_service_h handle)
{
  int ret = ML_ERROR_NONE;
  ml_service_s *mls = (ml_service_s *) handle;

  check_feature_state (ML_FEATURE_SERVICE);

  if (!handle)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'handle' is NULL. It should be a valid ml_service_h.");

  switch (mls->type) {
    case ML_SERVICE_TYPE_SERVER_PIPELINE:
      ret = ml_service_pipeline_release_internal (mls->priv);
      break;
    case ML_SERVICE_TYPE_CLIENT_QUERY:
      ret = ml_service_query_release_internal (mls->priv);
      break;
    case ML_SERVICE_TYPE_REMOTE:
      ret = ml_service_remote_release_internal (mls->priv);
      break;
    default:
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "Invalid type of ml_service_h.");
  }

  if (ret == ML_ERROR_NONE)
    g_free (mls);
  return ret;
}
