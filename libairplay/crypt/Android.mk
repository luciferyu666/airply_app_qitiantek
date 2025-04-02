#define CONFIG_BIGINT_BARRETT 1
#define CONFIG_BIGINT_CRT 1
#define CONFIG_BIGINT_SQUARE 1
#define CONFIG_BIGINT_32BIT 1
LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES = aes.c \
			base64.c \
			bigint.c \
			hmac.c \
			md5.c \
			new_aes.c \
			rc4.c \
			rsakey.c \
			rsapem.c \
			sha1.c

LOCAL_CFLAGS = -O3 -DCONFIG_BIGINT_BARRETT -DCONFIG_BIGINT_CRT -DCONFIG_BIGINT_SQUARE -DCONFIG_BIGINT_32BIT
LOCAL_MODULE := libcrypt
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/../include
include $(BUILD_STATIC_LIBRARY)
