LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
#prebuilt_stdcxx_PATH := /Users/xiaguoqiang/Develope/android-sdk-macosx/ndk-bundle/sources/cxx-stl/gnu-libstdc++/4.9
LOCAL_SRC_FILES = src/Array.cpp \
		src/Boolean.cpp \
		src/Data.cpp \
		src/Date.cpp \
		src/Dictionary.cpp \
		src/Integer.cpp \
		src/Key.cpp \
		src/Node.cpp \
		src/Real.cpp \
		src/String.cpp \
		src/Structure.cpp \
		src/Uid.cpp \
		src/base64.c \
		src/bplist.c \
		src/bytearray.c \
		src/hashtable.c \
		src/plist.c \
		src/ptrarray.c \
		src/time64.c \
		src/xplist.c \
		libcnary/node.c \
		libcnary/list.c \
		libcnary/node_list.c \
		libcnary/iterator.c \
		libcnary/node_iterator.c \

LOCAL_MODULE := libplist
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/include \
		$(LOCAL_PATH)/libcnary/include \
		$(LOCAL_PATH)/src \
		prebuilts/ndk/8/sources/cxx-stl/stlport \
		prebuilts/ndk/8/sources/cxx-stl/stlport/stlport \

#-L$(SYSROOT)/usr/lib
#LOCAL_LDLIBS := prebuilts/ndk/8/sources/cxx-stl/stlport/libs/armeabi-v7a/thumb/libstlport_static.a
#LOCAL_LDLIBS := prebuilts/ndk/8/sources/cxx-stl/stlport/libs/armeabi-v7a/libstlport_static.a

#LOCAL_C_INCLUDES += $(prebuilt_stdcxx_PATH)
include $(BUILD_STATIC_LIBRARY)
