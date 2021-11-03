/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer / Tizen Machine-Learning "Service API" Header for OS packages.
 * Copyright (C) 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
 */
/**
 * @file    ml-api-service-internal.h
 * @date    03 Nov 2021
 * @brief   ML-API Internal Platform Service Header
 * @see     https://github.com/nnstreamer/api
 * @author  MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *      This provides interfaces of ML Service APIs for
 *    platform packages (with "root" or "OS" priveleges).
 *      Application developers should use ml-api-service.h
 *    instead.
 *      However, whether to mandate this or not can be decided by
 *    vendors. For Tizen, this is mandated; thus, .rpm packages
 *    may use internal headers and .tpk packages cannot use
 *    internal headers.
 */
#ifndef __ML_API_SERVICE_INTERNAL_H__
#define __ML_API_SERVICE_INTERNAL_H__

#include "ml-api-service.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief TBU
 * @param[in] desc provider_type of desc is not restricted.
 */
int ml_service_pipeline_description_add_privileged (const ml_service_pipeline_description * desc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_H__ */
