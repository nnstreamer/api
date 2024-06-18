#------------------------------------------------------
# tensorflow-lite
#
# This mk file defines tensorflow-lite module with prebuilt static/shared library.
# To build and run the example with gstreamer binaries, we built a static library
#   (elibtensorflow-lite.a) or shared libs (libtensorflowlite.so and libtensorflowlite_gpu_delegate.so).
# for Android/Tensorflow-lite from the Tensorflow repository of the Tizen software platform.
# - [Tizen] Tensorflow git repository:
#    * Repository: https://review.tizen.org/gerrit/p/platform/upstream/tensorflow
#------------------------------------------------------
LOCAL_PATH := $(call my-dir)

ifndef NNSTREAMER_ROOT
$(error NNSTREAMER_ROOT is not defined!)
endif

include $(NNSTREAMER_ROOT)/jni/nnstreamer.mk

TFLITE_VERSION := 2.16.1

_TFLITE_VERSIONS = $(subst ., , $(TFLITE_VERSION))
TFLITE_VERSION_MAJOR := $(word 1, $(_TFLITE_VERSIONS))
TFLITE_VERSION_MINOR := $(word 2, $(_TFLITE_VERSIONS))
TFLITE_VERSION_MICRO := $(word 3, $(_TFLITE_VERSIONS))

TFLITE_FLAGS := \
    -DTFLITE_SUBPLUGIN_NAME=\"tensorflow-lite\" \
    -DTFLITE_VERSION=$(TFLITE_VERSION) \
    -DTFLITE_VERSION_MAJOR=$(TFLITE_VERSION_MAJOR) \
    -DTFLITE_VERSION_MINOR=$(TFLITE_VERSION_MINOR) \
    -DTFLITE_VERSION_MICRO=$(TFLITE_VERSION_MICRO)

TFLITE_ENABLE_GPU_DELEGATE := true
TFLITE_ENABLE_NNAPI_DELEGATE := true
TFLITE_ENABLE_XNNPACK_DELEGATE := true
TFLITE_EXPORT_LDLIBS :=

ifeq ($(TFLITE_ENABLE_NNAPI_DELEGATE),true)
TFLITE_FLAGS += -DTFLITE_NNAPI_DELEGATE_SUPPORTED=1
endif

ifeq ($(TFLITE_ENABLE_GPU_DELEGATE),true)
TFLITE_FLAGS += -DTFLITE_GPU_DELEGATE_SUPPORTED=1
TFLITE_EXPORT_LDLIBS += -lEGL -lGLESv2
endif

ifeq ($(TFLITE_ENABLE_XNNPACK_DELEGATE),true)
TFLITE_FLAGS += -DTFLITE_XNNPACK_DELEGATE_SUPPORTED=1
endif

TF_LITE_DIR := $(TFLITE_ROOT)
TF_LITE_INCLUDES := $(TF_LITE_DIR)/include

# Check Target ABI. Only supports arm64 and x86_64.
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
TF_LITE_LIB_PATH := $(TF_LITE_DIR)/lib/arm64
else ifeq ($(TARGET_ARCH_ABI),x86_64)
TF_LITE_LIB_PATH := $(TF_LITE_DIR)/lib/x86_64
else
$(error Target arch ABI not supported: $(TARGET_ARCH_ABI))
endif

ifeq ($(TFLITE_VERSION_MINOR),8) # v2.8.1
#------------------------------------------------------
# tensorflow-lite (prebuilt static library)
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := tensorflow-lite-lib
LOCAL_SRC_FILES := $(TF_LITE_LIB_PATH)/libtensorflow-lite.a
LOCAL_EXPORT_LDFLAGS := -Wl,--exclude-libs,libtensorflow-lite.a

include $(PREBUILT_STATIC_LIBRARY)

else # NOT v2.8.1
#------------------------------------------------------
# tensorflow-lite (prebuilt shared libraries)
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libtensorflowlite
LOCAL_SRC_FILES := $(TF_LITE_LIB_PATH)/libtensorflowlite.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libtensorflowlite-gpu-delegate
LOCAL_SRC_FILES := $(TF_LITE_LIB_PATH)/libtensorflowlite_gpu_delegate.so
include $(PREBUILT_SHARED_LIBRARY)
endif # tflite version check v2.8.1 or other

#------------------------------------------------------
# tensor-filter sub-plugin for tensorflow-lite
#------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := tensorflow-lite-subplugin
LOCAL_SRC_FILES := $(NNSTREAMER_FILTER_TFLITE_SRCS)
LOCAL_CXXFLAGS := -O3 -fPIC -frtti -fexceptions $(NNS_API_FLAGS) $(TFLITE_FLAGS)
LOCAL_C_INCLUDES := $(TF_LITE_INCLUDES) $(NNSTREAMER_INCLUDES) $(GST_HEADERS_COMMON)
LOCAL_EXPORT_LDLIBS := $(TFLITE_EXPORT_LDLIBS)
ifeq ($(TFLITE_VERSION_MINOR),8) # v2.8.1
LOCAL_WHOLE_STATIC_LIBRARIES := tensorflow-lite-lib cpufeatures
else
LOCAL_SHARED_LIBRARIES := libtensorflowlite libtensorflowlite-gpu-delegate
endif # check v2.8.1

include $(BUILD_STATIC_LIBRARY)
