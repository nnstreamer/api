/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-query.h
 * @date 30 Aug 2022
 * @brief Query client implementation of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Yongjoo Ahn <yongjoo1.ahn@samsung.com>
 * @bug No known bugs except for NYI items
 */
#ifndef __ML_API_SERVICE_QUERY_H__
#define __ML_API_SERVICE_QUERY_H__

#include "ml-api-service-private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(ENABLE_NNSTREAMER_EDGE)
/**
 * @brief Internal function to release ml-service query data.
 */
int _ml_service_query_release_internal (ml_service_s *mls);

/**
 * @brief Internal function to create query client service handle with given ml-option handle.
 */
int _ml_service_query_create (ml_service_s *mls, ml_option_h option);

/**
 * @brief Internal function to request an output to query client service with given input data.
 */
int _ml_service_query_request (ml_service_s *mls, const ml_tensors_data_h input, ml_tensors_data_h *output);
#else
#define _ml_service_query_release_internal(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_query_create(...) ML_ERROR_NOT_SUPPORTED
#define _ml_service_query_request(...) ML_ERROR_NOT_SUPPORTED
#endif /*ENABLE_NNSTREAMER_EDGE */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_QUERY_H__ */
