/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    test-dbus-interface.h
 * @date    16 Jul 2022
 * @brief   Test header for the definition of DBus node and interfaces.
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This defines the machine learning agent's bus, interface, and method names for Test.
 */

#ifndef __TEST_DBUS_INTERFACE_H__
#define __TEST_DBUS_INTERFACE_H__

#define DBUS_TEST_INTERFACE          "org.tizen.machinelearning.service.test"
#define DBUS_TEST_PATH               "/Org/Tizen/MachineLearning/Service/Test"

#define DBUS_TEST_I_GET_STATE_HANDLER       "handle-get-state"

#endif /* __TEST_DBUS_INTERFACE_H__ */
