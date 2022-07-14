/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer API / Machine Learning Agent Daemon
 * Copyright (C) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 */

/**
 * @file    dbus-interface.h
 * @date    25 June 2022
 * @brief   Internal header for the definition of DBus node and interfaces.
 * @see     https://github.com/nnstreamer/api
 * @author  Sangjung Woo <sangjung.woo@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *    This defines the machine learning agent's bus, interface, and method names.
 */

#ifndef __GDBUS_INTERFACE_H__
#define __GDBUS_INTERFACE_H__

#define DBUS_ML_BUS_NAME                "org.tizen.machinelearning.service"
#define DBUS_ML_PATH                    "/Org/Tizen/MachineLearning/Service"

/* Pipeline Interface */
#define DBUS_PIPELINE_INTERFACE         "org.tizen.machinelearning.service.pipeline"
#define DBUS_PIPELINE_PATH              "/Org/Tizen/MachineLearning/Service/Pipeline"

#define DBUS_PIPELINE_HANDLER_SET       "handle_set"
#define DBUS_PIPELINE_HANDLER_GET       "handle_get"
#define DBUS_PIPELINE_HANDLER_DELETE    "handle_delete"

#endif /* __GDBUS_INTERFACE_H__ */
