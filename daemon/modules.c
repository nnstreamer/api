/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file modules.c
 * @date 25 June 2022
 * @brief NNStreamer/Utilities C-API Wrapper.
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <glib.h>
#include <stdio.h>

#include "common.h"
#include "modules.h"
#include "log.h"

static GList *module_head = NULL;

/**
 * @brief Add the specific DBus interface into the Machine Learning agent daemon.
 */
void
add_module (const struct module_ops *module)
{
  module_head = g_list_append (module_head, (gpointer) module);
}

/**
 * @brief Remove the specific DBus interface from the Machine Learning agent daemon.
 */
void
remove_module (const struct module_ops *module)
{
  module_head = g_list_remove (module_head, (gconstpointer) module);
}

/**
 * @brief Initialize all added modules by calling probe and init callback functions.
 */
void
init_modules (void *data)
{
  GList *elem, *elem_n;
  const struct module_ops *module;

  elem = module_head;
  while (elem != NULL) {
    module = elem->data;
    elem_n = elem->next;

    if (module->probe && module->probe (data) != 0) {
      _E ("[%s] probe fail", module->name);
      module_head = g_list_remove (module_head, (gconstpointer) module);
      elem = elem_n;
      continue;
    }

    if (module->init)
      module->init (data);
    elem = elem_n;
  }
}

/**
 * @brief Clean up all added modules by calling the exit callback function.
 */
void
exit_modules (void *data)
{
  GList *elem;
  const struct module_ops *module;

  for (elem = module_head; elem != NULL; elem = elem->next) {
    module = elem->data;
    if (module->exit)
      module->exit (data);
  }
}
