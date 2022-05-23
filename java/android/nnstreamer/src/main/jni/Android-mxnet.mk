LOCAL_PATH := $(call my-dir)

MXNET_DIR := $(LOCAL_PATH)/mxnet
MXNET_INCLUDES := $(MXNET_DIR)/include
MXNET_LIB_PATH := $(MXNET_DIR)/lib

$(info MXNET_DIR is $(MXNET_DIR))
$(info MXNET_INCLUDES is $(MXNET_INCLUDES))

#------------------------------------------------------
# mxnet prebuilt shared libraries
#------------------------------------------------------
include $(LOCAL_PATH)/Android-mxnet-prebuilt.mk

#------------------------------------------------------
# tensor-filter sub-plugin for mxnet
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := mxnet-subplugin
LOCAL_SRC_FILES := $(NNSTREAMER_FILTER_MXNET_SRCS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS)
LOCAL_C_INCLUDES := $(MXNET_INCLUDES) $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_STATIC_LIBRARIES := nnstreamer
LOCAL_SHARED_LIBRARIES := $(MXNET_PREBUILT_LIBS)

include $(BUILD_STATIC_LIBRARY)
