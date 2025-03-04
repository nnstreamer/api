#------------------------------------------------------
# tensorflow-lite
#
# This mk file defines prebuilt shared libraries for tensorflow-lite.
# (version 2.16.1 or higher, arm64-v8a only)
#------------------------------------------------------
ifndef TF_LITE_LIB_PATH
$(error TF_LITE_LIB_PATH is not defined!)
endif

TFLITE_ENABLE_QNN_DELEGATE := false

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

TF_LITE_PREBUILT_LIBS := libtensorflowlite libtensorflowlite-gpu-delegate

ifeq ($(TFLITE_ENABLE_QNN_DELEGATE),true)
include $(CLEAR_VARS)
LOCAL_MODULE := libtensorflowlite-qnn-delegate
LOCAL_SRC_FILES := $(TF_LITE_LIB_PATH)/libQnnTFLiteDelegate.so
include $(PREBUILT_SHARED_LIBRARY)

TF_LITE_PREBUILT_LIBS += libtensorflowlite-qnn-delegate
endif
