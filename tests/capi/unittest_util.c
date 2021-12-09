/**
 * @file        unittest_util.c
 * @date        9 Dec 2021
 * @brief       Unittest utility functions for ML C-API
 * @see         https://github.com/nnstreamer/api
 * @author      MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug         No known bugs.
 */
#ifdef FAKEDLOG
#include <glib.h>

/**
 * @brief A faked dlog_print for unittest execution.
 */
int dlog_print (int level, const char *tag, const char *fmt, ...)
{
  va_list arg_ptr;
  va_start (arg_ptr, fmt);
  g_logv (G_LOG_LEVEL_CRITICAL, fmt, arg_ptr);
  va_end (arg_ptr);

  return 0;
}
#endif
