#------------------------------------------------------
# QNN
#
# This mk file defines prebuilt shared libraries for QNN module.
#------------------------------------------------------
ifndef QNN_LIB_PATH
$(error QNN_LIB_PATH is not defined!)
endif

#------------------------------------------------------
# qnn-sdk (prebuilt shared library)
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libQnnCpu
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnCpu.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libQnnGpu
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnGpu.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libQnnDsp
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnDsp.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libQnnHtp
LOCAL_SRC_FILES := $(QNN_LIB_PATH)/libQnnHtp.so
include $(PREBUILT_SHARED_LIBRARY)

QNN_PREBUILT_LIBS := libQnnCpu libQnnGpu libQnnDsp libQnnHtp
