/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file nnstreamer-capi-util.c
 * @date 10 June 2019
 * @brief NNStreamer/Utilities C-API Wrapper.
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <glib.h>
#include <nnstreamer_plugin_api_util.h>
#include "nnstreamer.h"
#include "nnstreamer-tizen-internal.h"
#include "ml-api-internal.h"

/**
 * @brief Enumeration for ml_info type.
 */
typedef enum
{
  ML_INFO_TYPE_UNKNOWN = 0,
  ML_INFO_TYPE_OPTION = 0xfeed0001,
  ML_INFO_TYPE_INFORMATION = 0xfeed0010,
  ML_INFO_TYPE_INFORMATION_LIST = 0xfeed0011,

  ML_INFO_TYPE_MAX = 0xfeedffff
} ml_info_type_e;

/**
 * @brief Data structure for value of ml_info.
 */
typedef struct
{
  void *value; /**< The data given by user. */
  ml_data_destroy_cb destroy; /**< The destroy func given by user. */
} ml_info_value_s;

/**
 * @brief Data structure for ml_info.
 */
typedef struct
{
  ml_info_type_e type; /**< The type of ml_info. */
  GHashTable *table; /**< hash table used by ml_info. */
} ml_info_s;

/**
 * @brief Data structure for ml_info_list.
 */
typedef struct
{
  ml_info_type_e type; /**< The type of ml_info. */
  unsigned int length; /**< The length of data. */
  ml_info_s **info; /**< array of ml_info. */
} ml_info_list_s;

/**
 * @brief Gets the version number of machine-learning API.
 */
void
ml_api_get_version (unsigned int *major, unsigned int *minor,
    unsigned int *micro)
{
  if (major)
    *major = VERSION_MAJOR;
  if (minor)
    *minor = VERSION_MINOR;
  if (micro)
    *micro = VERSION_MICRO;
}

/**
 * @brief Convert the type from ml_tensor_type_e to tensor_type.
 * @note This code is based on the same order between NNS type and ML type.
 * The index should be the same in case of adding a new type.
 */
static tensor_type
convert_tensor_type_from (ml_tensor_type_e type)
{
  if (type < ML_TENSOR_TYPE_INT32 || type >= ML_TENSOR_TYPE_UNKNOWN) {
    _ml_error_report
        ("Failed to convert the type. Input ml_tensor_type_e %d is invalid.",
        type);
    return _NNS_END;
  }

  return (tensor_type) type;
}

/**
 * @brief Convert the type from tensor_type to ml_tensor_type_e.
 * @note This code is based on the same order between NNS type and ML type.
 * The index should be the same in case of adding a new type.
 */
static ml_tensor_type_e
convert_ml_tensor_type_from (tensor_type type)
{
  if (type < _NNS_INT32 || type >= _NNS_END) {
    _ml_error_report
        ("Failed to convert the type. Input tensor_type %d is invalid.", type);
    return ML_TENSOR_TYPE_UNKNOWN;
  }

  return (ml_tensor_type_e) type;
}

/**
 * @brief Gets the version string of machine-learning API.
 */
char *
ml_api_get_version_string (void)
{
  return g_strdup_printf ("Machine Learning API %s", VERSION);
}

/**
 * @brief Internal function to create tensors-info handle.
 */
static int
_ml_tensors_info_create_internal (ml_tensors_info_h * info, bool extended)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. Provide a valid pointer.");

  *info = tensors_info = g_new0 (ml_tensors_info_s, 1);
  if (tensors_info == NULL)
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate the tensors info handle. Out of memory?");

  g_mutex_init (&tensors_info->lock);
  tensors_info->is_extended = extended;

  /* init tensors info struct */
  return _ml_tensors_info_initialize (tensors_info);
}

/**
 * @brief Allocates a tensors information handle with default value.
 */
int
ml_tensors_info_create (ml_tensors_info_h * info)
{
  return _ml_tensors_info_create_internal (info, false);
}

/**
 * @brief Allocates an extended tensors information handle with default value.
 */
int
ml_tensors_info_create_extended (ml_tensors_info_h * info)
{
  return _ml_tensors_info_create_internal (info, true);
}

/**
 * @brief Frees the given handle of a tensors information.
 */
int
ml_tensors_info_destroy (ml_tensors_info_h info)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state (ML_FEATURE);

  tensors_info = (ml_tensors_info_s *) info;

  if (!tensors_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. Provide a valid pointer.");

  G_LOCK_UNLESS_NOLOCK (*tensors_info);
  _ml_tensors_info_free (tensors_info);
  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);

  g_mutex_clear (&tensors_info->lock);
  g_free (tensors_info);

  return ML_ERROR_NONE;
}

/**
 * @brief Validates the given tensors info is valid.
 */
int
ml_tensors_info_validate (const ml_tensors_info_h info, bool *valid)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state (ML_FEATURE);

  if (!valid)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The data-return parameter, valid, is NULL. It should be a pointer pre-allocated by the caller.");

  tensors_info = (ml_tensors_info_s *) info;

  if (!tensors_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The input parameter, tensors_info, is NULL. It should be a valid ml_tensors_info_h, which is usually created by ml_tensors_info_create().");

  G_LOCK_UNLESS_NOLOCK (*tensors_info);
  *valid = gst_tensors_info_validate (&tensors_info->info);
  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);

  return ML_ERROR_NONE;
}

/**
 * @brief Compares the given tensors information.
 */
int
_ml_tensors_info_compare (const ml_tensors_info_h info1,
    const ml_tensors_info_h info2, bool *equal)
{
  ml_tensors_info_s *i1, *i2;

  check_feature_state (ML_FEATURE);

  if (info1 == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The input parameter, info1, should be a valid ml_tensors_info_h handle, which is usually created by ml_tensors_info_create(). However, info1 is NULL.");
  if (info2 == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The input parameter, info2, should be a valid ml_tensors_info_h handle, which is usually created by ml_tensors_info_create(). However, info2 is NULL.");
  if (equal == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The output parameter, equal, should be a valid pointer allocated by the caller. However, equal is NULL.");

  i1 = (ml_tensors_info_s *) info1;
  G_LOCK_UNLESS_NOLOCK (*i1);
  i2 = (ml_tensors_info_s *) info2;
  G_LOCK_UNLESS_NOLOCK (*i2);

  *equal = gst_tensors_info_is_equal (&i1->info, &i2->info);

  G_UNLOCK_UNLESS_NOLOCK (*i2);
  G_UNLOCK_UNLESS_NOLOCK (*i1);
  return ML_ERROR_NONE;
}

/**
 * @brief Sets the number of tensors with given handle of tensors information.
 */
int
ml_tensors_info_set_count (ml_tensors_info_h info, unsigned int count)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid ml_tensors_info_h handle, which is usually created by ml_tensors_info_create().");
  if (count > ML_TENSOR_SIZE_LIMIT || count == 0)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, count, is the number of tensors, which should be between 1 and %d. The given count is %u.",
        ML_TENSOR_SIZE_LIMIT, count);

  tensors_info = (ml_tensors_info_s *) info;

  /* This is atomic. No need for locks */
  tensors_info->info.num_tensors = count;

  return ML_ERROR_NONE;
}

/**
 * @brief Gets the number of tensors with given handle of tensors information.
 */
int
ml_tensors_info_get_count (ml_tensors_info_h info, unsigned int *count)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The paramter, info, is NULL. It should be a valid ml_tensors_info_h handle, which is usually created by ml_tensors_info_create().");
  if (!count)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, count, is NULL. It should be a valid unsigned int * pointer, allocated by the caller.");

  tensors_info = (ml_tensors_info_s *) info;
  /* This is atomic. No need for locks */
  *count = tensors_info->info.num_tensors;

  return ML_ERROR_NONE;
}

/**
 * @brief Sets the tensor name with given handle of tensors information.
 */
int
ml_tensors_info_set_tensor_name (ml_tensors_info_h info,
    unsigned int index, const char *name)
{
  ml_tensors_info_s *tensors_info;
  GstTensorInfo *_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid ml_tensors_info_h handle, which is usually created by ml_tensors_info_create().");

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->info.num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The number of tensors in 'info' parameter is %u, which is not larger than the given 'index' %u. Thus, we cannot get %u'th tensor from 'info'. Please set the number of tensors of 'info' correctly or check the value of the given 'index'.",
        tensors_info->info.num_tensors, index, index);
  }

  _info = gst_tensors_info_get_nth_info (&tensors_info->info, index);
  if (!_info) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  g_free (_info->name);
  _info->name = g_strdup (name);

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Gets the tensor name with given handle of tensors information.
 */
int
ml_tensors_info_get_tensor_name (ml_tensors_info_h info,
    unsigned int index, char **name)
{
  ml_tensors_info_s *tensors_info;
  GstTensorInfo *_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid ml_tensors_info_h handle, which is usually created by ml_tensors_info_create().");
  if (!name)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, name, is NULL. It should be a valid char ** pointer, allocated by the caller. E.g., char *name; ml_tensors_info_get_tensor_name (info, index, &name);");

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->info.num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The number of tensors in 'info' parameter is %u, which is not larger than the given 'index' %u. Thus, we cannot get %u'th tensor from 'info'. Please set the number of tensors of 'info' correctly or check the value of the given 'index'.",
        tensors_info->info.num_tensors, index, index);
  }

  _info = gst_tensors_info_get_nth_info (&tensors_info->info, index);
  if (!_info) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  *name = g_strdup (_info->name);

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Sets the tensor type with given handle of tensors information.
 */
int
ml_tensors_info_set_tensor_type (ml_tensors_info_h info,
    unsigned int index, const ml_tensor_type_e type)
{
  ml_tensors_info_s *tensors_info;
  GstTensorInfo *_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid pointer of ml_tensors_info_h, which is usually created by ml_tensors_info_create().");

  if (type >= ML_TENSOR_TYPE_UNKNOWN || type < 0)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, type, ML_TENSOR_TYPE_UNKNOWN or out of bound. The value of type should be between 0 and ML_TENSOR_TYPE_UNKNOWN - 1. type = %d, ML_TENSOR_TYPE_UNKNOWN = %d.",
        type, ML_TENSOR_TYPE_UNKNOWN);

#ifndef FLOAT16_SUPPORT
  if (type == ML_TENSOR_TYPE_FLOAT16)
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "Float16 (IEEE 754) is not supported by the machine (or the compiler or your build configuration). You cannot configure ml_tensors_info instance with Float16 type.");
#endif
  /** @todo add BFLOAT16 when nnstreamer is ready for it. */

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->info.num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The number of tensors in 'info' parameter is %u, which is not larger than the given 'index' %u. Thus, we cannot get %u'th tensor from 'info'. Please set the number of tensors of 'info' correctly or check the value of the given 'index'.",
        tensors_info->info.num_tensors, index, index);
  }

  _info = gst_tensors_info_get_nth_info (&tensors_info->info, index);
  if (!_info) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  _info->type = convert_tensor_type_from (type);

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Gets the tensor type with given handle of tensors information.
 */
int
ml_tensors_info_get_tensor_type (ml_tensors_info_h info,
    unsigned int index, ml_tensor_type_e * type)
{
  ml_tensors_info_s *tensors_info;
  GstTensorInfo *_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid ml_tensors_info_h handle, which is usually created by ml_tensors_info_create().");
  if (!type)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, type, is NULL. It should be a valid pointer of ml_tensor_type_e *, allocated by the caller. E.g., ml_tensor_type_e t; ml_tensors_info_get_tensor_type (info, index, &t);");

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->info.num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The number of tensors in 'info' parameter is %u, which is not larger than the given 'index' %u. Thus, we cannot get %u'th tensor from 'info'. Please set the number of tensors of 'info' correctly or check the value of the given 'index'.",
        tensors_info->info.num_tensors, index, index);
  }

  _info = gst_tensors_info_get_nth_info (&tensors_info->info, index);
  if (!_info) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  *type = convert_ml_tensor_type_from (_info->type);

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Sets the tensor dimension with given handle of tensors information.
 */
int
ml_tensors_info_set_tensor_dimension (ml_tensors_info_h info,
    unsigned int index, const ml_tensor_dimension dimension)
{
  ml_tensors_info_s *tensors_info;
  GstTensorInfo *_info;
  guint i, rank, max_rank;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid pointer of ml_tensors_info_h, which is usually created by ml_tensors_info_create().");

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->info.num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The number of tensors in 'info' parameter is %u, which is not larger than the given 'index' %u. Thus, we cannot get %u'th tensor from 'info'. Please set the number of tensors of 'info' correctly or check the value of the given 'index'.",
        tensors_info->info.num_tensors, index, index);
  }

  /**
   * Validate dimension.
   * We cannot use util function to get the rank of tensor dimension here.
   * The old rank limit is 4, and testcases or app may set old dimension.
   */
  max_rank = tensors_info->is_extended ?
      ML_TENSOR_RANK_LIMIT : ML_TENSOR_RANK_LIMIT_PREV;
  rank = max_rank + 1;
  for (i = 0; i < max_rank; i++) {
    if (dimension[i] == 0) {
      if (rank > max_rank)
        rank = i;
    }

    if (rank == 0 || (i > rank && dimension[i] > 0)) {
      G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The parameter, dimension, is invalid. It should be a valid unsigned integer array.");
    }
  }

  _info = gst_tensors_info_get_nth_info (&tensors_info->info, index);
  if (!_info) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  for (i = 0; i < ML_TENSOR_RANK_LIMIT_PREV; i++) {
    _info->dimension[i] = dimension[i];
  }

  for (i = ML_TENSOR_RANK_LIMIT_PREV; i < ML_TENSOR_RANK_LIMIT; i++) {
    _info->dimension[i] = (tensors_info->is_extended ? dimension[i] : 0);
  }

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Gets the tensor dimension with given handle of tensors information.
 */
int
ml_tensors_info_get_tensor_dimension (ml_tensors_info_h info,
    unsigned int index, ml_tensor_dimension dimension)
{
  ml_tensors_info_s *tensors_info;
  GstTensorInfo *_info;
  guint i, valid_rank = ML_TENSOR_RANK_LIMIT;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid pointer of ml_tensors_info_h, which is usually created by ml_tensors_info_create().");

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->info.num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The number of tensors in 'info' parameter is %u, which is not larger than the given 'index' %u. Thus, we cannot get %u'th tensor from 'info'. Please set the number of tensors of 'info' correctly or check the value of the given 'index'.",
        tensors_info->info.num_tensors, index, index);
  }

  _info = gst_tensors_info_get_nth_info (&tensors_info->info, index);
  if (!_info) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (!tensors_info->is_extended)
    valid_rank = ML_TENSOR_RANK_LIMIT_PREV;

  for (i = 0; i < valid_rank; i++) {
    dimension[i] = _info->dimension[i];
  }

  for (; i < ML_TENSOR_RANK_LIMIT; i++)
    dimension[i] = 0;

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Gets the byte size of the given handle of tensors information.
 */
int
ml_tensors_info_get_tensor_size (ml_tensors_info_h info,
    int index, size_t *data_size)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state (ML_FEATURE);

  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. Provide a valid pointer.");
  if (!data_size)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data_size, is NULL. It should be a valid size_t * pointer allocated by the caller. E.g., size_t d; ml_tensors_info_get_tensor_size (info, index, &d);");

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  /* init 0 */
  *data_size = 0;

  if (index >= 0 && tensors_info->info.num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The number of tensors in 'info' parameter is %u, which is not larger than the given 'index' %u. Thus, we cannot get %u'th tensor from 'info'. Please set the number of tensors of 'info' correctly or check the value of the given 'index'.",
        tensors_info->info.num_tensors, index, index);
  }

  *data_size = gst_tensors_info_get_size (&tensors_info->info, index);

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Initializes the tensors information with default value.
 */
int
_ml_tensors_info_initialize (ml_tensors_info_s * info)
{
  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. Provide a valid pointer.");

  gst_tensors_info_init (&info->info);

  return ML_ERROR_NONE;
}

/**
 * @brief Frees and initialize the data in tensors info.
 * @note This does not touch the lock. The caller should lock.
 */
void
_ml_tensors_info_free (ml_tensors_info_s * info)
{
  if (!info)
    return;

  gst_tensors_info_free (&info->info);
}

/**
 * @brief Frees the tensors data handle and its data.
 * @param[in] data The handle of tensors data.
 * @param[in] free_data The flag to free the buffers in handle.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int
_ml_tensors_data_destroy_internal (ml_tensors_data_h data, gboolean free_data)
{
  int status = ML_ERROR_NONE;
  ml_tensors_data_s *_data;
  guint i;

  if (data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data, is NULL. It should be a valid ml_tensors_data_h handle, which is usually created by ml_tensors_data_create ().");

  _data = (ml_tensors_data_s *) data;
  G_LOCK_UNLESS_NOLOCK (*_data);

  if (free_data) {
    if (_data->destroy) {
      status = _data->destroy (_data, _data->user_data);
      if (status != ML_ERROR_NONE) {
        G_UNLOCK_UNLESS_NOLOCK (*_data);
        _ml_error_report_return_continue (status,
            "Tried to destroy internal user_data of the given parameter, data, with its destroy callback; however, it has failed with %d.",
            status);
      }
    } else {
      for (i = 0; i < ML_TENSOR_SIZE_LIMIT; i++) {
        if (_data->tensors[i].data) {
          g_free (_data->tensors[i].data);
          _data->tensors[i].data = NULL;
        }
      }
    }
  }

  if (_data->info)
    ml_tensors_info_destroy (_data->info);

  G_UNLOCK_UNLESS_NOLOCK (*_data);
  g_mutex_clear (&_data->lock);
  g_free (_data);
  return status;
}

/**
 * @brief Frees the tensors data pointer.
 * @note This does not touch the lock
 */
int
ml_tensors_data_destroy (ml_tensors_data_h data)
{
  int ret;
  check_feature_state (ML_FEATURE);
  ret = _ml_tensors_data_destroy_internal (data, TRUE);
  if (ret != ML_ERROR_NONE)
    _ml_error_report_return_continue (ret,
        "Call to _ml_tensors_data_destroy_internal failed with %d", ret);
  return ret;
}

/**
 * @brief Creates a tensor data frame without buffer with the given tensors information.
 * @note Memory for tensor data buffers is not allocated.
 */
int
_ml_tensors_data_create_no_alloc (const ml_tensors_info_h info,
    ml_tensors_data_h * data)
{
  ml_tensors_data_s *_data;
  ml_tensors_info_s *_info;
  gint i;

  check_feature_state (ML_FEATURE);

  if (data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data, is NULL. It should be a valid ml_tensors_info_h handle that may hold a space for ml_tensors_info_h. E.g., ml_tensors_data_h data; _ml_tensors_data_create_no_alloc (info, &data);.");

  /* init null */
  *data = NULL;

  _data = g_new0 (ml_tensors_data_s, 1);
  if (!_data)
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for tensors data with g_new0(). Probably the system is out of memory.");

  g_mutex_init (&_data->lock);

  _info = (ml_tensors_info_s *) info;
  if (_info != NULL) {
    if (_info->is_extended)
      ml_tensors_info_create_extended (&_data->info);
    else
      ml_tensors_info_create (&_data->info);
    ml_tensors_info_clone (_data->info, info);

    G_LOCK_UNLESS_NOLOCK (*_info);
    _data->num_tensors = _info->info.num_tensors;
    for (i = 0; i < _data->num_tensors; i++) {
      _data->tensors[i].size = gst_tensors_info_get_size (&_info->info, i);
      _data->tensors[i].data = NULL;
    }
    G_UNLOCK_UNLESS_NOLOCK (*_info);
  }

  *data = _data;
  return ML_ERROR_NONE;
}

/**
 * @brief Clones the given tensor data frame from the given tensors data. (more info in nnstreamer.h)
 * @note Memory ptr for data buffer is copied. No new memory for data buffer is allocated.
 */
int
_ml_tensors_data_clone_no_alloc (const ml_tensors_data_s * data_src,
    ml_tensors_data_h * data)
{
  int status;
  ml_tensors_data_s *_data;

  check_feature_state (ML_FEATURE);

  if (data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data, is NULL. It should be a valid ml_tensors_data_h handle, which is usually created by ml_tensors_data_create ().");
  if (data_src == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data_src, the source data to be cloned, is NULL. It should be a valid ml_tensors_data_s struct (internal representation of ml_tensors_data_h handle).");

  status = _ml_tensors_data_create_no_alloc (data_src->info,
      (ml_tensors_data_h *) & _data);
  if (status != ML_ERROR_NONE)
    _ml_error_report_return_continue (status,
        "The call to _ml_tensors_data_create_no_alloc has failed with %d.",
        status);

  G_LOCK_UNLESS_NOLOCK (*_data);

  _data->num_tensors = data_src->num_tensors;
  memcpy (_data->tensors, data_src->tensors,
      sizeof (GstTensorMemory) * data_src->num_tensors);

  *data = _data;
  G_UNLOCK_UNLESS_NOLOCK (*_data);
  return ML_ERROR_NONE;
}

/**
 * @brief Copies the tensor data frame.
 */
int
ml_tensors_data_clone (const ml_tensors_data_h in, ml_tensors_data_h * out)
{
  int status;
  unsigned int i;
  ml_tensors_data_s *_in, *_out;

  check_feature_state (ML_FEATURE);

  if (in == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, in, is NULL. It should be a valid ml_tensors_data_h handle, which is usually created by ml_tensors_data_create ().");

  if (out == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, out, is NULL. It should be a valid pointer to a space that can hold a ml_tensors_data_h handle. E.g., ml_tensors_data_h out; ml_tensors_data_clone (in, &out);.");

  _in = (ml_tensors_data_s *) in;
  G_LOCK_UNLESS_NOLOCK (*_in);

  status = ml_tensors_data_create (_in->info, out);
  if (status != ML_ERROR_NONE) {
    _ml_loge ("Failed to create new handle to copy tensor data.");
    goto error;
  }

  _out = (ml_tensors_data_s *) (*out);

  for (i = 0; i < _out->num_tensors; ++i) {
    memcpy (_out->tensors[i].data, _in->tensors[i].data, _in->tensors[i].size);
  }

error:
  G_UNLOCK_UNLESS_NOLOCK (*_in);
  return status;
}

/**
 * @brief Allocates a tensor data frame with the given tensors info. (more info in nnstreamer.h)
 */
int
ml_tensors_data_create (const ml_tensors_info_h info, ml_tensors_data_h * data)
{
  gint status = ML_ERROR_STREAMS_PIPE;
  ml_tensors_data_s *_data = NULL;
  gint i;
  bool valid;

  check_feature_state (ML_FEATURE);

  if (info == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is NULL. It should be a valid pointer of ml_tensors_info_h, which is usually created by ml_tensors_info_create().");
  if (data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data, is NULL. It should be a valid space to hold a ml_tensors_data_h handle. E.g., ml_tensors_data_h data; ml_tensors_data_create (info, &data);.");

  status = ml_tensors_info_validate (info, &valid);
  if (status != ML_ERROR_NONE)
    _ml_error_report_return_continue (status,
        "ml_tensors_info_validate() has reported that the parameter, info, is not NULL, but its contents are not valid. The user must provide a valid tensor information with it.");
  if (!valid)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info, is not NULL, but its contents are not valid. The user must provide a valid tensor information with it. Probably, there is an entry that is not allocated or dimension/type information not available. The given info should have valid number of tensors, entries of every tensor along with its type and dimension info.");

  status =
      _ml_tensors_data_create_no_alloc (info, (ml_tensors_data_h *) & _data);

  if (status != ML_ERROR_NONE) {
    _ml_error_report_return_continue (status,
        "Failed to allocate tensor data based on the given info with the call to _ml_tensors_data_create_no_alloc (): %d. Check if it's out-of-memory.",
        status);
  }

  for (i = 0; i < _data->num_tensors; i++) {
    _data->tensors[i].data = g_malloc0 (_data->tensors[i].size);
    if (_data->tensors[i].data == NULL) {
      goto failed_oom;
    }
  }

  *data = _data;
  return ML_ERROR_NONE;

failed_oom:
  _ml_tensors_data_destroy_internal (_data, TRUE);

  _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
      "Failed to allocate memory blocks for tensors data. Check if it's out-of-memory.");
}

/**
 * @brief Gets a tensor data of given handle.
 */
int
ml_tensors_data_get_tensor_data (ml_tensors_data_h data, unsigned int index,
    void **raw_data, size_t *data_size)
{
  ml_tensors_data_s *_data;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE);

  if (data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data, is NULL. It should be a valid ml_tensors_data_h handle, which is usually created by ml_tensors_data_create ().");
  if (raw_data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, raw_data, is NULL. It should be a valid, non-NULL, void ** pointer, which is supposed to point to the raw data of tensors[index] after the call.");
  if (data_size == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data_size, is NULL. It should be a valid, non-NULL, size_t * pointer, which is supposed to point to the size of returning raw_data after the call.");

  _data = (ml_tensors_data_s *) data;
  G_LOCK_UNLESS_NOLOCK (*_data);

  if (_data->num_tensors <= index) {
    _ml_error_report
        ("The parameter, index, is out of bound. The number of tensors of 'data' is %u while you requested %u'th tensor (index = %u).",
        _data->num_tensors, index, index);
    status = ML_ERROR_INVALID_PARAMETER;
    goto report;
  }

  *raw_data = _data->tensors[index].data;
  *data_size = _data->tensors[index].size;

report:
  G_UNLOCK_UNLESS_NOLOCK (*_data);
  return status;
}

/**
 * @brief Copies a tensor data to given handle.
 */
int
ml_tensors_data_set_tensor_data (ml_tensors_data_h data, unsigned int index,
    const void *raw_data, const size_t data_size)
{
  ml_tensors_data_s *_data;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE);

  if (data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, data, is NULL. It should be a valid ml_tensors_data_h handle, which is usually created by ml_tensors_data_create ().");
  if (raw_data == NULL)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, raw_data, is NULL. It should be a valid, non-NULL, void * pointer, which is supposed to point to the raw data of tensors[index: %u].",
        index);

  _data = (ml_tensors_data_s *) data;
  G_LOCK_UNLESS_NOLOCK (*_data);

  if (_data->num_tensors <= index) {
    _ml_error_report
        ("The parameter, index, is out of bound. The number of tensors of 'data' is %u, while you've requested index of %u.",
        _data->num_tensors, index);
    status = ML_ERROR_INVALID_PARAMETER;
    goto report;
  }

  if (data_size <= 0 || _data->tensors[index].size < data_size) {
    _ml_error_report
        ("The parameter, data_size (%zu), is invalid. It should be larger than 0 and not larger than the required size of tensors[index: %u] (%zu).",
        data_size, index, _data->tensors[index].size);
    status = ML_ERROR_INVALID_PARAMETER;
    goto report;
  }

  if (_data->tensors[index].data != raw_data)
    memcpy (_data->tensors[index].data, raw_data, data_size);

report:
  G_UNLOCK_UNLESS_NOLOCK (*_data);
  return status;
}

/**
 * @brief Copies tensor meta info.
 */
int
ml_tensors_info_clone (ml_tensors_info_h dest, const ml_tensors_info_h src)
{
  ml_tensors_info_s *dest_info, *src_info;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE);

  dest_info = (ml_tensors_info_s *) dest;
  src_info = (ml_tensors_info_s *) src;

  if (!dest_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, dest, is NULL. It should be an allocated handle (ml_tensors_info_h), usually allocated by ml_tensors_info_create ().");
  if (!src_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, src, is NULL. It should be a handle (ml_tensors_info_h) with valid data.");

  G_LOCK_UNLESS_NOLOCK (*dest_info);
  G_LOCK_UNLESS_NOLOCK (*src_info);

  if (gst_tensors_info_validate (&src_info->info)) {
    dest_info->is_extended = src_info->is_extended;
    gst_tensors_info_copy (&dest_info->info, &src_info->info);
  } else {
    _ml_error_report
        ("The parameter, src, is a ml_tensors_info_h handle without valid data. Every tensor-info of tensors-info should have a valid type and dimension information and the number of tensors should be between 1 and %d.",
        ML_TENSOR_SIZE_LIMIT);
    status = ML_ERROR_INVALID_PARAMETER;
  }

  G_UNLOCK_UNLESS_NOLOCK (*src_info);
  G_UNLOCK_UNLESS_NOLOCK (*dest_info);

  return status;
}

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
gchar *
_ml_replace_string (gchar * source, const gchar * what, const gchar * to,
    const gchar * delimiters, guint * count)
{
  GString *builder;
  gchar *start, *pos, *result;
  guint changed = 0;
  gsize len;

  g_return_val_if_fail (source, NULL);
  g_return_val_if_fail (what && to, source);

  len = strlen (what);
  start = source;

  builder = g_string_new (NULL);
  while ((pos = g_strstr_len (start, -1, what)) != NULL) {
    gboolean skip = FALSE;

    if (delimiters) {
      const gchar *s;
      gchar *prev, *next;
      gboolean prev_split, next_split;

      prev = next = NULL;
      prev_split = next_split = FALSE;

      if (pos != source)
        prev = pos - 1;
      if (*(pos + len) != '\0')
        next = pos + len;

      for (s = delimiters; *s != '\0'; ++s) {
        if (!prev || *s == *prev)
          prev_split = TRUE;
        if (!next || *s == *next)
          next_split = TRUE;
        if (prev_split && next_split)
          break;
      }

      if (!prev_split || !next_split)
        skip = TRUE;
    }

    builder = g_string_append_len (builder, start, pos - start);

    /* replace string if found */
    if (skip)
      builder = g_string_append_len (builder, pos, len);
    else
      builder = g_string_append (builder, to);

    start = pos + len;
    if (!skip)
      changed++;
  }

  /* append remains */
  builder = g_string_append (builder, start);
  result = g_string_free (builder, FALSE);

  if (count)
    *count = changed;

  g_free (source);
  return result;
}

/**
 * @brief error reporting infra
 */
#define _ML_ERRORMSG_LENGTH (4096U)
static char errormsg[_ML_ERRORMSG_LENGTH] = { 0 };      /* one page limit */

static int reported = 0;
G_LOCK_DEFINE_STATIC (errlock);

/**
 * @brief public API function of error reporting.
 */
const char *
ml_error (void)
{
  G_LOCK (errlock);
  if (reported != 0) {
    errormsg[0] = '\0';
    reported = 0;
  }
  if (errormsg[0] == '\0') {
    G_UNLOCK (errlock);
    return NULL;
  }

  reported = 1;

  G_UNLOCK (errlock);
  return errormsg;
}

/**
 * @brief Internal interface to write messages for ml_error()
 */
void
_ml_error_report_ (const char *fmt, ...)
{
  int n;
  va_list arg_ptr;
  G_LOCK (errlock);

  va_start (arg_ptr, fmt);
  n = vsnprintf (errormsg, _ML_ERRORMSG_LENGTH, fmt, arg_ptr);
  va_end (arg_ptr);

  if (n > _ML_ERRORMSG_LENGTH) {
    errormsg[_ML_ERRORMSG_LENGTH - 2] = '.';
    errormsg[_ML_ERRORMSG_LENGTH - 3] = '.';
    errormsg[_ML_ERRORMSG_LENGTH - 4] = '.';
  }

  _ml_loge ("%s", errormsg);

  reported = 0;

  G_UNLOCK (errlock);
}

/**
 * @brief Internal interface to write messages for ml_error(), relaying previously reported errors.
 */
void
_ml_error_report_continue_ (const char *fmt, ...)
{
  size_t cursor = 0;
  va_list arg_ptr;
  char buf[_ML_ERRORMSG_LENGTH];
  G_LOCK (errlock);

  /* Check if there is a message to relay */
  if (reported == 0) {
    cursor = strlen (errormsg);
    if (cursor < (_ML_ERRORMSG_LENGTH - 1)) {
      errormsg[cursor] = '\n';
      errormsg[cursor + 1] = '\0';
      cursor++;
    }
  } else {
    errormsg[0] = '\0';
  }

  va_start (arg_ptr, fmt);
  vsnprintf (buf, _ML_ERRORMSG_LENGTH - 1, fmt, arg_ptr);
  _ml_loge ("%s", buf);

  memcpy (errormsg + cursor, buf, _ML_ERRORMSG_LENGTH - strlen (errormsg) - 1);
  if (strlen (errormsg) >= (_ML_ERRORMSG_LENGTH - 2)) {
    errormsg[_ML_ERRORMSG_LENGTH - 2] = '.';
    errormsg[_ML_ERRORMSG_LENGTH - 3] = '.';
    errormsg[_ML_ERRORMSG_LENGTH - 4] = '.';
  }

  va_end (arg_ptr);

  errormsg[_ML_ERRORMSG_LENGTH - 1] = '\0';
  reported = 0;
  G_UNLOCK (errlock);
}

static const char *strerrors[] = {
  [0] = "Not an error",
  [EINVAL] =
      "Invalid parameters are given to a function. Check parameter values. (EINVAL)",
};

/**
 * @brief public API function of error code descriptions
 */
const char *
ml_strerror (int errnum)
{
  int size = sizeof (strerrors) / sizeof (strerrors[0]);

  if (errnum < 0)
    errnum = errnum * -1;

  if (errnum == 0 || errnum >= size)
    return NULL;
  return strerrors[errnum];
}

/**
 * @brief Internal function to check the handle is valid.
 */
static bool
_ml_info_is_valid (gpointer handle, ml_info_type_e expected)
{
  ml_info_type_e current;

  if (!handle)
    return false;

  /* The first field should be an enum value of ml_info_type_e. */
  current = *((ml_info_type_e *) handle);
  if (current != expected)
    return false;

  switch (current) {
    case ML_INFO_TYPE_OPTION:
    case ML_INFO_TYPE_INFORMATION:
    {
      ml_info_s *_info = (ml_info_s *) handle;

      if (!_info->table)
        return false;

      break;
    }
    case ML_INFO_TYPE_INFORMATION_LIST:
    {
      ml_info_list_s *_list = (ml_info_list_s *) handle;

      if (_list->length == 0 || !_list->info)
        return false;

      break;
    }
    default:
      /* Unknown type */
      return false;
  }

  return true;
}

/**
 * @brief Internal function for destroy value of option table
 */
static void
_ml_info_value_free (gpointer data)
{
  ml_info_value_s *_value;

  _value = (ml_info_value_s *) data;
  if (_value) {
    if (_value->destroy)
      _value->destroy (_value->value);
    g_free (_value);
  }
}

/**
 * @brief Internal function for create ml_info
 */
static ml_info_s *
_ml_info_create (ml_info_type_e type)
{
  ml_info_s *info;

  info = g_new0 (ml_info_s, 1);
  if (info == NULL) {
    _ml_error_report
        ("Failed to allocate memory for the ml_info. Out of memory?");
    return NULL;
  }

  info->type = type;
  info->table =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
      _ml_info_value_free);
  if (info->table == NULL) {
    _ml_error_report
        ("Failed to allocate memory for the table of ml_info. Out of memory?");
    g_free (info);
    return NULL;
  }

  return info;
}

/**
 * @brief Internal function for destroy ml_info
 */
static void
_ml_info_destroy (ml_info_s * info)
{
  if (!info)
    return;

  info->type = ML_INFO_TYPE_UNKNOWN;

  if (info->table) {
    g_hash_table_destroy (info->table);
    info->table = NULL;
  }

  g_free (info);
  return;
}

/**
 * @brief Internal function for set value of given ml_info
 */
static int
_ml_info_set_value (ml_info_s * info, const char *key, void *value,
    ml_data_destroy_cb destroy)
{
  ml_info_value_s *info_value;

  if (!info || !key || !value)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'info', 'key' or 'value' is NULL. It should be a valid ml_info, key and value.");

  info_value = g_new0 (ml_info_value_s, 1);
  if (!info_value)
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the info value. Out of memory?");

  info_value->value = value;
  info_value->destroy = destroy;
  g_hash_table_insert (info->table, g_strdup (key), (gpointer) info_value);

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function for get value of given ml_info
 */
static int
_ml_info_get_value (ml_info_s * info, const char *key, void **value)
{
  ml_info_value_s *info_value;

  if (!info || !key || !value)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'info', 'key' or 'value' is NULL. It should be a valid ml_info, key and value.");

  info_value = g_hash_table_lookup (info->table, key);
  if (!info_value)
    return ML_ERROR_INVALID_PARAMETER;

  *value = info_value->value;

  return ML_ERROR_NONE;
}

/**
 * @brief Creates an option and returns the instance a handle.
 */
int
ml_option_create (ml_option_h * option)
{
  ml_info_s *_option = NULL;

  check_feature_state (ML_FEATURE);

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h");
  }

  _option = _ml_info_create (ML_INFO_TYPE_OPTION);
  if (_option == NULL)
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the option handle. Out of memory?");

  *option = _option;
  return ML_ERROR_NONE;
}

/**
 * @brief Frees the given handle of a ml_option.
 */
int
ml_option_destroy (ml_option_h option)
{
  check_feature_state (ML_FEATURE);

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }

  if (!_ml_info_is_valid (option, ML_INFO_TYPE_OPTION))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is not a ml-option handle.");

  _ml_info_destroy ((ml_info_s *) option);

  return ML_ERROR_NONE;
}

/**
 * @brief Set key-value pair in given option handle. Note that if duplicated key is given, the value is updated with the new one.
 * If some options are changed or there are newly added options, please modify below description.
 * The list of valid key-values are:
 *
 * key (char *)     || value (expected type (pointer))
 * ---------------------------------------------------------
 * "framework_name" ||  explicit name of framework (char *)
 * ...
 */
int
ml_option_set (ml_option_h option, const char *key, void *value,
    ml_data_destroy_cb destroy)
{
  check_feature_state (ML_FEATURE);

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }

  if (!key) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'key' is NULL. It should be a valid const char*");
  }

  if (!value) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'value' is NULL. It should be a valid void*");
  }

  if (!_ml_info_is_valid (option, ML_INFO_TYPE_OPTION))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is not a ml-option handle.");

  return _ml_info_set_value ((ml_info_s *) option, key, value, destroy);
}

/**
 * @brief Gets a value of key in ml_option instance.
 */
int
ml_option_get (ml_option_h option, const char *key, void **value)
{
  check_feature_state (ML_FEATURE);

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }

  if (!key) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'key' is NULL. It should be a valid const char*");
  }

  if (!value) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'value' is NULL. It should be a valid void**");
  }

  if (!_ml_info_is_valid (option, ML_INFO_TYPE_OPTION))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is not a ml-option handle.");

  return _ml_info_get_value ((ml_info_s *) option, key, value);
}

/**
 * @brief Creates an ml_information instance and returns the handle.
 */
int
_ml_information_create (ml_information_h * info)
{
  ml_info_s *_info = NULL;

  check_feature_state (ML_FEATURE);

  if (!info) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'info' is NULL. It should be a valid ml_information_h");
  }

  _info = _ml_info_create (ML_INFO_TYPE_INFORMATION);
  if (!_info)
    _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
        "Failed to allocate memory for the info handle. Out of memory?");

  *info = _info;
  return ML_ERROR_NONE;
}

/**
 * @brief Set key-value pair in given information handle.
 * @note If duplicated key is given, the value is updated with the new one.
 */
int
_ml_information_set (ml_information_h information, const char *key, void *value,
    ml_data_destroy_cb destroy)
{
  check_feature_state (ML_FEATURE);

  if (!information) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'information' is NULL. It should be a valid ml_information_h, which should be created by ml_information_create().");
  }

  if (!key) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'key' is NULL. It should be a valid const char*");
  }

  if (!value) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'value' is NULL. It should be a valid void*");
  }

  if (!_ml_info_is_valid (information, ML_INFO_TYPE_INFORMATION))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'information' is not a ml-information handle.");

  return _ml_info_set_value ((ml_info_s *) information, key, value, destroy);
}

/**
 * @brief Frees the given handle of a ml_information.
 */
int
ml_information_destroy (ml_information_h information)
{
  check_feature_state (ML_FEATURE);

  if (!information) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'information' is NULL. It should be a valid ml_information_h, which should be created by ml_information_create().");
  }

  if (!_ml_info_is_valid (information, ML_INFO_TYPE_INFORMATION))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'information' is not a ml-information handle.");

  _ml_info_destroy ((ml_info_s *) information);

  return ML_ERROR_NONE;
}

/**
 * @brief Gets the value corresponding to the given key in ml_information instance.
 */
int
ml_information_get (ml_information_h information, const char *key, void **value)
{
  check_feature_state (ML_FEATURE);

  if (!information) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'information' is NULL. It should be a valid ml_information_h, which should be created by ml_information_create().");
  }

  if (!key) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'key' is NULL. It should be a valid const char*");
  }

  if (!value) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'value' is NULL. It should be a valid void**");
  }

  if (!_ml_info_is_valid (information, ML_INFO_TYPE_INFORMATION))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'information' is not a ml-information handle.");

  return _ml_info_get_value ((ml_info_s *) information, key, value);
}

/**
 * @brief Internal function for destroy ml_info_list_s instance.
 */
static void
_ml_info_list_destroy (ml_info_list_s * list)
{
  guint i;

  if (!list)
    return;

  list->type = ML_INFO_TYPE_UNKNOWN;

  for (i = 0; i < list->length; ++i) {
    _ml_info_destroy (list->info[i]);
  }

  g_free (list->info);
  g_free (list);
}

/**
 * @brief Internal function for getting length of ml_info_list_s instance.
 */
static unsigned int
_ml_info_list_length (ml_info_list_s * list)
{
  return list ? list->length : 0;
}

/**
 * @brief Internal function for getting index-th ml_information_h instance in ml_info_list_s instance.
 */
static ml_info_s *
_ml_info_list_get (ml_info_list_s * list, unsigned int index)
{
  if (!list || index >= list->length)
    return NULL;

  return list->info[index];
}

/**
 * @brief Creates an ml-information-list instance and returns the handle.
 */
int
_ml_information_list_create (const unsigned int length,
    ml_information_list_h * list)
{
  ml_info_list_s *_info_list;
  guint i;

  check_feature_state (ML_FEATURE);

  if (length == 0U)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The length of information list should be a positive value.");

  if (!list)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'list' is NULL. It should be a valid ml_information_list_h.");

  _info_list = g_try_new0 (ml_info_list_s, 1);
  if (!_info_list)
    goto error;

  _info_list->info = g_try_new0 (ml_info_s *, length);
  if (!_info_list->info)
    goto error;

  _info_list->type = ML_INFO_TYPE_INFORMATION_LIST;
  _info_list->length = length;

  for (i = 0; i < length; i++) {
    _info_list->info[i] = _ml_info_create (ML_INFO_TYPE_INFORMATION);
    if (_info_list->info[i] == NULL)
      goto error;
  }

  *list = _info_list;
  return ML_ERROR_NONE;

error:
  _ml_info_list_destroy (_info_list);
  _ml_error_report_return (ML_ERROR_OUT_OF_MEMORY,
      "Failed to allocate memory for ml_information_list_h. Out of memory?");
}

/**
 * @brief Destroys the ml-information-list instance.
 */
int
ml_information_list_destroy (ml_information_list_h list)
{
  check_feature_state (ML_FEATURE);

  if (!list) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'list' is NULL. It should be a valid ml_information_list_h, which should be created by ml_information_list_create().");
  }

  if (!_ml_info_is_valid (list, ML_INFO_TYPE_INFORMATION_LIST))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'list' is not a ml-information-list handle.");

  _ml_info_list_destroy ((ml_info_list_s *) list);

  return ML_ERROR_NONE;
}

/**
 * @brief Gets the number of ml-information in ml-information-list instance.
 */
int
ml_information_list_length (ml_information_list_h list, unsigned int *length)
{
  check_feature_state (ML_FEATURE);

  if (!list) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'list' is NULL. It should be a valid ml_information_list_h, which should be created by ml_information_list_create().");
  }

  if (!length) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'length' is NULL. It should be a valid unsigned int*");
  }

  if (!_ml_info_is_valid (list, ML_INFO_TYPE_INFORMATION_LIST))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'list' is not a ml-information-list handle.");

  *length = _ml_info_list_length ((ml_info_list_s *) list);

  return ML_ERROR_NONE;
}

/**
 * @brief Gets a ml-information in ml-information-list instance with given index.
 */
int
ml_information_list_get (ml_information_list_h list, unsigned int index,
    ml_information_h * information)
{
  ml_info_s *info_s;
  check_feature_state (ML_FEATURE);

  if (!list) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'list' is NULL. It should be a valid ml_information_list_h, which should be created by ml_information_list_create().");
  }

  if (!information) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'information' is NULL. It should be a valid ml_information_h*");
  }

  if (!_ml_info_is_valid (list, ML_INFO_TYPE_INFORMATION_LIST))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'list' is not a ml-information-list handle.");

  info_s = _ml_info_list_get ((ml_info_list_s *) list, index);
  if (info_s == NULL) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'index' is invalid. It should be less than the length of ml_information_list_h");
  }

  *information = (ml_information_h) info_s;

  return ML_ERROR_NONE;
}
