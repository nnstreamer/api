/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file        ml-api-service-extension.h
 * @date        1 September 2023
 * @brief       ML service extension C-API.
 * @see         https://github.com/nnstreamer/api
 * @author      Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug         No known bugs except for NYI items
 */
#ifndef __ML_API_SERVICE_EXTENSION_H__
#define __ML_API_SERVICE_EXTENSION_H__

#include "ml-api-service-private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Internal function to create ml-service extension.
 */
int _ml_service_extension_create (ml_service_s *mls, JsonObject *object);

/**
 * @brief Internal function to release ml-service extension.
 */
int _ml_service_extension_destroy (ml_service_s *mls);

/**
 * @brief Internal function to start ml-service extension.
 */
int _ml_service_extension_start (ml_service_s *mls);

/**
 * @brief Internal function to stop ml-service extension.
 */
int _ml_service_extension_stop (ml_service_s *mls);

/**
 * @brief Internal function to get the information of required input data.
 */
int _ml_service_extension_get_input_information (ml_service_s *mls, const char *name, ml_tensors_info_h *info);

/**
 * @brief Internal function to get the information of output data.
 */
int _ml_service_extension_get_output_information (ml_service_s *mls, const char *name, ml_tensors_info_h *info);

/**
 * @brief Internal function to set the information for ml-service extension.
 */
int _ml_service_extension_set_information (ml_service_s *mls, const char *name, const char *value);

/**
 * @brief Internal function to add an input data to process the model in ml-service extension handle.
 */
int _ml_service_extension_request (ml_service_s *mls, const char *name, const ml_tensors_data_h data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_EXTENSION_H__ */
