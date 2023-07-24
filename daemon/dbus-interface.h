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
#define DBUS_PIPELINE_INTERFACE          "org.tizen.machinelearning.service.pipeline"
#define DBUS_PIPELINE_PATH               "/Org/Tizen/MachineLearning/Service/Pipeline"

#define DBUS_PIPELINE_I_SET_HANDLER             "handle-set-pipeline"
#define DBUS_PIPELINE_I_GET_HANDLER             "handle-get-pipeline"
#define DBUS_PIPELINE_I_DELETE_HANDLER          "handle-delete-pipeline"

#define DBUS_PIPELINE_I_LAUNCH_HANDLER          "handle-launch-pipeline"
#define DBUS_PIPELINE_I_START_HANDLER           "handle-start-pipeline"
#define DBUS_PIPELINE_I_STOP_HANDLER            "handle-stop-pipeline"
#define DBUS_PIPELINE_I_DESTROY_HANDLER         "handle-destroy-pipeline"
#define DBUS_PIPELINE_I_GET_STATE_HANDLER       "handle-get-state"

/* Model Interface */
#define DBUS_MODEL_INTERFACE            "org.tizen.machinelearning.service.model"
#define DBUS_MODEL_PATH                 "/Org/Tizen/MachineLearning/Service/Model"

#define DBUS_MODEL_I_HANDLER_REGISTER           "handle-register"
#define DBUS_MODEL_I_HANDLER_UPDATE_DESCRIPTION "handle-update-description"
#define DBUS_MODEL_I_HANDLER_ACTIVATE           "handle-activate"
#define DBUS_MODEL_I_HANDLER_GET                "handle-get"
#define DBUS_MODEL_I_HANDLER_GET_ACTIVATED      "handle-get-activated"
#define DBUS_MODEL_I_HANDLER_GET_ALL            "handle-get-all"
#define DBUS_MODEL_I_HANDLER_DELETE             "handle-delete"

/* Resource Interface */
#define DBUS_RESOURCE_INTERFACE         "org.tizen.machinelearning.service.resource"
#define DBUS_RESOURCE_PATH              "/Org/Tizen/MachineLearning/Service/Resource"

#define DBUS_RESOURCE_I_HANDLER_ADD                "handle-add"
#define DBUS_RESOURCE_I_HANDLER_GET                "handle-get"
#define DBUS_RESOURCE_I_HANDLER_DELETE             "handle-delete"

#endif /* __GDBUS_INTERFACE_H__ */
