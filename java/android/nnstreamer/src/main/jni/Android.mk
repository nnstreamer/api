LOCAL_PATH := $(call my-dir)

ifndef GSTREAMER_ROOT_ANDROID
$(error GSTREAMER_ROOT_ANDROID is not defined!)
endif

ifndef NNSTREAMER_ROOT
$(error NNSTREAMER_ROOT is not defined!)
endif

ifndef ML_API_ROOT
$(error ML_API_ROOT is not defined!)
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/armv7
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/arm64
else ifeq ($(TARGET_ARCH_ABI),x86)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/x86
else ifeq ($(TARGET_ARCH_ABI),x86_64)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/x86_64
else
$(error Target arch ABI not supported: $(TARGET_ARCH_ABI))
endif

# Set ML API Version
ML_API_VERSION  := 1.8.6
ML_API_VERSION_MAJOR := $(word 1,$(subst ., ,${ML_API_VERSION}))
ML_API_VERSION_MINOR := $(word 2,$(subst ., ,${ML_API_VERSION}))
ML_API_VERSION_MICRO := $(word 3,$(subst ., ,${ML_API_VERSION}))

#------------------------------------------------------
# API build option
#------------------------------------------------------
include $(NNSTREAMER_ROOT)/jni/nnstreamer.mk

NNSTREAMER_API_OPTION := all

# tensor-query support
ENABLE_TENSOR_QUERY := true

ENABLE_TF_LITE ?= false
# TensorFlow Lite  (nnstreamer tf-lite sub-plugin)
ifeq ($(ENABLE_TF_LITE),true)

ifndef TFLITE_ROOT_ANDROID
$(error To enable the NNStreamer TensorFlow Lite sub-plugin, TFLITE_ROOT_ANDROID must be specified.)
endif #endif TFLITE_ROOT_ANDROID

ifneq ($(filter $(TARGET_ARCH_ABI), arm64-v8a x86_64),)
TFLITE_ROOT := $(TFLITE_ROOT_ANDROID)
_ENABLE_TF_LITE := true
else #ifeq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a x86),)
$(warning Warning: TensorFlow Lite is available only for the following ABIs: x86_64 and armv6-v8a.)
$(warning For the other ABIs, ENABLE_TF_LITE is overridden to false.)
_ENABLE_TF_LITE := false
endif #endif ($(filter $(TARGET_ARCH_ABI), arm64-v8a x86_64),)
endif #endif ($(ENABLE_TF_LITE),true)

# SNAP (Samsung Neural Acceleration Platform)
ENABLE_SNAP := false

# NNFW (On-device neural network inference framework, Samsung Research)
ENABLE_NNFW := false

ENABLE_SNPE ?= false
# SNPE (Snapdragon Neural Processing Engine)
ifeq ($(ENABLE_SNPE),true)
ifneq ($(filter $(TARGET_ARCH_ABI), arm64-v8a),)
_ENABLE_SNPE := true
else #ifeq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a x86 x86_64),)
$(warning Warning: SNPEv2 is available only for the ABI: armv6-v8a.)
$(warning For the other ABIs (i.e., $(TARGET_ARCH_ABI)), ENABLE_SNPE is overridden to false.)
_ENABLE_SNPE := false
endif #endif ($(filter $(TARGET_ARCH_ABI), arm64-v8a,)
endif #endif ($(ENABLE_SNPE),true)

# QNN
ENABLE_QNN := false

# PyTorch
ENABLE_PYTORCH := false

# MXNet
ENABLE_MXNET := false

# Converter/decoder sub-plugin for flatbuffers support
ENABLE_FLATBUF := false

# MQTT (paho.mqtt.c) support
ENABLE_MQTT := false

ifeq ($(ENABLE_SNAP),true)
ifeq ($(ENABLE_SNPE),true)
$(error DO NOT enable SNAP and SNPE both. The app would fail to use DSP or NPU runtime.)
endif
endif

ENABLE_LLAMA2C := false

NNS_API_FLAGS := -DVERSION=\"$(ML_API_VERSION)\" -DVERSION_MAJOR=$(ML_API_VERSION_MAJOR) -DVERSION_MINOR=$(ML_API_VERSION_MINOR) -DVERSION_MICRO=$(ML_API_VERSION_MICRO)
NNS_SUBPLUGINS :=

ifeq ($(NNSTREAMER_API_OPTION),single)
NNS_API_FLAGS += -DNNS_SINGLE_ONLY=1
endif

#------------------------------------------------------
# nnstreamer + ML API build
#------------------------------------------------------
include $(LOCAL_PATH)/Android-nnstreamer.mk

#------------------------------------------------------
# external libs and sub-plugins
#------------------------------------------------------
ifeq ($(_ENABLE_TF_LITE),true)
NNS_API_FLAGS += -DENABLE_TENSORFLOW_LITE=1
NNS_SUBPLUGINS += tensorflow-lite-subplugin

include $(LOCAL_PATH)/Android-tensorflow-lite.mk
endif

ifeq ($(ENABLE_SNAP),true)
NNS_API_FLAGS += -DENABLE_SNAP=1
NNS_SUBPLUGINS += snap-subplugin

include $(LOCAL_PATH)/Android-snap.mk
endif

ifeq ($(ENABLE_NNFW),true)
NNS_API_FLAGS += -DENABLE_NNFW_RUNTIME=1
NNS_SUBPLUGINS += nnfw-subplugin

include $(LOCAL_PATH)/Android-nnfw.mk
endif

ifeq ($(_ENABLE_SNPE),true)
NNS_API_FLAGS += -DENABLE_SNPE=1
NNS_SUBPLUGINS += snpe-subplugin

include $(LOCAL_PATH)/Android-snpe.mk
endif

ifeq ($(ENABLE_QNN),true)
NNS_API_FLAGS += -DENABLE_QNN=1
NNS_SUBPLUGINS += qnn-subplugin

include $(LOCAL_PATH)/Android-qnn.mk
endif

ifeq ($(ENABLE_PYTORCH),true)
NNS_API_FLAGS += -DENABLE_PYTORCH=1
NNS_SUBPLUGINS += pytorch-subplugin

include $(LOCAL_PATH)/Android-pytorch.mk
endif

ifeq ($(ENABLE_MXNET), true)
NNS_API_FLAGS += -DENABLE_MXNET=1
NNS_SUBPLUGINS += mxnet-subplugin

include $(LOCAL_PATH)/Android-mxnet.mk
endif

ifeq ($(ENABLE_LLAMA2C), true)
NNS_API_FLAGS += -DENABLE_LLAMA2C=1
NNS_SUBPLUGINS += llama2c-subplugin

include $(LOCAL_PATH)/Android-llama2c.mk
endif

ifneq ($(NNSTREAMER_API_OPTION),single)
ifeq ($(ENABLE_FLATBUF),true)
include $(LOCAL_PATH)/Android-flatbuf.mk
NNS_API_FLAGS += -DENABLE_FLATBUF=1
NNS_SUBPLUGINS += flatbuffers-subplugin
endif
endif

# Remove any duplicates.
NNS_SUBPLUGINS := $(sort $(NNS_SUBPLUGINS))

#------------------------------------------------------
# native code for api
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := nnstreamer-native

LOCAL_SRC_FILES := \
    nnstreamer-native-api.c \
    nnstreamer-native-singleshot.c

ifneq ($(NNSTREAMER_API_OPTION),single)
LOCAL_SRC_FILES += \
    nnstreamer-native-customfilter.c \
    nnstreamer-native-pipeline.c
endif

LOCAL_C_INCLUDES := $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_CFLAGS := -O3 -fPIC $(NNS_API_FLAGS)
LOCAL_STATIC_LIBRARIES := nnstreamer $(NNS_SUBPLUGINS)
LOCAL_SHARED_LIBRARIES := gstreamer_android
LOCAL_LDLIBS := -llog -landroid

ifneq ($(NNSTREAMER_API_OPTION),single)
# For amcsrc element
LOCAL_LDLIBS += -lmediandk
endif

include $(BUILD_SHARED_LIBRARY)

#------------------------------------------------------
# gstreamer for android
#------------------------------------------------------
GSTREAMER_NDK_BUILD_PATH := $(GSTREAMER_ROOT)/share/gst-android/ndk-build
include $(LOCAL_PATH)/Android-gst-plugins.mk

GST_BLOCKED_PLUGINS      := \
        fallbackswitch livesync rsinter rstracers \
        threadshare togglerecord cdg claxon dav1d rsclosedcaption \
        ffv1 fmp4 mp4 gif hsv lewton rav1e json rspng regex textwrap textahead \
        aws hlssink3 ndi rsonvif raptorq reqwest rsrtp rsrtsp webrtchttp rswebrtc uriplaylistbin \
        rsaudiofx rsvideofx

GSTREAMER_PLUGINS        := $(filter-out $(GST_BLOCKED_PLUGINS), $(GST_REQUIRED_PLUGINS))
GSTREAMER_EXTRA_DEPS     := $(GST_REQUIRED_DEPS) glib-2.0 gio-2.0 gmodule-2.0 orc-0.4
GSTREAMER_EXTRA_LIBS     := $(GST_REQUIRED_LIBS) -liconv

ifeq ($(NNSTREAMER_API_OPTION),all)
GSTREAMER_EXTRA_LIBS += -lcairo
endif

GSTREAMER_INCLUDE_FONTS := no
GSTREAMER_INCLUDE_CA_CERTIFICATES := no

include $(GSTREAMER_NDK_BUILD_PATH)/gstreamer-1.0.mk

#------------------------------------------------------
# NDK cpu-features
#------------------------------------------------------
$(call import-module, android/cpufeatures)
