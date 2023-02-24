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

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
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
  ML_ERROR_IO_ERROR             = TIZEN_ERROR_IO_ERROR, /**< I/O error for database and filesystem (Since 7.0) */
} ml_error_e;

/**
 * @brief Types of NNFWs.
 * @details To check if a nnfw-type is supported in a system, an application may call the API, ml_check_nnfw_availability().
 * @since_tizen 5.5
 */
typedef enum {
  ML_NNFW_TYPE_ANY = 0,               /**< NNFW is not specified (Try to determine the NNFW with file extension). */
  ML_NNFW_TYPE_CUSTOM_FILTER = 1,     /**< Custom filter (Independent shared object). */
  ML_NNFW_TYPE_TENSORFLOW_LITE = 2,   /**< Tensorflow-lite (.tflite). */
  ML_NNFW_TYPE_TENSORFLOW = 3,        /**< Tensorflow (.pb). */
  ML_NNFW_TYPE_NNFW = 4,              /**< Neural Network Inference framework, which is developed by SR (Samsung Research). */
  ML_NNFW_TYPE_MVNC = 5,              /**< Intel Movidius Neural Compute SDK (libmvnc). (Since 6.0) */
  ML_NNFW_TYPE_OPENVINO = 6,          /**< Intel OpenVINO. (Since 6.0) */
  ML_NNFW_TYPE_VIVANTE = 7,           /**< VeriSilicon's Vivante. (Since 6.0) */
  ML_NNFW_TYPE_EDGE_TPU = 8,          /**< Google Coral Edge TPU (USB). (Since 6.0) */
  ML_NNFW_TYPE_ARMNN = 9,             /**< Arm Neural Network framework (support for caffe and tensorflow-lite). (Since 6.0) */
  ML_NNFW_TYPE_SNPE = 10,             /**< Qualcomm SNPE (Snapdragon Neural Processing Engine (.dlc). (Since 6.0) */
  ML_NNFW_TYPE_PYTORCH = 11,          /**< PyTorch (.pt). (Since 6.5) */
  ML_NNFW_TYPE_NNTR_INF = 12,         /**< Inference supported from NNTrainer, SR On-device Training Framework (Since 6.5) */
  ML_NNFW_TYPE_VD_AIFW = 13,          /**< Inference framework for Samsung Tizen TV (Since 6.5) */
  ML_NNFW_TYPE_TRIX_ENGINE = 14,      /**< TRIxENGINE accesses TRIV/TRIA NPU low-level drivers directly (.tvn). (Since 6.5) You may need to use high-level drivers wrapping this low-level driver in some devices: e.g., AIFW */
  ML_NNFW_TYPE_MXNET = 15,            /**< Apache MXNet (Since 7.0) */
  ML_NNFW_TYPE_TVM = 16,              /**< Apache TVM (Since 7.0) */
  ML_NNFW_TYPE_SNAP = 0x2001,         /**< SNAP (Samsung Neural Acceleration Platform), only for Android. (Since 6.0) */
} ml_nnfw_type_e;

/**
 * @brief Types of hardware resources to be used for NNFWs. Note that if the affinity (nnn) is not supported by the driver or hardware, it is ignored.
 * @since_tizen 5.5
 */
typedef enum {
  ML_NNFW_HW_ANY          = 0,      /**< Hardware resource is not specified. */
  ML_NNFW_HW_AUTO         = 1,      /**< Try to schedule and optimize if possible. */
  ML_NNFW_HW_CPU          = 0x1000, /**< 0x1000: any CPU. 0x1nnn: CPU # nnn-1. */
  ML_NNFW_HW_CPU_SIMD     = 0x1100, /**< 0x1100: SIMD in CPU. (Since 6.0) */
  ML_NNFW_HW_CPU_NEON     = 0x1100, /**< 0x1100: NEON (alias for SIMD) in CPU. (Since 6.0) */
  ML_NNFW_HW_GPU          = 0x2000, /**< 0x2000: any GPU. 0x2nnn: GPU # nnn-1. */
  ML_NNFW_HW_NPU          = 0x3000, /**< 0x3000: any NPU. 0x3nnn: NPU # nnn-1. */
  ML_NNFW_HW_NPU_MOVIDIUS = 0x3001, /**< 0x3001: Intel Movidius Stick. (Since 6.0) */
  ML_NNFW_HW_NPU_EDGE_TPU = 0x3002, /**< 0x3002: Google Coral Edge TPU (USB). (Since 6.0) */
  ML_NNFW_HW_NPU_VIVANTE  = 0x3003, /**< 0x3003: VeriSilicon's Vivante. (Since 6.0) */
  ML_NNFW_HW_NPU_SLSI     = 0x3004, /**< 0x3004: Samsung S.LSI. (Since 6.5) */
  ML_NNFW_HW_NPU_SR       = 0x13000, /**< 0x13000: any SR (Samsung Research) made NPU. (Since 6.0) */
} ml_nnfw_hw_e;

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
  ML_TENSOR_TYPE_FLOAT16,        /**< FP16, IEEE 754. Note that this type is supported only in aarch64/arm devices. (Since 7.0) */
  ML_TENSOR_TYPE_UNKNOWN         /**< Unknown type */
} ml_tensor_type_e;

/**
 * @brief The function to be called when destroying the data in machine learning API.
 * @since_tizen 7.0
 * @param[in] data The data to be destroyed.
 */
typedef void (*ml_data_destroy_cb) (void *data);

/**
 * @brief Callback to execute the custom-easy filter in NNStreamer pipelines.
 * @details Note that if ml_custom_easy_invoke_cb() returns negative error values, the constructed pipeline does not work properly anymore.
 *          So developers should release the pipeline handle and recreate it again.
 * @since_tizen 6.0
 * @remarks The @a in can be used only in the callback. To use outside, make a copy.
 * @remarks The @a out can be used only in the callback. To use outside, make a copy.
 * @param[in] in The handle of the tensor input (a single frame. tensor/tensors).
 * @param[out] out The handle of the tensor output to be filled (a single frame. tensor/tensors).
 * @param[in,out] user_data User application's private data.
 * @return @c 0 on success. @c 1 to ignore the input data. Otherwise a negative error value.
 */
typedef int (*ml_custom_easy_invoke_cb) (const ml_tensors_data_h in, ml_tensors_data_h out, void *user_data);

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
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid. Note that src should be a valid tensors info handle and dest should be a created (allocated) tensors info handle.
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
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported. E.g., in a machine without fp16 support, trying FLOAT16 is not supported.
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
 * @details This returns the pointer of memory block in the handle. Do not deallocate the returned tensor data. The returned pointer (raw_data) directly points to the internal data of data. If you modify the returned memory block (raw_data), the contents of data is updated.
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
 * @brief Returns a human-readable string describing the last error.
 * @details This returns a human-readable, null-terminated string describing
 *         the most recent error that occurred from a call to one of the
 *         functions in the Machine Learning API since the last call to
 *         ml_error(). The returned string should *not* be freed or
 *         overwritten by the caller.
 * @since_tizen 7.0
 * @return @c NULL if no error to be reported. Otherwise the error description.
 */
const char * ml_error (void);

/**
 * @brief Returns a human-readable string describing an error code.
 * @details This returns a human-readable, null-terminated string describing
 *         the error code of machine learning API.
 *         The returned string should *not* be freed or
 *         overwritten by the caller.
 * @since_tizen 7.0
 * @param[in] error_code The error code of machine learning API.
 * @return @c NULL for invalid error code. Otherwise the error description.
 */
const char * ml_strerror (int error_code);

/*************
 * ML OPTION *
 *************/

/**
 * @brief A handle of a ml-option instance.
 * @since_tizen 7.0
 */
typedef void *ml_option_h;

/**
 * @brief Creates ml-option instance.
 * @since_tizen 7.0
 * @remarks The @a option should be released using ml_option_destroy().
 * @param[out] option Newly created option handle is returned.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_option_create (ml_option_h *option);

/**
 * @brief Destroys the ml-option instance.
 * @details Note that, user should free the allocated values of ml-option in the case that destroy function is not given.
 * @since_tizen 7.0
 * @param[in] option The option handle to be destroyed.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 */
int ml_option_destroy (ml_option_h option);

/**
 * @brief Sets a new key-value in ml-option instance.
 * @details Note that the @a value should be valid during single task and be freed after destroying the ml-option instance unless proper @a destroy function is given. When duplicated @a key is given, the corresponding @a value is updated with the new one.
 * @since_tizen 7.0
 * @param[in] option The handle of ml-option.
 * @param[in] key The key to be set.
 * @param[in] value The value to be set.
 * @param[in] destroy The function to destroy the value. It is called when the ml-option instance is destroyed.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 */
int ml_option_set (ml_option_h option, const char *key, void *value, ml_data_destroy_cb destroy);

/**
 * @brief Gets a value of key in ml-option instance.
 * @details This returns the pointer of memory in the handle. Do not deallocate the returned value. If you modify the returned memory (value), the contents of value is updated.
 * @since_tizen 7.5
 * @param[in] option The handle of ml-option.
 * @param[in] key The key to get the corresponding value.
 * @param[out] value The value of the key.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 */
int ml_option_get (ml_option_h option, const char *key, void **value);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_COMMON_H__ */
