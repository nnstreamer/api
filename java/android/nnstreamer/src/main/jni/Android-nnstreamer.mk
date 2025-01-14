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

NNSTREAMER_EXTERNAL_LIBS :=

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
    $(NNSTREAMER_DECODER_TR_SRCS) \
    $(NNSTREAMER_DECODER_DV_SRCS) \
    $(NNSTREAMER_DECODER_IL_SRCS) \
    $(NNSTREAMER_DECODER_PE_SRCS) \
    $(NNSTREAMER_DECODER_IS_SRCS) \
    $(NNSTREAMER_JOIN_SRCS)

ifeq ($(shell which orcc),)
$(info No 'orcc' in your PATH, install it to enable orc.)
else
$(info Compile ORC code)
$(shell mkdir -p $(LOCAL_PATH)/orc)
$(shell orcc --header -o $(LOCAL_PATH)/orc/nnstreamer-orc.h $(NNSTREAMER_ORC_SRC))
$(shell orcc --implementation -o $(LOCAL_PATH)/orc/nnstreamer-orc.c $(NNSTREAMER_ORC_SRC))

NNSTREAMER_SRC_FILES     += $(LOCAL_PATH)/orc/nnstreamer-orc.c
NNSTREAMER_CAPI_INCLUDES += $(LOCAL_PATH)/orc
NNSTREAMER_CAPI_INCLUDES += $(GSTREAMER_ROOT)/include/orc-0.4

NNS_API_FLAGS += -DHAVE_ORC=1
endif

ifeq ($(ENABLE_MQTT), true)
NNSTREAMER_SRC_FILES += $(NNSTREAMER_MQTT_SRCS)
NNSTREAMER_EXTERNAL_LIBS += libpaho-mqtt-c
endif

ifeq ($(ENABLE_ML_OFFLOADING), true)
NNSTREAMER_SRC_FILES += \
    $(NNSTREAMER_QUERY_SRCS) \
    $(NNSTREAMER_ELEM_EDGE_SRCS) \
    $(NNSTREAMER_EDGE_SRCS)

ifeq ($(ENABLE_MQTT), true)
NNSTREAMER_SRC_FILES += $(NNSTREAMER_EDGE_MQTT_SRCS)
endif

NNSTREAMER_CAPI_INCLUDES += $(NNSTREAMER_EDGE_INCLUDES)
endif

ifeq ($(ENABLE_ML_SERVICE), true)
NNSTREAMER_SRC_FILES += \
    $(ML_API_ROOT)/c/src/ml-api-service.c \
    $(ML_API_ROOT)/c/src/ml-api-service-agent-client.c \
    $(ML_API_ROOT)/c/src/ml-api-service-extension.c \
    $(NNSTREAMER_AGENT_SRCS) \
    $(MLOPS_AGENT_SRCS)

ifeq ($(ENABLE_ML_OFFLOADING), true)
NNSTREAMER_SRC_FILES += \
    $(ML_API_ROOT)/c/src/ml-api-service-query-client.c \
    $(ML_API_ROOT)/c/src/ml-api-service-offloading.c

NNSTREAMER_EXTERNAL_LIBS += libcurl
endif

NNSTREAMER_CAPI_INCLUDES += $(MLOPS_AGENT_INCLUDE)
endif
endif # ifneq ($(NNSTREAMER_API_OPTION),single)

include $(CLEAR_VARS)

LOCAL_MODULE := nnstreamer
LOCAL_SRC_FILES := $(sort $(NNSTREAMER_SRC_FILES))
LOCAL_C_INCLUDES := $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON) $(NNSTREAMER_CAPI_INCLUDES)
LOCAL_CFLAGS := -O3 -fPIC $(NNS_API_FLAGS) -Wno-deprecated-declarations
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS)
LOCAL_STATIC_LIBRARIES := $(NNSTREAMER_EXTERNAL_LIBS)

include $(BUILD_STATIC_LIBRARY)
