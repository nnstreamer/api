/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-inference-single.c
 * @date 29 Aug 2019
 * @brief NNStreamer/Single C-API Wrapper.
 *        This allows to invoke individual input frame with NNStreamer.
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @author Parichay Kapoor <pk.kapoor@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <string.h>
#include <nnstreamer-single.h>
#include <nnstreamer-tizen-internal.h>  /* Tizen platform header */
#include <nnstreamer_internal.h>
#include <nnstreamer_plugin_api_util.h>
#include <tensor_filter_single.h>

#include "ml-api-inference-internal.h"
#include "ml-api-internal.h"
#include "ml-api-inference-single-internal.h"

#define ML_SINGLE_MAGIC 0xfeedfeed

/**
 * @brief Default time to wait for an output in milliseconds (0 will wait for the output).
 */
#define SINGLE_DEFAULT_TIMEOUT 0

/**
 * @brief Global lock for single shot API
 * @detail This lock ensures that ml_single_close is thread safe. All other API
 *         functions use the mutex from the single handle. However for close,
 *         single handle mutex cannot be used as single handle is destroyed at
 *         close
 * @note This mutex is automatically initialized as it is statically declared
 */
G_LOCK_DEFINE_STATIC (magic);

/**
 * @brief Get valid handle after magic verification
 * @note handle's mutex (single_h->mutex) is acquired after this
 * @param[out] single_h The handle properly casted: (ml_single *).
 * @param[in] single The handle to be validated: (void *).
 * @param[in] reset Set TRUE if the handle is to be reset (magic = 0).
 */
#define ML_SINGLE_GET_VALID_HANDLE_LOCKED(single_h, single, reset) do { \
  G_LOCK (magic); \
  single_h = (ml_single *) single; \
  if (G_UNLIKELY(single_h->magic != ML_SINGLE_MAGIC)) { \
    _ml_error_report \
        ("The given param, %s (ml_single_h), is invalid. It is not a single_h instance or the user thread has modified it.", \
        #single); \
    G_UNLOCK (magic); \
    return ML_ERROR_INVALID_PARAMETER; \
  } \
  if (G_UNLIKELY(reset)) \
    single_h->magic = 0; \
  g_mutex_lock (&single_h->mutex); \
  G_UNLOCK (magic); \
} while (0)

/**
 * @brief This is for the symmetricity with ML_SINGLE_GET_VALID_HANDLE_LOCKED
 * @param[in] single_h The casted handle (ml_single *).
 */
#define ML_SINGLE_HANDLE_UNLOCK(single_h) g_mutex_unlock (&single_h->mutex);

/** define string names for input/output */
#define INPUT_STR "input"
#define OUTPUT_STR "output"
#define TYPE_STR "type"
#define NAME_STR "name"

/** concat string from #define */
#define CONCAT_MACRO_STR(STR1,STR2) STR1 STR2

/** States for invoke thread */
typedef enum
{
  IDLE = 0,           /**< ready to accept next input */
  RUNNING,            /**< running an input, cannot accept more input */
  JOIN_REQUESTED      /**< should join the thread, will exit soon */
} thread_state;

/**
 * @brief The name of sub-plugin for defined neural net frameworks.
 * @note The sub-plugin for Android is not declared (e.g., snap)
 */
static const char *ml_nnfw_subplugin_name[] = {
  [ML_NNFW_TYPE_ANY] = "any",   /* DO NOT use this name ('any') to get the sub-plugin */
  [ML_NNFW_TYPE_CUSTOM_FILTER] = "custom",
  [ML_NNFW_TYPE_TENSORFLOW_LITE] = "tensorflow-lite",
  [ML_NNFW_TYPE_TENSORFLOW] = "tensorflow",
  [ML_NNFW_TYPE_NNFW] = "nnfw",
  [ML_NNFW_TYPE_MVNC] = "movidius-ncsdk2",
  [ML_NNFW_TYPE_OPENVINO] = "openvino",
  [ML_NNFW_TYPE_VIVANTE] = "vivante",
  [ML_NNFW_TYPE_EDGE_TPU] = "edgetpu",
  [ML_NNFW_TYPE_ARMNN] = "armnn",
  [ML_NNFW_TYPE_SNPE] = "snpe",
  [ML_NNFW_TYPE_PYTORCH] = "pytorch",
  [ML_NNFW_TYPE_NNTR_INF] = "nntrainer",
  [ML_NNFW_TYPE_VD_AIFW] = "vd_aifw",
  [ML_NNFW_TYPE_TRIX_ENGINE] = "trix-engine",
  [ML_NNFW_TYPE_MXNET] = "mxnet",
  [ML_NNFW_TYPE_TVM] = "tvm",
  NULL
};

/** ML single api data structure for handle */
typedef struct
{
  GTensorFilterSingleClass *klass;    /**< tensor filter class structure*/
  GTensorFilterSingle *filter;        /**< tensor filter element */
  ml_tensors_info_s in_info;          /**< info about input */
  ml_tensors_info_s out_info;         /**< info about output */
  ml_nnfw_type_e nnfw;                /**< nnfw type for this filter */
  guint magic;                        /**< code to verify valid handle */

  GThread *thread;                    /**< thread for invoking */
  GMutex mutex;                       /**< mutex for synchronization */
  GCond cond;                         /**< condition for synchronization */
  ml_tensors_data_h input;            /**< input received from user */
  ml_tensors_data_h output;           /**< output to be sent back to user */
  guint timeout;                      /**< timeout for invoking */
  thread_state state;                 /**< current state of the thread */
  gboolean free_output;               /**< true if output tensors are allocated in single-shot */
  int status;                         /**< status of processing */
  gboolean invoking;                  /**< invoke running flag */
  ml_tensors_data_s in_tensors;    /**< input tensor wrapper for processing */
  ml_tensors_data_s out_tensors;   /**< output tensor wrapper for processing */

  /** @todo Use only ml_tensor_info_s dimension instead of saving ranks value */
  guint input_ranks[ML_TENSOR_SIZE_LIMIT];   /**< the rank list of input tensors, it is calculated based on the dimension string. */
  guint output_ranks[ML_TENSOR_SIZE_LIMIT];  /**< the rank list of output tensors, it is calculated based on the dimension string. */

  GList *destroy_data_list;         /**< data to be freed by filter */
} ml_single;

/**
 * @brief Internal function to get the nnfw type.
 */
ml_nnfw_type_e
_ml_get_nnfw_type_by_subplugin_name (const char *name)
{
  ml_nnfw_type_e nnfw_type = ML_NNFW_TYPE_ANY;
  int idx = -1;

  if (name == NULL)
    return ML_NNFW_TYPE_ANY;

  idx = find_key_strv (ml_nnfw_subplugin_name, name);
  if (idx < 0) {
    /* check sub-plugin for android */
    if (g_ascii_strcasecmp (name, "snap") == 0)
      nnfw_type = ML_NNFW_TYPE_SNAP;
    else
      _ml_error_report ("Cannot find nnfw, %s is an invalid name.",
          _STR_NULL (name));
  } else {
    nnfw_type = (ml_nnfw_type_e) idx;
  }

  return nnfw_type;
}

/**
 * @brief Internal function to get the sub-plugin name.
 */
const char *
_ml_get_nnfw_subplugin_name (ml_nnfw_type_e nnfw)
{
  /* check sub-plugin for android */
  if (nnfw == ML_NNFW_TYPE_SNAP)
    return "snap";

  return ml_nnfw_subplugin_name[nnfw];
}

/**
 * @brief Convert c-api based hw to internal representation
 */
accl_hw
_ml_nnfw_to_accl_hw (const ml_nnfw_hw_e hw)
{
  switch (hw) {
    case ML_NNFW_HW_ANY:
      return ACCL_DEFAULT;
    case ML_NNFW_HW_AUTO:
      return ACCL_AUTO;
    case ML_NNFW_HW_CPU:
      return ACCL_CPU;
#if defined (__aarch64__) || defined (__arm__)
    case ML_NNFW_HW_CPU_NEON:
      return ACCL_CPU_NEON;
#else
    case ML_NNFW_HW_CPU_SIMD:
      return ACCL_CPU_SIMD;
#endif
    case ML_NNFW_HW_GPU:
      return ACCL_GPU;
    case ML_NNFW_HW_NPU:
      return ACCL_NPU;
    case ML_NNFW_HW_NPU_MOVIDIUS:
      return ACCL_NPU_MOVIDIUS;
    case ML_NNFW_HW_NPU_EDGE_TPU:
      return ACCL_NPU_EDGE_TPU;
    case ML_NNFW_HW_NPU_VIVANTE:
      return ACCL_NPU_VIVANTE;
    case ML_NNFW_HW_NPU_SLSI:
      return ACCL_NPU_SLSI;
    case ML_NNFW_HW_NPU_SR:
      /** @todo how to get srcn npu */
      return ACCL_NPU_SR;
    default:
      return ACCL_AUTO;
  }
}

/**
 * @brief Checks the availability of the given execution environments with custom option.
 */
int
ml_check_nnfw_availability_full (ml_nnfw_type_e nnfw, ml_nnfw_hw_e hw,
    const char *custom, bool *available)
{
  const char *fw_name = NULL;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!available)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, available (bool *), is NULL. It should be a valid pointer of bool. E.g., bool a; ml_check_nnfw_availability_full (..., &a);");

  /* init false */
  *available = false;

  if (nnfw == ML_NNFW_TYPE_ANY)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, nnfw (ml_nnfw_type_e), is ML_NNFW_TYPE_ANY. It should specify the framework to be probed for the hardware availability.");

  fw_name = _ml_get_nnfw_subplugin_name (nnfw);

  if (fw_name) {
    if (nnstreamer_filter_find (fw_name) != NULL) {
      accl_hw accl = _ml_nnfw_to_accl_hw (hw);

      if (gst_tensor_filter_check_hw_availability (fw_name, accl, custom)) {
        *available = true;
      } else {
        _ml_logi ("%s is supported but not with the specified hardware.",
            fw_name);
      }
    } else {
      _ml_logi ("%s is not supported.", fw_name);
    }
  } else {
    _ml_logw ("Cannot get the name of sub-plugin for given nnfw.");
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Checks the availability of the given execution environments.
 */
int
ml_check_nnfw_availability (ml_nnfw_type_e nnfw, ml_nnfw_hw_e hw,
    bool *available)
{
  return ml_check_nnfw_availability_full (nnfw, hw, NULL, available);
}

/**
 * @brief setup input and output tensor memory to pass to the tensor_filter.
 * @note this tensor memory wrapper will be reused for each invoke.
 */
static void
__setup_in_out_tensors (ml_single * single_h)
{
  int i;
  ml_tensors_data_s *in_tensors = &single_h->in_tensors;
  ml_tensors_data_s *out_tensors = &single_h->out_tensors;

  /** Setup input buffer */
  _ml_tensors_info_free (in_tensors->info);
  ml_tensors_info_clone (in_tensors->info, &single_h->in_info);

  in_tensors->num_tensors = single_h->in_info.num_tensors;
  for (i = 0; i < single_h->in_info.num_tensors; i++) {
    /** memory will be allocated by tensor_filter_single */
    in_tensors->tensors[i].tensor = NULL;
    in_tensors->tensors[i].size =
        _ml_tensor_info_get_size (&single_h->in_info.info[i],
        single_h->in_info.is_extended);
  }

  /** Setup output buffer */
  _ml_tensors_info_free (out_tensors->info);
  ml_tensors_info_clone (out_tensors->info, &single_h->out_info);

  out_tensors->num_tensors = single_h->out_info.num_tensors;
  for (i = 0; i < single_h->out_info.num_tensors; i++) {
    /** memory will be allocated by tensor_filter_single */
    out_tensors->tensors[i].tensor = NULL;
    out_tensors->tensors[i].size =
        _ml_tensor_info_get_size (&single_h->out_info.info[i],
        single_h->out_info.is_extended);
  }
}

/**
 * @brief To call the framework to destroy the allocated output data
 */
static inline void
__destroy_notify (gpointer data_h, gpointer single_data)
{
  ml_single *single_h;
  ml_tensors_data_s *data;

  data = (ml_tensors_data_s *) data_h;
  single_h = (ml_single *) single_data;

  if (G_LIKELY (single_h->filter)) {
    if (single_h->klass->allocate_in_invoke (single_h->filter)) {
      single_h->klass->destroy_notify (single_h->filter,
          (GstTensorMemory *) data->tensors);
    }
  }

  /* reset callback function */
  data->destroy = NULL;
}

/**
 * @brief Wrapper function for __destroy_notify
 */
static int
ml_single_destroy_notify_cb (void *handle, void *user_data)
{
  ml_tensors_data_h data = (ml_tensors_data_h) handle;
  ml_single_h single = (ml_single_h) user_data;
  ml_single *single_h;
  int status = ML_ERROR_NONE;

  if (G_UNLIKELY (!single))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to destroy data buffer. Callback function argument from _ml_tensors_data_destroy_internal is invalid. The given 'user_data' is NULL. It appears to be an internal error of ML-API or the user thread has touched private data structure.");
  if (G_UNLIKELY (!data))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "Failed to destroy data buffer. Callback function argument from _ml_tensors_data_destroy_internal is invalid. The given 'handle' is NULL. It appears to be an internal error of ML-API or the user thread has touched private data structure.");

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 0);

  if (G_UNLIKELY (!single_h->filter)) {
    status = ML_ERROR_INVALID_PARAMETER;
    _ml_error_report
        ("Failed to destroy the data buffer. The handle instance (single_h) is invalid. It appears to be an internal error of ML-API of the user thread has touched private data structure.");
    goto exit;
  }

  single_h->destroy_data_list =
      g_list_remove (single_h->destroy_data_list, data);
  __destroy_notify (data, single_h);

exit:
  ML_SINGLE_HANDLE_UNLOCK (single_h);

  return status;
}

/**
 * @brief setup the destroy notify for the allocated output data.
 * @note this stores the data entry in the single list.
 * @note this has not overhead if the allocation of output is not performed by
 * the framework but by tensor filter element.
 */
static void
set_destroy_notify (ml_single * single_h, ml_tensors_data_s * data,
    gboolean add)
{
  if (single_h->klass->allocate_in_invoke (single_h->filter)) {
    data->destroy = ml_single_destroy_notify_cb;
    data->user_data = single_h;
    add = TRUE;
  }

  if (add) {
    single_h->destroy_data_list = g_list_append (single_h->destroy_data_list,
        (gpointer) data);
  }
}

/**
 * @brief Internal function to call subplugin's invoke
 */
static inline int
__invoke (ml_single * single_h, ml_tensors_data_h in, ml_tensors_data_h out)
{
  ml_tensors_data_s *in_data, *out_data;
  int status = ML_ERROR_NONE;
  GstTensorMemory *in_tensors, *out_tensors;

  in_data = (ml_tensors_data_s *) in;
  out_data = (ml_tensors_data_s *) out;

  /* Prevent error case when input or output is null in invoke thread. */
  if (!in_data || !out_data) {
    _ml_error_report ("Failed to invoke a model, invalid data handle.");
    return ML_ERROR_STREAMS_PIPE;
  }

  in_tensors = (GstTensorMemory *) in_data->tensors;
  out_tensors = (GstTensorMemory *) out_data->tensors;

  /** invoke the thread */
  if (!single_h->klass->invoke (single_h->filter, in_tensors, out_tensors,
          single_h->free_output)) {
    const char *fw_name = _ml_get_nnfw_subplugin_name (single_h->nnfw);
    _ml_error_report
        ("Failed to invoke the tensors. The invoke callback of the tensor-filter subplugin '%s' has failed. Please contact the author of tensor-filter-%s (nnstreamer-%s) or review its source code. Note that this usually happens when the designated framework does not support the given model (e.g., trying to run tf-lite 2.6 model with tf-lite 1.13).",
        fw_name, fw_name, fw_name);
    status = ML_ERROR_STREAMS_PIPE;
  }

  return status;
}

/**
 * @brief Internal function to post-process given output.
 */
static inline void
__process_output (ml_single * single_h, ml_tensors_data_h output)
{
  ml_tensors_data_s *out_data;

  if (!single_h->free_output) {
    /* Do nothing. The output handle is not allocated in single-shot process. */
    return;
  }

  if (g_list_find (single_h->destroy_data_list, output)) {
    /**
     * Caller of the invoke thread has returned back with timeout.
     * So, free the memory allocated by the invoke as their is no receiver.
     */
    single_h->destroy_data_list =
        g_list_remove (single_h->destroy_data_list, output);
    ml_tensors_data_destroy (output);
  } else {
    out_data = (ml_tensors_data_s *) output;
    set_destroy_notify (single_h, out_data, FALSE);
  }
}

/**
 * @brief Initializes the rank information with default value.
 */
static int
_ml_tensors_rank_initialize (guint * rank)
{
  guint i;

  if (!rank)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, rank, is NULL. Provide a valid pointer.");

  for (i = 0; i < ML_TENSOR_SIZE_LIMIT; i++) {
    rank[i] = 0;
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Sets the rank information with given value.
 */
static int
_ml_tensors_set_rank (guint * rank, guint val)
{
  guint i;

  if (!rank)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, rank, is NULL. Provide a valid pointer.");

  for (i = 0; i < ML_TENSOR_SIZE_LIMIT; i++) {
    rank[i] = val;
  }

  return ML_ERROR_NONE;
}

/**
 * @brief thread to execute calls to invoke
 *
 * @details The thread behavior is detailed as below:
 *          - Starting with IDLE state, the thread waits for an input or change
 *          in state externally.
 *          - If state is not RUNNING, exit this thread, else process the
 *          request.
 *          - Process input, call invoke, process output. Any error in this
 *          state sets the status to be used by ml_single_invoke().
 *          - State is set back to IDLE and thread moves back to start.
 *
 *          State changes performed by this function when:
 *          RUNNING -> IDLE - processing is finished.
 *          JOIN_REQUESTED -> IDLE - close is requested.
 *
 * @note Error while processing an input is provided back to requesting
 *       function, and further processing of invoke_thread is not affected.
 */
static void *
invoke_thread (void *arg)
{
  ml_single *single_h;
  ml_tensors_data_h input, output;

  single_h = (ml_single *) arg;

  g_mutex_lock (&single_h->mutex);

  while (single_h->state <= RUNNING) {
    int status = ML_ERROR_NONE;

    /** wait for data */
    while (single_h->state != RUNNING) {
      g_cond_wait (&single_h->cond, &single_h->mutex);
      if (single_h->state >= JOIN_REQUESTED)
        goto exit;
    }

    input = single_h->input;
    output = single_h->output;
    /* Set null to prevent double-free. */
    single_h->input = NULL;

    single_h->invoking = TRUE;
    g_mutex_unlock (&single_h->mutex);
    status = __invoke (single_h, input, output);
    g_mutex_lock (&single_h->mutex);
    /* Clear input data after invoke is done. */
    ml_tensors_data_destroy (input);
    single_h->invoking = FALSE;

    if (status != ML_ERROR_NONE) {
      if (single_h->free_output) {
        single_h->destroy_data_list =
            g_list_remove (single_h->destroy_data_list, output);
        ml_tensors_data_destroy (output);
      }

      goto wait_for_next;
    }

    __process_output (single_h, output);

    /** loop over to wait for the next element */
  wait_for_next:
    single_h->status = status;
    if (single_h->state == RUNNING)
      single_h->state = IDLE;
    g_cond_broadcast (&single_h->cond);
  }

exit:
  /* Do not set IDLE if JOIN_REQUESTED */
  if (single_h->state == RUNNING)
    single_h->state = IDLE;
  g_mutex_unlock (&single_h->mutex);
  return NULL;
}

/**
 * @brief Sets the information (tensor dimension, type, name and so on) of required input data for the given model, and get updated output data information.
 * @details Note that a model/framework may not support setting such information.
 * @since_tizen 6.0
 * @param[in] single The model handle.
 * @param[in] in_info The handle of input tensors information.
 * @param[out] out_info The handle of output tensors information. The caller is responsible for freeing the information with ml_tensors_info_destroy().
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful
 * @retval #ML_ERROR_NOT_SUPPORTED This implies that the given framework does not support dynamic dimensions.
 *         Use ml_single_get_input_info() and ml_single_get_output_info() instead for this framework.
 * @retval #ML_ERROR_INVALID_PARAMETER Fail. The parameter is invalid.
 */
static int
ml_single_update_info (ml_single_h single,
    const ml_tensors_info_h in_info, ml_tensors_info_h * out_info)
{
  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, single (ml_single_h), is NULL. It should be a valid ml_single_h instance, usually created by ml_single_open().");
  if (!in_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, in_info (const ml_tensors_info_h), is NULL. It should be a valid instance of ml_tensors_info_h, usually created by ml_tensors_info_create() and configured by the application.");
  if (!out_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, out_info (ml_tensors_info_h *), is NULL. It should be a valid pointer to an instance ml_tensors_info_h, usually created by ml_tensors_info_h(). Note that out_info is supposed to be overwritten by this API call.");

  /* init null */
  *out_info = NULL;

  _ml_error_report_return_continue_iferr (ml_single_set_input_info (single,
          in_info),
      "Configuring the neural network model with the given input information has failed with %d error code. The given input information ('in_info' parameter) might be invalid or the given neural network cannot accept it as its input data.",
      _ERRNO);

  __setup_in_out_tensors (single);
  _ml_error_report_return_continue_iferr (ml_single_get_output_info (single,
          out_info),
      "Fetching output info after configuring input information has failed with %d error code.",
      _ERRNO);

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to get the gst info from tensor-filter.
 */
static void
ml_single_get_gst_info (ml_single * single_h, gboolean is_input,
    GstTensorsInfo * gst_info)
{
  const gchar *prop_prefix, *prop_name, *prop_type;
  gchar *val;
  guint num;

  if (is_input) {
    prop_prefix = INPUT_STR;
    prop_type = CONCAT_MACRO_STR (INPUT_STR, TYPE_STR);
    prop_name = CONCAT_MACRO_STR (INPUT_STR, NAME_STR);
  } else {
    prop_prefix = OUTPUT_STR;
    prop_type = CONCAT_MACRO_STR (OUTPUT_STR, TYPE_STR);
    prop_name = CONCAT_MACRO_STR (OUTPUT_STR, NAME_STR);
  }

  gst_tensors_info_init (gst_info);

  /* get dimensions */
  g_object_get (single_h->filter, prop_prefix, &val, NULL);
  num = gst_tensors_info_parse_dimensions_string (gst_info, val);
  g_free (val);

  /* set the number of tensors */
  gst_info->num_tensors = num;

  /* get types */
  g_object_get (single_h->filter, prop_type, &val, NULL);
  num = gst_tensors_info_parse_types_string (gst_info, val);
  g_free (val);

  if (gst_info->num_tensors != num) {
    _ml_logw ("The number of tensor type is mismatched in filter.");
  }

  /* get names */
  g_object_get (single_h->filter, prop_name, &val, NULL);
  num = gst_tensors_info_parse_names_string (gst_info, val);
  g_free (val);

  if (gst_info->num_tensors != num) {
    _ml_logw ("The number of tensor name is mismatched in filter.");
  }
}

/**
 * @brief Internal function to set the gst info in tensor-filter.
 */
static int
ml_single_set_gst_info (ml_single * single_h, const ml_tensors_info_h info)
{
  GstTensorsInfo gst_in_info, gst_out_info;
  int status = ML_ERROR_NONE;
  int ret = -EINVAL;

  _ml_error_report_return_continue_iferr
      (_ml_tensors_info_copy_from_ml (&gst_in_info, info),
      "Cannot fetch tensor-info from the given info parameter. Error code: %d",
      _ERRNO);

  ret = single_h->klass->set_input_info (single_h->filter, &gst_in_info,
      &gst_out_info);
  if (ret == 0) {
    _ml_error_report_return_continue_iferr
        (_ml_tensors_info_copy_from_gst (&single_h->in_info, &gst_in_info),
        "Fetching input information from the given single_h instance has failed with %d",
        _ERRNO);
    _ml_error_report_return_continue_iferr (_ml_tensors_info_copy_from_gst
        (&single_h->out_info, &gst_out_info),
        "Fetching output information from the given single_h instance has failed with %d",
        _ERRNO);
    __setup_in_out_tensors (single_h);
  } else if (ret == -ENOENT) {
    status = ML_ERROR_NOT_SUPPORTED;
  } else {
    status = ML_ERROR_INVALID_PARAMETER;
  }

  return status;
}

/**
 * @brief Set the info for input/output tensors
 */
static int
ml_single_set_inout_tensors_info (GObject * object,
    const gboolean is_input, ml_tensors_info_s * tensors_info)
{
  int status = ML_ERROR_NONE;
  GstTensorsInfo info;
  gchar *str_dim, *str_type, *str_name;
  const gchar *str_type_name, *str_name_name;
  const gchar *prefix;

  if (is_input) {
    prefix = INPUT_STR;
    str_type_name = CONCAT_MACRO_STR (INPUT_STR, TYPE_STR);
    str_name_name = CONCAT_MACRO_STR (INPUT_STR, NAME_STR);
  } else {
    prefix = OUTPUT_STR;
    str_type_name = CONCAT_MACRO_STR (OUTPUT_STR, TYPE_STR);
    str_name_name = CONCAT_MACRO_STR (OUTPUT_STR, NAME_STR);
  }

  _ml_error_report_return_continue_iferr
      (_ml_tensors_info_copy_from_ml (&info, tensors_info),
      "Cannot fetch tensor-info from the given information. Error code: %d",
      _ERRNO);

  /* Set input option */
  str_dim = gst_tensors_info_get_dimensions_string (&info);
  str_type = gst_tensors_info_get_types_string (&info);
  str_name = gst_tensors_info_get_names_string (&info);

  if (!str_dim || !str_type || !str_name) {
    if (!str_dim)
      _ml_error_report
          ("Cannot fetch specific tensor-info from the given information: cannot fetch tensor dimension information.");
    if (!str_type)
      _ml_error_report
          ("Cannot fetch specific tensor-info from the given information: cannot fetch tensor type information.");
    if (!str_name)
      _ml_error_report
          ("Cannot fetch specific tensor-info from the given information: cannot fetch tensor name information. Even if tensor names are not defined, this should be able to fetch a list of empty strings.");

    status = ML_ERROR_INVALID_PARAMETER;
  } else {
    g_object_set (object, prefix, str_dim, str_type_name, str_type,
        str_name_name, str_name, NULL);
  }

  g_free (str_dim);
  g_free (str_type);
  g_free (str_name);

  gst_tensors_info_free (&info);

  return status;
}

/**
 * @brief Internal static function to set tensors info in the handle.
 */
static gboolean
ml_single_set_info_in_handle (ml_single_h single, gboolean is_input,
    ml_tensors_info_s * tensors_info)
{
  int status;
  ml_single *single_h;
  ml_tensors_info_s *dest;
  gboolean configured = FALSE;
  gboolean is_valid = FALSE;
  GObject *filter_obj;

  single_h = (ml_single *) single;
  filter_obj = G_OBJECT (single_h->filter);

  if (is_input) {
    dest = &single_h->in_info;
    configured = single_h->klass->input_configured (single_h->filter);
  } else {
    dest = &single_h->out_info;
    configured = single_h->klass->output_configured (single_h->filter);
  }

  if (configured) {
    /* get configured info and compare with input info */
    GstTensorsInfo gst_info;
    ml_tensors_info_h info = NULL;

    ml_single_get_gst_info (single_h, is_input, &gst_info);
    _ml_tensors_info_create_from_gst (&info, &gst_info);

    gst_tensors_info_free (&gst_info);

    if (tensors_info && !ml_tensors_info_is_equal (tensors_info, info)) {
      /* given input info is not matched with configured */
      ml_tensors_info_destroy (info);
      if (is_input) {
        /* try to update tensors info */
        status = ml_single_update_info (single, tensors_info, &info);
        if (status != ML_ERROR_NONE)
          goto done;
      } else {
        goto done;
      }
    }

    ml_tensors_info_clone (dest, info);
    ml_tensors_info_destroy (info);
  } else if (tensors_info) {
    status =
        ml_single_set_inout_tensors_info (filter_obj, is_input, tensors_info);
    if (status != ML_ERROR_NONE)
      goto done;
    ml_tensors_info_clone (dest, tensors_info);
  }

  is_valid = ml_tensors_info_is_valid (dest);

done:
  return is_valid;
}

/**
 * @brief Internal function to create and initialize the single handle.
 */
static ml_single *
ml_single_create_handle (ml_nnfw_type_e nnfw)
{
  ml_single *single_h;
  GError *error;

  single_h = g_new0 (ml_single, 1);
  if (single_h == NULL)
    _ml_error_report_return (NULL,
        "Failed to allocate memory for the single_h handle. Out of memory?");

  single_h->filter = g_object_new (G_TYPE_TENSOR_FILTER_SINGLE, NULL);
  if (single_h->filter == NULL) {
    _ml_error_report
        ("Failed to create a new instance for filter. Out of memory?");
    g_free (single_h);
    return NULL;
  }

  single_h->magic = ML_SINGLE_MAGIC;
  single_h->timeout = SINGLE_DEFAULT_TIMEOUT;
  single_h->nnfw = nnfw;
  single_h->state = IDLE;
  single_h->thread = NULL;
  single_h->input = NULL;
  single_h->output = NULL;
  single_h->destroy_data_list = NULL;
  single_h->invoking = FALSE;

  _ml_tensors_info_initialize (&single_h->in_info);
  _ml_tensors_info_initialize (&single_h->out_info);
  _ml_tensors_rank_initialize (single_h->input_ranks);
  _ml_tensors_rank_initialize (single_h->output_ranks);
  g_mutex_init (&single_h->mutex);
  g_cond_init (&single_h->cond);

  single_h->klass = g_type_class_ref (G_TYPE_TENSOR_FILTER_SINGLE);
  if (single_h->klass == NULL) {
    _ml_error_report
        ("Failed to get class of the tensor-filter of single API. This binary is not compiled properly or required libraries are not loaded.");
    ml_single_close (single_h);
    return NULL;
  }

  single_h->thread =
      g_thread_try_new (NULL, invoke_thread, (gpointer) single_h, &error);
  if (single_h->thread == NULL) {
    _ml_error_report
        ("Failed to create the invoke thread of single API, g_thread_try_new has reported an error: %s.",
        error->message);
    g_clear_error (&error);
    ml_single_close (single_h);
    return NULL;
  }

  return single_h;
}

/**
 * @brief Validate arguments for open
 */
static int
_ml_single_open_custom_validate_arguments (ml_single_h * single,
    ml_single_preset * info)
{
  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'single' (ml_single_h *), is NULL. It should be a valid pointer to an instance of ml_single_h.");
  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'info' (ml_single_preset *), is NULL. It should be a valid pointer to a valid instance of ml_single_preset.");

  /* Validate input tensor info. */
  if (info->input_info && !ml_tensors_info_is_valid (info->input_info))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'info' (ml_single_preset *), is not valid. It has 'input_info' entry that cannot be validated. ml_tensors_info_is_valid(info->input_info) has failed while info->input_info exists.");

  /* Validate output tensor info. */
  if (info->output_info && !ml_tensors_info_is_valid (info->output_info))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'info' (ml_single_preset *), is not valid. It has 'output_info' entry that cannot be validated. ml_tensors_info_is_valid(info->output_info) has failed while info->output_info exists.");

  if (!info->models)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'info' (ml_single_preset *), is not valid. Its models entry if NULL (info->models is NULL).");

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to convert accelerator as tensor_filter property format.
 * @note returned value must be freed by the caller
 * @note More details on format can be found in gst_tensor_filter_install_properties() in tensor_filter_common.c.
 */
char *
_ml_nnfw_to_str_prop (const ml_nnfw_hw_e hw)
{
  const gchar *hw_name;
  const gchar *use_accl = "true:";
  gchar *str_prop = NULL;

  hw_name = get_accl_hw_str (_ml_nnfw_to_accl_hw (hw));
  str_prop = g_strdup_printf ("%s%s", use_accl, hw_name);

  return str_prop;
}

/**
 * @brief Opens an ML model with the custom options and returns the instance as a handle.
 */
int
ml_single_open_custom (ml_single_h * single, ml_single_preset * info)
{
  ml_single *single_h;
  GObject *filter_obj;
  int status = ML_ERROR_NONE;
  ml_tensors_info_s *in_tensors_info, *out_tensors_info;
  ml_nnfw_type_e nnfw;
  ml_nnfw_hw_e hw;
  const gchar *fw_name;
  gchar **list_models;
  guint num_models;
  char *hw_name;

  check_feature_state (ML_FEATURE_INFERENCE);

  /* Validate the params */
  _ml_error_report_return_continue_iferr
      (_ml_single_open_custom_validate_arguments (single, info),
      "The parameter, 'info' (ml_single_preset *), cannot be validated. Please provide valid information for this object.");

  /* init null */
  *single = NULL;

  in_tensors_info = (ml_tensors_info_s *) info->input_info;
  out_tensors_info = (ml_tensors_info_s *) info->output_info;
  nnfw = info->nnfw;
  hw = info->hw;
  fw_name = _ml_get_nnfw_subplugin_name (nnfw);

  /**
   * 1. Determine nnfw and validate model file
   */
  list_models = g_strsplit (info->models, ",", -1);
  num_models = g_strv_length (list_models);

  status = _ml_validate_model_file ((const char **) list_models, num_models,
      &nnfw);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_continue
        ("Cannot validate the model (1st model: %s. # models: %d). Error code: %d",
        list_models[0], num_models, status);
    g_strfreev (list_models);
    return status;
  }

  g_strfreev (list_models);

  /**
   * 2. Determine hw
   * (Supposed CPU only) Support others later.
   */
  if (!_ml_nnfw_is_available (nnfw, hw)) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "The given nnfw, '%s', is not supported. There is no corresponding tensor-filter subplugin available or the given hardware requirement is not supported for the given nnfw.",
        fw_name);
  }

                                        /** Create ml_single object */
  if ((single_h = ml_single_create_handle (nnfw)) == NULL) {
    _ml_error_report_return_continue (ML_ERROR_OUT_OF_MEMORY,
        "Cannot create handle for the given nnfw, %s", fw_name);
  }

  filter_obj = G_OBJECT (single_h->filter);

  /**
   * 3. Construct a direct connection with the nnfw.
   * Note that we do not construct a pipeline since 2019.12.
   */
  if (nnfw == ML_NNFW_TYPE_TENSORFLOW || nnfw == ML_NNFW_TYPE_SNAP ||
      nnfw == ML_NNFW_TYPE_PYTORCH || nnfw == ML_NNFW_TYPE_TRIX_ENGINE) {
    /* set input and output tensors information */
    if (in_tensors_info && out_tensors_info) {
      status =
          ml_single_set_inout_tensors_info (filter_obj, TRUE, in_tensors_info);
      if (status != ML_ERROR_NONE) {
        _ml_error_report_continue
            ("Input tensors info is given; however, failed to set input tensors info. Error code: %d",
            status);
        goto error;
      }

      status =
          ml_single_set_inout_tensors_info (filter_obj, FALSE,
          out_tensors_info);
      if (status != ML_ERROR_NONE) {
        _ml_error_report_continue
            ("Output tensors info is given; however, failed to set output tensors info. Error code: %d",
            status);
        goto error;
      }
    } else {
      _ml_error_report
          ("To run the given nnfw, '%s', with a neural network model, both input and output information should be provided.",
          fw_name);
      status = ML_ERROR_INVALID_PARAMETER;
      goto error;
    }
  } else if (nnfw == ML_NNFW_TYPE_ARMNN) {
    /* set input and output tensors information, if available */
    if (in_tensors_info) {
      status =
          ml_single_set_inout_tensors_info (filter_obj, TRUE, in_tensors_info);
      if (status != ML_ERROR_NONE) {
        _ml_error_report_continue
            ("With nnfw '%s', input tensors info is optional. However, the user has provided an invalid input tensors info. Error code: %d",
            fw_name, status);
        goto error;
      }
    }
    if (out_tensors_info) {
      status =
          ml_single_set_inout_tensors_info (filter_obj, FALSE,
          out_tensors_info);
      if (status != ML_ERROR_NONE) {
        _ml_error_report_continue
            ("With nnfw '%s', output tensors info is optional. However, the user has provided an invalid output tensors info. Error code: %d",
            fw_name, status);
        goto error;
      }
    }
  }

  /* set accelerator, framework, model files and custom option */
  if (info->fw_name) {
    fw_name = (const char *) info->fw_name;
  } else {
    fw_name = _ml_get_nnfw_subplugin_name (nnfw);       /* retry for "auto" */
  }
  hw_name = _ml_nnfw_to_str_prop (hw);
  g_object_set (filter_obj, "framework", fw_name, "accelerator", hw_name,
      "model", info->models, NULL);
  g_free (hw_name);

  if (info->custom_option) {
    g_object_set (filter_obj, "custom", info->custom_option, NULL);
  }

  /* 4. Start the nnfw to get inout configurations if needed */
  if (!single_h->klass->start (single_h->filter)) {
    _ml_error_report
        ("Failed to start NNFW, '%s', to get inout configurations. Subplugin class method has failed to start.",
        fw_name);
    status = ML_ERROR_STREAMS_PIPE;
    goto error;
  }

  if (nnfw == ML_NNFW_TYPE_NNTR_INF) {
    if (!in_tensors_info || !out_tensors_info) {
      if (!in_tensors_info) {
        ml_tensors_info_h in_info;
        status = ml_tensors_info_create (&in_info);
        if (status != ML_ERROR_NONE) {
          _ml_error_report_continue
              ("NNTrainer-inference-single cannot create tensors-info handle (ml_tensors_info_h) with ml_tensors_info_create. Error Code: %d",
              status);
          goto error;
        }

        /* ml_single_set_input_info() can't be done as it checks num_tensors */
        status = ml_single_set_gst_info (single_h, in_info);
        ml_tensors_info_destroy (in_info);
        if (status != ML_ERROR_NONE) {
          _ml_error_report_continue
              ("NNTrainer-inference-single cannot configure single_h handle instance with the given in_info. This might be an ML-API / NNTrainer internal error. Error Code: %d",
              status);
          goto error;
        }
      } else {
        status = ml_single_set_input_info (single_h, in_tensors_info);
        if (status != ML_ERROR_NONE) {
          _ml_error_report_continue
              ("NNTrainer-inference-single cannot configure single_h handle instance with the given in_info from the user. Error code: %d",
              status);
          goto error;
        }
      }
    }
  }

  /* 5. Set in/out configs and metadata */
  if (!ml_single_set_info_in_handle (single_h, TRUE, in_tensors_info)) {
    _ml_error_report
        ("The input tensors info is invalid. Cannot configure single_h handle with the given input tensors info.");
    status = ML_ERROR_INVALID_PARAMETER;
    goto error;
  }

  if (!ml_single_set_info_in_handle (single_h, FALSE, out_tensors_info)) {
    _ml_error_report
        ("The output tensors info is invalid. Cannot configure single_h handle with the given output tensors info.");
    status = ML_ERROR_INVALID_PARAMETER;
    goto error;
  }

  /* Setup input and output memory buffers for invoke */
  if (in_tensors_info && in_tensors_info->is_extended) {
    ml_tensors_info_create_extended (&single_h->in_tensors.info);
    _ml_tensors_set_rank (single_h->input_ranks, ML_TENSOR_RANK_LIMIT);
  } else {
    ml_tensors_info_create (&single_h->in_tensors.info);
    _ml_tensors_set_rank (single_h->input_ranks, ML_TENSOR_RANK_LIMIT_PREV);
  }

  if (out_tensors_info && out_tensors_info->is_extended) {
    ml_tensors_info_create_extended (&single_h->out_tensors.info);
    _ml_tensors_set_rank (single_h->output_ranks, ML_TENSOR_RANK_LIMIT);
  } else {
    ml_tensors_info_create (&single_h->out_tensors.info);
    _ml_tensors_set_rank (single_h->output_ranks, ML_TENSOR_RANK_LIMIT_PREV);
  }

  __setup_in_out_tensors (single_h);

  *single = single_h;
  return ML_ERROR_NONE;

error:
  ml_single_close (single_h);
  return status;
}

/**
 * @brief Opens an ML model and returns the instance as a handle.
 */
int
ml_single_open (ml_single_h * single, const char *model,
    const ml_tensors_info_h input_info, const ml_tensors_info_h output_info,
    ml_nnfw_type_e nnfw, ml_nnfw_hw_e hw)
{
  return ml_single_open_full (single, model, input_info, output_info, nnfw, hw,
      NULL);
}

/**
 * @brief Opens an ML model and returns the instance as a handle.
 */
int
ml_single_open_full (ml_single_h * single, const char *model,
    const ml_tensors_info_h input_info, const ml_tensors_info_h output_info,
    ml_nnfw_type_e nnfw, ml_nnfw_hw_e hw, const char *custom_option)
{
  ml_single_preset info = { 0, };

  info.input_info = input_info;
  info.output_info = output_info;
  info.nnfw = nnfw;
  info.hw = hw;
  info.models = (char *) model;
  info.custom_option = (char *) custom_option;

  return ml_single_open_custom (single, &info);
}

/**
 * @brief Open new single handle with given option.
 */
int
ml_single_open_with_option (ml_single_h * single, const ml_option_h option)
{
  ml_option_s *_option;
  GHashTable *table;
  GHashTableIter iter;
  gchar *key;
  ml_option_value_s *_option_value;
  ml_single_preset info = { 0, };

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!option) {
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'option' is NULL. It should be a valid ml_option_h, which should be created by ml_option_create().");
  }

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'single' (ml_single_h), is NULL. It should be a valid ml_single_h instance, usually created by ml_single_open().");

  _option = (ml_option_s *) option;
  table = _option->option_table;

  g_hash_table_iter_init (&iter, table);
  while (g_hash_table_iter_next (&iter, (gpointer *) & key,
          (gpointer *) & _option_value)) {
    if (g_ascii_strcasecmp (key, "input_info") == 0) {
      info.input_info = _option_value->value;
    } else if (g_ascii_strcasecmp (key, "output_info") == 0) {
      info.output_info = _option_value->value;
    } else if (g_ascii_strcasecmp (key, "nnfw") == 0) {
      info.nnfw = *((ml_nnfw_type_e *) _option_value->value);
    } else if (g_ascii_strcasecmp (key, "hw") == 0) {
      info.hw = *((ml_nnfw_hw_e *) _option_value->value);
    } else if (g_ascii_strcasecmp (key, "models") == 0) {
      info.models = (gchar *) _option_value->value;
    } else if (g_ascii_strcasecmp (key, "custom") == 0) {
      info.custom_option = (gchar *) _option_value->value;
    } else if (g_ascii_strcasecmp (key, "framework_name") == 0) {
      info.fw_name = (gchar *) _option_value->value;
    } else {
      _ml_logw ("Ignore unknown key for ml_option: %s", key);
    }
  }

  return ml_single_open_custom (single, &info);
}

/**
 * @brief Closes the opened model handle.
 *
 * @details State changes performed by this function:
 *          ANY STATE -> JOIN REQUESTED - on receiving a request to close
 *
 *          Once requested to close, invoke_thread() will exit after processing
 *          the current input (if any).
 */
int
ml_single_close (ml_single_h single)
{
  ml_single *single_h;
  gboolean invoking;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, 'single' (ml_single_h), is NULL. It should be a valid ml_single_h instance, usually created by ml_single_open().");

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 1);

  single_h->state = JOIN_REQUESTED;
  g_cond_broadcast (&single_h->cond);
  invoking = single_h->invoking;
  ML_SINGLE_HANDLE_UNLOCK (single_h);

  /** Wait until invoke process is finished */
  while (invoking) {
    _ml_logd ("Wait 1 ms until invoke is finished and close the handle.");
    g_usleep (1000);
    invoking = single_h->invoking;
    /**
     * single_h->invoking is the only protected value here and we are
     * doing a read-only operation and do not need to project its value
     * after the assignment.
     * Thus, we do not need to lock single_h here.
     */
  }

  if (single_h->thread != NULL)
    g_thread_join (single_h->thread);

  /** locking ensures correctness with parallel calls on close */
  if (single_h->filter) {
    g_list_foreach (single_h->destroy_data_list, __destroy_notify, single_h);
    g_list_free (single_h->destroy_data_list);

    if (single_h->klass)
      single_h->klass->stop (single_h->filter);

    g_object_unref (single_h->filter);
    single_h->filter = NULL;
  }

  if (single_h->klass) {
    g_type_class_unref (single_h->klass);
    single_h->klass = NULL;
  }

  _ml_tensors_info_free (&single_h->in_info);
  _ml_tensors_info_free (&single_h->out_info);

  g_cond_clear (&single_h->cond);
  g_mutex_clear (&single_h->mutex);

  g_free (single_h);
  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to validate input/output data.
 */
static int
_ml_single_invoke_validate_data (ml_single_h single,
    const ml_tensors_data_h data, const gboolean is_input)
{
  ml_single *single_h;
  ml_tensors_data_s *_data;
  ml_tensors_data_s *_model;
  guint i;
  size_t raw_size;

  single_h = (ml_single *) single;
  _data = (ml_tensors_data_s *) data;

  if (G_UNLIKELY (!_data))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "(internal function) The parameter, 'data' (const ml_tensors_data_h), is NULL. It should be a valid instance of ml_tensors_data_h.");

  if (is_input)
    _model = &single_h->in_tensors;
  else
    _model = &single_h->out_tensors;

  if (G_UNLIKELY (_data->num_tensors != _model->num_tensors))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "(internal function) The number of %s tensors is not compatible with model. Given: %u, Expected: %u.",
        (is_input) ? "input" : "output", _data->num_tensors,
        _model->num_tensors);

  for (i = 0; i < _data->num_tensors; i++) {
    if (G_UNLIKELY (!_data->tensors[i].tensor))
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The %d-th input tensor is not valid. There is no valid dimension metadata for this tensor.",
          i);

    raw_size = _model->tensors[i].size;
    if (G_UNLIKELY (_data->tensors[i].size != raw_size))
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The size of %d-th %s tensor is not compatible with model. Given: %zu, Expected: %zu (type: %d).",
          i, (is_input) ? "input" : "output", _data->tensors[i].size, raw_size,
          single_h->in_info.info[i].type);
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to invoke the model.
 *
 * @details State changes performed by this function:
 *          IDLE -> RUNNING - on receiving a valid request
 *
 *          Invoke returns error if the current state is not IDLE.
 *          If IDLE, then invoke is requested to the thread.
 *          Invoke waits for the processing to be complete, and returns back
 *          the result once notified by the processing thread.
 *
 * @note IDLE is the valid thread state before and after this function call.
 */
static int
_ml_single_invoke_internal (ml_single_h single,
    const ml_tensors_data_h input, ml_tensors_data_h * output,
    const gboolean need_alloc)
{
  ml_single *single_h;
  gint64 end_time;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (G_UNLIKELY (!single))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "(internal function) The parameter, single (ml_single_h), is NULL. It should be a valid instance of ml_single_h, usually created by ml_single_open().");

  if (G_UNLIKELY (!input))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "(internal function) The parameter, input (ml_tensors_data_h), is NULL. It should be a valid instance of ml_tensors_data_h.");

  if (G_UNLIKELY (!output))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "(internal functino) The parameter, output (ml_tensors_data_h *), is NULL. It should be a valid pointer to an instance of ml_tensors_data_h to store the inference results.");

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 0);

  if (G_UNLIKELY (!single_h->filter)) {
    _ml_error_report
        ("The tensor_filter element of this single handle (single_h) is not valid. It appears that the handle (ml_single_h single) is not appropriately created by ml_single_open(), user thread has touched its internal data, or the handle is already closed or freed by user.");
    status = ML_ERROR_INVALID_PARAMETER;
    goto exit;
  }

  /* Validate input/output data */
  status = _ml_single_invoke_validate_data (single, input, TRUE);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_continue
        ("The input data for the inference is not valid: error code %d. Please check the dimensions, type, number-of-tensors, and size information of the input data.",
        status);
    goto exit;
  }

  if (!need_alloc) {
    status = _ml_single_invoke_validate_data (single, *output, FALSE);
    if (status != ML_ERROR_NONE) {
      _ml_error_report_continue
          ("The output data buffer provided by the user is not valid for the given neural network mode: error code %d. Please check the dimensions, type, number-of-tensors, and size information of the output data buffer.",
          status);
      goto exit;
    }
  }

  if (single_h->state != IDLE) {
    if (G_UNLIKELY (single_h->state == JOIN_REQUESTED)) {
      _ml_error_report
          ("The handle (single_h single) is closed or being closed awaiting for the last ongoing invocation. Invoking with such a handle is not allowed. Please open another single_h handle to invoke.");
      status = ML_ERROR_STREAMS_PIPE;
      goto exit;
    }
    _ml_error_report
        ("The handle (single_h single) is busy. There is another thread waiting for inference results with this handle. Please retry invoking again later when the handle becomes idle after completing the current inference task.");
    status = ML_ERROR_TRY_AGAIN;
    goto exit;
  }

  /* prepare output data */
  if (need_alloc) {
    *output = NULL;

    status = _ml_tensors_data_clone_no_alloc (&single_h->out_tensors,
        &single_h->output);
    if (status != ML_ERROR_NONE)
      goto exit;
  } else {
    single_h->output = *output;
  }

  /**
   * Clone input data here to prevent use-after-free case.
   * We should release single_h->input after calling __invoke() function.
   */
  status = ml_tensors_data_clone (input, &single_h->input);
  if (status != ML_ERROR_NONE)
    goto exit;

  single_h->state = RUNNING;
  single_h->free_output = need_alloc;

  if (single_h->timeout > 0) {
    /* Wake up "invoke_thread" */
    g_cond_broadcast (&single_h->cond);

    /* set timeout */
    end_time = g_get_monotonic_time () +
        single_h->timeout * G_TIME_SPAN_MILLISECOND;

    if (g_cond_wait_until (&single_h->cond, &single_h->mutex, end_time)) {
      status = single_h->status;
    } else {
      _ml_logw ("Wait for invoke has timed out");
      status = ML_ERROR_TIMED_OUT;
      /** This is set to notify invoke_thread to not process if timed out */
      if (need_alloc)
        set_destroy_notify (single_h, single_h->output, TRUE);
    }
  } else {
    /**
     * Don't worry. We have locked single_h->mutex, thus there is no
     * other thread with ml_single_invoke function on the same handle
     * that are in this if-then-else block, which means that there is
     * no other thread with active invoke-thread (calling __invoke())
     * with the same handle. Thus we can call __invoke without
     * having yet another mutex for __invoke.
     */
    single_h->invoking = TRUE;
    status = __invoke (single_h, single_h->input, single_h->output);
    ml_tensors_data_destroy (single_h->input);
    single_h->input = NULL;
    single_h->invoking = FALSE;
    single_h->state = IDLE;

    if (status != ML_ERROR_NONE) {
      if (need_alloc)
        ml_tensors_data_destroy (single_h->output);
      goto exit;
    }

    __process_output (single_h, single_h->output);
  }

exit:
  if (status == ML_ERROR_NONE) {
    if (need_alloc)
      *output = single_h->output;
  }

  single_h->output = NULL;
  ML_SINGLE_HANDLE_UNLOCK (single_h);
  return status;
}

/**
 * @brief Invokes the model with the given input data.
 */
int
ml_single_invoke (ml_single_h single,
    const ml_tensors_data_h input, ml_tensors_data_h * output)
{
  return _ml_single_invoke_internal (single, input, output, TRUE);
}

/**
 * @brief Invokes the model with the given input data and fills the output data handle.
 */
int
ml_single_invoke_fast (ml_single_h single,
    const ml_tensors_data_h input, ml_tensors_data_h output)
{
  return _ml_single_invoke_internal (single, input, &output, FALSE);
}

/**
 * @brief Gets the tensors info for the given handle.
 * @param[out] info A pointer to a NULL (unallocated) instance.
 */
static int
ml_single_get_tensors_info (ml_single_h single, gboolean is_input,
    ml_tensors_info_h * info)
{
  ml_single *single_h;
  int status = ML_ERROR_NONE;
  ml_tensors_info_s *input_info;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "(internal function) The parameter, 'single' (ml_single_h), is NULL. It should be a valid ml_single_h instance, usually created by ml_single_open().");
  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "(internal function) The parameter, 'info' (ml_tensors_info_h *) is NULL. It should be a valid pointer to an empty (NULL) instance of ml_tensor_info_h, which is supposed to be filled with the fetched info by this function.");

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 0);

  /* allocate handle for tensors info */
  status = ml_tensors_info_create (info);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_continue
        ("(internal function) Failed to create an entry for the ml_tensors_info_h instance. Error code: %d",
        status);
    goto exit;
  }

  input_info = (ml_tensors_info_s *) (*info);

  if (is_input)
    status = ml_tensors_info_clone (input_info, &single_h->in_info);
  else
    status = ml_tensors_info_clone (input_info, &single_h->out_info);

  if (status != ML_ERROR_NONE) {
    _ml_error_report_continue
        ("(internal function) Failed to clone fetched input/output metadata to output pointer (ml_tensors_info *info). Error code: %d",
        status);
    ml_tensors_info_destroy (input_info);
  }

exit:
  ML_SINGLE_HANDLE_UNLOCK (single_h);
  return status;
}

/**
 * @brief Gets the information of required input data for the given handle.
 * @note information = (tensor dimension, type, name and so on)
 */
int
ml_single_get_input_info (ml_single_h single, ml_tensors_info_h * info)
{
  return ml_single_get_tensors_info (single, TRUE, info);
}

/**
 * @brief Gets the information of output data for the given handle.
 * @note information = (tensor dimension, type, name and so on)
 */
int
ml_single_get_output_info (ml_single_h single, ml_tensors_info_h * info)
{
  return ml_single_get_tensors_info (single, FALSE, info);
}

/**
 * @brief Sets the maximum amount of time to wait for an output, in milliseconds.
 */
int
ml_single_set_timeout (ml_single_h single, unsigned int timeout)
{
  ml_single *single_h;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, single (ml_single_h), is NULL. It should be a valid instance of ml_single_h, which is usually created by ml_single_open().");

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 0);

  single_h->timeout = (guint) timeout;

  ML_SINGLE_HANDLE_UNLOCK (single_h);
  return ML_ERROR_NONE;
}

/**
 * @brief Sets the information (tensor dimension, type, name and so on) of required input data for the given model.
 */
int
ml_single_set_input_info (ml_single_h single, const ml_tensors_info_h info)
{
  ml_single *single_h;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, single (ml_single_h), is NULL. It should be a valid instance of ml_single_h, which is usually created by ml_single_open().");
  if (!info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info (const ml_tensors_info_h), is NULL. It should be a valid instance of ml_tensors_info_h, which is usually created by ml_tensors_info_create() or other APIs.");

  if (!ml_tensors_info_is_valid (info))
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, info (const ml_tensors_info_h), is not valid. Although it is not NULL, the content of 'info' is invalid. If it is created by ml_tensors_info_create(), which creates an empty instance, it should be filled by users afterwards. Please check if 'info' has all elements filled with valid values.");

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 0);
  status = ml_single_set_gst_info (single_h, info);
  ML_SINGLE_HANDLE_UNLOCK (single_h);

  if (status != ML_ERROR_NONE)
    _ml_error_report_continue
        ("ml_single_set_gst_info() has failed to configure the single_h handle with the given info. Error code: %d",
        status);

  return status;
}

/**
 * @brief Invokes the model with the given input data with the given info.
 */
int
ml_single_invoke_dynamic (ml_single_h single,
    const ml_tensors_data_h input, const ml_tensors_info_h in_info,
    ml_tensors_data_h * output, ml_tensors_info_h * out_info)
{
  int status;
  ml_tensors_info_h cur_in_info = NULL;

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, single (ml_single_h), is NULL. It should be a valid instance of ml_single_h, which is usually created by ml_single_open().");
  if (!input)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, input (const ml_tensors_data_h), is NULL. It should be a valid instance of ml_tensors_data_h with input data frame for inference.");
  if (!in_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, in_info (const ml_tensors_info_h), is NULL. It should be a valid instance of ml_tensor_info_h that describes metadata of the given input for inference (input).");
  if (!output)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, output (ml_tensors_data_h *), is NULL. It should be a pointer to an empty (NULL or do-not-care) instance of ml_tensors_data_h, which is filled by this API with the result of inference.");
  if (!out_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, out_info (ml_tensors_info_h *), is NULL. It should be a pointer to an empty (NULL or do-not-care) instance of ml_tensors_info_h, which is filled by this API with the neural network model info.");

  /* init null */
  *output = NULL;
  *out_info = NULL;

  status = ml_single_get_input_info (single, &cur_in_info);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_continue
        ("Failed to get input metadata configured by the opened single_h handle instance. Error code: %d.",
        status);
    goto exit;
  }
  status = ml_single_update_info (single, in_info, out_info);
  if (status != ML_ERROR_NONE) {
    _ml_error_report_continue
        ("Failed to reconfigure the opened single_h handle instance with the updated input/output metadata. Error code: %d.",
        status);
    goto exit;
  }

  status = ml_single_invoke (single, input, output);
  if (status != ML_ERROR_NONE) {
    ml_single_set_input_info (single, cur_in_info);
    if (status != ML_ERROR_TRY_AGAIN) {
      /* If it's TRY_AGAIN, ml_single_invoke() has already gave enough info. */
      _ml_error_report_continue
          ("Invoking the given neural network has failed. Error code: %d.",
          status);
    }
  }

exit:
  if (cur_in_info)
    ml_tensors_info_destroy (cur_in_info);

  if (status != ML_ERROR_NONE) {
    if (*out_info) {
      ml_tensors_info_destroy (*out_info);
      *out_info = NULL;
    }
  }

  return status;
}

/**
 * @brief Sets the property value for the given model.
 */
int
ml_single_set_property (ml_single_h single, const char *name, const char *value)
{
  ml_single *single_h;
  int status = ML_ERROR_NONE;
  char *old_value = NULL;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, single (ml_single_h), is NULL. It should be a valid instance of ml_single_h, which is usually created by ml_single_open().");
  if (!name)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, name (const char *), is NULL. It should be a valid string representing a property key.");

  /* get old value, also check the property is updatable. */
  _ml_error_report_return_continue_iferr
      (ml_single_get_property (single, name, &old_value),
      "Cannot fetch the previous value for the given property name, '%s'. It appears that the property key, '%s', is invalid (not supported).",
      name, name);

  /* if sets same value, do not change. */
  if (old_value && value && g_ascii_strcasecmp (old_value, value) == 0) {
    g_free (old_value);
    return ML_ERROR_NONE;
  }

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 0);

  /* update property */
  if (g_str_equal (name, "is-updatable")) {
    if (!value)
      goto error;
    /* boolean */
    if (g_ascii_strcasecmp (value, "true") == 0) {
      if (g_ascii_strcasecmp (old_value, "true") != 0)
        g_object_set (G_OBJECT (single_h->filter), name, (gboolean) TRUE, NULL);
    } else if (g_ascii_strcasecmp (value, "false") == 0) {
      if (g_ascii_strcasecmp (old_value, "false") != 0)
        g_object_set (G_OBJECT (single_h->filter), name, (gboolean) FALSE,
            NULL);
    } else {
      _ml_error_report
          ("The property value, '%s', is not appropriate for a boolean property 'is-updatable'. It should be either 'true' or 'false'.",
          value);
      status = ML_ERROR_INVALID_PARAMETER;
    }
  } else if (g_str_equal (name, "input") || g_str_equal (name, "inputtype")
      || g_str_equal (name, "inputname") || g_str_equal (name, "output")
      || g_str_equal (name, "outputtype") || g_str_equal (name, "outputname")) {
    GstTensorsInfo gst_info;
    gboolean is_input = g_str_has_prefix (name, "input");
    guint num;

    if (!value)
      goto error;

    ml_single_get_gst_info (single_h, is_input, &gst_info);

    if (g_str_has_suffix (name, "type"))
      num = gst_tensors_info_parse_types_string (&gst_info, value);
    else if (g_str_has_suffix (name, "name"))
      num = gst_tensors_info_parse_names_string (&gst_info, value);
    else {
      guint *rank;
      gchar **str_dims;
      guint i;

      if (is_input) {
        rank = single_h->input_ranks;
      } else {
        rank = single_h->output_ranks;
      }

      str_dims = g_strsplit_set (value, ",.", -1);
      num = g_strv_length (str_dims);

      if (num > ML_TENSOR_SIZE_LIMIT) {
        _ml_error_report ("Invalid param, dimensions (%d) max (%d)\n",
            num, ML_TENSOR_SIZE_LIMIT);

        num = ML_TENSOR_SIZE_LIMIT;
      }

      for (i = 0; i < num; ++i) {
        rank[i] = gst_tensor_parse_dimension (str_dims[i],
            gst_info.info[i].dimension);
      }
      g_strfreev (str_dims);
    }

    if (num == gst_info.num_tensors) {
      ml_tensors_info_h ml_info;

      _ml_tensors_info_create_from_gst (&ml_info, &gst_info);

      /* change configuration */
      status = ml_single_set_gst_info (single_h, ml_info);

      ml_tensors_info_destroy (ml_info);
    } else {
      _ml_error_report
          ("The property value, '%s', is not appropriate for the given property key, '%s'. The API has failed to parse the given property value.",
          value, name);
      status = ML_ERROR_INVALID_PARAMETER;
    }

    gst_tensors_info_free (&gst_info);
  } else {
    g_object_set (G_OBJECT (single_h->filter), name, value, NULL);
  }
  goto done;
error:
  _ml_error_report
      ("The parameter, value (const char *), is NULL. It should be a valid string representing the value to be set for the given property key, '%s'",
      name);
  status = ML_ERROR_INVALID_PARAMETER;
done:
  ML_SINGLE_HANDLE_UNLOCK (single_h);

  g_free (old_value);
  return status;
}

/**
 * @brief Gets the property value for the given model.
 */
int
ml_single_get_property (ml_single_h single, const char *name, char **value)
{
  ml_single *single_h;
  int status = ML_ERROR_NONE;

  check_feature_state (ML_FEATURE_INFERENCE);

  if (!single)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, single (ml_single_h), is NULL. It should be a valid instance of ml_single_h, which is usually created by ml_single_open().");
  if (!name)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, name (const char *), is NULL. It should be a valid string representing a property key.");
  if (!value)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, value (const char *), is NULL. It should be a valid string representing the value to be set for the given property key, '%s'",
        name);

  /* init null */
  *value = NULL;

  ML_SINGLE_GET_VALID_HANDLE_LOCKED (single_h, single, 0);

  if (g_str_equal (name, "inputtype") || g_str_equal (name, "inputname") ||
      g_str_equal (name, "inputlayout") || g_str_equal (name, "outputtype") ||
      g_str_equal (name, "outputname") || g_str_equal (name, "outputlayout") ||
      g_str_equal (name, "accelerator") || g_str_equal (name, "custom")) {
    /* string */
    g_object_get (G_OBJECT (single_h->filter), name, value, NULL);
  } else if (g_str_equal (name, "is-updatable")) {
    gboolean bool_value = FALSE;

    /* boolean */
    g_object_get (G_OBJECT (single_h->filter), name, &bool_value, NULL);
    *value = (bool_value) ? g_strdup ("true") : g_strdup ("false");
  } else if (g_str_equal (name, "input") || g_str_equal (name, "output")) {
    gchar *dim_str = NULL;
    const guint *rank;
    gboolean is_input = g_str_has_prefix (name, "input");
    GstTensorsInfo gst_info;

    if (is_input) {
      rank = single_h->input_ranks;
    } else {
      rank = single_h->output_ranks;
    }

    ml_single_get_gst_info (single_h, is_input, &gst_info);

    if (gst_info.num_tensors > 0) {
      guint i;
      GString *dimensions = g_string_new (NULL);

      for (i = 0; i < gst_info.num_tensors; ++i) {
        dim_str =
            gst_tensor_get_rank_dimension_string (gst_info.info[i].dimension,
            *(rank + i));
        g_string_append (dimensions, dim_str);

        if (i < gst_info.num_tensors - 1) {
          g_string_append (dimensions, ",");
        }
        g_free (dim_str);
      }
      dim_str = g_string_free (dimensions, FALSE);
    } else {
      dim_str = g_strdup ("");
    }
    *value = dim_str;
  } else {
    _ml_error_report
        ("The property key, '%s', is not available for get_property and not recognized by the API. It should be one of {input, inputtype, inputname, inputlayout, output, outputtype, outputname, outputlayout, accelerator, custom, is-updatable}.",
        name);
    status = ML_ERROR_NOT_SUPPORTED;
  }

  ML_SINGLE_HANDLE_UNLOCK (single_h);
  return status;
}

/**
 * @brief Internal helper function to validate model files.
 */
static int
__ml_validate_model_file (const char *const *model,
    const unsigned int num_models, gboolean * is_dir)
{
  guint i;

  if (!model)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, model, is NULL. It should be a valid array of strings, where each string is a valid file path for a neural network model file.");
  if (num_models < 1)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, num_models, is 0. It should be the number of files for the given neural network model.");

  if (g_file_test (model[0], G_FILE_TEST_IS_DIR)) {
    *is_dir = TRUE;
    return ML_ERROR_NONE;
  }

  for (i = 0; i < num_models; i++) {
    if (!model[i] || !g_file_test (model[i], G_FILE_TEST_IS_REGULAR)) {
      _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
          "The given param, model path [%d] = \"%s\" is invalid or the file is not found or accessible.",
          i, _STR_NULL (model[i]));
    }
  }

  *is_dir = FALSE;

  return ML_ERROR_NONE;
}

/**
 * @brief Validates the nnfw model file.
 * @since_tizen 5.5
 * @param[in] model The path of model file.
 * @param[in/out] nnfw The type of NNFW.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported, or framework to support this model file is unavailable in the environment.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int
_ml_validate_model_file (const char *const *model,
    const unsigned int num_models, ml_nnfw_type_e * nnfw)
{
  int status = ML_ERROR_NONE;
  ml_nnfw_type_e detected = ML_NNFW_TYPE_ANY;
  gboolean is_dir = FALSE;
  gchar *pos, *fw_name;
  gchar **file_ext = NULL;
  guint i;

  if (!nnfw)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, nnfw, is NULL. It should be a valid pointer of ml_nnfw_type_e.");

  _ml_error_report_return_continue_iferr (__ml_validate_model_file (model,
          num_models, &is_dir),
      "The parameters, model and num_models, are not valid.");

  /**
   * @note detect-fw checks the file ext and returns proper fw name for given models.
   * If detected fw and given nnfw are same, we don't need to check the file extension.
   * If any condition for auto detection is added later, below code also should be updated.
   */
  fw_name = gst_tensor_filter_detect_framework (model, num_models, TRUE);
  detected = _ml_get_nnfw_type_by_subplugin_name (fw_name);
  g_free (fw_name);

  if (*nnfw == ML_NNFW_TYPE_ANY) {
    if (detected == ML_NNFW_TYPE_ANY) {
      _ml_error_report
          ("The given neural network model (1st path is \"%s\", and there are %d paths declared) has unknown or unsupported extension. Please check its corresponding neural network framework and try to specify it instead of \"ML_NNFW_TYPE_ANY\".",
          model[0], num_models);
      status = ML_ERROR_INVALID_PARAMETER;
    } else {
      _ml_logi ("The given model is supposed a %s model.",
          _ml_get_nnfw_subplugin_name (detected));
      *nnfw = detected;
    }

    goto done;
  } else if (is_dir && *nnfw != ML_NNFW_TYPE_NNFW) {
    /* supposed it is ONE if given model is directory */
    _ml_error_report
        ("The given model (1st path is \"%s\", and there are %d paths declared) is directory, which is allowed by \"NNFW (One Runtime)\" only, Please check the model and framework.",
        model[0], num_models);
    status = ML_ERROR_INVALID_PARAMETER;
    goto done;
  } else if (detected == *nnfw) {
    /* Expected framework, nothing to do. */
    goto done;
  }

  /* Handle mismatched case, check file extension. */
  file_ext = g_malloc0 (sizeof (char *) * (num_models + 1));
  for (i = 0; i < num_models; i++) {
    if ((pos = strrchr (model[i], '.')) == NULL) {
      _ml_error_report ("The given model [%d]=\"%s\" has invalid extension.", i,
          model[i]);
      status = ML_ERROR_INVALID_PARAMETER;
      goto done;
    }

    file_ext[i] = g_ascii_strdown (pos, -1);
  }

  /** @todo Make sure num_models is correct for each nnfw type */
  switch (*nnfw) {
    case ML_NNFW_TYPE_NNFW:
    case ML_NNFW_TYPE_TVM:
      /**
       * We cannot check the file ext with NNFW.
       * NNFW itself will validate metadata and model file.
       */
      break;
    case ML_NNFW_TYPE_MVNC:
    case ML_NNFW_TYPE_OPENVINO:
    case ML_NNFW_TYPE_EDGE_TPU:
      /**
       * @todo Need to check method to validate model
       * Although nnstreamer supports these frameworks,
       * ML-API implementation is not ready.
       */
      _ml_error_report
          ("Given NNFW is not supported by ML-API Inference.Single, yet, although it is supported by NNStreamer. If you have such NNFW integrated into your machine and want to access via ML-API, please update the corresponding implementation or report and discuss at github.com/nnstreamer/nnstreamer/issues.");
      status = ML_ERROR_NOT_SUPPORTED;
      break;
    case ML_NNFW_TYPE_VD_AIFW:
      if (!g_str_equal (file_ext[0], ".nb") &&
          !g_str_equal (file_ext[0], ".ncp") &&
          !g_str_equal (file_ext[0], ".tvn") &&
          !g_str_equal (file_ext[0], ".bin")) {
        status = ML_ERROR_INVALID_PARAMETER;
      }
      break;
    case ML_NNFW_TYPE_SNAP:
#if !defined (__ANDROID__)
      _ml_error_report ("SNAP is supported by Android/arm64-v8a devices only.");
      status = ML_ERROR_NOT_SUPPORTED;
#endif
      /* SNAP requires multiple files, set supported if model file exists. */
      break;
    case ML_NNFW_TYPE_ARMNN:
      if (!g_str_equal (file_ext[0], ".caffemodel") &&
          !g_str_equal (file_ext[0], ".tflite") &&
          !g_str_equal (file_ext[0], ".pb") &&
          !g_str_equal (file_ext[0], ".prototxt")) {
        _ml_error_report
            ("ARMNN accepts .caffemodel, .tflite, .pb, and .prototxt files only. Please support correct file extension. You have specified: \"%s\"",
            file_ext[0]);
        status = ML_ERROR_INVALID_PARAMETER;
      }
      break;
    case ML_NNFW_TYPE_MXNET:
      if (!g_str_equal (file_ext[0], ".params") &&
          !g_str_equal (file_ext[0], ".json")) {
        status = ML_ERROR_INVALID_PARAMETER;
      }
      break;
    default:
      _ml_error_report
          ("You have designated an incorrect neural network framework (out of bound).");
      status = ML_ERROR_INVALID_PARAMETER;
      break;
  }

done:
  if (status == ML_ERROR_NONE) {
    if (!_ml_nnfw_is_available (*nnfw, ML_NNFW_HW_ANY)) {
      status = ML_ERROR_NOT_SUPPORTED;
      _ml_error_report
          ("The subplugin for tensor-filter \"%s\" is not available. Please install the corresponding tensor-filter subplugin file (usually, \"libnnstreamer_filter_${NAME}.so\") at the correct path. Please use \"nnstreamer-check\" utility to check related configurations. If you do not have the utility ready, build and install \"confchk\", which is located at ${nnstreamer_source}/tools/development/confchk/ .",
          _ml_get_nnfw_subplugin_name (*nnfw));
    }
  } else {
    _ml_error_report
        ("The given model file, \"%s\" (1st of %d files), is invalid.",
        model[0], num_models);
  }

  g_strfreev (file_ext);
  return status;
}
