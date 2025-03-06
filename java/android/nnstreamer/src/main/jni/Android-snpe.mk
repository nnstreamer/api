#------------------------------------------------------
# SNPE (The Snapdragon Neural Processing Engine)
#
# This mk file defines snpe module with prebuilt shared library.
# (snpe-sdk, arm64-v8a only)
# See Qualcomm Neural Processing SDK for AI (https://developer.qualcomm.com/software/qualcomm-neural-processing-sdk) for the details.
#
# You should check your `gradle.properties` to set the variable `NNS_EXT_LIBRARY_PATH` properly.
# The variable should be assigned with path for external shared libs.
# An example: "NNS_EXT_LIBRARY_PATH=src/main/jni/external/lib"
#------------------------------------------------------
LOCAL_PATH := $(call my-dir)

ifndef NNSTREAMER_ROOT
$(error NNSTREAMER_ROOT is not defined!)
endif

include $(NNSTREAMER_ROOT)/jni/nnstreamer.mk

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
SNPE_LIB_PATH := $(EXT_LIB_PATH)
else
$(error Target arch ABI not supported: $(TARGET_ARCH_ABI))
endif

SNPE_INCLUDES :=
SNPE_FLAGS :=

# Check the version of SNPE SDK (v1 or v2)
ifeq ($(shell test -d $(EXT_INCLUDE_PATH)/zdl; echo $$?),0)
SNPE_FLAGS += -DSNPE_VERSION_MAJOR=1
SNPE_INCLUDES += $(EXT_INCLUDE_PATH)/zdl
else ifeq ($(shell test -d $(EXT_INCLUDE_PATH)/SNPE; echo $$?),0)
SNPE_FLAGS += -DSNPE_VERSION_MAJOR=2
SNPE_INCLUDES += $(EXT_INCLUDE_PATH)/SNPE
else
$(error Cannot find SNPE SDK)
endif

#------------------------------------------------------
# snpe-sdk (prebuilt shared library)
#------------------------------------------------------
include $(LOCAL_PATH)/Android-snpe-prebuilt.mk

#------------------------------------------------------
# tensor-filter sub-plugin for snpe
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := snpe-subplugin
LOCAL_SRC_FILES := $(NNSTREAMER_FILTER_SNPE_SRCS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS) $(SNPE_FLAGS)
LOCAL_C_INCLUDES := $(SNPE_INCLUDES) $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_STATIC_LIBRARIES := nnstreamer
LOCAL_SHARED_LIBRARIES := $(SNPE_PREBUILT_LIBS)

include $(BUILD_STATIC_LIBRARY)
