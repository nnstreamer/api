#------------------------------------------------------
# QNN
#------------------------------------------------------
LOCAL_PATH := $(call my-dir)

ifndef NNSTREAMER_ROOT
$(error NNSTREAMER_ROOT is not defined!)
endif

include $(NNSTREAMER_ROOT)/jni/nnstreamer.mk

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
QNN_LIB_PATH := $(EXT_LIB_PATH)
else
$(error Target arch ABI not supported: $(TARGET_ARCH_ABI))
endif

QNN_INCLUDES := $(EXT_INCLUDE_PATH)/QNN

#------------------------------------------------------
# qnn-sdk (prebuilt shared library)
#------------------------------------------------------
include $(LOCAL_PATH)/Android-qnn-prebuilt.mk

#------------------------------------------------------
# tensor-filter sub-plugin for qnn
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := qnn-subplugin
LOCAL_SRC_FILES := $(NNSTREAMER_FILTER_QNN_SRCS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS)
LOCAL_C_INCLUDES := $(QNN_INCLUDES) $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_STATIC_LIBRARIES := nnstreamer
LOCAL_SHARED_LIBRARIES := $(QNN_PREBUILT_LIBS)

include $(BUILD_STATIC_LIBRARY)
