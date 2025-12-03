/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer Android API
 * Copyright (C) 2019 Samsung Electronics Co., Ltd.
 *
 * @file	nnstreamer-native-common.c
 * @date	10 July 2019
 * @brief	Native code for NNStreamer API
 * @author	Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#include "nnstreamer-native-internal.h"

/**
 * @brief Global lock for native functions.
 */
G_LOCK_DEFINE_STATIC (nns_native_lock);

/**
 * @brief The flag to check initialization of native library.
 */
static gboolean g_nns_is_initialized = FALSE;

/**
 * @brief The data path of an application.
 */
static gchar *g_files_dir = NULL;

#if defined(__ANDROID__)
#if defined(ENABLE_ML_AGENT)
extern void ml_agent_initialize (const char *db_path);
extern void ml_agent_finalize (void);
#endif
/* nnstreamer plugins and sub-plugins declaration */
#if !defined(NNS_SINGLE_ONLY)
GST_PLUGIN_STATIC_DECLARE (nnstreamer);
GST_PLUGIN_STATIC_DECLARE (amcsrc);
GST_PLUGIN_STATIC_DECLARE (join);
#if defined(ENABLE_NNSTREAMER_EDGE)
GST_PLUGIN_STATIC_DECLARE (edge);
#endif
#if defined(ENABLE_MQTT)
GST_PLUGIN_STATIC_DECLARE (mqtt);
#endif
extern void init_dv (void);
extern void fini_dv (void);
extern void init_bb (void);
extern void fini_bb (void);
extern void init_il (void);
extern void fini_il (void);
extern void init_pose (void);
extern void fini_pose (void);
extern void init_is (void);
extern void fini_is (void);
#if defined(ENABLE_FLATBUF)
extern void init_fbd (void);
extern void fini_fbd (void);
extern void init_fbc (void);
extern void fini_fbc (void);
extern void init_flxc (void);
extern void fini_flxc (void);
extern void init_flxd (void);
extern void fini_flxd (void);
#endif /* ENABLE_FLATBUF */
#endif

extern void init_filter_cpp (void);
extern void fini_filter_cpp (void);
extern void init_filter_custom (void);
extern void fini_filter_custom (void);
extern void init_filter_custom_easy (void);
extern void fini_filter_custom_easy (void);

#if defined(ENABLE_TENSORFLOW_LITE)
extern void init_filter_tflite (void);
extern void fini_filter_tflite (void);
#endif
#if defined(ENABLE_SNAP)
extern void init_filter_snap (void);
extern void fini_filter_snap (void);
#endif
#if defined(ENABLE_NNFW_RUNTIME)
extern void init_filter_nnfw (void);
extern void fini_filter_nnfw (void);
#endif
#if defined(ENABLE_SNPE)
extern void init_filter_snpe (void);
extern void fini_filter_snpe (void);
#endif
#if defined(ENABLE_QNN)
extern void init_filter_qnn (void);
extern void fini_filter_qnn (void);
#endif
#if defined(ENABLE_PYTORCH)
extern void init_filter_torch (void);
extern void fini_filter_torch (void);
#endif
#if defined(ENABLE_MXNET)
extern void init_filter_mxnet (void);
extern void fini_filter_mxnet (void);
#endif
#if defined(ENABLE_LLAMA2C)
extern void init_filter_llama2c (void);
extern void fini_filter_llama2c (void);
#endif
#if defined(ENABLE_LLAMACPP)
extern void init_filter_llamacpp (void);
extern void fini_filter_llamacpp (void);
#endif

#if defined(ENABLE_QNN) || defined(ENABLE_SNPE) || defined(ENABLE_TFLITE_QNN_DELEGATE)
#define ANDROID_QC_ENV 1
#endif

/**
 * @brief Internal function to check exception.
 */
static gboolean
_env_check_exception (JNIEnv * env) {
  if ((*env)->ExceptionCheck (env)) {
    (*env)->ExceptionDescribe (env);
    (*env)->ExceptionClear (env);
    return TRUE;
  }

  return FALSE;
}

#if defined(ANDROID_QC_ENV)
/**
 * @brief Set additional environment (ADSP_LIBRARY_PATH) for Qualcomm Android.
 */
static gboolean
_qc_android_set_env (JNIEnv * env, jobject context)
{
  gboolean is_done = FALSE;
  jclass context_class = NULL;
  jmethodID get_application_info_method_id = NULL;
  jobject application_info_object = NULL;
  jclass application_info_object_class = NULL;
  jfieldID native_library_dir_field_id = NULL;
  jstring native_library_dir_path = NULL;

  const gchar *native_library_dir_path_str;
  gchar *new_path;

  g_return_val_if_fail (env != NULL, FALSE);
  g_return_val_if_fail (context != NULL, FALSE);

  context_class = (*env)->GetObjectClass (env, context);
  if (!context_class) {
    _ml_loge ("Failed to get context class.");
    goto done;
  }

  get_application_info_method_id = (*env)->GetMethodID (env, context_class,
      "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
  if (!get_application_info_method_id) {
    _ml_loge ("Failed to get method ID for `ApplicationInfo()`.");
    goto done;
  }

  application_info_object = (*env)->CallObjectMethod (env, context, get_application_info_method_id);
  if (_env_check_exception (env)) {
    _ml_loge ("Failed to call method `ApplicationInfo()`.");
    goto done;
  }

  application_info_object_class = (*env)->GetObjectClass (env, application_info_object);
  if (!application_info_object_class) {
    _ml_loge ("Failed to get `ApplicationInfo` object class");
    goto done;
  }

  native_library_dir_field_id = (*env)->GetFieldID (env,
      application_info_object_class, "nativeLibraryDir", "Ljava/lang/String;");
  if (!native_library_dir_field_id) {
    _ml_loge ("Failed to get field ID for `nativeLibraryDir`.");
    goto done;
  }

  native_library_dir_path = (jstring) (
      (*env)->GetObjectField (env, application_info_object, native_library_dir_field_id));
  if (!native_library_dir_path) {
    _ml_loge ("Failed to get field `nativeLibraryDir`.");
    goto done;
  }

  native_library_dir_path_str = (*env)->GetStringUTFChars (env, native_library_dir_path, NULL);
  if (_env_check_exception (env)) {
    _ml_loge ("Failed to get string `nativeLibraryDir`");
    goto done;
  }

  new_path = g_strconcat (native_library_dir_path_str,
      ";/vendor/dsp/cdsp;/vendor/lib/rfsa/adsp;/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp", NULL);

  /**
   * See https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-2/dsp_runtime.html for details
   */
  _ml_logi ("Set env ADSP_LIBRARY_PATH for Qualcomm SoC: %s", new_path);
  g_setenv ("ADSP_LIBRARY_PATH", new_path, TRUE);

  g_free (new_path);

  (*env)->ReleaseStringUTFChars (env, native_library_dir_path, native_library_dir_path_str);

  is_done = TRUE;

done:

  if (native_library_dir_path) {
    (*env)->DeleteLocalRef (env, native_library_dir_path);
  }

  if (application_info_object_class) {
    (*env)->DeleteLocalRef (env, application_info_object_class);
  }

  if (application_info_object) {
    (*env)->DeleteLocalRef (env, application_info_object);
  }

  if (context_class) {
    (*env)->DeleteLocalRef (env, context_class);
  }

  return is_done;
}
#endif /* ANDROID_QC_ENV */

/**
 * @brief Get additional data from application context.
 */
static gboolean
_load_app_context (JNIEnv * env, jobject context)
{
  jclass cls_context = NULL;
  jclass cls_file = NULL;
  jmethodID mid_get_files_dir = NULL;
  jmethodID mid_get_absolute_path = NULL;
  jobject dir;

  g_return_val_if_fail (env, FALSE);
  g_return_val_if_fail (context, FALSE);

  /* Clear old value. */
  g_clear_pointer (&g_files_dir, g_free);

  cls_context = (*env)->GetObjectClass (env, context);
  if (!cls_context) {
    goto error;
  }

  mid_get_files_dir = (*env)->GetMethodID (env, cls_context, "getFilesDir", "()Ljava/io/File;");
  if (!mid_get_files_dir) {
    goto error;
  }

  cls_file = (*env)->FindClass (env, "java/io/File");
  if (!cls_file) {
    goto error;
  }

  mid_get_absolute_path = (*env)->GetMethodID (env, cls_file, "getAbsolutePath", "()Ljava/lang/String;");
  if (!mid_get_absolute_path) {
    goto error;
  }

  /* Get files directory. */
  dir = (*env)->CallObjectMethod (env, context, mid_get_files_dir);
  if (_env_check_exception (env)) {
    goto error;
  }

  if (dir) {
    jstring abs_path;
    const gchar *abs_path_str;

    abs_path = (*env)->CallObjectMethod (env, dir, mid_get_absolute_path);
    if (_env_check_exception (env)) {
      (*env)->DeleteLocalRef (env, dir);
      goto error;
    }

    abs_path_str = (*env)->GetStringUTFChars (env, abs_path, NULL);
    if (_env_check_exception (env)) {
      (*env)->DeleteLocalRef (env, abs_path);
      (*env)->DeleteLocalRef (env, dir);
      goto error;
    }

    g_files_dir = g_strdup (abs_path_str);

    (*env)->ReleaseStringUTFChars (env, abs_path, abs_path_str);
    (*env)->DeleteLocalRef (env, abs_path);
    (*env)->DeleteLocalRef (env, dir);
  }

error:
  if (cls_context) {
    (*env)->DeleteLocalRef (env, cls_context);
  }

  if (cls_file) {
    (*env)->DeleteLocalRef (env, cls_file);
  }

  return (g_files_dir != NULL);
}

/**
 * @brief Internal function to register plugins.
 */
static void
_register_plugins (void)
{
#if !defined(NNS_SINGLE_ONLY)
  /* register nnstreamer plugins */
  GST_PLUGIN_STATIC_REGISTER (nnstreamer);

  /* Android MediaCodec */
  GST_PLUGIN_STATIC_REGISTER (amcsrc);

  /* join element of nnstreamer */
  GST_PLUGIN_STATIC_REGISTER (join);

#if defined(ENABLE_NNSTREAMER_EDGE)
  /* edge element of nnstreamer */
  GST_PLUGIN_STATIC_REGISTER (edge);
#endif

#if defined(ENABLE_MQTT)
  /* MQTT element of nnstreamer */
  GST_PLUGIN_STATIC_REGISTER (mqtt);
#endif

  /* tensor-decoder sub-plugins */
  init_dv ();
  init_bb ();
  init_il ();
  init_pose ();
  init_is ();
#if defined(ENABLE_FLATBUF)
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

#if defined(ENABLE_TENSORFLOW_LITE)
  init_filter_tflite ();
#endif
#if defined(ENABLE_SNAP)
  init_filter_snap ();
#endif
#if defined(ENABLE_NNFW_RUNTIME)
  init_filter_nnfw ();
#endif
#if defined(ENABLE_SNPE)
  init_filter_snpe ();
#endif
#if defined(ENABLE_QNN)
  init_filter_qnn ();
#endif
#if defined(ENABLE_PYTORCH)
  init_filter_torch ();
#endif
#if defined(ENABLE_MXNET)
  init_filter_mxnet ();
#endif
#if defined(ENABLE_LLAMA2C)
  init_filter_llama2c ();
#endif
#if defined(ENABLE_LLAMACPP)
  init_filter_llamacpp ();
#endif
}

/**
 * @brief Internal function to unregister plugins.
 */
static void
_unregister_plugins (void)
{
#if !defined(NNS_SINGLE_ONLY)
  /* tensor-decoder sub-plugins */
  fini_dv ();
  fini_bb ();
  fini_il ();
  fini_pose ();
  fini_is ();
#if defined(ENABLE_FLATBUF)
  fini_fbd ();
  fini_fbc ();
  fini_flxc ();
  fini_flxd ();
#endif /* ENABLE_FLATBUF */
#endif

  /* tensor-filter sub-plugins */
  fini_filter_cpp ();
  fini_filter_custom ();
  fini_filter_custom_easy ();

#if defined(ENABLE_TENSORFLOW_LITE)
  fini_filter_tflite ();
#endif
#if defined(ENABLE_SNAP)
  fini_filter_snap ();
#endif
#if defined(ENABLE_NNFW_RUNTIME)
  fini_filter_nnfw ();
#endif
#if defined(ENABLE_SNPE)
  fini_filter_snpe ();
#endif
#if defined(ENABLE_QNN)
  fini_filter_qnn ();
#endif
#if defined(ENABLE_PYTORCH)
  fini_filter_torch ();
#endif
#if defined(ENABLE_MXNET)
  fini_filter_mxnet ();
#endif
#if defined(ENABLE_LLAMA2C)
  fini_filter_llama2c ();
#endif
#if defined(ENABLE_LLAMACPP)
  fini_filter_llamacpp ();
#endif
}
#endif /* __ANDROID__ */

/**
 * @brief Initialize NNStreamer, register required plugins.
 */
jboolean
nnstreamer_native_initialize (JNIEnv * env, jobject context)
{
  _ml_logi ("Called native initialize.");

  G_LOCK (nns_native_lock);

#if !defined(NNS_SINGLE_ONLY)
  /* single-shot does not require gstreamer */
  if (!gst_is_initialized ()) {
    /**
     * @note Initialize GStreamer on Android.
     * We now call gst_init_check() to initialize GStreamer.
     * Consider calling gst_android_init() to set proper environment variables on Android.
     */
    if (_ml_initialize_gstreamer () != ML_ERROR_NONE) {
      _ml_loge ("Invalid params, cannot initialize GStreamer.");
      goto done;
    }
  }

  if (!gst_is_initialized ()) {
    _ml_loge ("GStreamer is not initialized.");
    goto done;
  }
#endif

  if (g_nns_is_initialized == FALSE) {
#if defined(__ANDROID__)
    if (!_load_app_context (env, context)) {
      _ml_loge ("Cannot load application context.");
        goto done;
    }

#if defined(ANDROID_QC_ENV)
    /* some filters require additional tasks */
    if (!_qc_android_set_env (env, context)) {
      _ml_logw ("Failed to set environment variables for QC Android. Some features may not work properly.");
    }
#endif

    _register_plugins ();

#if defined(ENABLE_ML_AGENT)
    {
      gchar *mlops_db_path = g_build_filename (g_files_dir, "mlops-db", NULL);

      g_mkdir (mlops_db_path, 0777);
      ml_agent_initialize (mlops_db_path);
      g_free (mlops_db_path);
    }
#endif
#endif /* __ANDROID__ */

    g_nns_is_initialized = TRUE;
  }

  if (g_nns_is_initialized) {
    /* print version info */
    gchar *gst_ver = gst_version_string ();
    gchar *nns_ver = nnstreamer_version_string ();

    _ml_logi ("%s %s GLib %d.%d.%d", nns_ver, gst_ver, GLIB_MAJOR_VERSION,
        GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

    g_free (gst_ver);
    g_free (nns_ver);
  } else {
    _ml_loge ("Failed to initialize NNStreamer.");
  }

done:
  G_UNLOCK (nns_native_lock);
  return (g_nns_is_initialized == TRUE);
}

/**
 * @brief Release NNStreamer, close internal resources.
 */
void
nnstreamer_native_finalize (void)
{
  _ml_logi ("Called native finalize.");

  G_LOCK (nns_native_lock);

  if (g_nns_is_initialized) {
#if defined(__ANDROID__)
    _unregister_plugins ();

#if defined(ENABLE_ML_AGENT)
    ml_agent_finalize ();
#endif
#endif

    g_clear_pointer (&g_files_dir, g_free);
    g_nns_is_initialized = FALSE;
  }

  G_UNLOCK (nns_native_lock);
}

/**
 * @brief Get the data path of an application, extracted using getFilesDir() for Android.
 */
const char *
nnstreamer_native_get_data_path (void)
{
  const char *data_path = NULL;

  G_LOCK (nns_native_lock);

  if (g_nns_is_initialized) {
    data_path = g_files_dir;
  } else {
    _ml_loge ("NNStreamer native library is not initialized.");    
  }

  G_UNLOCK (nns_native_lock);

  return data_path;
}
