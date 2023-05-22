/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    modules.h
 * @date    25 June 2022
 * @brief   Internal module utility header of Machine Learning agent daemon
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This provides the DBus module utility functions for the Machine Learning agent daemon.
 */
#ifndef __MODULES_H__
#define __MODULES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Data structure contains the name and callback functions for a specific DBus interface.
 */
struct module_ops
{
  const char *name;     /**< Name of DBus Interface. */
  int (*probe) (void *data);    /**< Callback function for probing the DBus Interface */
  void (*init) (void *data);    /**< Callback function for initializing the DBus Interface */
  void (*exit) (void *data);    /**< Callback function for exiting the DBus Interface */
};

/**
 * @brief Utility macro for adding and removing the specific DBus interface.
 */
#define MODULE_OPS_REGISTER(module)	\
static void __CONSTRUCTOR__ module_init(void)	\
{	\
  add_module (module);	\
}	\
static void __DESTRUCTOR__ module_exit(void)	\
{	\
  remove_module (module);	\
}

/**
 * @brief Initialize all added modules by calling probe and init callback functions.
 * @param[in/out] data user data for passing the callback functions.
 */
void init_modules (void *data);

/**
 * @brief Clean up all added modules by calling the exit callback function.
 * @param[in/out] data user data for passing the callback functions.
 */
void exit_modules (void *data);

/**
 * @brief Add the specific DBus interface into the Machine Learning agent daemon.
 * @param[in] module DBus interface information.
 */
void add_module (const struct module_ops *module);

/**
 * @brief Remove the specific DBus interface from the Machine Learning agent daemon.
 * @param[in] module DBus interface information.
 */
void remove_module (const struct module_ops *module);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MODULES_H__ */
