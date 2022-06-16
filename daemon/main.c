/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file main.c
 * @date 25 June 2022
 * @brief core module for the Machine Learning agent daemon
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <stdio.h>
#include <fcntl.h>
#include <glib.h>
#include <gio/gio.h>

#include "common.h"
#include "modules.h"
#include "gdbus-util.h"
#include "log.h"
#include "dbus-interface.h"

static GMainLoop *g_mainloop;

/**
 * @brief Handle the SIGTERM signal and quit the main loop
 */
static void
handle_sigterm (int signo)
{
  _D ("received SIGTERM signal %d", signo);
  g_main_loop_quit (g_mainloop);
}

/**
 * @brief Handle the post init tasks before starting the main loop.
 * @return @c 0 on success. Otherwise a negative error value.
 */
static int
postinit (void)
{
  int ret;
  /** Register signal handler */
  signal (SIGTERM, handle_sigterm);

  ret = gdbus_get_name (DBUS_ML_BUS_NAME);
  if (ret < 0)
    return ret;

  return 0;
}

/**
 * @brief main function of the Machine Learning agent daemon.
 */
int
main (int argc, char **argv)
{
  g_mainloop = g_main_loop_new (NULL, FALSE);
  gdbus_get_system_connection ();

  init_modules (NULL);
  if (postinit () < 0)
    _E ("cannot init system\n");

  g_main_loop_run (g_mainloop);
  exit_modules (NULL);

  gdbus_put_system_connection ();
  g_main_loop_unref (g_mainloop);

  return 0;
}
