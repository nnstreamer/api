/**
 * @file        unittest_util.c
 * @date        9 Dec 2021
 * @brief       Unittest utility functions for ML C-API
 * @see         https://github.com/nnstreamer/api
 * @author      MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug         No known bugs.
 */

#include <unistd.h>
#include "unittest_util.h"

#ifdef FAKEDLOG
#include <glib.h>

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
