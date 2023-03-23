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
#ifdef FAKEDLOG
#define _ml_loge(...) dlog_print (0, "MLAPI ERR", __VA_ARGS__)
#define _ml_logi(...) dlog_print (0, "MLAPI_INFO", __VA_ARGS__)
#define _ml_logw(...) dlog_print (0, "MLAPI_WARN", __VA_ARGS__)
#define _ml_logd(...) dlog_print (0, "MLAPI_DEBUG", __VA_ARGS__)
#else /* FAKEDLOG */
#include <dlog.h>
#define _ml_loge(...) \
    dlog_print (DLOG_ERROR, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logi(...) \
    dlog_print (DLOG_INFO, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logw(...) \
    dlog_print (DLOG_WARN, MLAPI_TAG_NAME, __VA_ARGS__)
#define _ml_logd(...) \
    dlog_print (DLOG_DEBUG, MLAPI_TAG_NAME, __VA_ARGS__)
#endif /* FAKEDLOG */
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

/**
 * @brief Enumeration for machine_learning feature.
 * @note DO NOT export this enumeration. This is internal value to validate Tizen feature state.
 */
typedef enum {
  ML_FEATURE = 0,
  ML_FEATURE_INFERENCE,
  ML_FEATURE_TRAINING,
  ML_FEATURE_SERVICE,

  ML_FEATURE_MAX
} ml_feature_e;

#if defined (__FEATURE_CHECK_SUPPORT__)
#define check_feature_state(ml_feature) \
  do { \
    int feature_ret = _ml_tizen_get_feature_enabled (ml_feature); \
    if (ML_ERROR_NONE != feature_ret) { \
      _ml_error_report_return (feature_ret, \
          "Failed to get feature: %s with an error: %d. " \
          "Please check the feature is enabled.", \
          _ml_tizen_get_feature_path(ml_feature), feature_ret); \
    } \
  } while (0);

#define set_feature_state(...) _ml_tizen_set_feature_state(__VA_ARGS__)
#else /* __FEATURE_CHECK_SUPPORT__ */
#define check_feature_state(...)
#define set_feature_state(...)
#endif  /* __FEATURE_CHECK_SUPPORT__ */

#if (TIZENVERSION >= 5) && (TIZENVERSION < 9999)
#define TIZEN5PLUS 1
#if ((TIZENVERSION > 6) || (TIZENVERSION == 6 && TIZENVERSIONMINOR >= 5))
#define TIZENMMCONF 1
#endif
#endif
#if ((TIZENVERSION < 7) || (TIZENVERSION == 7 && TIZENVERSIONMINOR < 5))
#define TIZENPPM 1
#endif

#else /* __TIZEN__ */
#define check_feature_state(...)
#define set_feature_state(...)
#endif  /* __TIZEN__ */

#ifndef TIZEN5PLUS
#define TIZEN5PLUS 0
#endif /* TIZEN5PLUS */
#ifndef TIZENMMCONF
#define TIZENMMCONF 0
#endif /* TIZENMMCONF */
#ifndef TIZENPPM
#define TIZENPPM 0
#endif /* TIZENPPM */

#define EOS_MESSAGE_TIME_LIMIT 100
#define WAIT_PAUSED_TIME_LIMIT 100

/**
 * @brief The previous maximum rank that NNStreamer supports.
 * @details NNStreamer supports max rank 4 before 2.3.1
 */
#define ML_TENSOR_RANK_LIMIT_PREV  (4)

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
  bool is_extended; /**< True if tensors are extended */
} ml_tensors_info_s;

/**
 * @brief Data structure for value of ml_option.
 * @since_tizen 7.0
 */
typedef struct
{
  void *value; /**< The data given by user. */
  ml_data_destroy_cb destroy; /**< The destroy func given by user. */
} ml_option_value_s;

/**
 * @brief Data structure for ml_option.
 * @since_tizen 7.0
 */
typedef struct
{
  GHashTable *option_table; /**< hash table used by ml_option. */
} ml_option_s;

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
        _ml_error_report_return (ML_ERROR_INVALID_PARAMETER, \
            "The lock of an object %s is not locked. It should've been locked already.", \
            #sname); \
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
#define ml_tensors_info_is_equal(i1,i2) ({bool e; (_ml_tensors_info_compare ((i1), (i2), &e) == ML_ERROR_NONE && e);})

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
size_t _ml_tensor_info_get_size (const ml_tensor_info_s *info, bool is_extended);

/**
 * @brief Initializes the tensors information with default value.
 * @since_tizen 5.5
 * @param[in] info The tensors info pointer to be initialized.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int _ml_tensors_info_initialize (ml_tensors_info_s *info);

/**
 * @brief Frees and initialize the data in tensors info.
 * @since_tizen 5.5
 * @param[in] info The tensors info pointer to be freed.
 */
void _ml_tensors_info_free (ml_tensors_info_s *info);

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
int _ml_tensors_data_clone_no_alloc (const ml_tensors_data_s * data_src, ml_tensors_data_h * data);

/**
 * @brief Copies the tensor data frame.
 * @since_tizen 7.5
 * @param[in] in The handle of tensors data to be cloned.
 * @param[out] out The handle of tensors data. The caller is responsible for freeing the allocated data with ml_tensors_data_destroy().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful.
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 * @retval #ML_ERROR_OUT_OF_MEMORY Failed to allocate required memory.
 * @todo Consider adding new API from tizen 7.5.
 */
int ml_tensors_data_clone (const ml_tensors_data_h in, ml_tensors_data_h *out);

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
gchar * _ml_replace_string (gchar * source, const gchar * what, const gchar * to, const gchar * delimiters, guint * count);

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
int _ml_tensors_info_compare (const ml_tensors_info_h info1, const ml_tensors_info_h info2, bool *equal);

/**
 * @brief Frees the tensors data handle and its data.
 * @param[in] data The handle of tensors data.
 * @param[in] free_data The flag to free the buffers in handle.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int _ml_tensors_data_destroy_internal (ml_tensors_data_h data, gboolean free_data);

/**
 * @brief Creates a tensor data frame without buffer with the given tensors information.
 * @details If @a info is null, this allocates data handle with empty tensor data.
 * @param[in] info The handle of tensors information for the allocation.
 * @param[out] data The handle of tensors data.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int _ml_tensors_data_create_no_alloc (const ml_tensors_info_h info, ml_tensors_data_h *data);

#if defined (__TIZEN__)
/****** TIZEN CHECK FEATURE BEGINS *****/
/**
 * @brief Get machine learning feature path.
 */
const char* _ml_tizen_get_feature_path (ml_feature_e ml_feature);

/**
 * @brief Checks whether machine_learning.inference feature is enabled or not.
 */
int _ml_tizen_get_feature_enabled (ml_feature_e ml_feature);

/**
 * @brief Set the feature status of machine_learning.inference.
 * This is only used for Unit test.
 */
int _ml_tizen_set_feature_state (ml_feature_e ml_feature, int state);
/****** TIZEN CHECK FEATURE ENDS *****/
#endif /* __TIZEN__ */


/***** Begin: Error reporting internal interfaces ****
 * ml-api-* implementation needs to use these interfaces to provide
 * proper error messages for ml_error();
 */

/**
 * @brief Private function for error reporting infrastructure. Don't use.
 * @note Use _ml_error_report instead!
 */
void _ml_error_report_ (const char *fmt, ...);

/**
 * @brief Private function for error reporting infrastructure. Don't use.
 * @note Use _ml_error_report instead!
 */
void _ml_error_report_continue_ (const char *fmt, ...);

/**
 * @brief Private macro for error repoting infra. Don't use.
 */
#define _ml_error_report_return_(errno, ...)  do { \
  _ml_error_report_ (__VA_ARGS__); \
  return errno; \
} while(0)

/**
 * @brief Private macro for error repoting infra. Don't use.
 */
#define _ml_error_report_return_continue_(errno, ...)  do { \
  _ml_error_report_continue_ (__VA_ARGS__); \
  return errno; \
} while(0)

#define _ml_detail(fmt, ...) "%s:%s:%d: " fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__

/**
 * @brief Error report API. W/o return & previous report reset.
 * @param[in] fmt The printf-styled error message.
 * @note This provides source file, function name, and line number as well.
 */
#define _ml_error_report(fmt, ...) \
  _ml_error_report_ (_ml_detail (fmt, ##__VA_ARGS__))

/**
 * @brief Error report API. With return / W/o previous report reset.
 * @param[in] errno The error code (negative numbers)
 * @param[in] fmt The printf-styled error message.
 * @note This provides source file, function name, and line number as well.
 */
#define _ml_error_report_return(errno, fmt, ...) \
  _ml_error_report_return_ (errno, _ml_detail (fmt, ##__VA_ARGS__))

/**
 * @brief Error report API. W/o return / With previous report reset.
 * @param[in] fmt The printf-styled error message.
 * @note This provides source file, function name, and line number as well.
 */
#define _ml_error_report_continue(fmt, ...) \
  _ml_error_report_continue_ (_ml_detail (fmt, ##__VA_ARGS__))

/**
 * @brief Error report API. With return & previous report reset.
 * @param[in] errno The error code (negative numbers)
 * @param[in] fmt The printf-styled error message.
 * @note This provides source file, function name, and line number as well.
 */
#define _ml_error_report_return_continue(errno, fmt, ...) \
  _ml_error_report_return_continue_ (errno, _ml_detail (fmt, ##__VA_ARGS__))

/**
 * @brief macro to relay previous error only if the errno is nonzero
 * @param[in] op The statement to be executed, which will return an errno.
 * @param[in] fmt The printf-styled error message.
 * @note Execute _ml_error_report_return_continue if errno is nonzero. The errno should be int. Use _ERRNO to refer to the error return value.
 */
#define _ml_error_report_return_continue_iferr(op, fmt, ...) \
  do { \
    int _ERRNO = (op); \
    if (_ERRNO) \
      _ml_error_report_return_continue (_ERRNO, fmt, ##__VA_ARGS__); \
  } while (0)

/***** End: Error reporting internal interfaces *****/

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_INTERNAL_H__ */
