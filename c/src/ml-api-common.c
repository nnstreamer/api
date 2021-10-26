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
#include <glib.h>

#include "nnstreamer.h"
#include "ml-api-internal.h"

/**
 * @brief Allocates a tensors information handle with default value.
 */
int
ml_tensors_info_create (ml_tensors_info_h * info)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state ();

  if (!info)
    return ML_ERROR_INVALID_PARAMETER;

  *info = tensors_info = g_new0 (ml_tensors_info_s, 1);
  if (tensors_info == NULL) {
    _ml_loge ("Failed to allocate the tensors info handle.");
    return ML_ERROR_OUT_OF_MEMORY;
  }
  g_mutex_init (&tensors_info->lock);

  /* init tensors info struct */
  return ml_tensors_info_initialize (tensors_info);
}

/**
 * @brief Frees the given handle of a tensors information.
 */
int
ml_tensors_info_destroy (ml_tensors_info_h info)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state ();

  tensors_info = (ml_tensors_info_s *) info;

  if (!tensors_info)
    return ML_ERROR_INVALID_PARAMETER;

  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  ml_tensors_info_free (tensors_info);
  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  g_mutex_clear (&tensors_info->lock);
  g_free (tensors_info);

  return ML_ERROR_NONE;
}

/**
 * @brief Initializes the tensors information with default value.
 */
int
ml_tensors_info_initialize (ml_tensors_info_s * info)
{
  guint i, j;

  if (!info)
    return ML_ERROR_INVALID_PARAMETER;

  info->num_tensors = 0;

  for (i = 0; i < ML_TENSOR_SIZE_LIMIT; i++) {
    info->info[i].name = NULL;
    info->info[i].type = ML_TENSOR_TYPE_UNKNOWN;

    for (j = 0; j < ML_TENSOR_RANK_LIMIT; j++) {
      info->info[i].dimension[j] = 0;
    }
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Compares the given tensor info.
 */
static gboolean
ml_tensor_info_compare (const ml_tensor_info_s * i1,
    const ml_tensor_info_s * i2)
{
  guint i;

  if (i1 == NULL || i2 == NULL)
    return FALSE;

  if (i1->type != i2->type)
    return FALSE;

  for (i = 0; i < ML_TENSOR_RANK_LIMIT; i++) {
    if (i1->dimension[i] != i2->dimension[i])
      return FALSE;
  }

  return TRUE;
}

/**
 * @brief Validates the given tensor info is valid.
 * @note info should be locked by caller if nolock == 0.
 */
static gboolean
ml_tensor_info_validate (const ml_tensor_info_s * info)
{
  guint i;

  if (!info)
    return FALSE;

  if (info->type < 0 || info->type >= ML_TENSOR_TYPE_UNKNOWN)
    return FALSE;

  for (i = 0; i < ML_TENSOR_RANK_LIMIT; i++) {
    if (info->dimension[i] == 0)
      return FALSE;
  }

  return TRUE;
}

/**
 * @brief Validates the given tensors info is valid without acquiring lock
 * @note This function assumes that lock on ml_tensors_info_h has already been acquired
 */
static int
_ml_tensors_info_validate_nolock (const ml_tensors_info_s * info, bool *valid)
{
  guint i;
  int ret = ML_ERROR_NONE;

  G_VERIFYLOCK_UNLESS_NOLOCK (*info);
  /* init false */
  *valid = false;

  if (info->num_tensors < 1) {
    ret = ML_ERROR_INVALID_PARAMETER;
    goto done;
  }

  for (i = 0; i < info->num_tensors; i++) {
    if (!ml_tensor_info_validate (&info->info[i]))
      goto done;
  }

  *valid = true;

done:
  return ret;
}

/**
 * @brief Validates the given tensors info is valid.
 */
int
ml_tensors_info_validate (const ml_tensors_info_h info, bool *valid)
{
  ml_tensors_info_s *tensors_info;
  int ret = ML_ERROR_NONE;

  check_feature_state ();

  if (!valid)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;

  if (!tensors_info)
    return ML_ERROR_INVALID_PARAMETER;

  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  ret = _ml_tensors_info_validate_nolock (info, valid);

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ret;
}

/**
 * @brief Compares the given tensors information.
 */
int
ml_tensors_info_compare (const ml_tensors_info_h info1,
    const ml_tensors_info_h info2, bool *equal)
{
  ml_tensors_info_s *i1, *i2;
  guint i;

  check_feature_state ();

  if (info1 == NULL || info2 == NULL || equal == NULL)
    return ML_ERROR_INVALID_PARAMETER;

  i1 = (ml_tensors_info_s *) info1;
  G_LOCK_UNLESS_NOLOCK (*i1);
  i2 = (ml_tensors_info_s *) info2;
  G_LOCK_UNLESS_NOLOCK (*i2);

  /* init false */
  *equal = false;

  if (i1->num_tensors != i2->num_tensors)
    goto done;

  for (i = 0; i < i1->num_tensors; i++) {
    if (!ml_tensor_info_compare (&i1->info[i], &i2->info[i]))
      goto done;
  }

  *equal = true;

done:
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

  check_feature_state ();

  if (!info || count > ML_TENSOR_SIZE_LIMIT)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;

  /* This is atomic. No need for locks */
  tensors_info->num_tensors = count;

  return ML_ERROR_NONE;
}

/**
 * @brief Gets the number of tensors with given handle of tensors information.
 */
int
ml_tensors_info_get_count (ml_tensors_info_h info, unsigned int *count)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state ();

  if (!info || !count)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  /* This is atomic. No need for locks */
  *count = tensors_info->num_tensors;

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

  check_feature_state ();

  if (!info)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (tensors_info->info[index].name) {
    g_free (tensors_info->info[index].name);
    tensors_info->info[index].name = NULL;
  }

  if (name)
    tensors_info->info[index].name = g_strdup (name);

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

  check_feature_state ();

  if (!info || !name)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  *name = g_strdup (tensors_info->info[index].name);

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

  check_feature_state ();

  if (!info)
    return ML_ERROR_INVALID_PARAMETER;

  if (type == ML_TENSOR_TYPE_UNKNOWN)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  tensors_info->info[index].type = type;

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

  check_feature_state ();

  if (!info || !type)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  *type = tensors_info->info[index].type;

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
  guint i;

  check_feature_state ();

  if (!info)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  for (i = 0; i < ML_TENSOR_RANK_LIMIT; i++) {
    tensors_info->info[index].dimension[i] = dimension[i];
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
  guint i;

  check_feature_state ();

  if (!info)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  if (tensors_info->num_tensors <= index) {
    G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
    return ML_ERROR_INVALID_PARAMETER;
  }

  for (i = 0; i < ML_TENSOR_RANK_LIMIT; i++) {
    dimension[i] = tensors_info->info[index].dimension[i];
  }

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Gets the byte size of the given tensor info.
 */
size_t
ml_tensor_info_get_size (const ml_tensor_info_s * info)
{
  size_t tensor_size;
  gint i;

  if (!info)
    return 0;

  switch (info->type) {
    case ML_TENSOR_TYPE_INT8:
    case ML_TENSOR_TYPE_UINT8:
      tensor_size = 1;
      break;
    case ML_TENSOR_TYPE_INT16:
    case ML_TENSOR_TYPE_UINT16:
      tensor_size = 2;
      break;
    case ML_TENSOR_TYPE_INT32:
    case ML_TENSOR_TYPE_UINT32:
    case ML_TENSOR_TYPE_FLOAT32:
      tensor_size = 4;
      break;
    case ML_TENSOR_TYPE_FLOAT64:
    case ML_TENSOR_TYPE_INT64:
    case ML_TENSOR_TYPE_UINT64:
      tensor_size = 8;
      break;
    default:
      _ml_loge ("In the given param, tensor type is invalid.");
      return 0;
  }

  for (i = 0; i < ML_TENSOR_RANK_LIMIT; i++) {
    tensor_size *= info->dimension[i];
  }

  return tensor_size;
}

/**
 * @brief Gets the byte size of the given handle of tensors information.
 */
int
ml_tensors_info_get_tensor_size (ml_tensors_info_h info,
    int index, size_t *data_size)
{
  ml_tensors_info_s *tensors_info;

  check_feature_state ();

  if (!info || !data_size)
    return ML_ERROR_INVALID_PARAMETER;

  tensors_info = (ml_tensors_info_s *) info;
  G_LOCK_UNLESS_NOLOCK (*tensors_info);

  /* init 0 */
  *data_size = 0;

  if (index < 0) {
    guint i;

    /* get total byte size */
    for (i = 0; i < tensors_info->num_tensors; i++) {
      *data_size += ml_tensor_info_get_size (&tensors_info->info[i]);
    }
  } else {
    if (tensors_info->num_tensors <= index) {
      G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
      return ML_ERROR_INVALID_PARAMETER;
    }

    *data_size = ml_tensor_info_get_size (&tensors_info->info[index]);
  }

  G_UNLOCK_UNLESS_NOLOCK (*tensors_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Frees and initialize the data in tensors info.
 * @note This does not touch the lock
 */
void
ml_tensors_info_free (ml_tensors_info_s * info)
{
  gint i;

  if (!info)
    return;

  for (i = 0; i < ML_TENSOR_SIZE_LIMIT; i++) {
    if (info->info[i].name) {
      g_free (info->info[i].name);
      info->info[i].name = NULL;
    }
  }

  ml_tensors_info_initialize (info);
}

/**
 * @brief Frees the tensors data handle and its data.
 * @param[in] data The handle of tensors data.
 * @param[in] free_data The flag to free the buffers in handle.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int
ml_tensors_data_destroy_internal (ml_tensors_data_h data, gboolean free_data)
{
  int status = ML_ERROR_NONE;
  ml_tensors_data_s *_data;
  guint i;

  if (!data)
    return ML_ERROR_INVALID_PARAMETER;

  _data = (ml_tensors_data_s *) data;
  G_LOCK_UNLESS_NOLOCK (*_data);

  if (free_data) {
    if (_data->destroy) {
      status = _data->destroy (_data, _data->user_data);
    } else {
      for (i = 0; i < ML_TENSOR_SIZE_LIMIT; i++) {
        if (_data->tensors[i].tensor) {
          g_free (_data->tensors[i].tensor);
          _data->tensors[i].tensor = NULL;
        }
      }
    }
  }

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
  check_feature_state ();
  return ml_tensors_data_destroy_internal (data, TRUE);
}

/**
 * @brief Allocates a tensor data frame with the given tensors info. (more info in nnstreamer.h)
 * @note Memory for data buffer is not allocated.
 */
int
ml_tensors_data_create_no_alloc (const ml_tensors_info_h info,
    ml_tensors_data_h * data)
{
  ml_tensors_data_s *_data;
  ml_tensors_info_s *_info;
  gint i;

  check_feature_state ();

  if (data == NULL)
    return ML_ERROR_INVALID_PARAMETER;

  /* init null */
  *data = NULL;

  _data = g_new0 (ml_tensors_data_s, 1);
  if (!_data) {
    _ml_loge ("Failed to allocate the tensors data handle.");
    return ML_ERROR_OUT_OF_MEMORY;
  }
  g_mutex_init (&_data->lock);

  _info = (ml_tensors_info_s *) info;
  if (_info != NULL) {
    ml_tensors_info_create (&_data->info);
    ml_tensors_info_clone (_data->info, info);

    G_LOCK_UNLESS_NOLOCK (*_info);
    _data->num_tensors = _info->num_tensors;
    for (i = 0; i < _data->num_tensors; i++) {
      _data->tensors[i].size = ml_tensor_info_get_size (&_info->info[i]);
      _data->tensors[i].tensor = NULL;
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
ml_tensors_data_clone_no_alloc (const ml_tensors_data_s * data_src,
    ml_tensors_data_h * data)
{
  int status;
  ml_tensors_data_s *_data;

  check_feature_state ();

  if (data == NULL || data_src == NULL)
    return ML_ERROR_INVALID_PARAMETER;

  status = ml_tensors_data_create_no_alloc (data_src->info,
      (ml_tensors_data_h *) & _data);
  if (status != ML_ERROR_NONE)
    return status;

  G_LOCK_UNLESS_NOLOCK (*_data);

  _data->num_tensors = data_src->num_tensors;
  memcpy (_data->tensors, data_src->tensors,
      sizeof (ml_tensor_data_s) * data_src->num_tensors);

  *data = _data;
  G_UNLOCK_UNLESS_NOLOCK (*_data);
  return ML_ERROR_NONE;
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

  check_feature_state ();

  if (info == NULL || data == NULL)
    return ML_ERROR_INVALID_PARAMETER;

  if (!ml_tensors_info_is_valid (info)) {
    _ml_loge ("Given tensors information is invalid.");
    return ML_ERROR_INVALID_PARAMETER;
  }

  status =
      ml_tensors_data_create_no_alloc (info, (ml_tensors_data_h *) & _data);

  if (status != ML_ERROR_NONE) {
    return status;
  }

  for (i = 0; i < _data->num_tensors; i++) {
    _data->tensors[i].tensor = g_malloc0 (_data->tensors[i].size);
    if (_data->tensors[i].tensor == NULL) {
      status = ML_ERROR_OUT_OF_MEMORY;
      goto failed;
    }
  }

  *data = _data;
  return ML_ERROR_NONE;

failed:
  for (i = 0; i < _data->num_tensors; i++) {
    g_free (_data->tensors[i].tensor);
  }
  g_free (_data);

  _ml_loge ("Failed to allocate the memory block.");
  return status;
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

  check_feature_state ();

  if (!data || !raw_data || !data_size)
    return ML_ERROR_INVALID_PARAMETER;

  _data = (ml_tensors_data_s *) data;
  G_LOCK_UNLESS_NOLOCK (*_data);

  if (_data->num_tensors <= index) {
    status = ML_ERROR_INVALID_PARAMETER;
    goto report;
  }

  *raw_data = _data->tensors[index].tensor;
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

  check_feature_state ();

  if (!data || !raw_data)
    return ML_ERROR_INVALID_PARAMETER;

  _data = (ml_tensors_data_s *) data;
  G_LOCK_UNLESS_NOLOCK (*_data);

  if (_data->num_tensors <= index) {
    status = ML_ERROR_INVALID_PARAMETER;
    goto report;
  }

  if (data_size <= 0 || _data->tensors[index].size < data_size) {
    status = ML_ERROR_INVALID_PARAMETER;
    goto report;
  }

  if (_data->tensors[index].tensor != raw_data)
    memcpy (_data->tensors[index].tensor, raw_data, data_size);

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
  guint i, j;
  bool valid;
  int status = ML_ERROR_NONE;

  check_feature_state ();

  dest_info = (ml_tensors_info_s *) dest;
  src_info = (ml_tensors_info_s *) src;

  if (!dest_info || !src_info)
    return ML_ERROR_INVALID_PARAMETER;

  G_LOCK_UNLESS_NOLOCK (*dest_info);
  G_LOCK_UNLESS_NOLOCK (*src_info);

  status = _ml_tensors_info_validate_nolock (src_info, &valid);
  if (status != ML_ERROR_NONE || !valid)
    goto done;

  ml_tensors_info_initialize (dest_info);

  dest_info->num_tensors = src_info->num_tensors;

  for (i = 0; i < dest_info->num_tensors; i++) {
    dest_info->info[i].name =
        (src_info->info[i].name) ? g_strdup (src_info->info[i].name) : NULL;
    dest_info->info[i].type = src_info->info[i].type;

    for (j = 0; j < ML_TENSOR_RANK_LIMIT; j++)
      dest_info->info[i].dimension[j] = src_info->info[i].dimension[j];
  }

done:
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
ml_replace_string (gchar * source, const gchar * what, const gchar * to,
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
#define _ML_ERRORMSG_LENGTH (4096)
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
_ml_error_report (const char *message)
{
  G_LOCK (errlock);
  strncpy (errormsg, message, _ML_ERRORMSG_LENGTH);
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
  if (errnum < 0 || errnum >= size)
    return NULL;
  return strerrors[errnum];
}
