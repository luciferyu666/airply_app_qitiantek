LOCAL_PATH := $(my-dir)

# 使用c代码调用安卓mdns发布服务( 基本不再使用 )
EMBEDDED_MDNS := false
 
 
include $(CLEAR_VARS)
LOCAL_SRC_FILES := airplay.c \
		digest.c \
		fairplay.c \
		http_parser.c \
		http_request.c \
		http_response.c \
		httpd.c \
		netutils.c \
		raop.c \
		raop_buffer.c \
		raop_rtp.c \
		sdp.c \
		utils.c \
		alac/alac.c \
		aac_eld/aac_eld.c \
		dmr/hand_garble.c \
		dmr/modified_md5.c \
		dmr/omg_hax.c \
		dmr/playfair.c \
		dmr/sap_hash.c \
		main.c \
		dnssd.c \


LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include \
		$(LOCAL_PATH)/../include/openssl \
		$(LOCAL_PATH) \
		aac/libAACdec/include \
		aac/libAACenc/include \
		aac/libPCMutils/include \
		aac/libFDK/include \
		aac/libSYS/include \
		aac/libMpegTPDec/include \
		aac/libMpegTPEnc/include \
		aac/libSBRdec/include \
		aac/libSBRenc/include \
		aac/libFDK/aarch64/include \
		mdnsresponder/mDNSShared \
		lollipop_wifi/socket_ipc \


LOCAL_CFLAGS += -std=c99
LOCAL_CPPFLAGS += -std=c++14

#LOCAL_CPPFLAGS += -fexceptions -frtti
LOCAL_MODULE := libairplay
LOCAL_MODULE_TAGS := optional

#LOCAL_CFLAGS :=  -Wextra
#LOCAL_CFLAGS += -pie -fPIE
#LOCAL_LDFLAGS := -pie -fPIE
LOCAL_CFLAGS += -O3 -W -Wall -Wno-unused-variable -Wno-unused-parameter -D__ANDROID__ -D_GNU_SOURCE -DHAVE_IPV6 -DNOT_HAVE_SA_LEN -DUSES_NETLINK -DTARGET_OS_LINUX -fno-strict-aliasing -DHAVE_LINUX -DMDNS_UDS_SERVERPATH=\"/dev/socket/mdnsd\" -DMDNS_DEBUGMSGS=0

LOCAL_CFLAGS += -fdiagnostics-color=always -pipe -D_FILE_OFFSET_BITS=64 -Winvalid-pch -D_REENTRANT

#LOCAL_CFLAGS += -Werror=pointer-to-int-cast   -Werror=int-to-pointer-cast   -Werror=shorten-64-to-32
#LOCAL_CPPFLAGS += -Werror=pointer-to-int-cast   -Werror=int-to-pointer-cast   -Werror=shorten-64-to-32
#LOCAL_CFLAGS +=-Werror=implicit-function-declaration

LOCAL_CPPFLAGS += -O3 -W -Wall -Wno-unused-variable -Wno-unused-parameter -DANDROID -fdata-sections -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security -DNDEBUG0

LOCAL_LDLIBS := -lOpenSLES -llog

LOCAL_STRIP_MODULE := true

# for ijkplayer
LOCAL_SRC_FILES += \
       	jniinterface/jnimain.cpp \
		jniinterface/hwdecode.cpp \
		jniinterface/ring_buffer.c \
		jniinterface/parse_sps.c \
 
 

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../ffmpeg5.1.3/include \
					$(LOCAL_PATH)/oboe_out

LOCAL_SRC_FILES += \
		oboe_out/audio-output.cpp

LOCAL_STATIC_LIBRARIES += \
			libcrypt \
			libcurve25519 \
			libed25519 \
			libFraunhoferAAC \
			libplist \
			liboboe

ifeq ($(EMBEDDED_MDNS),true)
LOCAL_CFLAGS += -DEMBEDDED_MDNS=1
LOCAL_SHARED_LIBRARIES := libmdnssd
endif


include $(BUILD_SHARED_LIBRARY)

