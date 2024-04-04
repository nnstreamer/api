/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer Android API
 * Copyright (C) 2019 Samsung Electronics Co., Ltd.
 *
 * @file	nnstreamer-native-api.c
 * @date	10 July 2019
 * @brief	Native code for NNStreamer API
 * @author	Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#include "nnstreamer-native.h"

#if defined(__ANDROID__)
/* nnstreamer plugins and sub-plugins declaration */
#if !defined (NNS_SINGLE_ONLY)
GST_PLUGIN_STATIC_DECLARE (nnstreamer);
GST_PLUGIN_STATIC_DECLARE (amcsrc);
GST_PLUGIN_STATIC_DECLARE (join);
#if defined (ENABLE_MQTT)
GST_PLUGIN_STATIC_DECLARE (mqtt);
#endif
extern void init_dv (void);
extern void init_bb (void);
extern void init_il (void);
extern void init_pose (void);
extern void init_is (void);
#if defined (ENABLE_FLATBUF)
extern void init_fbd (void);
extern void init_fbc (void);
extern void init_flxc (void);
extern void init_flxd (void);
#endif /* ENABLE_FLATBUF */
#endif

extern void init_filter_cpp (void);
extern void init_filter_custom (void);
extern void init_filter_custom_easy (void);

#if defined (ENABLE_TENSORFLOW_LITE)
extern void init_filter_tflite (void);
#endif
#if defined (ENABLE_SNAP)
extern void init_filter_snap (void);
#endif
#if defined (ENABLE_NNFW_RUNTIME)
extern void init_filter_nnfw (void);
#endif
#if defined (ENABLE_SNPE)
extern void init_filter_snpe (JNIEnv * env, jobject context);
#endif
#if defined (ENABLE_PYTORCH)
extern void init_filter_torch (void);
#endif
#if defined (ENABLE_MXNET)
extern void init_filter_mxnet (void);
#endif


#if defined GST_VERSION_MINOR && GST_VERSION_MINOR >= 24
/**
 * @brief External function from GStreamer Android. (Dummy function)
 */
static void gst_android_init (JNIEnv * env, jobject context) {
    do {

    } while(0);
}
#else
/**
 * @brief External function from GStreamer Android.
 */
extern void gst_android_init (JNIEnv * env, jobject context);
#endif /* GST_VERSION_MINOR >= 24 */
#endif /* __ANDROID__ */

/**
 * @brief Global lock for native functions.
 */
G_LOCK_DEFINE_STATIC (nns_native_lock);

/**
 * @brief Attach thread with Java VM.
 */
static JNIEnv *
nns_attach_current_thread (pipeline_info_s * pipe_info)
{
  JNIEnv *env;
  JavaVM *jvm;
  JavaVMAttachArgs args;

  g_assert (pipe_info);
  jvm = pipe_info->jvm;

  args.version = pipe_info->version;
  args.name = NULL;
  args.group = NULL;
#if defined(__ANDROID__)
  if ((*jvm)->AttachCurrentThread (jvm, &env, &args) < 0) {
#else
  if ((*jvm)->AttachCurrentThread (jvm, (void **) &env, &args) < 0) {
#endif
    _ml_loge ("Failed to attach current thread.");
    return NULL;
  }

  return env;
}

/**
 * @brief Get JNI environment.
 */
JNIEnv *
nns_get_jni_env (pipeline_info_s * pipe_info)
{
  JNIEnv *env;

  g_assert (pipe_info);

  if ((env = pthread_getspecific (pipe_info->jni_env)) == NULL) {
    env = nns_attach_current_thread (pipe_info);
    pthread_setspecific (pipe_info->jni_env, env);
  }

  return env;
}

/**
 * @brief Free element handle pointer.
 */
void
nns_free_element_data (gpointer data)
{
  element_data_s *item = (element_data_s *) data;

  if (item) {
    /* release private data */
    if (item->priv_data) {
      JNIEnv *env = nns_get_jni_env (item->pipe_info);
      item->priv_destroy_func (item->priv_data, env);
    }

    switch (item->type) {
#if !defined (NNS_SINGLE_ONLY)
      case NNS_ELEMENT_TYPE_SRC:
        ml_pipeline_src_release_handle ((ml_pipeline_src_h) item->handle);
        break;
      case NNS_ELEMENT_TYPE_SINK:
        ml_pipeline_sink_unregister ((ml_pipeline_sink_h) item->handle);
        break;
      case NNS_ELEMENT_TYPE_VALVE:
        ml_pipeline_valve_release_handle ((ml_pipeline_valve_h) item->handle);
        break;
      case NNS_ELEMENT_TYPE_SWITCH:
        ml_pipeline_switch_release_handle ((ml_pipeline_switch_h) item->handle);
        break;
      case NNS_ELEMENT_TYPE_VIDEO_SINK:
        ml_pipeline_element_release_handle ((ml_pipeline_element_h) item->handle);
        break;
#endif
      default:
        _ml_logw ("Given element type %d is unknown.", item->type);
        if (item->handle)
          g_free (item->handle);
        break;
    }

    g_free (item->name);
    g_free (item);
  }
}

/**
 * @brief Construct TensorsData class info.
 */
static void
nns_construct_tdata_class_info (JNIEnv * env, data_class_info_s * info)
{
  jclass cls;

  cls = (*env)->FindClass (env, NNS_CLS_TDATA);
  info->cls = (*env)->NewGlobalRef (env, cls);
  (*env)->DeleteLocalRef (env, cls);

  info->mid_init = (*env)->GetMethodID (env, info->cls, "<init>",
      "(L" NNS_CLS_TINFO ";)V");
  info->mid_alloc = (*env)->GetStaticMethodID (env, info->cls, "allocate",
      "(L" NNS_CLS_TINFO ";)L" NNS_CLS_TDATA ";");
  info->mid_get_array = (*env)->GetMethodID (env, info->cls, "getDataArray",
      "()[Ljava/lang/Object;");
  info->mid_get_info = (*env)->GetMethodID (env, info->cls, "getTensorsInfo",
      "()L" NNS_CLS_TINFO ";");
}

/**
 * @brief Destroy TensorsData class info.
 */
static void
nns_destroy_tdata_class_info (JNIEnv * env, data_class_info_s * info)
{
  if (info->cls)
    (*env)->DeleteGlobalRef (env, info->cls);
}

/**
 * @brief Construct TensorsInfo class info.
 */
static void
nns_construct_tinfo_class_info (JNIEnv * env, info_class_info_s * info)
{
  jclass cls;

  cls = (*env)->FindClass (env, NNS_CLS_TINFO);
  info->cls = (*env)->NewGlobalRef (env, cls);
  (*env)->DeleteLocalRef (env, cls);

  cls = (*env)->FindClass (env, NNS_CLS_TINFO "$TensorInfo");
  info->cls_info = (*env)->NewGlobalRef (env, cls);
  (*env)->DeleteLocalRef (env, cls);

  info->mid_init = (*env)->GetMethodID (env, info->cls, "<init>", "()V");
  info->mid_add_info = (*env)->GetMethodID (env, info->cls, "appendInfo",
      "(Ljava/lang/String;I[I)V");
  info->mid_get_array = (*env)->GetMethodID (env, info->cls, "getInfoArray",
      "()[Ljava/lang/Object;");

  info->fid_info_name = (*env)->GetFieldID (env, info->cls_info, "name",
      "Ljava/lang/String;");
  info->fid_info_type = (*env)->GetFieldID (env, info->cls_info, "type", "I");
  info->fid_info_dim = (*env)->GetFieldID (env, info->cls_info, "dimension",
      "[I");
}

/**
 * @brief Destroy TensorsInfo class info.
 */
static void
nns_destroy_tinfo_class_info (JNIEnv * env, info_class_info_s * info)
{
  if (info->cls_info)
    (*env)->DeleteGlobalRef (env, info->cls_info);
  if (info->cls)
    (*env)->DeleteGlobalRef (env, info->cls);
}

/**
 * @brief Construct pipeline info.
 */
gpointer
nns_construct_pipe_info (JNIEnv * env, jobject thiz, gpointer handle,
    nns_pipe_type_e type)
{
  pipeline_info_s *pipe_info;
  jclass cls;

  pipe_info = g_new0 (pipeline_info_s, 1);
  g_return_val_if_fail (pipe_info != NULL, NULL);

  pipe_info->pipeline_type = type;
  pipe_info->pipeline_handle = handle;
  pipe_info->element_handles =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
      nns_free_element_data);
  g_mutex_init (&pipe_info->lock);

  (*env)->GetJavaVM (env, &pipe_info->jvm);
  g_assert (pipe_info->jvm);
  pthread_key_create (&pipe_info->jni_env, NULL);

  pipe_info->version = (*env)->GetVersion (env);
  pipe_info->instance = (*env)->NewGlobalRef (env, thiz);

  cls = (*env)->GetObjectClass (env, pipe_info->instance);
  pipe_info->cls = (*env)->NewGlobalRef (env, cls);
  (*env)->DeleteLocalRef (env, cls);

  nns_construct_tdata_class_info (env, &pipe_info->data_cls_info);
  nns_construct_tinfo_class_info (env, &pipe_info->info_cls_info);

  return pipe_info;
}

/**
 * @brief Destroy pipeline info.
 */
void
nns_destroy_pipe_info (pipeline_info_s * pipe_info, JNIEnv * env)
{
  g_return_if_fail (pipe_info != NULL);

  g_mutex_lock (&pipe_info->lock);
  if (pipe_info->priv_data) {
    if (pipe_info->priv_destroy_func)
      pipe_info->priv_destroy_func (pipe_info->priv_data, env);
    else
      g_free (pipe_info->priv_data);

    pipe_info->priv_data = NULL;
  }

  g_hash_table_destroy (pipe_info->element_handles);
  pipe_info->element_handles = NULL;
  g_mutex_unlock (&pipe_info->lock);

  switch (pipe_info->pipeline_type) {
#if !defined (NNS_SINGLE_ONLY)
    case NNS_PIPE_TYPE_PIPELINE:
      ml_pipeline_destroy (pipe_info->pipeline_handle);
      break;
    case NNS_PIPE_TYPE_CUSTOM:
      ml_pipeline_custom_easy_filter_unregister (pipe_info->pipeline_handle);
      break;
#endif
    case NNS_PIPE_TYPE_SINGLE:
      ml_single_close (pipe_info->pipeline_handle);
      break;
    default:
      _ml_logw ("Given pipe type %d is unknown.", pipe_info->pipeline_type);
      if (pipe_info->pipeline_handle)
        g_free (pipe_info->pipeline_handle);
      break;
  }

  g_mutex_clear (&pipe_info->lock);

  nns_destroy_tdata_class_info (env, &pipe_info->data_cls_info);
  nns_destroy_tinfo_class_info (env, &pipe_info->info_cls_info);
  (*env)->DeleteGlobalRef (env, pipe_info->cls);
  (*env)->DeleteGlobalRef (env, pipe_info->instance);

  pthread_key_delete (pipe_info->jni_env);
  g_free (pipe_info);
}

/**
 * @brief Set private data in pipeline info. If destroy_func is NULL, priv_data will be released using g_free().
 */
void
nns_set_priv_data (pipeline_info_s * pipe_info, gpointer data,
    nns_priv_destroy destroy_func)
{
  g_return_if_fail (pipe_info != NULL);

  g_mutex_lock (&pipe_info->lock);
  pipe_info->priv_data = data;
  pipe_info->priv_destroy_func = destroy_func;
  g_mutex_unlock (&pipe_info->lock);
}

/**
 * @brief Get element data of given name.
 */
element_data_s *
nns_get_element_data (pipeline_info_s * pipe_info, const gchar * name)
{
  element_data_s *item;

  g_return_val_if_fail (pipe_info, NULL);
  g_return_val_if_fail (name, NULL);

  g_mutex_lock (&pipe_info->lock);
  item = g_hash_table_lookup (pipe_info->element_handles, name);
  g_mutex_unlock (&pipe_info->lock);

  return item;
}

/**
 * @brief Get element handle of given name and type.
 */
gpointer
nns_get_element_handle (pipeline_info_s * pipe_info, const gchar * name,
    const nns_element_type_e type)
{
  element_data_s *item = nns_get_element_data (pipe_info, name);

  /* check element type */
  return (item && item->type == type) ? item->handle : NULL;
}

/**
 * @brief Remove element data of given name.
 */
gboolean
nns_remove_element_data (pipeline_info_s * pipe_info, const gchar * name)
{
  gboolean ret;

  g_return_val_if_fail (pipe_info, FALSE);
  g_return_val_if_fail (name, FALSE);

  g_mutex_lock (&pipe_info->lock);
  ret = g_hash_table_remove (pipe_info->element_handles, name);
  g_mutex_unlock (&pipe_info->lock);

  return ret;
}

/**
 * @brief Add new element data of given name.
 */
gboolean
nns_add_element_data (pipeline_info_s * pipe_info, const gchar * name,
    element_data_s * item)
{
  gboolean ret;

  g_return_val_if_fail (pipe_info, FALSE);
  g_return_val_if_fail (name && item, FALSE);

  g_mutex_lock (&pipe_info->lock);
  ret = g_hash_table_insert (pipe_info->element_handles, g_strdup (name), item);
  g_mutex_unlock (&pipe_info->lock);

  return ret;
}

/**
 * @brief Create new data object with given tensors info. Caller should unref the result object.
 */
gboolean
nns_create_tensors_data_object (pipeline_info_s * pipe_info, JNIEnv * env,
    jobject obj_info, jobject * result)
{
  data_class_info_s *dcls_info;
  jobject obj_data;

  g_return_val_if_fail (pipe_info, FALSE);
  g_return_val_if_fail (env, FALSE);
  g_return_val_if_fail (result, FALSE);
  g_return_val_if_fail (obj_info, FALSE);

  dcls_info = &pipe_info->data_cls_info;
  *result = NULL;

  obj_data = (*env)->CallStaticObjectMethod (env, dcls_info->cls,
      dcls_info->mid_alloc, obj_info);
  if ((*env)->ExceptionCheck (env) || !obj_data) {
    _ml_loge ("Failed to allocate object for tensors data.");
    (*env)->ExceptionClear (env);

    if (obj_data)
      (*env)->DeleteLocalRef (env, obj_data);

    return FALSE;
  }

  *result = obj_data;
  return TRUE;
}

/**
 * @brief Convert tensors data to TensorsData object.
 */
gboolean
nns_convert_tensors_data (pipeline_info_s * pipe_info, JNIEnv * env,
    ml_tensors_data_h data_h, jobject obj_info, jobject * result)
{
  guint i;
  data_class_info_s *dcls_info;
  jobject obj_data = NULL;
  jobjectArray data_arr;
  ml_tensors_data_s *data;

  g_return_val_if_fail (pipe_info, FALSE);
  g_return_val_if_fail (env, FALSE);
  g_return_val_if_fail (data_h, FALSE);
  g_return_val_if_fail (result, FALSE);
  g_return_val_if_fail (obj_info, FALSE);

  dcls_info = &pipe_info->data_cls_info;
  data = (ml_tensors_data_s *) data_h;
  *result = NULL;

  if (!nns_create_tensors_data_object (pipe_info, env, obj_info, &obj_data))
    return FALSE;

  data_arr = (*env)->CallObjectMethod (env, obj_data, dcls_info->mid_get_array);

  for (i = 0; i < data->num_tensors; i++) {
    jobject tensor = (*env)->GetObjectArrayElement (env, data_arr, i);
    gpointer data_ptr = (*env)->GetDirectBufferAddress (env, tensor);

    memcpy (data_ptr, data->tensors[i].data, data->tensors[i].size);
    (*env)->DeleteLocalRef (env, tensor);
  }

  (*env)->DeleteLocalRef (env, data_arr);

  *result = obj_data;
  return TRUE;
}

/**
 * @brief Parse tensors data from TensorsData object.
 */
gboolean
nns_parse_tensors_data (pipeline_info_s * pipe_info, JNIEnv * env,
    jobject obj_data, gboolean clone, const ml_tensors_info_h info_h,
    ml_tensors_data_h * data_h)
{
  guint i;
  data_class_info_s *dcls_info;
  ml_tensors_data_s *data;
  jobjectArray data_arr;
  gboolean failed = FALSE;
  int status;

  g_return_val_if_fail (pipe_info, FALSE);
  g_return_val_if_fail (env, FALSE);
  g_return_val_if_fail (obj_data, FALSE);
  g_return_val_if_fail (data_h, FALSE);

  dcls_info = &pipe_info->data_cls_info;

  /* if data is not allocated, create new handle. */
  if (*data_h == NULL) {
    ml_tensors_info_h _info = info_h;

    /* parse tensors info in data class */
    if (_info == NULL) {
      jobject obj_info =
          (*env)->CallObjectMethod (env, obj_data, dcls_info->mid_get_info);

      if (obj_info) {
        nns_parse_tensors_info (pipe_info, env, obj_info, &_info);
        (*env)->DeleteLocalRef (env, obj_info);
      }
    }

    status = _ml_tensors_data_create_no_alloc (_info, data_h);
    if (_info != info_h)
      ml_tensors_info_destroy (_info);

    if (status != ML_ERROR_NONE) {
      _ml_loge ("Failed to create handle for tensors data.");
      return FALSE;
    }
  }

  data = (ml_tensors_data_s *) (*data_h);
  data_arr = (*env)->CallObjectMethod (env, obj_data, dcls_info->mid_get_array);

  /* number of tensors data */
  data->num_tensors = (unsigned int) (*env)->GetArrayLength (env, data_arr);

  /* set tensor data */
  for (i = 0; i < data->num_tensors; i++) {
    jobject tensor = (*env)->GetObjectArrayElement (env, data_arr, i);

    if (tensor) {
      gsize data_size = (gsize) (*env)->GetDirectBufferCapacity (env, tensor);
      gpointer data_ptr = (*env)->GetDirectBufferAddress (env, tensor);

      if (clone) {
        if (data->tensors[i].data == NULL)
          data->tensors[i].data = g_malloc (data_size);

        memcpy (data->tensors[i].data, data_ptr, data_size);
      } else {
        data->tensors[i].data = data_ptr;
      }

      data->tensors[i].size = data_size;

      (*env)->DeleteLocalRef (env, tensor);
    } else {
      _ml_loge ("Failed to get array element in tensors data object.");
      failed = TRUE;
      goto done;
    }
  }

done:
  (*env)->DeleteLocalRef (env, data_arr);

  if (failed) {
    ml_tensors_data_destroy (*data_h);
    *data_h = NULL;
  }

  return !failed;
}

/**
 * @brief Convert tensors info to TensorsInfo object.
 */
gboolean
nns_convert_tensors_info (pipeline_info_s * pipe_info, JNIEnv * env,
    ml_tensors_info_h info_h, jobject * result)
{
  guint i;
  info_class_info_s *icls_info;
  ml_tensors_info_s *info;
  GstTensorInfo *_info;
  jobject obj_info = NULL;

  g_return_val_if_fail (pipe_info, FALSE);
  g_return_val_if_fail (env, FALSE);
  g_return_val_if_fail (info_h, FALSE);
  g_return_val_if_fail (result, FALSE);

  icls_info = &pipe_info->info_cls_info;
  info = (ml_tensors_info_s *) info_h;

  obj_info = (*env)->NewObject (env, icls_info->cls, icls_info->mid_init);
  if (!obj_info) {
    _ml_loge ("Failed to allocate object for tensors info.");
    goto done;
  }

  for (i = 0; i < info->info.num_tensors; i++) {
    jstring name = NULL;
    jint type;
    jintArray dimension;

    _info = gst_tensors_info_get_nth_info (&info->info, i);

    if (_info->name)
      name = (*env)->NewStringUTF (env, _info->name);

    type = (jint) _info->type;

    dimension = (*env)->NewIntArray (env, ML_TENSOR_RANK_LIMIT);
    (*env)->SetIntArrayRegion (env, dimension, 0, ML_TENSOR_RANK_LIMIT,
        (jint *) _info->dimension);

    (*env)->CallVoidMethod (env, obj_info, icls_info->mid_add_info,
        name, type, dimension);

    if (name)
      (*env)->DeleteLocalRef (env, name);
    (*env)->DeleteLocalRef (env, dimension);
  }

done:
  *result = obj_info;
  return (obj_info != NULL);
}

/**
 * @brief Parse tensors info from TensorsInfo object.
 */
gboolean
nns_parse_tensors_info (pipeline_info_s * pipe_info, JNIEnv * env,
    jobject obj_info, ml_tensors_info_h * info_h)
{
  guint i;
  info_class_info_s *icls_info;
  ml_tensors_info_s *info;
  GstTensorInfo *_info;
  jobjectArray info_arr;

  g_return_val_if_fail (pipe_info, FALSE);
  g_return_val_if_fail (env, FALSE);
  g_return_val_if_fail (obj_info, FALSE);
  g_return_val_if_fail (info_h, FALSE);

  if (ml_tensors_info_create_extended (info_h) != ML_ERROR_NONE) {
    _ml_loge ("Failed to create handle for tensors info.");
    return FALSE;
  }

  icls_info = &pipe_info->info_cls_info;
  info = (ml_tensors_info_s *) (*info_h);

  info_arr = (*env)->CallObjectMethod (env, obj_info, icls_info->mid_get_array);

  /* number of tensors info */
  info->info.num_tensors = (unsigned int) (*env)->GetArrayLength (env, info_arr);

  /* read tensor info */
  for (i = 0; i < info->info.num_tensors; i++) {
    jobject item;
    jstring name_str;
    jintArray dimension;

    item = (*env)->GetObjectArrayElement (env, info_arr, i);
    _info = gst_tensors_info_get_nth_info (&info->info, i);

    /* tensor name */
    name_str = (jstring) (*env)->GetObjectField (env, item,
        icls_info->fid_info_name);
    if (name_str) {
      const gchar *name = (*env)->GetStringUTFChars (env, name_str, NULL);

      _info->name = g_strdup (name);
      (*env)->ReleaseStringUTFChars (env, name_str, name);
      (*env)->DeleteLocalRef (env, name_str);
    }

    /* tensor type */
    _info->type = (tensor_type) (*env)->GetIntField (env, item,
        icls_info->fid_info_type);

    /* tensor dimension */
    dimension = (jintArray) (*env)->GetObjectField (env, item,
        icls_info->fid_info_dim);
    (*env)->GetIntArrayRegion (env, dimension, 0, ML_TENSOR_RANK_LIMIT,
        (jint *) _info->dimension);
    (*env)->DeleteLocalRef (env, dimension);

    (*env)->DeleteLocalRef (env, item);
  }

  (*env)->DeleteLocalRef (env, info_arr);
  return TRUE;
}

/**
 * @brief Get NNFW from integer value.
 */
gboolean
nns_get_nnfw_type (jint fw_type, ml_nnfw_type_e * nnfw)
{
  if (!nnfw)
    return FALSE;

  *nnfw = ML_NNFW_TYPE_ANY;

  /* enumeration defined in NNStreamer.java */
  switch (fw_type) {
    case 0: /* NNFWType.TENSORFLOW_LITE */
      *nnfw = ML_NNFW_TYPE_TENSORFLOW_LITE;
      break;
    case 1: /* NNFWType.SNAP */
      *nnfw = ML_NNFW_TYPE_SNAP;
      break;
    case 2: /* NNFWType.NNFW */
      *nnfw = ML_NNFW_TYPE_NNFW;
      break;
    case 3: /* NNFWType.SNPE */
      *nnfw = ML_NNFW_TYPE_SNPE;
      break;
    case 4: /* NNFWType.PYTORCH */
      *nnfw = ML_NNFW_TYPE_PYTORCH;
      break;
    case 5: /* NNFWType.MXNET */
      *nnfw = ML_NNFW_TYPE_MXNET;
      break;
    default: /* Unknown */
      _ml_logw ("Unknown NNFW type (%d).", fw_type);
      return FALSE;
  }

  return _ml_nnfw_is_available (*nnfw, ML_NNFW_HW_ANY);
}

/**
 * @brief Initialize NNStreamer, register required plugins.
 */
jboolean
nnstreamer_native_initialize (JNIEnv * env, jobject context)
{
  jboolean result = JNI_FALSE;
  gchar *gst_ver, *nns_ver;
  static gboolean nns_is_initilaized = FALSE;

  _ml_logi ("Called native initialize.");

  G_LOCK (nns_native_lock);

#if !defined (NNS_SINGLE_ONLY)
  /* single-shot does not require gstreamer */
  if (!gst_is_initialized ()) {
#if defined(__ANDROID__)
    if (env && context) {
      gst_android_init (env, context);
    } else {
#else
    if (_ml_initialize_gstreamer () != ML_ERROR_NONE) {
#endif
      _ml_loge ("Invalid params, cannot initialize GStreamer.");
      goto done;
    }
  }

  if (!gst_is_initialized ()) {
    _ml_loge ("GStreamer is not initialized.");
    goto done;
  }
#endif

  if (nns_is_initilaized == FALSE) {
#if defined(__ANDROID__)
    /* register nnstreamer plugins */
#if !defined (NNS_SINGLE_ONLY)
    GST_PLUGIN_STATIC_REGISTER (nnstreamer);

    /* Android MediaCodec */
    GST_PLUGIN_STATIC_REGISTER (amcsrc);

    /* GStreamer join element */
    GST_PLUGIN_STATIC_REGISTER (join);

#if defined (ENABLE_MQTT)
    /* GStreamer MQTT element */
    GST_PLUGIN_STATIC_REGISTER (mqtt);
#endif

    /* tensor-decoder sub-plugins */
    init_dv ();
    init_bb ();
    init_il ();
    init_pose ();
    init_is ();
#if defined (ENABLE_FLATBUF)
    init_fbd ();
    init_fbc ();
    init_flxc ();
    init_flxd ();
#endif /* ENABLE_FLATBUF */
#endif

    /* tensor-filter sub-plugins */
    init_filter_cpp ();
    init_filter_custom ();
    init_filter_custom_easy ();

#if defined (ENABLE_TENSORFLOW_LITE)
    init_filter_tflite ();
#endif
#if defined (ENABLE_SNAP)
    init_filter_snap ();
#endif
#if defined (ENABLE_NNFW_RUNTIME)
    init_filter_nnfw ();
#endif
#if defined (ENABLE_SNPE)
    init_filter_snpe (env, context);
#endif
#if defined (ENABLE_PYTORCH)
    init_filter_torch ();
#endif
#if defined (ENABLE_MXNET)
    init_filter_mxnet ();
#endif
#endif /* __ANDROID__ */
    nns_is_initilaized = TRUE;
  }

  result = JNI_TRUE;

  /* print version info */
  gst_ver = gst_version_string ();
  nns_ver = nnstreamer_version_string ();

  _ml_logi ("%s %s GLib %d.%d.%d", nns_ver, gst_ver, GLIB_MAJOR_VERSION,
      GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

  g_free (gst_ver);
  g_free (nns_ver);

done:
  G_UNLOCK (nns_native_lock);
  return result;
}

/**
 * @brief Native method to initialize NNStreamer.
 */
static jboolean
nns_native_initialize (JNIEnv * env, jclass clazz, jobject context)
{
  return nnstreamer_native_initialize (env, context);
}

/**
 * @brief Native method to check the availability of NNFW.
 */
static jboolean
nns_native_check_nnfw_availability (JNIEnv * env, jclass clazz, jint fw_type)
{
  ml_nnfw_type_e nnfw;

  if (!nns_get_nnfw_type (fw_type, &nnfw)) {
    return JNI_FALSE;
  }

  return JNI_TRUE;
}

/**
 * @brief Native method to get the version string of NNStreamer.
 */
static jstring
nns_native_get_version (JNIEnv * env, jclass clazz)
{
  gchar *nns_ver = nnstreamer_version_string ();
  jstring version = (*env)->NewStringUTF (env, nns_ver);

  g_free (nns_ver);
  return version;
}

/**
 * @brief List of implemented native methods for NNStreamer class.
 */
static JNINativeMethod native_methods_nnstreamer[] = {
  {(char *) "nativeInitialize", (char *) "(Ljava/lang/Object;)Z",
      (void *) nns_native_initialize},
  {(char *) "nativeCheckNNFWAvailability", (char *) "(I)Z",
      (void *) nns_native_check_nnfw_availability},
  {(char *) "nativeGetVersion", (char *) "()Ljava/lang/String;",
      (void *) nns_native_get_version}
};

/**
 * @brief Initialize native library.
 */
jint
JNI_OnLoad (JavaVM * vm, void *reserved)
{
  JNIEnv *env = NULL;
  jclass klass;

  if ((*vm)->GetEnv (vm, (void **) &env, JNI_VERSION_1_4) != JNI_OK) {
    _ml_loge ("On initializing, failed to get JNIEnv.");
    return 0;
  }

  klass = (*env)->FindClass (env, NNS_CLS_NNSTREAMER);
  if (klass) {
    if ((*env)->RegisterNatives (env, klass, native_methods_nnstreamer,
            G_N_ELEMENTS (native_methods_nnstreamer))) {
      _ml_loge ("Failed to register native methods for NNStreamer class.");
      return 0;
    }
  }

  if (!nns_native_single_register_natives (env)) {
    return 0;
  }

#if !defined (NNS_SINGLE_ONLY)
  if (!nns_native_pipe_register_natives (env) ||
      !nns_native_custom_register_natives (env)) {
    return 0;
  }
#endif

  return JNI_VERSION_1_4;
}
