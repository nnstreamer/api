/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-training-offloading.h
 * @date 30 Apr 2024
 * @brief ml-service extension internal header.
 *        This file should NOT be exported to SDK or devel package.
 * @brief ML training offloading service of NNStreamer/Service C-API
 * @see https://github.com/nnstreamer/nnstreamer
 * @author Hyunil Park <hyunil46.park@samsung.com>
 * @bug No known bugs except for NYI items
 */

#ifndef __ML_SERVICE_TRAINING_OFFLOADING_H__
#define __ML_SERVICE_TRAINING_OFFLOADING_H__

#include <ml-api-service.h>
#include "nnstreamer-tizen-internal.h"
#include "ml-api-service-offloading.h"

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/**
 * @brief Internal enumeration for ml-service training offloading types.
 */
  typedef enum
  {
    ML_TRAINING_OFFLOADING_TYPE_UNKNOWN = 0,
    ML_TRAINING_OFFLOADING_TYPE_SENDER,
    ML_TRAINING_OFFLOADING_TYPE_RECEIVER,

    ML_TRAINING_OFFLOADING_TYPE_MAX,
  } ml_training_offloaing_type_e;

/**
 * @brief Internal structure for ml-service training offloading handle.
 */
  typedef struct
  {
    ml_training_offloaing_type_e type;
    ml_pipeline_h pipeline_h;

    //gchar *model_config;
    //gchar *pretrained_model;
    //gchar *trained_model;

    gchar *receiver_pipe; /** @REMOTE_MODEL_CONFIG@, @REMOTE_MODEL@, @REMOTE_FILE_PATH@ in the receiver pipeline is converted to model_config_path, model_path, and data_path. */
    gchar *sender_pipe;

    // /* SENDER use the below path for resource location, and RECEIVER uses these for received data path */
    // gchar *model_config_path;
    // gchar *model_path;
    // /* SENDER use this path to register trained model from remote */
    // /* RECEIVER use this path to register pretrined model, model config and to save trained model */
    // gchar *data_path;
    gchar *trained_model_path;  /* reply to remote sender, */

    gboolean is_received;
    GMutex received_lock;
    GCond received_cond;
    GThread *received_thread;

    GHashTable *transfer_data_table;
  } ml_training_services_s;

/**
 * @brief Creates a handle for ml-service training offloading handle.
 * @param[in] offloading_s The handle of ml-service offloading.
 * @param[in] object The Json object containing the service option.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the media storage or external storage.
 * @retval #ML_ERROR_INVALID_PARAMETER The parameter is invalid.
 * @retval #ML_ERROR_IO_ERROR Failed to parse the configuration file.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to open the model.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
  int ml_service_training_offloading_create (_ml_service_offloading_s *
      offloading_s, JsonObject * object);

// /**
//  * @brief Set path in ml-service training offloading handle.
//  * @note This is not official and public API but experimental API.
//  * @param[in] offloading_s The handle of ml-service offloading.
//  * @param[in] name The name of path (path: received model file, model-path: to send pretrained model, model-config-path: to send model-config, data-path: to send training data)
//  * @param[in] path The path to save file.
//  * @return @c 0 on success. Otherwise a negative error value.
//  * @retval #ML_ERROR_NONE Successful.
//  * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
//  * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
//  */
//   int ml_service_training_offloading_set_path (_ml_service_offloading_s *
//       offloading_s, const char *name, const gchar * path);

/**
 * @brief Start ml training offloading service.
 * @remarks The @a handle should be destroyed using ml_service_destroy().
 * @param[in] mls ml-service handle created by ml_service_new().
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 */
  int ml_service_training_offloading_start (ml_service_s * mls);

/**
 * @brief Stop ml training offloading service.
 * @remarks The @a handle should be destroyed using ml_service_destroy().
 * @param[in] mls ml-service handle created by ml_service_new().
 * @return @c 0 on Success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @retval #ML_ERROR_STREAMS_PIPE Failed to launch the pipeline.
 * @retval #ML_ERROR_TRY_AGAIN The pipeline is not ready yet.
 * @retval #ML_ERROR_PERMISSION_DENIED The application does not have the privilege to access to the storage.
 */
  int ml_service_training_offloading_stop (ml_service_s * mls);

/**
 * @brief Request all services to ml-service offloading.
 * @param[in] offloading_s  offloading_s The handle of ml-service offloading.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
  int ml_service_training_offloading_all_services_request
      (_ml_service_offloading_s * offloading_s);

/**
 * @brief Save received file path.
 * @param[in] offloading_s  offloading_s The handle of ml-service offloading.
 * @param[in] data_h nnstreamer edge data handle
 * @param[in] dir_path dir path of received file
 * @param[in] data data of received file
 * @param[in] service_type received service type from remote edge
 */
  void ml_service_training_offloading_save_received_file_path
      (_ml_service_offloading_s * offloading_s, nns_edge_data_h data_h,
      const gchar * dir_path, const gchar * data, int service_type);

/**
 * @brief Internal function to destroy ml-service training offloading data.
 * @param[in] offloading_s  The handle of ml-service offloading.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
  int ml_service_training_offloading_destroy (_ml_service_offloading_s *
      offloading_s);
#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* __ML_SERVICE_TRAINING_OFFLOADING_H__ */
