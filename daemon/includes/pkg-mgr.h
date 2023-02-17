/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2023 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    pkg-mgr.h
 * @date    16 Feb 2023
 * @brief   Internal package manager utility header of Machine Learning agent daemon
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This provides the Tizen package manager utility functions for the Machine Learning agent daemon.
 */
#ifndef __PKG_MGR_H__
#define __PKG_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(__TIZEN__)
#include <glib.h>
#include <stdio.h>

#include "log.h"
#include "package_manager.h"

/**
 * @brief Initialize the package manager handler for the resource package.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int pkg_mgr_init (void);

/**
 * @brief Finalize the package manager handler for the resource package.
 * @return @c 0 on success. Otherwise a negative error value.
 */
int pkg_mgr_deinit (void);
#else
#define pkg_mgr_init(...) ((int) 0)
#define pkg_mgr_deinit(...) ((int) 0)
#endif /* __TIZEN__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __PKG_MGR_H__ */
