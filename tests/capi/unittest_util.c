/**
 * @file        unittest_util.c
 * @date        9 Dec 2021
 * @brief       Unittest utility functions for ML C-API
 * @see         https://github.com/nnstreamer/api
 * @author      MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug         No known bugs.
 */

#include <unistd.h>
#include <stdint.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include "unittest_util.h"

#ifdef FAKEDLOG

/**
 * @brief A faked dlog_print for unittest execution.
 */
int dlog_print (int level, const char *tag, const char *fmt, ...)
{
  va_list arg_ptr;
  va_start (arg_ptr, fmt);
  g_logv (tag, G_LOG_LEVEL_CRITICAL, fmt, arg_ptr);
  va_end (arg_ptr);

  return 0;
}
#endif

/**
 * @brief Get available port number.
 */
guint get_available_port (void)
{
  struct sockaddr_in sin;
  guint port = 0;
  gint sock;
  socklen_t len = sizeof (struct sockaddr);

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons (0);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return 0;

  if (bind (sock, (struct sockaddr *) &sin, sizeof (struct sockaddr)) == 0) {
    if (getsockname (sock, (struct sockaddr *) &sin, &len) == 0) {
      port = ntohs (sin.sin_port);
    }
  }
  close (sock);

  return port;
}

/**
 * @brief Wait until the change in pipeline status is done
 * @return ML_ERROR_NONE success, ML_ERROR_UNKNOWN if failed, ML_ERROR_TIMED_OUT if timeout happens.
 */
int
waitPipelineStateChange (ml_pipeline_h handle, ml_pipeline_state_e state, guint timeout_ms)
{
  int status = ML_ERROR_UNKNOWN;
  guint counter = 0;
  ml_pipeline_state_e cur_state = ML_PIPELINE_STATE_NULL;

  do {
    status = ml_pipeline_get_state (handle, &cur_state);
    if (ML_ERROR_NONE != status)
      return status;
    if (cur_state == ML_PIPELINE_STATE_UNKNOWN)
      return ML_ERROR_UNKNOWN;
    if (cur_state == state)
      return ML_ERROR_NONE;
    g_usleep (10000);
  } while ((timeout_ms / 10) >= counter++);

  return ML_ERROR_TIMED_OUT;
}
