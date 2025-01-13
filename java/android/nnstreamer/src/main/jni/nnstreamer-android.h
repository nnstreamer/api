/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer Android API
 * Copyright (C) 2025 Samsung Electronics Co., Ltd.
 *
 * @file	nnstreamer-android.h
 * @date	15 January 2025
 * @brief	Android native function for NNStreamer and ML API
 * @author	Jaeyun Jung <jy1210.jung@samsung.com>
 * @bug		No known bugs except for NYI items
 */

#ifndef __NNSTREAMER_ANDROID_H__
#define __NNSTREAMER_ANDROID_H__

#include <jni.h>

/**
 * @brief Initialize NNStreamer, register required plugins.
 */
extern jboolean
nnstreamer_native_initialize (JNIEnv *env, jobject context);

#endif /* __NNSTREAMER_ANDROID_H__ */
