LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES = \
		add_scalar.c \
		fe.c \
		ge.c \
		key_exchange.c \
		keypair.c \
		sc.c \
		seed.c \
		sha512.c \
		sign.c \
		verify.c \

LOCAL_MODULE := libed25519

LOCAL_CFLAGS += -O3 -W -Wall 

LOCAL_MODULE_TAGS := optional

LOCAL_SYSTEM_SHARED_LIBRARIES := libc

LOCAL_C_INCLUDES:= $(LOCAL_PATH) \
		$(LOCAL_PATH)/../include \
		$(LOCAL_PATH)/../airplay \

include $(BUILD_STATIC_LIBRARY)
