LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
CVR_DIR := $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_SRC_FILES = curve25519-donna.c \

LOCAL_CFLAGS += -O3 -W -Wall 
LOCAL_MODULE := libcurve25519
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/../include
include $(BUILD_STATIC_LIBRARY)
