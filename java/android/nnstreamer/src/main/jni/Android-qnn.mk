#------------------------------------------------------
# QNN
#------------------------------------------------------
LOCAL_PATH := $(call my-dir)

ifndef NNSTREAMER_ROOT
$(error NNSTREAMER_ROOT is not defined!)
endif

include $(NNSTREAMER_ROOT)/jni/nnstreamer.mk

QNN_DIR := $(LOCAL_PATH)/qnn

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
QNN_LIB_PATH := $(QNN_DIR)/lib
else
$(error Target arch ABI not supported: $(TARGET_ARCH_ABI))
endif

QNN_INCLUDES := $(QNN_DIR)/include/QNN

#------------------------------------------------------
# qnn-sdk (prebuilt shared library)
#------------------------------------------------------
QNN_PREBUILT_LIBS :=

include $(CLEAR_VARS)
LOCAL_MODULE := libQnnCpu
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnCpu.so
include $(PREBUILT_SHARED_LIBRARY)
QNN_PREBUILT_LIBS += libQnnCpu

include $(CLEAR_VARS)
LOCAL_MODULE := libQnnGpu
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnGpu.so
include $(PREBUILT_SHARED_LIBRARY)
QNN_PREBUILT_LIBS += libQnnGpu

include $(CLEAR_VARS)
LOCAL_MODULE := libQnnDsp
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnDsp.so
include $(PREBUILT_SHARED_LIBRARY)
QNN_PREBUILT_LIBS += libQnnDsp

include $(CLEAR_VARS)
LOCAL_MODULE := libQnnHtp
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnHtp.so
include $(PREBUILT_SHARED_LIBRARY)
QNN_PREBUILT_LIBS += libQnnHtp

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
