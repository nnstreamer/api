/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    common.h
 * @date    25 June 2022
 * @brief   Internal common header of Machine Learning agent daemon
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This provides the common utility macros for Machine Learning agent daemon.
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#define ARRAY_SIZE(name) (sizeof(name)/sizeof(name[0]))

#ifndef __CONSTRUCTOR__
#define __CONSTRUCTOR__ __attribute__ ((constructor))
#endif

#ifndef __DESTRUCTOR__
#define __DESTRUCTOR__ __attribute__ ((destructor))
#endif

#endif /* __COMMON_H__ */
