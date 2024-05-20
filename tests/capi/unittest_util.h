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
#include <glib/gstdio.h>
#include "nnstreamer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Get available port number.
 */
guint get_available_port (void);

/**
 * @brief Util function to get the config file path.
 * @return The conf file path for unittest. You should release returned path string.
 */
gchar * get_config_path (const gchar *config_name);

/**
 * @brief Prepare conf file for unittest, update available port number.
 * @return Temporal conf file path. You should release and remove returned path string.
 */
gchar * prepare_test_config (const gchar *config_name, const guint port);

/**
 * @brief Wait until the change in pipeline status is done
 * @return ML_ERROR_NONE success, ML_ERROR_UNKNOWN if failed, ML_ERROR_TIMED_OUT if timeout happens.
 */
int waitPipelineStateChange (ml_pipeline_h handle, ml_pipeline_state_e state, guint timeout_ms);

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
