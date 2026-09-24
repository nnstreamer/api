/**
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file        ml_api_customfilter_slow_allocator.c
 * @date        4 Sep 2026
 * @brief       Custom filter for the ML API unittests.
 * @see         https://github.com/nnstreamer/api
 * @author      MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug         No known bugs
 *
 * This copies the input into a buffer that the filter itself allocates, so a
 * single-shot handle using it takes the "allocate_in_invoke" path. Each invoke
 * deliberately takes longer than a short ml_single_set_timeout(), which lets
 * the unittests reach the timeout handling without depending on the machine.
 */

#include <string.h>
#include <glib.h>
#include <tensor_filter_custom.h>
#include <nnstreamer_plugin_api.h>
#include <nnstreamer_util.h>

#define INVOKE_DELAY_USEC (200000U)

/**
 * @brief init callback of tensor_filter custom
 */
static void *
pt_init (const GstTensorFilterProperties * prop)
{
  UNUSED (prop);
  return g_new0 (guint, 1);
}

/**
 * @brief exit callback of tensor_filter custom
 */
static void
pt_exit (void *private_data, const GstTensorFilterProperties * prop)
{
  UNUSED (prop);
  g_free (private_data);
}

/**
 * @brief setInputDimension callback of tensor_filter custom
 */
static int
set_inputDim (void *private_data, const GstTensorFilterProperties * prop,
    const GstTensorsInfo * in_info, GstTensorsInfo * out_info)
{
  UNUSED (private_data);
  UNUSED (prop);

  gst_tensors_info_copy (out_info, in_info);
  return 0;
}

/**
 * @brief allocate-invoke callback of tensor_filter custom
 */
static int
pt_allocate_invoke (void *private_data, const GstTensorFilterProperties * prop,
    const GstTensorMemory * input, GstTensorMemory * output)
{
  GstTensorsInfo *out_meta, *in_meta;
  guint i;
  UNUSED (private_data);

  if (prop->input_meta.num_tensors != prop->output_meta.num_tensors)
    return -1;

  out_meta = (GstTensorsInfo *) & prop->output_meta;
  in_meta = (GstTensorsInfo *) & prop->input_meta;

  g_usleep (INVOKE_DELAY_USEC);

  for (i = 0; i < out_meta->num_tensors; i++) {
    GstTensorInfo *_out = gst_tensors_info_get_nth_info (out_meta, i);
    GstTensorInfo *_in = gst_tensors_info_get_nth_info (in_meta, i);
    gsize size = gst_tensor_info_get_size (_out);
    gsize in_size = gst_tensor_info_get_size (_in);

    output[i].data = g_malloc (size);
    memcpy (output[i].data, input[i].data, MIN (size, in_size));
  }

  return 0;
}

/**
 * @brief destroy-notify callback of tensor_filter custom
 */
static void
pt_destroy_notify (void *data)
{
  g_free (data);
}

/**
 * @brief tensor_filter custom subplugin definition
 */
static NNStreamer_custom_class NNStreamer_custom_body = {
  .initfunc = pt_init,
  .exitfunc = pt_exit,
  .setInputDim = set_inputDim,
  .allocate_invoke = pt_allocate_invoke,
  .destroy_notify = pt_destroy_notify,
};

/* The dyn-loaded object */
NNStreamer_custom_class *NNStreamer_custom = &NNStreamer_custom_body;
