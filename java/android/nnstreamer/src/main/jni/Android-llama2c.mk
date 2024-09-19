LOCAL_PATH := $(call my-dir)

LLAMA2C_DIR := $(LOCAL_PATH)/llama2.c
LLAMA2C_INCLUDES := $(LLAMA2C_DIR)/include
LLAMA2C_LIB_PATH := $(LLAMA2C_DIR)/lib

#------------------------------------------------------
# Import LLAMA2C_PREBUILT_LIBS
#------------------------------------------------------
include $(LOCAL_PATH)/Android-llama2c-prebuilt.mk

#------------------------------------------------------
# tensor-filter sub-plugin for llama2.c
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := llama2c-subplugin
LOCAL_SRC_FILES := $(NNSTREAMER_FILTER_LLAMA2C_SRCS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS)
LOCAL_C_INCLUDES := $(LLAMA2C_INCLUDES) $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_STATIC_LIBRARIES := nnstreamer
LOCAL_SHARED_LIBRARIES := $(LLAMA2C_PREBUILT_LIBS)

include $(BUILD_STATIC_LIBRARY)
