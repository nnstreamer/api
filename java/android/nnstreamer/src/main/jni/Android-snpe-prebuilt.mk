#------------------------------------------------------
# SNPE (The Snapdragon Neural Processing Engine)
#
# This mk file defines prebuilt libraries for snpe module.
# (snpe-sdk, arm64-v8a only)
#------------------------------------------------------
ifndef SNPE_LIB_PATH
$(error SNPE_LIB_PATH is not defined!)
endif

#------------------------------------------------------
# snpe-sdk (prebuilt shared library)
#------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libSNPE
LOCAL_SRC_FILES := $(SNPE_LIB_PATH)/libSNPE.so
include $(PREBUILT_SHARED_LIBRARY)

SNPE_PREBUILT_LIBS := libSNPE
