/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer Android API
 * Copyright (C) 2025 Samsung Electronics Co., Ltd.
 *
 * @file	nnstreamer-native.h
 * @date	15 January 2025
 * @brief	Android native function for NNStreamer and ML API
 * @author	Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#ifndef __NNSTREAMER_NATIVE_H__
#define __NNSTREAMER_NATIVE_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Initialize NNStreamer, register required plugins.
 * @note You should call initialize with application context.
 */
extern jboolean
nnstreamer_native_initialize (JNIEnv *env, jobject context);

/**
 * @brief Get the data path of an application, extracted using getFilesDir() for Android.
 * @note DO NOT release returned string, it is constant.
 */
extern const char *
nnstreamer_native_get_data_path (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __NNSTREAMER_NATIVE_H__ */
