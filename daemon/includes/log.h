/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    log.h
 * @date    25 June 2022
 * @brief   Internal log header of Machine Learning agent daemon
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This provides the log macro for Machine Learning agent daemon.
 */
#ifndef __LOG_H__
#define __LOG_H__

#if defined(__TIZEN__)
#include <dlog.h>

#define _D(fmt, arg...)		do { SLOGD(fmt, ##arg); } while (0)
#define _I(fmt, arg...)		do { SLOGI(fmt, ##arg); } while (0)
#define _W(fmt, arg...)		do { SLOGW(fmt, ##arg); } while (0)
#define _E(fmt, arg...)		do { SLOGE(fmt, ##arg); } while (0)
#else
#include <glib.h>

#define _D g_debug
#define _I g_info
#define _W g_warning
#define _E g_critical
#endif

#endif /* __LOG_H__ */
