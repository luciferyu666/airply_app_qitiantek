LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := liboboe

LOCAL_SRC_FILES :=  $(TARGET_ARCH_ABI)/liboboe.so

include $(PREBUILT_SHARED_LIBRARY)
 
 