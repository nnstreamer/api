#------------------------------------------------------
# nnstreamer + ML API
#------------------------------------------------------
LOCAL_PATH := $(call my-dir)

ifndef NNSTREAMER_ROOT
$(error NNSTREAMER_ROOT is not defined!)
endif

ifndef ML_API_ROOT
$(error ML_API_ROOT is not defined!)
endif

# nnstreamer c-api
NNSTREAMER_CAPI_INCLUDES := \
    $(NNSTREAMER_ROOT)/gst/nnstreamer/tensor_filter \
    $(ML_API_ROOT)/c/include/platform \
    $(ML_API_ROOT)/c/include \
    $(ML_API_ROOT)/c/src

# nnstreamer and single-shot api
NNSTREAMER_SRC_FILES := \
    $(NNSTREAMER_COMMON_SRCS) \
    $(ML_API_ROOT)/c/src/ml-api-common.c \
    $(ML_API_ROOT)/c/src/ml-api-inference-internal.c \
    $(ML_API_ROOT)/c/src/ml-api-inference-single.c

# pipeline api and nnstreamer plugins
ifneq ($(NNSTREAMER_API_OPTION),single)
NNSTREAMER_SRC_FILES += \
    $(ML_API_ROOT)/c/src/ml-api-inference-pipeline.c \
    $(NNSTREAMER_PLUGINS_SRCS) \
    $(NNSTREAMER_SOURCE_AMC_SRCS) \
    $(NNSTREAMER_DECODER_BB_SRCS) \
    $(NNSTREAMER_DECODER_DV_SRCS) \
    $(NNSTREAMER_DECODER_IL_SRCS) \
    $(NNSTREAMER_DECODER_PE_SRCS) \
    $(NNSTREAMER_DECODER_IS_SRCS) \
    $(NNSTREAMER_JOIN_SRCS)

# TODO: Add nnsquery prebuilt-lib and enable mqtt-hybrid
ifeq ($(ENABLE_MQTT), true)
NNSTREAMER_SRC_FILES += \
    $(NNSTREAMER_MQTT_SRCS)

NNS_API_FLAGS += -DENABLE_MQTT=1

PAHO_MQTT_C_DIR := $(LOCAL_PATH)/paho-mqtt-c
PAHO_MQTT_C_INCLUDES := $(PAHO_MQTT_C_DIR)/include

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
PAHO_MQTT_C_LIB_PATH := $(PAHO_MQTT_C_DIR)/lib
else
$(error For MQTT, target arch ABI not supported: $(TARGET_ARCH_ABI))
endif

include $(CLEAR_VARS)
LOCAL_MODULE := paho-mqtt3a
LOCAL_SRC_FILES := $(PAHO_MQTT_C_LIB_PATH)/libpaho-mqtt3a.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := paho-mqtt3c
LOCAL_SRC_FILES := $(PAHO_MQTT_C_LIB_PATH)/libpaho-mqtt3c.a
include $(PREBUILT_STATIC_LIBRARY)
endif # ifeq ($(ENABLE_MQTT), true)

endif # ifneq ($(NNSTREAMER_API_OPTION),single)

include $(CLEAR_VARS)

LOCAL_MODULE := nnstreamer
LOCAL_SRC_FILES := $(sort $(NNSTREAMER_SRC_FILES))
LOCAL_C_INCLUDES := $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON) $(NNSTREAMER_CAPI_INCLUDES)
ifeq ($(ENABLE_MQTT), true)
LOCAL_C_INCLUDES += $(PAHO_MQTT_C_INCLUDES)
endif

LOCAL_EXPORT_C_INCLUDES := $(NNSTREAMER_CAPI_INCLUDES)
LOCAL_CFLAGS := -O3 -fPIC $(NNS_API_FLAGS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS)

ifeq ($(ENABLE_MQTT), true)
LOCAL_STATIC_LIBRARIES := paho-mqtt3a paho-mqtt3c
endif

include $(BUILD_STATIC_LIBRARY)
