/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2021 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-internal.h
 * @date 20 October 2021
 * @brief ML C-API internal herader without NNStreamer deps.
 *        This file should NOT be exported to SDK or devel package.
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */
#ifndef __ML_API_INTERNAL_H__
#define __ML_API_INTERNAL_H__

#include <glib.h>
#include <ml-api-common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * DO NOT USE THE LOG INFRA of NNSTREAMER.
 *  This header is supposed to be independent from nnstreamer.git
 */
#define MLAPI_TAG_NAME "ml-api"

#if defined(__TIZEN__)
#include <dlog.h>
#define _ml_loge(...) \
    dlog_print (DLOG_ERROR, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logi(...) \
    dlog_print (DLOG_INFO, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logw(...) \
    dlog_print (DLOG_WARN, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logd(...) \
    dlog_print (DLOG_DEBUG, MLAPI_TAG_NAME, __VA_ARGS__)
#elif defined(__ANDROID__)
#include <android/log.h>
#define _ml_loge(...) \
    __android_log_print (ANDROID_LOG_ERROR, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logi(...) \
    __android_log_print (ANDROID_LOG_INFO, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logw(...) \
    __android_log_print (ANDROID_LOG_WARN, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logd(...) \
    __android_log_print (ANDROID_LOG_DEBUG, MLAPI_TAG_NAME, __VA_ARGS__)
#else /* Linux distro */
#include <glib.h>
#define _ml_loge g_critical
#define _ml_logi g_info
#define _ml_logw g_warning
#define _ml_logd g_debug
#endif

#if defined (__TIZEN__)
typedef enum
{
  NOT_CHECKED_YET = -1,
  NOT_SUPPORTED = 0,
  SUPPORTED = 1
} feature_state_t;

#if defined (__FEATURE_CHECK_SUPPORT__)
#define check_feature_state() \
  do { \
    int feature_ret = ml_tizen_get_feature_enabled (); \
    if (ML_ERROR_NONE != feature_ret) \
      return feature_ret; \
  } while (0);

#define set_feature_state(...) ml_tizen_set_feature_state(__VA_ARGS__)
#else /* __FEATURE_CHECK_SUPPORT__ */
#define check_feature_state()
#define set_feature_state(...)
#endif  /* __FEATURE_CHECK_SUPPORT__ */

#if (TIZENVERSION >= 5) && (TIZENVERSION < 9999)
#define TIZEN5PLUS 1
#if ((TIZENVERSION > 6) || (TIZENVERSION == 6 && TIZENVERSIONMINOR >= 5))
#define TIZENMMCONF 1
#endif
#endif

#else /* __TIZEN__ */
#define check_feature_state()
#define set_feature_state(...)
#endif  /* __TIZEN__ */

#ifndef TIZEN5PLUS
#define TIZEN5PLUS 0
#endif /* TIZEN5PLUS */
#ifndef TIZENMMCONF
#define TIZENMMCONF 0
#endif /* TIZENMMCONF */

#define EOS_MESSAGE_TIME_LIMIT 100
#define WAIT_PAUSED_TIME_LIMIT 100

/**
 * @brief Data structure for tensor information.
 * @since_tizen 5.5
 */
typedef struct {
  char *name;              /**< Name of each element in the tensor. */
  ml_tensor_type_e type;   /**< Type of each element in the tensor. */
  ml_tensor_dimension dimension;     /**< Dimension information. */
} ml_tensor_info_s;

/**
 * @brief Data structure for tensors information, which contains multiple tensors.
 * @since_tizen 5.5
 */
typedef struct {
  unsigned int num_tensors; /**< The number of tensors. */
  ml_tensor_info_s info[ML_TENSOR_SIZE_LIMIT];  /**< The list of tensor info. */
  GMutex lock; /**< Lock for thread safety */
  int nolock; /**< Set non-zero to avoid using m (giving up thread safety) */
} ml_tensors_info_s;

/**
 * @brief Macro to control private lock with nolock condition (lock)
 * @param sname The name of struct (ml_tensors_info_s or ml_tensors_data_s)
 */
#define G_LOCK_UNLESS_NOLOCK(sname) \
  do { \
    GMutex *l = (GMutex *) &(sname).lock; \
    if (!(sname).nolock) \
      g_mutex_lock (l); \
  } while (0)

/**
 * @brief Macro to control private lock with nolock condition (unlock)
 * @param sname The name of struct (ml_tensors_info_s or ml_tensors_data_s)
 */
#define G_UNLOCK_UNLESS_NOLOCK(sname) \
  do { \
    GMutex *l = (GMutex *) &(sname).lock; \
    if (!(sname).nolock) \
      g_mutex_unlock (l); \
  } while (0)

/**
 * @brief Macro to verify private lock acquired with nolock condition (lock)
 * @param sname The name of struct (ml_tensors_info_s or ml_tensors_data_s)
 */
#define G_VERIFYLOCK_UNLESS_NOLOCK(sname) \
  do { \
    GMutex *l = (GMutex *) &(sname).lock; \
    if (!(sname).nolock) { \
      if (g_mutex_trylock(l)) { \
        g_mutex_unlock(l); \
        return ML_ERROR_INVALID_PARAMETER; \
      } \
    } \
  } while (0)

/**
 * @brief Macro to check the tensors info is valid.
 */
#define ml_tensors_info_is_valid(i) ({bool v; (ml_tensors_info_validate ((i), &v) == ML_ERROR_NONE && v);})

/**
 * @brief Macro to compare the tensors info.
 */
#define ml_tensors_info_is_equal(i1,i2) ({bool e; (ml_tensors_info_compare ((i1), (i2), &e) == ML_ERROR_NONE && e);})

/**
 * @brief The function to be called when destroying the allocated handle.
 * @since_tizen 6.5
 * @param[in] handle The handle created for ML API.
 * @param[in,out] user_data The user data to pass to the callback function.
 * @return @c 0 on success. Otherwise a negative error value.
 */
typedef int (*ml_handle_destroy_cb) (void *handle, void *user_data);

/**
 * @brief An instance of a single input or output frame.
 * @since_tizen 5.5
 */
typedef struct {
  void *tensor; /**< The instance of tensor data. */
  size_t size; /**< The size of tensor. */
} ml_tensor_data_s;

/**
 * @brief An instance of input or output frames. #ml_tensors_info_h is the handle for tensors metadata.
 * @since_tizen 5.5
 */
typedef struct {
  unsigned int num_tensors; /**< The number of tensors. */
  ml_tensor_data_s tensors[ML_TENSOR_SIZE_LIMIT]; /**< The list of tensor data. NULL for unused tensors. */

  /* private */
  ml_tensors_info_h info;
  void *user_data; /**< The user data to pass to the callback function */
  ml_handle_destroy_cb destroy; /**< The function to be called to release the allocated buffer */
  GMutex lock; /**< Lock for thread safety */
  int nolock; /**< Set non-zero to avoid using m (giving up thread safety) */
} ml_tensors_data_s;

/**
 * @brief Gets the byte size of the given tensor info.
 * @note This is not thread safe.
 */
size_t ml_tensor_info_get_size (const ml_tensor_info_s *info);

/**
 * @brief Initializes the tensors information with default value.
 * @since_tizen 5.5
 * @param[in] info The tensors info pointer to be initialized.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_initialize (ml_tensors_info_s *info);

/**
 * @brief Frees and initialize the data in tensors info.
 * @since_tizen 5.5
 * @param[in] info The tensors info pointer to be freed.
 */
void ml_tensors_info_free (ml_tensors_info_s *info);

/**
 * @brief Creates a tensor data frame without allocating new buffer cloning the given tensors data.
 * @details If @a data_src is null, this returns error.
 * @param[in] data_src The handle of tensors data to be cloned.
 * @param[out] data The handle of tensors data.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 */
int ml_tensors_data_clone_no_alloc (const ml_tensors_data_s * data_src, ml_tensors_data_h * data);

/**
 * @brief Replaces string.
 * This function deallocates the input source string.
 * This is copied from nnstreamer/tensor_common.c by the nnstreamer maintainer.
 * @param[in] source The input string. This will be freed when returning the replaced string.
 * @param[in] what The string to search for.
 * @param[in] to The string to be replaced.
 * @param[in] delimiters The characters which specify the place to split the string. Set NULL to replace all matched string.
 * @param[out] count The count of replaced. Set NULL if it is unnecessary.
 * @return Newly allocated string. The returned string should be freed with g_free().
 */
gchar * ml_replace_string (gchar * source, const gchar * what, const gchar * to, const gchar * delimiters, guint * count);

/**
 * @brief Compares the given tensors information.
 * @details If the function returns an error, @a equal is not changed.
 * @since_tizen 6.0
 * @param[in] info1 The handle of tensors information to be compared.
 * @param[in] info2 The handle of tensors information to be compared.
 * @param[out] equal @c true if given tensors information is equal, @c false if it's not equal.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int ml_tensors_info_compare (const ml_tensors_info_h info1, const ml_tensors_info_h info2, bool *equal);

/**
 * @brief Frees the tensors data handle and its data.
 * @param[in] data The handle of tensors data.
 * @param[in] free_data The flag to free the buffers in handle.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int ml_tensors_data_destroy_internal (ml_tensors_data_h data, gboolean free_data);

/**
 * @brief Creates a tensor data frame without buffer with the given tensors information.
 * @details If @a info is null, this allocates data handle with empty tensor data.
 * @param[in] info The handle of tensors information for the allocation.
 * @param[out] data The handle of tensors data.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int ml_tensors_data_create_no_alloc (const ml_tensors_info_h info, ml_tensors_data_h *data);

#if defined (__TIZEN__)
/****** TIZEN CHECK FEATURE BEGINS *****/
/**
 * @brief Checks whether machine_learning.inference feature is enabled or not.
 */
int ml_tizen_get_feature_enabled (void);

/**
 * @brief Set the feature status of machine_learning.inference.
 * This is only used for Unit test.
 */
int ml_tizen_set_feature_state (int state);
/****** TIZEN CHECK FEATURE ENDS *****/
#endif /* __TIZEN__ */


/***** Begin: Error reporting internal interfaces ****
 * ml-api-* implementation needs to use these interfaces to provide
 * proper error messages for ml_error();
 */

/**
 * @brief Call when an error occurs during API execution.
 * @param[in] message The error description
 */
void _ml_error_report (const char *message);

#define _ml_error_report_return (errno, message)  do { \
  _ml_error_report (message); \
  return errno; \
} while(0)

/***** End: Error reporting internal interfaces *****/


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_INTERNAL_H__ */
