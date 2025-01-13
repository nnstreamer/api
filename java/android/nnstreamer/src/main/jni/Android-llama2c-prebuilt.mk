#------------------------------------------------------
# Llama2.c: Inference Llama 2 in one file of pure C
#   https://github.com/karpathy/llama2.c (original upstream repository)
#   https://github.com/nnsuite/llama2.c (NNSuite forked repository)
#
# This mk file defines a prebuilt shared library for the llama2.c module.
#------------------------------------------------------
ifndef LLAMA2C_DIR
$(error LLAMA2C_DIR is not defined!)
endif

LLAMA2C_LIB_PATH := $(LLAMA2C_DIR)/lib

#------------------------------------------------------
# The prebuilt shared library for llama2.c
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := llama2c
LOCAL_SRC_FILES := \
    $(LLAMA2C_DIR)/api.c \
    $(LLAMA2C_DIR)/sampler.c \
    $(LLAMA2C_DIR)/tokenizer.c \
    $(LLAMA2C_DIR)/transformer.c \
    $(LLAMA2C_DIR)/util.c
include $(BUILD_SHARED_LIBRARY)

LLAMA2C_PREBUILT_LIBS := llama2c
