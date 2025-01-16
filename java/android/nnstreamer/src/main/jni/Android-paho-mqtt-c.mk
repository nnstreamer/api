LOCAL_PATH := $(call my-dir)

PAHO_MQTT_C_DIR := $(LOCAL_PATH)/paho-mqtt-c
PAHO_MQTT_C_INCLUDES := $(PAHO_MQTT_C_DIR)/include

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
PAHO_MQTT_C_LIB_PATH := $(PAHO_MQTT_C_DIR)/lib
else
$(error For MQTT, target arch ABI is not supported: $(TARGET_ARCH_ABI))
endif

#------------------------------------------------------
# paho-mqtt-c prebuilt libraries
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := paho-mqtt3a
LOCAL_SRC_FILES := $(PAHO_MQTT_C_LIB_PATH)/libpaho-mqtt3a.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := paho-mqtt3c
LOCAL_SRC_FILES := $(PAHO_MQTT_C_LIB_PATH)/libpaho-mqtt3c.a
include $(PREBUILT_STATIC_LIBRARY)

#------------------------------------------------------
# paho-mqtt-c module
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libpaho-mqtt-c
LOCAL_STATIC_LIBRARIES := paho-mqtt3a paho-mqtt3c
LOCAL_EXPORT_C_INCLUDES := $(PAHO_MQTT_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)
