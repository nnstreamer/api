/* SPDX-License-Identifier: Apache-2.0 */
/**
 * NNStreamer / Tizen Machine-Learning "Service API" Header for OS packages.
 * Copyright (C) 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
 */
/**
 * @file    ml-api-service-internal.h
 * @date    03 Nov 2021
 * @brief   ML-API Internal Platform Service Header
 * @see     https://github.com/nnstreamer/api
 * @author  MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug     No known bugs except for NYI items
 *
 * @details
 *      This provides interfaces of ML Service APIs for
 *    platform packages (with "root" or "OS" priveleges).
 *      Application developers should use ml-api-service.h
 *    instead.
 *      However, whether to mandate this or not can be decided by
 *    vendors. For Tizen, this is mandated; thus, .rpm packages
 *    may use internal headers and .tpk packages cannot use
 *    internal headers.
 */
#ifndef __ML_API_SERVICE_INTERNAL_H__
#define __ML_API_SERVICE_INTERNAL_H__

#include "ml-api-service.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************
 * Phase 1 APIs: WIP
 ***************************************************/

/**
 * @brief TBU
 */
typedef enum {
  ML_SERV_MODEL_INSTNACE = 0,
  ML_SERV_MODEL_FUNC,
  ML_SERV_MODEL_SERIES,

  ML_SERV_MODEL_MAX,
} ml_service_model_t;
/**
 * @brief TBU
 */
typedef struct {
  ml_service_model_t type;
  union {
    struct {
      char *model; /**< [Mandatory] Ususally, this is the path to model file or directory. Sometimes, this can be a "name" of mechanism supplied by a subplugin */
      ml_tensors_info_h input_info; /**< [Optional] Supply the initial input dimension if the model has flexible input dimension. Otherwise, you may set it NULL. */
      ml_tensors_info_h output_info; /**< [Optional] Supply the initial output dimension if the model has flexible output dimension. Otherwise, you may set it NULL. */
      ml_nnfw_type_e nnfw; /**< [Mandatory] The neural network framework used to open the given model */
      ml_nnfw_hw_e hw; /**< [Mandatory] The hardware to be used for the inference. Users may override by not using "ANY" with ml_single_open or tensor_filter's option. */
    } instance; /* instance */
    struct {
      ml_custom_easy_invoke_cb func; /**< [Mandatory] The function to be registered */
      void *user_data; /**< [Optional] Additional information fed to the func in run-time */
      ml_tensors_info_h input_info; /**< [Mandatory] the input dimension */
      ml_tensors_info_h output_info; /**< [Mandatory] the output dimension */
    } func; /* func */
    struct {
      const char **names; /**< [Mandatory] The names of models in the series. Terminated by NULL */
      ml_tensors_info_h input_info; /**< [Mandatory] the input dimension */
      ml_tensors_info_h output_info; /**< [Mandatory] the output dimension */
    } series; /* serires */
  };
} ml_service_model_description;

/**
 * @brief TBU
 * @detail
 *    You may handle the given pipe as if it's fetched by ml_pipeline_construct().
 */
int ml_service_pipeline_construct (const char *name, ml_pipeline_state_cb cb, void *user_data, ml_pipeline_h *pipe);

/**
 * @brief Add a model for single API and tensor-filter in a pipeline.
 * @detail Usage Example
 *
 * Service App or Middleware
 *   ml_service_model_add ("V1", desc1);
 *   ml_service_model_add ("V2", desc2);
 *   ml_service_model_add ("V3", desc3);
 *   ml_service_model_description descS1 = { .type = ML_SERV_MODEL_SERIES, .series = { .num_models=3, .names = { "V1", "V2", "V3", NULL }, .input_info = in, .output_info = out } };
 *   ml_service_model_add ("SERIES1", descS1);
 *
 * App
 *   ml_single_open(&handle, "V1", NULL, NULL, ML_NNFW_TYPE_SERVICE, ML_NNFW_HW_ANY);
 *   ml_single_invoke(handle, input, &output);
 *   ...
 *   ml_single_open(&handle2, "SERIES1", in, out, ML_SERV_MODEL_SERIES, ML_NNFW_HW_ANY);
 *   ml_single_invoke(handle, input, &output); // output = model_v3 ( model_v2 ( model_v1 ( input ) ) );
 */
int ml_service_model_add (const char *name, const ml_service_model_description * desc);


/***************************************************
 * Phase 2 APIs: NYI
 *
 * Mode 1.
 * Register a pipeline, a model, or a series of models
 * as an AI service as a pair of tensor-query-server-*.
 *
 * Mode 2.
 * Reigster a pipeline, a model, or a series of models
 * as an AI service, streaming out with mqtt-sink.
 ***************************************************/
typedef void *ml_service_server_h;

/** @brief TBU */
int ml_service_server_getstate (ml_service_server_h h, ml_pipeline_state_e *state);
/** @brief TBU */
int ml_service_server_getdesc (ml_service_server_h h, char ** desc);
/** @brief TBU */
int ml_service_server_start (ml_service_server_h h);
/** @brief TBU */
int ml_service_server_stop (ml_service_server_h h);
/** @brief TBU */
int ml_service_server_close (ml_service_server_h h);

/**
 * @brief TBU / Query Server AI Service
 * @detail
 *   Rule 1. The pipeline should not have appsink, tensor_sink, appsrc or any other app-thread dependencies.
 *   Rule 2. Add "#INPUT#" and "#OUTPUT#" elements where input/output streams exist.
 *     E.g., " #INPUT# ! ... ! tensor-filter ... ! ... ! #OUTPUT# ".
 *   Rule 3. There should be exactly one pair of #INPUT# and #OUTPUT#.
 *   Rule 4. Supply input/output metadata with input_info & output_info.
 *   This is the simplist method, but restricted to static tensor streams.
 */
int ml_service_server_open_queryserver_static_tensors (ml_service_server_h *h, const char *topic_name, const char * desc, const ml_tensors_info_h input_info, const ml_tensors_info_h output_info);
/**
 * @brief TBU / Query Server AI Service
 * @detail
 *   Rule 1. The pipeline should not have appsink, tensor_sink, appsrc or any other app-thread dependencies.
 *   Rule 2. You may add "#INPUT#" and "#OUTPUT#" elements if you do not know how to use tensor-query-server.
 *     E.g., " #INPUT# ! tensor-filter ... ! ... ! #OUTPUT# ".
 *   Rule 3. There should be exactly one pair of #INPUT# and #OUTPUT#.
 *   Rule 4. Supply input/output metadata with gstcap_in and gstcap_out.
 *   This supports general GStreamer streams and general Tensor streams.
 */
int ml_service_server_open_queryserver_gstcaps (ml_service_server_h *h, const char *topic_name, const char * desc, const char *gstcap_in, const char *gstcap_out);
/**
 * @brief TBU / Query Server AI Service
 * @detail
 *   Rule 1. The pipeline should have a single pair of tensor-query-server-{sink / src}.
 *   Rule 2. The pipeline should not have appsink, tensor_sink, appsrc or any other app-thread dependencies.
 *   Rule 3. There should be exactly one pair of #INPUT# and #OUTPUT# if you use them.
 *   Rule 4. Add capsfilter or capssetter after src and before sink.
 *   This is for seasoned gstreamer/nnstreamer users who have some experiences in pipeline writing.
 */
int ml_service_server_open_queryserver_fulldesc (ml_service_server_h *h, const char *topic_name, const char * desc);

/**
 * @brief TBU / PUB/SUB AI Service
 * @detail
 * use "#OUTPUT#" unless you use fulldesc
 * don't rely on app threads (no appsink, appsrc, tensorsink or so on)
 */
int ml_service_server_open_publisher_static_tensors (ml_service_server_h *h, const char *topic_name, const char * desc, const ml_tensors_data_h out);
/** @brief TBU */
int ml_service_server_open_publisher_gstcaps (ml_service_server_h *h, const char *topic_name, const char * desc, const char *gstcap_out);
/** @brief TBU */
int ml_service_server_open_publisher_fulldesc (ml_service_server_h *h, const char *topic_name, const char * desc);

typedef void *ml_service_client_h;

/**
 * @brief TBU / Client-side helpers
 * @detail
 *    Please use a pipeline for more efficient usage. This API is for testing or apps that can afford high-latency
 * @param [out] in Input tensors info. Set null if you don't need this info.
 * @param [out] out Output tensors info. Set null if you don't need this info.
 *    Note that we do not know if in/out is possible for remote clients, yet.
 */
int ml_service_client_open_query (ml_service_client_h *h, const char *topic_name, ml_tensors_info_h *in, ml_tensors_info_h *out);
/** @brief TBU */
int ml_service_client_open_subscriber (ml_service_client_h *h, const char *topic_name, ml_pipeline_sink_cb func, void *user_data);

/** @brief TBU */
int ml_service_client_query (ml_service_client_h h, const ml_tensors_data_h in, ml_tensors_data_h out);

/** @brief TBU */
int ml_service_client_close (ml_service_client_h h);

/**
 * @brief TBU
 * @param[in] desc provider_type of desc is not restricted.
 */
int ml_service_pipeline_description_add_privileged (const ml_service_pipeline_description * desc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ML_API_SERVICE_H__ */
