#------------------------------------------------------
# MXNet Deep learning framework
# https://github.com/apache/incubator-mxnet
#
# This mk file defines prebuilt libraries for mxnet module.
# (mxnet core libraries, arm64-v8a only)
#------------------------------------------------------
ifndef MXNET_LIB_PATH
$(error MXNET_LIB_PATH is not defined!)
endif

#------------------------------------------------------
# mxnet prebuilt shared libraries
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := mxnet
LOCAL_SRC_FILES := $(MXNET_LIB_PATH)/libmxnet.so
include $(PREBUILT_SHARED_LIBRARY)

MXNET_PREBUILT_LIBS := mxnet
