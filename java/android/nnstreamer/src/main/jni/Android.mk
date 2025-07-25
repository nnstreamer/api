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

# Path for external libraries
EXT_INCLUDE_PATH := $(LOCAL_PATH)/external/include
EXT_LIB_PATH := $(LOCAL_PATH)/external/lib/$(TARGET_ARCH_ABI)

# Set ML API Version
ML_API_VERSION  := 1.8.6
ML_API_VERSION_MAJOR := $(word 1,$(subst ., ,${ML_API_VERSION}))
ML_API_VERSION_MINOR := $(word 2,$(subst ., ,${ML_API_VERSION}))
ML_API_VERSION_MICRO := $(word 3,$(subst ., ,${ML_API_VERSION}))

#------------------------------------------------------
# API build option
#------------------------------------------------------
NNSTREAMER_API_OPTION := all

# support tensor-query and offloading
ENABLE_ML_OFFLOADING := false

# enable service api and mlops-agent
ENABLE_ML_SERVICE := false

# TensorFlow-Lite
ENABLE_TF_LITE := false

# SNAP (Samsung Neural Acceleration Platform)
ENABLE_SNAP := false

# NNFW (On-device neural network inference framework, Samsung Research)
ENABLE_NNFW := false

# SNPE (Snapdragon Neural Processing Engine)
ENABLE_SNPE := false

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
ENABLE_LLAMACPP := false

NNS_API_FLAGS := -DVERSION=\"$(ML_API_VERSION)\" -DVERSION_MAJOR=$(ML_API_VERSION_MAJOR) -DVERSION_MINOR=$(ML_API_VERSION_MINOR) -DVERSION_MICRO=$(ML_API_VERSION_MICRO) -Wno-c99-designator
NNS_SUBPLUGINS :=

ifeq ($(NNSTREAMER_API_OPTION),single)
NNS_API_FLAGS += -DNNS_SINGLE_ONLY=1
endif

#------------------------------------------------------
# features and source files for build option
#------------------------------------------------------
include $(NNSTREAMER_ROOT)/jni/nnstreamer.mk

ifeq ($(ENABLE_ML_OFFLOADING), true)
ifndef NNSTREAMER_EDGE_ROOT
$(error NNSTREAMER_EDGE_ROOT is not defined!)
endif

NNS_API_FLAGS += -DENABLE_NNSTREAMER_EDGE=1
include $(NNSTREAMER_EDGE_ROOT)/jni/nnstreamer-edge.mk
endif

ifeq ($(ENABLE_ML_SERVICE), true)
NNS_API_FLAGS += -DENABLE_ML_SERVICE=1

ifndef MLOPS_AGENT_ROOT
$(error MLOPS_AGENT_ROOT is not defined!)
endif

NNS_API_FLAGS += -DENABLE_ML_AGENT=1 -DDB_KEY_PREFIX=\"mlops-android\"
include $(MLOPS_AGENT_ROOT)/jni/mlops-agent.mk

ifeq ($(ENABLE_ML_OFFLOADING), true)
NNS_API_FLAGS += -DENABLE_ML_OFFLOADING=1
include $(LOCAL_PATH)/Android-curl.mk
endif
endif

ifeq ($(ENABLE_MQTT), true)
NNS_API_FLAGS += -DENABLE_MQTT=1
include $(LOCAL_PATH)/Android-paho-mqtt-c.mk
endif

ifeq ($(ENABLE_TF_LITE), true)
NNS_API_FLAGS += -DENABLE_TENSORFLOW_LITE=1
endif

ifeq ($(ENABLE_SNAP), true)
NNS_API_FLAGS += -DENABLE_SNAP=1
endif

ifeq ($(ENABLE_NNFW), true)
NNS_API_FLAGS += -DENABLE_NNFW_RUNTIME=1
endif

ifeq ($(ENABLE_SNPE), true)
NNS_API_FLAGS += -DENABLE_SNPE=1
endif

ifeq ($(ENABLE_QNN), true)
NNS_API_FLAGS += -DENABLE_QNN=1
endif

ifeq ($(ENABLE_PYTORCH), true)
NNS_API_FLAGS += -DENABLE_PYTORCH=1
endif

ifeq ($(ENABLE_MXNET), true)
NNS_API_FLAGS += -DENABLE_MXNET=1
endif

ifeq ($(ENABLE_LLAMA2C), true)
NNS_API_FLAGS += -DENABLE_LLAMA2C=1
endif

ifeq ($(ENABLE_LLAMACPP), true)
NNS_API_FLAGS += -DENABLE_LLAMACPP=1
endif

ifeq ($(ENABLE_FLATBUF), true)
NNS_API_FLAGS += -DENABLE_FLATBUF=1
endif

#------------------------------------------------------
# nnstreamer + ML API build
#------------------------------------------------------
include $(LOCAL_PATH)/Android-nnstreamer.mk

#------------------------------------------------------
# external libs and sub-plugins
#------------------------------------------------------
ifeq ($(ENABLE_TF_LITE),true)
include $(LOCAL_PATH)/Android-tensorflow-lite.mk
NNS_SUBPLUGINS += tensorflow-lite-subplugin
endif

ifeq ($(ENABLE_SNAP),true)
include $(LOCAL_PATH)/Android-snap.mk
NNS_SUBPLUGINS += snap-subplugin
endif

ifeq ($(ENABLE_NNFW),true)
include $(LOCAL_PATH)/Android-nnfw.mk
NNS_SUBPLUGINS += nnfw-subplugin
endif

ifeq ($(ENABLE_SNPE),true)
include $(LOCAL_PATH)/Android-snpe.mk
NNS_SUBPLUGINS += snpe-subplugin
endif

ifeq ($(ENABLE_QNN),true)
include $(LOCAL_PATH)/Android-qnn.mk
NNS_SUBPLUGINS += qnn-subplugin
endif

ifeq ($(ENABLE_PYTORCH),true)
include $(LOCAL_PATH)/Android-pytorch.mk
NNS_SUBPLUGINS += pytorch-subplugin
endif

ifeq ($(ENABLE_MXNET), true)
include $(LOCAL_PATH)/Android-mxnet.mk
NNS_SUBPLUGINS += mxnet-subplugin
endif

ifeq ($(ENABLE_LLAMA2C), true)
include $(LOCAL_PATH)/Android-llama2c.mk
NNS_SUBPLUGINS += llama2c-subplugin
endif

ifeq ($(ENABLE_LLAMACPP), true)
include $(LOCAL_PATH)/Android-llamacpp.mk
NNS_SUBPLUGINS += llamacpp-subplugin
endif

ifeq ($(ENABLE_FLATBUF),true)
include $(LOCAL_PATH)/Android-flatbuf.mk
NNS_SUBPLUGINS += flatbuffers-subplugin
endif

# Remove any duplicates.
NNS_SUBPLUGINS := $(sort $(NNS_SUBPLUGINS))

#------------------------------------------------------
# library for nnstreamer and ML API
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := nnstreamer_android

LOCAL_SRC_FILES := nnstreamer-native-common.c
LOCAL_C_INCLUDES := $(NNSTREAMER_INCLUDES) $(NNSTREAMER_CAPI_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_CFLAGS := -O3 -fPIC $(NNS_API_FLAGS)
LOCAL_WHOLE_STATIC_LIBRARIES := nnstreamer $(NNS_SUBPLUGINS)
LOCAL_SHARED_LIBRARIES := gstreamer_android
LOCAL_LDLIBS := -llog -landroid
ifneq ($(NNSTREAMER_API_OPTION),single)
# For amcsrc element
LOCAL_LDLIBS += -lmediandk
endif

include $(BUILD_SHARED_LIBRARY)

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

ifeq ($(ENABLE_ML_SERVICE),true)
LOCAL_SRC_FILES += \
    nnstreamer-native-service.c
endif
endif

LOCAL_C_INCLUDES := $(NNSTREAMER_INCLUDES) $(NNSTREAMER_CAPI_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_CFLAGS := -O3 -fPIC $(NNS_API_FLAGS)
LOCAL_SHARED_LIBRARIES := gstreamer_android nnstreamer_android
LOCAL_LDLIBS := -llog -landroid

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
GSTREAMER_EXTRA_DEPS     := $(GST_REQUIRED_DEPS) glib-2.0 gio-2.0 gmodule-2.0 orc-0.4 json-glib-1.0
GSTREAMER_EXTRA_LIBS     := $(GST_REQUIRED_LIBS) -liconv

ifeq ($(NNSTREAMER_API_OPTION),all)
GSTREAMER_EXTRA_LIBS += -lcairo
endif

ifeq ($(ENABLE_ML_SERVICE),true)
GSTREAMER_EXTRA_DEPS += sqlite3
endif

GSTREAMER_INCLUDE_FONTS := no
GSTREAMER_INCLUDE_CA_CERTIFICATES := no

include $(GSTREAMER_NDK_BUILD_PATH)/gstreamer-1.0.mk
