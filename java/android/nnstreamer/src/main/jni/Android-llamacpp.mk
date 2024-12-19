LOCAL_PATH := $(call my-dir)

LLAMACPP_DIR := $(LOCAL_PATH)/llamacpp
LLAMACPP_INCLUDES := $(LLAMACPP_DIR)/include
LLAMACPP_LIB_PATH := $(LLAMACPP_DIR)/lib

#------------------------------------------------------
# Import LLAMACPP_PREBUILT_LIBS
#------------------------------------------------------
include $(LOCAL_PATH)/Android-llamacpp-prebuilt.mk

#------------------------------------------------------
# tensor-filter sub-plugin for llamacpp
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := llamacpp-subplugin
LOCAL_SRC_FILES := $(NNSTREAMER_FILTER_LLAMACPP_SRCS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS)
LOCAL_C_INCLUDES := $(LLAMACPP_INCLUDES) $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_SHARED_LIBRARIES := $(LLAMACPP_PREBUILT_LIBS)
LOCAL_STATIC_LIBRARIES := nnstreamer

include $(BUILD_STATIC_LIBRARY)
