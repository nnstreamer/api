/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Tizen Machine-Learning API Common Header
 * Copyright (C) 2020 MyungJoo Ham <myungjoo.ham@samsung.com>
 */
/**
 * @file    ml-api-common.h
 * @date    07 May 2020
 * @brief   ML-API Common Header
 * @see     https://github.com/nnstreamer/nnstreamer
 * @author  MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *      More entries might be migrated from nnstreamer.h if
 *    other modules of Tizen-ML APIs use common data structures.
 */
#ifndef __ML_API_COMMON_H__
#define __ML_API_COMMON_H__

#include <stddef.h>
#include <tizen_error.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/**
 * @addtogroup CAPI_ML_FRAMEWORK
 * @{
 */

/**
 * @brief Enumeration for the error codes of NNStreamer.
 * @since_tizen 5.5
 */
typedef enum {
  ML_ERROR_NONE                 = TIZEN_ERROR_NONE, /**< Success! */
  ML_ERROR_INVALID_PARAMETER    = TIZEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
  ML_ERROR_STREAMS_PIPE         = TIZEN_ERROR_STREAMS_PIPE, /**< Cannot create or access the pipeline. */
  ML_ERROR_TRY_AGAIN            = TIZEN_ERROR_TRY_AGAIN, /**< The pipeline is not ready, yet (not negotiated, yet) */
  ML_ERROR_UNKNOWN              = TIZEN_ERROR_UNKNOWN,  /**< Unknown error */
  ML_ERROR_TIMED_OUT            = TIZEN_ERROR_TIMED_OUT,  /**< Time out */
  ML_ERROR_NOT_SUPPORTED        = TIZEN_ERROR_NOT_SUPPORTED, /**< The feature is not supported */
  ML_ERROR_PERMISSION_DENIED    = TIZEN_ERROR_PERMISSION_DENIED, /**< Permission denied */
  ML_ERROR_OUT_OF_MEMORY        = TIZEN_ERROR_OUT_OF_MEMORY, /**< Out of memory (Since 6.0) */
} ml_error_e;


/******* ML API Common Data Structure for Inference, Training, and Service */
/**
 * @brief The maximum rank that NNStreamer supports with Tizen APIs.
 * @since_tizen 5.5
 */
#define ML_TENSOR_RANK_LIMIT  (4)

/**
 * @brief The maximum number of other/tensor instances that other/tensors may have.
 * @since_tizen 5.5
 */
#define ML_TENSOR_SIZE_LIMIT  (16)

/**
 * @brief The dimensions of a tensor that NNStreamer supports.
 * @since_tizen 5.5
 */
typedef unsigned int ml_tensor_dimension[ML_TENSOR_RANK_LIMIT];

/**
 * @brief A handle of a tensors metadata instance.
 * @since_tizen 5.5
 */
typedef void *ml_tensors_info_h;

/**
 * @brief A handle of input or output frames. #ml_tensors_info_h is the handle for tensors metadata.
 * @since_tizen 5.5
 */
typedef void *ml_tensors_data_h;

/**
 * @brief Possible data element types of tensor in NNStreamer.
 * @since_tizen 5.5
 */
typedef enum _ml_tensor_type_e
{
  ML_TENSOR_TYPE_INT32 = 0,      /**< Integer 32bit */
  ML_TENSOR_TYPE_UINT32,         /**< Unsigned integer 32bit */
  ML_TENSOR_TYPE_INT16,          /**< Integer 16bit */
  ML_TENSOR_TYPE_UINT16,         /**< Unsigned integer 16bit */
  ML_TENSOR_TYPE_INT8,           /**< Integer 8bit */
  ML_TENSOR_TYPE_UINT8,          /**< Unsigned integer 8bit */
  ML_TENSOR_TYPE_FLOAT64,        /**< Float 64bit */
  ML_TENSOR_TYPE_FLOAT32,        /**< Float 32bit */
  ML_TENSOR_TYPE_INT64,          /**< Integer 64bit */
  ML_TENSOR_TYPE_UINT64,         /**< Unsigned integer 64bit */
  ML_TENSOR_TYPE_UNKNOWN         /**< Unknown type */
} ml_tensor_type_e;

/****************************************************
 ** NNStreamer Utilities                           **
 ****************************************************/
/**
 * @brief Creates a tensors information handle with default value.
 * @since_tizen 5.5
 * @param[out] info The handle of tensors information.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_tensors_info_create (ml_tensors_info_h *info);

/**
 * @brief Frees the given handle of a tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @return 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_destroy (ml_tensors_info_h info);

/**
 * @brief Validates the given tensors information.
 * @details If the function returns an error, @a valid may not be changed.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information to be validated.
 * @param[out] valid @c true if it's valid, @c false if it's invalid.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_validate (const ml_tensors_info_h info, bool *valid);

/**
 * @brief Copies the tensors information.
 * @since_tizen 5.5
 * @param[out] dest A destination handle of tensors information.
 * @param[in] src The tensors information to be copied.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_clone (ml_tensors_info_h dest, const ml_tensors_info_h src);

/**
 * @brief Sets the number of tensors with given handle of tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[in] count The number of tensors.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_set_count (ml_tensors_info_h info, unsigned int count);

/**
 * @brief Gets the number of tensors with given handle of tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[out] count The number of tensors.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_get_count (ml_tensors_info_h info, unsigned int *count);

/**
 * @brief Sets the tensor name with given handle of tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[in] index The index of the tensor to be updated.
 * @param[in] name The tensor name to be set.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_set_tensor_name (ml_tensors_info_h info, unsigned int index, const char *name);

/**
 * @brief Gets the tensor name with given handle of tensors information.
 * @since_tizen 5.5
 * @remarks Before 6.0 this function returned the internal pointer so application developers do not need to free. Since 6.0 the name string is internally copied and returned. So if the function succeeds, @a name should be released using g_free().
 * @param[in] info The handle of tensors information.
 * @param[in] index The index of the tensor.
 * @param[out] name The tensor name.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_get_tensor_name (ml_tensors_info_h info, unsigned int index, char **name);

/**
 * @brief Sets the tensor type with given handle of tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[in] index The index of the tensor to be updated.
 * @param[in] type The tensor type to be set.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_set_tensor_type (ml_tensors_info_h info, unsigned int index, const ml_tensor_type_e type);

/**
 * @brief Gets the tensor type with given handle of tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[in] index The index of the tensor.
 * @param[out] type The tensor type.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_get_tensor_type (ml_tensors_info_h info, unsigned int index, ml_tensor_type_e *type);

/**
 * @brief Sets the tensor dimension with given handle of tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[in] index The index of the tensor to be updated.
 * @param[in] dimension The tensor dimension to be set.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_set_tensor_dimension (ml_tensors_info_h info, unsigned int index, const ml_tensor_dimension dimension);

/**
 * @brief Gets the tensor dimension with given handle of tensors information.
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[in] index The index of the tensor.
 * @param[out] dimension The tensor dimension.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_get_tensor_dimension (ml_tensors_info_h info, unsigned int index, ml_tensor_dimension dimension);

/**
 * @brief Gets the size of tensors data in the given tensors information handle in bytes.
 * @details If an application needs to get the total byte size of tensors, set the @a index '-1'. Note that the maximum number of tensors is 16 (#ML_TENSOR_SIZE_LIMIT).
 * @since_tizen 5.5
 * @param[in] info The handle of tensors information.
 * @param[in] index The index of the tensor.
 * @param[out] data_size The byte size of tensor data.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_get_tensor_size (ml_tensors_info_h info, int index, size_t *data_size);

/**
 * @brief Creates a tensor data frame with the given tensors information.
 * @since_tizen 5.5
 * @remarks Before 6.0, this function returned #ML_ERROR_STREAMS_PIPE in case of an internal error. Since 6.0, #ML_ERROR_OUT_OF_MEMORY is returned in such cases, so #ML_ERROR_STREAMS_PIPE is not returned by this function anymore.
 * @param[in] info The handle of tensors information for the allocation.
 * @param[out] data The handle of tensors data. The caller is responsible for freeing the allocated data with ml_tensors_data_destroy().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_tensors_data_create (const ml_tensors_info_h info, ml_tensors_data_h *data);

/**
 * @brief Frees the given tensors' data handle.
 * @details Note that the opened handle should be closed before calling this function in the case of a single API.
 *          If not, the inference engine might try to access the data that is already freed.
 *          And it causes the segmentation fault.
 * @since_tizen 5.5
 * @param[in] data The handle of tensors data.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_data_destroy (ml_tensors_data_h data);

/**
 * @brief Gets a tensor data of given handle.
 * @details This returns the pointer of memory block in the handle. Do not deallocate the returned tensor data.
 * @since_tizen 5.5
 * @param[in] data The handle of tensors data.
 * @param[in] index The index of the tensor.
 * @param[out] raw_data Raw tensor data in the handle.
 * @param[out] data_size Byte size of tensor data.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_data_get_tensor_data (ml_tensors_data_h data, unsigned int index, void **raw_data, size_t *data_size);

/**
 * @brief Copies a tensor data to given handle.
 * @since_tizen 5.5
 * @param[in] data The handle of tensors data.
 * @param[in] index The index of the tensor.
 * @param[in] raw_data Raw tensor data to be copied.
 * @param[in] data_size Byte size of raw data.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_data_set_tensor_data (ml_tensors_data_h data, unsigned int index, const void *raw_data, const size_t data_size);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_COMMON_H__ */
