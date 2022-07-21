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

#define DBUS_PIPELINE_INTERFACE          "org.tizen.machinelearning.service.pipeline"
#define DBUS_PIPELINE_PATH               "/Org/Tizen/MachineLearning/Service/Pipeline"

#define DBUS_PIPELINE_I_REGISTER_HANDLER        "handle_register_pipeline"
#define DBUS_PIPELINE_I_START_HANDLER           "handle_start_pipeline"
#define DBUS_PIPELINE_I_STOP_HANDLER            "handle_stop_pipeline"
#define DBUS_PIPELINE_I_DESTROY_HANDLER         "handle_destroy_pipeline"

#define DBUS_PIPELINE_I_GET_STATE_HANDLER       "handle_get_state"
#define DBUS_PIPELINE_I_GET_DESCRIPTION_HANDLER "handle_get_description"

#endif /* __GDBUS_INTERFACE_H__ */
