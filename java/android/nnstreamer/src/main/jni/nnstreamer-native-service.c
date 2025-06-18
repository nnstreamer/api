/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer Android API
 * Copyright (C) 2025 Samsung Electronics Co., Ltd.
 *
 * @file   nnstreamer-native-service.c
 * @date   12 May 2025
 * @brief  Native code for NNStreamer API
 * @author Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug    No known bugs except for NYI items
 */

#include "nnstreamer-native-internal.h"

/**
 * @brief Private data for ml-service class.
 */
typedef struct
{
  jmethodID mid_data_cb;
} nns_service_priv_s;

/**
 * @brief Release private data of ml-service.
 */
static void
nns_service_priv_free (gpointer data, JNIEnv * env)
{
  nns_service_priv_s *priv;

  priv = (nns_service_priv_s *) data;

  g_free (priv);
}

/**
 * @brief Get tensors info from node and convert to object.
 */
static jobject
nns_service_convert_node_info (pipeline_info_s * pipe_info, JNIEnv * env,
    const char *name, const gboolean is_input)
{
  ml_service_h service;
  ml_tensors_info_h info = NULL;
  jobject oinfo = NULL;
  int status;

  service = pipe_info->pipeline_handle;

  if (is_input) {
    status = ml_service_get_input_information (service, name, &info);
  } else {
    status = ml_service_get_output_information (service, name, &info);
  }

  if (status == ML_ERROR_NONE) {
    if (!nns_convert_tensors_info (pipe_info, env, info, &oinfo)) {
      _ml_loge ("Failed to convert tensors info.");
    }
  } else {
    _ml_loge ("Failed to get tensors info from ml-service handle.");
  }

  if (info) {
    ml_tensors_info_destroy (info);
  }

  return oinfo;
}

/**
 * @brief Callback function for scenario test.
 */
static void
nns_service_event_cb (ml_service_event_e event, ml_information_h event_data,
    void *user_data)
{
  pipeline_info_s *pipe_info;
  nns_service_priv_s *priv;
  JNIEnv *env;
  int status;

  pipe_info = (pipeline_info_s *) user_data;
  priv = (nns_service_priv_s *) pipe_info->priv_data;

  if ((env = nns_get_jni_env (pipe_info)) == NULL) {
    _ml_logw ("Cannot get jni env in the ml-service event callback.");
    return;
  }

  switch (event) {
    case ML_SERVICE_EVENT_NEW_DATA:
    {
      jstring node_name = NULL;
      jobject odata = NULL;
      jobject oinfo = NULL;
      ml_tensors_data_h data = NULL;
      gchar *name = NULL;

      status = ml_information_get (event_data, "name", (void **) &name);
      if (status == ML_ERROR_NONE) {
        node_name = (*env)->NewStringUTF (env, name);
      }

      status = ml_information_get (event_data, "data", (void **) &data);
      if (status == ML_ERROR_NONE) {
        oinfo = nns_service_convert_node_info (pipe_info, env, name, FALSE);

        if (nns_convert_tensors_data (pipe_info, env, data, oinfo, &odata)) {
          (*env)->CallVoidMethod (env, pipe_info->instance, priv->mid_data_cb,
              node_name, odata);

          if ((*env)->ExceptionCheck (env)) {
            _ml_loge ("Failed to call the new-data callback of ml-service.");
            (*env)->ExceptionClear (env);
          }
        }
      } else {
        _ml_loge ("Failed to get tensor data from ml-service event.");
      }

      if (node_name) {
        (*env)->DeleteLocalRef (env, node_name);
      }
      if (odata) {
        (*env)->DeleteLocalRef (env, odata);
      }
      if (oinfo) {
        (*env)->DeleteLocalRef (env, oinfo);
      }
      break;
    }
    default:
      break;
  }
}

/**
 * @brief Native method for ml-service API.
 */
static jlong
nns_native_service_open (JNIEnv * env, jobject thiz, jstring config_path)
{
  pipeline_info_s *pipe_info;
  nns_service_priv_s *priv;
  ml_service_h service;
  gboolean opened = FALSE;
  int status;

  const char *config = (*env)->GetStringUTFChars (env, config_path, NULL);

  pipe_info = nns_construct_pipe_info (env, thiz, NULL, NNS_PIPE_TYPE_SERVICE);
  if (pipe_info == NULL) {
    _ml_loge ("Failed to create pipe info.");
    goto done;
  }

  priv = g_new0 (nns_service_priv_s, 1);
  priv->mid_data_cb =
      (*env)->GetMethodID (env, pipe_info->cls, "newDataReceived",
      "(Ljava/lang/String;L" NNS_CLS_TDATA ";)V");

  nns_set_priv_data (pipe_info, priv, nns_service_priv_free);

  status = ml_service_new (config, &service);
  if (status != ML_ERROR_NONE) {
    _ml_loge ("Failed to create ml-service handle from config %s.", config);
    goto done;
  }

  pipe_info->pipeline_handle = service;

  status = ml_service_set_event_cb (service, nns_service_event_cb, pipe_info);
  if (status != ML_ERROR_NONE) {
    _ml_loge ("Failed to set event callback of ml-service handle.");
    goto done;
  }

  opened = TRUE;

done:
  if (!opened) {
    nns_destroy_pipe_info (pipe_info, env);
    pipe_info = NULL;
  }

  (*env)->ReleaseStringUTFChars (env, config_path, config);

  return CAST_TO_LONG (pipe_info);
}

/**
 * @brief Native method for ml-service API.
 */
static void
nns_native_service_close (JNIEnv * env, jobject thiz, jlong handle)
{
  pipeline_info_s *pipe_info;

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);

  nns_destroy_pipe_info (pipe_info, env);
}

/**
 * @brief Native method for ml-service API.
 */
static jboolean
nns_native_service_start (JNIEnv * env, jobject thiz, jlong handle)
{
  pipeline_info_s *pipe_info;
  ml_service_h service;
  int status;

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);
  service = pipe_info->pipeline_handle;

  status = ml_service_start (service);
  if (status != ML_ERROR_NONE) {
    _ml_loge ("Failed to start ml-service handle.");
  }

  return (status == ML_ERROR_NONE);
}

/**
 * @brief Native method for ml-service API.
 */
static jboolean
nns_native_service_stop (JNIEnv * env, jobject thiz, jlong handle)
{
  pipeline_info_s *pipe_info;
  ml_service_h service;
  int status;

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);
  service = pipe_info->pipeline_handle;

  status = ml_service_stop (service);
  if (status != ML_ERROR_NONE) {
    _ml_loge ("Failed to stop ml-service handle.");
  }

  return (status == ML_ERROR_NONE);
}

/**
 * @brief Native method for ml-service API.
 */
static jboolean
nns_native_service_input_data (JNIEnv * env, jobject thiz, jlong handle,
    jstring node_name, jobject in)
{
  pipeline_info_s *pipe_info;
  ml_service_h service;
  ml_tensors_data_h in_data = NULL;
  int status = ML_ERROR_UNKNOWN;

  const char *name = (*env)->GetStringUTFChars (env, node_name, NULL);

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);
  service = pipe_info->pipeline_handle;

  if (nns_parse_tensors_data (pipe_info, env, in, FALSE, NULL, &in_data)) {
    status = ml_service_request (service, name, in_data);
    if (status != ML_ERROR_NONE) {
      _ml_loge ("Failed to request ml-service processing.");
    }
  } else {
    _ml_loge ("Failed to parse input data.");
  }

  /* Do not free input tensors (direct access from object). */
  if (in_data) {
    _ml_tensors_data_destroy_internal (in_data, FALSE);
  }

  (*env)->ReleaseStringUTFChars (env, node_name, name);

  return (status == ML_ERROR_NONE);
}

/**
 * @brief Native method for ml-service API.
 */
static jobject
nns_native_service_get_input_info (JNIEnv * env, jobject thiz, jlong handle,
    jstring node_name)
{
  pipeline_info_s *pipe_info;
  jobject oinfo;

  const char *name = (*env)->GetStringUTFChars (env, node_name, NULL);

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);
  oinfo = nns_service_convert_node_info (pipe_info, env, name, TRUE);

  (*env)->ReleaseStringUTFChars (env, node_name, name);

  return oinfo;
}

/**
 * @brief Native method for ml-service API.
 */
static jobject
nns_native_service_get_output_info (JNIEnv * env, jobject thiz, jlong handle,
    jstring node_name)
{
  pipeline_info_s *pipe_info;
  jobject oinfo;

  const char *name = (*env)->GetStringUTFChars (env, node_name, NULL);

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);
  oinfo = nns_service_convert_node_info (pipe_info, env, name, FALSE);

  (*env)->ReleaseStringUTFChars (env, node_name, name);

  return oinfo;
}

/**
 * @brief Native method for ml-service API.
 */
static jboolean
nns_native_service_set_info (JNIEnv * env, jobject thiz, jlong handle,
    jstring name, jstring value)
{
  pipeline_info_s *pipe_info;
  ml_service_h service;
  int status;

  const char *info_name = (*env)->GetStringUTFChars (env, name, NULL);
  const char *info_value = (*env)->GetStringUTFChars (env, value, NULL);

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);
  service = pipe_info->pipeline_handle;

  status = ml_service_set_information (service, info_name, info_value);
  if (status != ML_ERROR_NONE) {
    _ml_loge ("Failed to set the information (%s:%s).", info_name, info_value);
  }

  (*env)->ReleaseStringUTFChars (env, name, info_name);
  (*env)->ReleaseStringUTFChars (env, value, info_value);

  return (status == ML_ERROR_NONE);
}

/**
 * @brief Native method for ml-service API.
 */
static jstring
nns_native_service_get_info (JNIEnv * env, jobject thiz, jlong handle,
    jstring name)
{
  pipeline_info_s *pipe_info;
  ml_service_h service;
  int status;

  const char *info_name = (*env)->GetStringUTFChars (env, name, NULL);
  char *info_value = NULL;
  jstring value = NULL;

  pipe_info = CAST_TO_TYPE (handle, pipeline_info_s *);
  service = pipe_info->pipeline_handle;

  status = ml_service_get_information (service, info_name, &info_value);
  if (status == ML_ERROR_NONE) {
    value = (*env)->NewStringUTF (env, info_value);
    g_free (info_value);
  } else {
    _ml_loge ("Failed to get the information (%s).", info_name);
  }

  (*env)->ReleaseStringUTFChars (env, name, info_name);

  return value;
}

/**
 * @brief List of implemented native methods for ml-service class.
 */
static JNINativeMethod native_methods_service[] = {
  {(char *) "nativeOpen", (char *) "(Ljava/lang/String;)J",
      (void *) nns_native_service_open},
  {(char *) "nativeClose", (char *) "(J)V",
      (void *) nns_native_service_close},
  {(char *) "nativeStart", (char *) "(J)Z",
      (void *) nns_native_service_start},
  {(char *) "nativeStop", (char *) "(J)Z",
      (void *) nns_native_service_stop},
  {(char *) "nativeInputData", (char *) "(JLjava/lang/String;L" NNS_CLS_TDATA ";)Z",
      (void *) nns_native_service_input_data},
  {(char *) "nativeGetInputInfo", (char *) "(JLjava/lang/String;)L" NNS_CLS_TINFO ";",
      (void *) nns_native_service_get_input_info},
  {(char *) "nativeGetOutputInfo", (char *) "(JLjava/lang/String;)L" NNS_CLS_TINFO ";",
      (void *) nns_native_service_get_output_info},
  {(char *) "nativeSetInfo", (char *) "(JLjava/lang/String;Ljava/lang/String;)Z",
      (void *) nns_native_service_set_info},
  {(char *) "nativeGetInfo", (char *) "(JLjava/lang/String;)Ljava/lang/String;",
      (void *) nns_native_service_get_info},
};

/**
 * @brief Register native methods for ml-service class.
 */
gboolean
nns_native_service_register_natives (JNIEnv * env)
{
  jclass klass = (*env)->FindClass (env, NNS_CLS_MLSERVICE);

  if (klass) {
    if ((*env)->RegisterNatives (env, klass, native_methods_service,
            G_N_ELEMENTS (native_methods_service))) {
      _ml_loge ("Failed to register native methods for ml-service class.");
      return FALSE;
    }
  }

  return TRUE;
}
