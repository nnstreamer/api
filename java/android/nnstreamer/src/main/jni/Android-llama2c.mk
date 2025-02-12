#------------------------------------------------------
# Llama2.c: Inference Llama 2 in one file of pure C
#   https://github.com/karpathy/llama2.c (original upstream repository)
#   https://github.com/nnsuite/llama2.c (NNSuite forked repository)
#
# This mk file defines a library for the llama2.c module.
#------------------------------------------------------
LOCAL_PATH := $(call my-dir)

ifndef LLAMA2C_DIR
$(error LLAMA2C_DIR is not defined!)
endif

LLAMA2C_INCLUDES := $(LLAMA2C_DIR)/include

#------------------------------------------------------
# Build static library for llama2.c
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := llama2c
LOCAL_SRC_FILES := \
    $(LLAMA2C_DIR)/api.c \
    $(LLAMA2C_DIR)/sampler.c \
    $(LLAMA2C_DIR)/tokenizer.c \
    $(LLAMA2C_DIR)/transformer.c \
    $(LLAMA2C_DIR)/util.c
LOCAL_CFLAGS := -O3 -fPIC

include $(BUILD_STATIC_LIBRARY)

#------------------------------------------------------
# tensor-filter sub-plugin for llama2.c
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := llama2c-subplugin
LOCAL_SRC_FILES := $(NNSTREAMER_FILTER_LLAMA2C_SRCS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS)
LOCAL_C_INCLUDES := $(LLAMA2C_INCLUDES) $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_STATIC_LIBRARIES := nnstreamer llama2c

include $(BUILD_STATIC_LIBRARY)
