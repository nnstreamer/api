/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-inference-internal.c
 * @date 19 October 2021
 * @brief ML-API Internal Utility Functions for inference implementations
 * @see	https://github.com/nnstreamer/api
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <string.h>

#include <nnstreamer_internal.h>
#include <nnstreamer_log.h>
#include <nnstreamer_plugin_api.h>
#include <nnstreamer_plugin_api_filter.h>
#include <tensor_typedef.h>
#include "ml-api-inference-internal.h"
#include "ml-api-internal.h"

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
  NULL
};

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
 * @brief Allocates a tensors information handle from gst info.
 */
int
_ml_tensors_info_create_from_gst (ml_tensors_info_h * ml_info,
    GstTensorsInfo * gst_info)
{
  if (!ml_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, ml_info, is NULL. It should be a valid ml_tensors_info_h instance usually created by ml_tensors_info_create(). This could be an internal bug of ML API.");

  if (!gst_info)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This could be an internal bug of ML API.");

  _ml_error_report_return_continue_iferr (ml_tensors_info_create (ml_info),
      "The call to ml_tensors_info_create has failed with %d.", _ERRNO);

  _ml_tensors_info_copy_from_gst (*ml_info, gst_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Copies tensor meta info from gst tensors info.
 * @bug Thread safety required. Check its internal users first!
 */
void
_ml_tensors_info_copy_from_gst (ml_tensors_info_s * ml_info,
    const GstTensorsInfo * gst_info)
{
  guint i, j;
  guint max_dim;

  if (!ml_info)
    _ml_error_report_return ((void) (NULL),
        "The parmater, ml_info, is NULL. It should be a valid ml_tensors_info_s instance, usually created by ml_tensors_info_create(). This is probably an internal bug of ML API.");
  if (!gst_info)
    _ml_error_report_return ((void) (NULL),
        "The parmater, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This is probably an internal bug of ML API.");

  _ml_tensors_info_initialize (ml_info);

  max_dim = MIN (ML_TENSOR_RANK_LIMIT, NNS_TENSOR_RANK_LIMIT);

  ml_info->num_tensors = gst_info->num_tensors;

  for (i = 0; i < gst_info->num_tensors; i++) {
    /* Copy name string */
    if (gst_info->info[i].name) {
      ml_info->info[i].name = g_strdup (gst_info->info[i].name);
    }

    /* Set tensor type */
    switch (gst_info->info[i].type) {
      case _NNS_INT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT32;
        break;
      case _NNS_UINT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT32;
        break;
      case _NNS_INT16:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT16;
        break;
      case _NNS_UINT16:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT16;
        break;
      case _NNS_INT8:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT8;
        break;
      case _NNS_UINT8:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT8;
        break;
      case _NNS_FLOAT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_FLOAT64;
        break;
      case _NNS_FLOAT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_FLOAT32;
        break;
      case _NNS_INT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT64;
        break;
      case _NNS_UINT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT64;
        break;
      default:
        ml_info->info[i].type = ML_TENSOR_TYPE_UNKNOWN;
        break;
    }

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      ml_info->info[i].dimension[j] = gst_info->info[i].dimension[j];
    }

    for (; j < ML_TENSOR_RANK_LIMIT; j++) {
      ml_info->info[i].dimension[j] = 1;
    }
  }
}

/**
 * @brief Copies tensor meta info from gst tensors info.
 * @bug Thread safety required. Check its internal users first!
 */
void
_ml_tensors_info_copy_from_ml (GstTensorsInfo * gst_info,
    const ml_tensors_info_s * ml_info)
{
  guint i, j;
  guint max_dim;

  if (!ml_info)
    _ml_error_report_return ((void) (NULL),
        "The parmater, ml_info, is NULL. It should be a valid ml_tensors_info_s instance, usually created by ml_tensors_info_create(). This is probably an internal bug of ML API.");
  if (!gst_info)
    _ml_error_report_return ((void) (NULL),
        "The parmater, gst_info, is NULL. It should be a valid GstTensorsInfo instance. This is probably an internal bug of ML API.");

  G_LOCK_UNLESS_NOLOCK (*ml_info);

  gst_tensors_info_init (gst_info);
  max_dim = MIN (ML_TENSOR_RANK_LIMIT, NNS_TENSOR_RANK_LIMIT);

  gst_info->num_tensors = ml_info->num_tensors;

  for (i = 0; i < ml_info->num_tensors; i++) {
    /* Copy name string */
    if (ml_info->info[i].name) {
      gst_info->info[i].name = g_strdup (ml_info->info[i].name);
    }

    /* Set tensor type */
    switch (ml_info->info[i].type) {
      case ML_TENSOR_TYPE_INT32:
        gst_info->info[i].type = _NNS_INT32;
        break;
      case ML_TENSOR_TYPE_UINT32:
        gst_info->info[i].type = _NNS_UINT32;
        break;
      case ML_TENSOR_TYPE_INT16:
        gst_info->info[i].type = _NNS_INT16;
        break;
      case ML_TENSOR_TYPE_UINT16:
        gst_info->info[i].type = _NNS_UINT16;
        break;
      case ML_TENSOR_TYPE_INT8:
        gst_info->info[i].type = _NNS_INT8;
        break;
      case ML_TENSOR_TYPE_UINT8:
        gst_info->info[i].type = _NNS_UINT8;
        break;
      case ML_TENSOR_TYPE_FLOAT64:
        gst_info->info[i].type = _NNS_FLOAT64;
        break;
      case ML_TENSOR_TYPE_FLOAT32:
        gst_info->info[i].type = _NNS_FLOAT32;
        break;
      case ML_TENSOR_TYPE_INT64:
        gst_info->info[i].type = _NNS_INT64;
        break;
      case ML_TENSOR_TYPE_UINT64:
        gst_info->info[i].type = _NNS_UINT64;
        break;
      default:
        gst_info->info[i].type = _NNS_END;
        break;
    }

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      gst_info->info[i].dimension[j] = ml_info->info[i].dimension[j];
    }

    for (; j < NNS_TENSOR_RANK_LIMIT; j++) {
      gst_info->info[i].dimension[j] = 1;
    }
  }
  G_UNLOCK_UNLESS_NOLOCK (*ml_info);
}

/**
 * @brief Initializes the GStreamer library. This is internal function.
 */
int
_ml_initialize_gstreamer (void)
{
  GError *err = NULL;

  if (!gst_init_check (NULL, NULL, &err)) {
    if (err) {
      _ml_error_report
          ("Initrializing ML-API failed: GStreamer has the following error from gst_init_check(): %s",
          err->message);
      g_clear_error (&err);
    } else {
      _ml_error_report ("Cannot initialize GStreamer. Unknown reason.");
    }

    return ML_ERROR_STREAMS_PIPE;
  }

  return ML_ERROR_NONE;
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
          i, GST_STR_NULL (model[i]));
    }
  }

  *is_dir = FALSE;

  return ML_ERROR_NONE;
}

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
          GST_STR_NULL (name));
  } else {
    nnfw_type = (ml_nnfw_type_e) idx;
  }

  return nnfw_type;
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
            ("ARMNN accepts .caffemodel, .tflite, .pb, and .probotxt files only. Please support correct file extension. You have specified: \"%s\"",
            file_ext[0]);
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
 * @brief Checks the element is registered and available on the pipeline.
 */
int
ml_check_element_availability (const char *element_name, bool *available)
{
  GstElementFactory *factory;
  int status;

  check_feature_state ();

  if (!element_name)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, element_name, is NULL. It should be a name (string) to be queried if it exists as a GStreamer/NNStreamer element.");

  if (!available)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, available, is NULL. It should be a valid pointer to a bool entry so that the API (ml_check_element_availability) may return the queried result via \"available\" parameter. E.g., bool available; ml_check_element_availability (\"tensor_converter\", &available);");

  _ml_error_report_return_continue_iferr (_ml_initialize_gstreamer (),
      "Internal error of _ml_initialize_gstreamer(). Check the availability of gstreamer libraries in your system.");

  /* init false */
  *available = false;

  factory = gst_element_factory_find (element_name);
  if (factory) {
    GstPluginFeature *feature = GST_PLUGIN_FEATURE (factory);
    const gchar *plugin_name = gst_plugin_feature_get_plugin_name (feature);

    /* check restricted element */
    status = _ml_check_plugin_availability (plugin_name, element_name);
    if (status == ML_ERROR_NONE)
      *available = true;

    gst_object_unref (factory);
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Checks the availability of the plugin.
 */
int
_ml_check_plugin_availability (const char *plugin_name,
    const char *element_name)
{
  static gboolean list_loaded = FALSE;
  static gchar **restricted_elements = NULL;

  if (!plugin_name)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, plugin_name, is NULL. It should be a valid string.");

  if (!element_name)
    _ml_error_report_return (ML_ERROR_INVALID_PARAMETER,
        "The parameter, element_name, is NULL. It should be a valid string.");

  if (!list_loaded) {
    gboolean restricted;

    restricted =
        nnsconf_get_custom_value_bool ("element-restriction",
        "enable_element_restriction", FALSE);
    if (restricted) {
      gchar *elements;

      /* check white-list of available plugins */
      elements =
          nnsconf_get_custom_value_string ("element-restriction",
          "restricted_elements");
      if (elements) {
        restricted_elements = g_strsplit_set (elements, " ,;", -1);
        g_free (elements);
      }
    }

    list_loaded = TRUE;
  }

  /* nnstreamer elements */
  if (g_str_has_prefix (plugin_name, "nnstreamer") &&
      g_str_has_prefix (element_name, "tensor_")) {
    return ML_ERROR_NONE;
  }

  if (restricted_elements &&
      find_key_strv ((const gchar **) restricted_elements, element_name) < 0) {
    _ml_error_report_return (ML_ERROR_NOT_SUPPORTED,
        "The element %s is restricted.", element_name);
  }

  return ML_ERROR_NONE;
}
