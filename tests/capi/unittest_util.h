/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file        unittest_util.h
 * @date        07 May 2024
 * @brief       Unit test utility.
 * @see         https://github.com/nnstreamer/api
 * @author      Gichan Jang <gichan2.jang@samsung.com>
 * @bug         No known bugs
 */
#ifndef __ML_API_UNITTEST_UTIL_H__
#define __ML_API_UNITTEST_UTIL_H__
#include <glib.h>
#include <stdint.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Get available port number.
 */
guint get_available_port (void);

#ifdef FAKEDLOG
/**
 * @brief A faked dlog_print for unittest execution.
 */
int dlog_print (int level, const char *tag, const char *fmt, ...);
#endif /* FAKELOG */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_UNITTEST_UTIL_H__ */
