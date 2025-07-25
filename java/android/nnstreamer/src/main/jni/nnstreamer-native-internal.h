/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer Android API
 * Copyright (C) 2019 Samsung Electronics Co., Ltd.
 *
 * @file	nnstreamer-native-internal.h
 * @date	10 July 2019
 * @brief	Native code for NNStreamer API
 * @author	Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#ifndef __NNSTREAMER_NATIVE_INTERNAL_H__
#define __NNSTREAMER_NATIVE_INTERNAL_H__

#include <jni.h>
#include <string.h>

#include <glib/gstdio.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include "nnstreamer-native.h"
#include "nnstreamer.h"
#include "nnstreamer-single.h"
#include "nnstreamer-tizen-internal.h"
#include "nnstreamer_plugin_api.h"
#include "nnstreamer_plugin_api_filter.h"
#include <ml-api-internal.h>
#include <ml-api-inference-internal.h>
#include <ml-api-inference-single-internal.h>
#include <ml-api-inference-pipeline-internal.h>
#if defined(ENABLE_ML_SERVICE)
#include <ml-api-service-private.h>
#endif

#if defined(__ANDROID__)
#if !GST_CHECK_VERSION(1, 24, 0)
#error "NNStreamer native library is available with GStreamer 1.24 or higher."
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if GLIB_SIZEOF_VOID_P == 8
#define CAST_TO_LONG(p) (jlong)(p)
#define CAST_TO_TYPE(l,type) (type)(l)
#else
#define CAST_TO_LONG(p) (jlong)(jint)(p)
#define CAST_TO_TYPE(l,type) (type)(jint)(l)
#endif

/**
 * @brief JNI version for NNStreamer Android, sync with GStreamer.
 */
#define NNS_JNI_VERSION (JNI_VERSION_1_4)

/**
 * @brief NNStreamer package name.
 */
#define NNS_PKG "org/nnsuite/nnstreamer"
#define NNS_CLS_TDATA NNS_PKG "/TensorsData"
#define NNS_CLS_TINFO NNS_PKG "/TensorsInfo"
#define NNS_CLS_PIPELINE NNS_PKG "/Pipeline"
#define NNS_CLS_SINGLE NNS_PKG "/SingleShot"
#define NNS_CLS_CUSTOM_FILTER NNS_PKG "/CustomFilter"
#define NNS_CLS_MLSERVICE NNS_PKG "/MLService"
#define NNS_CLS_NNSTREAMER NNS_PKG "/NNStreamer"

/**
 * @brief Callback to destroy private data in pipe info.
 */
typedef void (*nns_priv_destroy)(gpointer data, JNIEnv * env);

/**
 * @brief Pipeline type in native pipe info.
 */
typedef enum
{
  NNS_PIPE_TYPE_PIPELINE = 0,
  NNS_PIPE_TYPE_SINGLE,
  NNS_PIPE_TYPE_CUSTOM,
  NNS_PIPE_TYPE_SERVICE,

  NNS_PIPE_TYPE_UNKNOWN
} nns_pipe_type_e;

/**
 * @brief Element type in native pipe info.
 */
typedef enum
{
  NNS_ELEMENT_TYPE_SRC = 0,
  NNS_ELEMENT_TYPE_SINK,
  NNS_ELEMENT_TYPE_VALVE,
  NNS_ELEMENT_TYPE_SWITCH,
  NNS_ELEMENT_TYPE_VIDEO_SINK,

  NNS_ELEMENT_TYPE_UNKNOWN
} nns_element_type_e;

/**
 * @brief Struct for TensorsData class info.
 */
typedef struct
{
  jclass cls;
  jmethodID mid_init;
  jmethodID mid_alloc;
  jmethodID mid_get_array;
  jmethodID mid_get_info;
} tensors_data_class_info_s;

/**
 * @brief Struct for TensorsInfo class info.
 */
typedef struct
{
  jclass cls;
  jmethodID mid_init;
  jmethodID mid_add_info;
  jmethodID mid_get_array;

  jclass cls_info;
  jfieldID fid_info_name;
  jfieldID fid_info_type;
  jfieldID fid_info_dim;
} tensors_info_class_info_s;

/**
 * @brief Struct for constructed pipeline.
 */
typedef struct
{
  nns_pipe_type_e pipeline_type;
  gpointer pipeline_handle;
  GHashTable *element_handles;
  GMutex lock;

  JavaVM *jvm;
  jint version;
  pthread_key_t jni_env;

  jobject instance;
  jclass cls;
  tensors_data_class_info_s tensors_data_cls_info;
  tensors_info_class_info_s tensors_info_cls_info;

  gpointer priv_data;
  nns_priv_destroy priv_destroy_func;
} pipeline_info_s;

/**
 * @brief Struct for element data in pipeline.
 */
typedef struct
{
  gchar *name;
  nns_element_type_e type;
  gpointer handle;
  pipeline_info_s *pipe_info;

  gpointer priv_data;
  nns_priv_destroy priv_destroy_func;
} element_data_s;

/**
 * @brief Get JNI environment.
 */
extern JNIEnv *
nns_get_jni_env (pipeline_info_s * pipe_info);

/**
 * @brief Free element handle pointer.
 */
extern void
nns_free_element_data (gpointer data);

/**
 * @brief Construct pipeline info.
 */
extern gpointer
nns_construct_pipe_info (JNIEnv * env, jobject thiz, gpointer handle, nns_pipe_type_e type);

/**
 * @brief Destroy pipeline info.
 */
extern void
nns_destroy_pipe_info (pipeline_info_s * pipe_info, JNIEnv * env);

/**
 * @brief Set private data in pipeline info.
 */
extern void
nns_set_priv_data (pipeline_info_s * pipe_info, gpointer data, nns_priv_destroy destroy_func);

/**
 * @brief Get element data of given name.
 */
extern element_data_s *
nns_get_element_data (pipeline_info_s * pipe_info, const gchar * name);

/**
 * @brief Get element handle of given name and type.
 */
extern gpointer
nns_get_element_handle (pipeline_info_s * pipe_info, const gchar * name, const nns_element_type_e type);

/**
 * @brief Remove element data of given name.
 */
extern gboolean
nns_remove_element_data (pipeline_info_s * pipe_info, const gchar * name);

/**
 * @brief Add new element data of given name.
 */
extern gboolean
nns_add_element_data (pipeline_info_s * pipe_info, const gchar * name, element_data_s * item);

/**
 * @brief Create new data object with given tensors info. Caller should unref the result object.
 */
extern gboolean
nns_create_tensors_data_object (pipeline_info_s * pipe_info, JNIEnv * env, jobject obj_info, jobject * result);

/**
 * @brief Convert tensors data to TensorsData object.
 */
extern gboolean
nns_convert_tensors_data (pipeline_info_s * pipe_info, JNIEnv * env, ml_tensors_data_h data_h, jobject obj_info, jobject * result);

/**
 * @brief Parse tensors data from TensorsData object.
 */
extern gboolean
nns_parse_tensors_data (pipeline_info_s * pipe_info, JNIEnv * env, jobject obj_data, gboolean clone, const ml_tensors_info_h info_h, ml_tensors_data_h * data_h);

/**
 * @brief Convert tensors info to TensorsInfo object.
 */
extern gboolean
nns_convert_tensors_info (pipeline_info_s * pipe_info, JNIEnv * env, ml_tensors_info_h info_h, jobject * result);

/**
 * @brief Parse tensors info from TensorsInfo object.
 */
extern gboolean
nns_parse_tensors_info (pipeline_info_s * pipe_info, JNIEnv * env, jobject obj_info, ml_tensors_info_h * info_h);

/**
 * @brief Get NNFW from integer value.
 */
extern gboolean
nns_get_nnfw_type (jint fw_type, ml_nnfw_type_e * nnfw);

/**
 * @brief Register native methods for SingleShot class.
 */
extern gboolean
nns_native_single_register_natives (JNIEnv * env);

#if !defined (NNS_SINGLE_ONLY)
/**
 * @brief Register native methods for Pipeline class.
 */
extern gboolean
nns_native_pipe_register_natives (JNIEnv * env);

/**
 * @brief Register native methods for CustomFilter class.
 */
extern gboolean
nns_native_custom_register_natives (JNIEnv * env);

#if defined(ENABLE_ML_SERVICE)
/**
 * @brief Register native methods for ml-service class.
 */
extern gboolean
nns_native_service_register_natives (JNIEnv * env);
#endif
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __NNSTREAMER_NATIVE_INTERNAL_H__ */
