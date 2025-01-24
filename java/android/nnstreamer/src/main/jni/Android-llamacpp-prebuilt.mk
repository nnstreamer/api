#------------------------------------------------------
# llama.cpp : Inference of Meta's LLaMA model (and others) in pure C/C++
#   https://github.com/ggerganov/llama.cpp (original upstream repository
#
# This mk file defines a prebuilt shared library for the llamacpp module.
#------------------------------------------------------
ifndef LLAMACPP_LIB_PATH
$(error LLAMACPP_LIB_PATH is not defined!)
endif

#------------------------------------------------------
# llamacpp prebuilt shared libraries
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := llama
LOCAL_SRC_FILES := $(LLAMACPP_LIB_PATH)/libllama.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ggml
LOCAL_SRC_FILES := $(LLAMACPP_LIB_PATH)/libggml.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ggml-cpu
LOCAL_SRC_FILES := $(LLAMACPP_LIB_PATH)/libggml-cpu.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ggml-base
LOCAL_SRC_FILES := $(LLAMACPP_LIB_PATH)/libggml-base.so
include $(PREBUILT_SHARED_LIBRARY)

LLAMACPP_PREBUILT_LIBS := llama ggml ggml-cpu ggml-base
