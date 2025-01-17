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

#include "nnstreamer-native.h"

#if defined(__ANDROID__)
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
extern void init_bb (void);
extern void init_il (void);
extern void init_pose (void);
extern void init_is (void);
#if defined(ENABLE_FLATBUF)
extern void init_fbd (void);
extern void init_fbc (void);
extern void init_flxc (void);
extern void init_flxd (void);
#endif /* ENABLE_FLATBUF */
#endif

extern void init_filter_cpp (void);
extern void init_filter_custom (void);
extern void init_filter_custom_easy (void);

#if defined(ENABLE_TENSORFLOW_LITE)
extern void init_filter_tflite (void);
#endif
#if defined(ENABLE_SNAP)
extern void init_filter_snap (void);
#endif
#if defined(ENABLE_NNFW_RUNTIME)
extern void init_filter_nnfw (void);
#endif
#if defined(ENABLE_SNPE)
extern void init_filter_snpe (void);
#endif
#if defined(ENABLE_QNN)
extern void init_filter_qnn (void);
#endif
#if defined(ENABLE_PYTORCH)
extern void init_filter_torch (void);
#endif
#if defined(ENABLE_MXNET)
extern void init_filter_mxnet (void);
#endif
#if defined(ENABLE_LLAMA2C)
extern void init_filter_llama2c (void);
#endif
#if defined(ENABLE_LLAMACPP)
extern void init_filter_llamacpp (void);
#endif

#if !GST_CHECK_VERSION(1, 24, 0)
/**
 * @brief External function from GStreamer Android.
 */
extern void gst_android_init (JNIEnv * env, jobject context);
#endif

#if defined(ENABLE_QNN) || defined(ENABLE_SNPE) || defined(ENABLE_TFLITE_QNN_DELEGATE)
#define ANDROID_QC_ENV 1
#endif
#endif /* __ANDROID__ */

/**
 * @brief Global lock for native functions.
 */
G_LOCK_DEFINE_STATIC (nns_native_lock);

#if defined(ANDROID_QC_ENV)
/**
 * @brief Set additional environment (ADSP_LIBRARY_PATH) for Qualcomm Android.
 */
static gboolean
_qc_android_set_env (JNIEnv *env, jobject context)
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
  if ((*env)->ExceptionCheck (env)) {
    (*env)->ExceptionDescribe (env);
    (*env)->ExceptionClear (env);
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
  if ((*env)->ExceptionCheck (env)) {
    (*env)->ExceptionDescribe (env);
    (*env)->ExceptionClear (env);
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
 * @brief Initialize NNStreamer, register required plugins.
 */
jboolean
nnstreamer_native_initialize (JNIEnv * env, jobject context)
{
  gchar *gst_ver, *nns_ver;
  static gboolean nns_is_initilaized = FALSE;

  _ml_logi ("Called native initialize.");

  G_LOCK (nns_native_lock);

#if !defined(NNS_SINGLE_ONLY)
  /* single-shot does not require gstreamer */
  if (!gst_is_initialized ()) {
#if defined(__ANDROID__) && !GST_CHECK_VERSION(1, 24, 0)
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
#if !defined(NNS_SINGLE_ONLY)
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

#if defined(ANDROID_QC_ENV)
    /* some filters require additional tasks */
    if (!_qc_android_set_env (env, context)) {
      _ml_logw ("Failed to set environment variables for QC Android. Some features may not work properly.");
    }
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
#endif /* __ANDROID__ */
    nns_is_initilaized = TRUE;
  }

  /* print version info */
  gst_ver = gst_version_string ();
  nns_ver = nnstreamer_version_string ();

  _ml_logi ("%s %s GLib %d.%d.%d", nns_ver, gst_ver, GLIB_MAJOR_VERSION,
      GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

  g_free (gst_ver);
  g_free (nns_ver);

done:
  G_UNLOCK (nns_native_lock);
  return (nns_is_initilaized == TRUE);
}
