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

#define AGENT_LOG_TAG "ml-agent"

#define LOG_V(prio, tag, fmt, arg...) \
  ({ do { \
    dlog_print(prio, tag, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##arg);\
  } while (0); })

#define _D(fmt, arg...)		LOG_V(DLOG_DEBUG, AGENT_LOG_TAG, fmt, ##arg)
#define _I(fmt, arg...)		LOG_V(DLOG_INFO, AGENT_LOG_TAG, fmt, ##arg)
#define _W(fmt, arg...)		LOG_V(DLOG_WARN, AGENT_LOG_TAG, fmt, ##arg)
#define _E(fmt, arg...)		LOG_V(DLOG_ERROR, AGENT_LOG_TAG, fmt, ##arg)
#define _F(fmt, arg...)		LOG_V(DLOG_FATAL, AGENT_LOG_TAG, fmt, ##arg)
#else
#include <glib.h>

#define _D g_debug
#define _I g_info
#define _W g_warning
#define _E g_critical
#define _F g_error
#endif

#endif /* __LOG_H__ */
