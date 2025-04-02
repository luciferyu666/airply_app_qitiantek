LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
aacdec_sources := $(wildcard $(LOCAL_PATH)/libAACdec/src/*.cpp)
aacdec_sources := $(aacdec_sources:$(LOCAL_PATH)/libAACdec/src/%=%)

aacenc_sources := $(wildcard $(LOCAL_PATH)/libAACenc/src/*.cpp)
aacenc_sources := $(aacenc_sources:$(LOCAL_PATH)/libAACenc/src/%=%)

pcmutils_sources := $(wildcard $(LOCAL_PATH)/libPCMutils/src/*.cpp)
pcmutils_sources := $(pcmutils_sources:$(LOCAL_PATH)/libPCMutils/src/%=%)

fdk_sources := $(wildcard $(LOCAL_PATH)/libFDK/src/*.cpp)
fdk_sources := $(fdk_sources:$(LOCAL_PATH)/libFDK/src/%=%)

sys_sources := $(wildcard $(LOCAL_PATH)/libSYS/src/*.cpp)
sys_sources := $(sys_sources:$(LOCAL_PATH)/libSYS/src/%=%)

mpegtpdec_sources := $(wildcard $(LOCAL_PATH)/libMpegTPDec/src/*.cpp)
mpegtpdec_sources := $(mpegtpdec_sources:$(LOCAL_PATH)/libMpegTPDec/src/%=%)

mpegtpenc_sources := $(wildcard $(LOCAL_PATH)/libMpegTPEnc/src/*.cpp)
mpegtpenc_sources := $(mpegtpenc_sources:$(LOCAL_PATH)/libMpegTPEnc/src/%=%)

sbrdec_sources := $(wildcard $(LOCAL_PATH)/libSBRdec/src/*.cpp)
sbrdec_sources := $(sbrdec_sources:$(LOCAL_PATH)/libSBRdec/src/%=%)

sbrenc_sources := $(wildcard $(LOCAL_PATH)/libSBRenc/src/*.cpp)
sbrenc_sources := $(sbrenc_sources:$(LOCAL_PATH)/libSBRenc/src/%=%)

fdk_aac_arith_coding_src := $(wildcard $(LOCAL_PATH)/libArithCoding/src/*.cpp)
fdk_aac_arith_coding_src := $(fdk_aac_arith_coding_src:$(LOCAL_PATH)/libArithCoding/src/%=%)

fdk_aac_drc_dec_src := $(wildcard $(LOCAL_PATH)/libDRCdec/src/*.cpp)
fdk_aac_drc_dec_src := $(fdk_aac_drc_dec_src:$(LOCAL_PATH)/libDRCdec/src/%=%)

fdk_aac_sac_dec_src := $(wildcard $(LOCAL_PATH)/libSACdec/src/*.cpp)
fdk_aac_sac_dec_src := $(fdk_aac_sac_dec_src:$(LOCAL_PATH)/libSACdec/src/%=%)

fdk_aac_sac_enc_src := $(wildcard $(LOCAL_PATH)/libSACenc/src/*.cpp)
fdk_aac_sac_enc_src := $(fdk_aac_sac_enc_src:$(LOCAL_PATH)/libSACenc/src/%=%)
 


LOCAL_SRC_FILES := \
        $(aacdec_sources:%=libAACdec/src/%) \
        $(aacenc_sources:%=libAACenc/src/%) \
        $(pcmutils_sources:%=libPCMutils/src/%) \
        $(fdk_sources:%=libFDK/src/%) \
        $(sys_sources:%=libSYS/src/%) \
        $(mpegtpdec_sources:%=libMpegTPDec/src/%) \
        $(mpegtpenc_sources:%=libMpegTPEnc/src/%) \
        $(sbrdec_sources:%=libSBRdec/src/%) \
        $(sbrenc_sources:%=libSBRenc/src/%) \
        $(fdk_aac_arith_coding_src:%=libArithCoding/src/%) \
        $(fdk_aac_drc_dec_src:%=libDRCdec/src/%) \
        $(fdk_aac_sac_dec_src:%=libSACdec/src/%) \
        $(fdk_aac_sac_enc_src:%=libSACenc/src/%) 

LOCAL_CFLAGS := -DANDROID
LOCAL_CFLAGS += -O3 -Wno-sequence-point -Wno-extra -Wno-c++11-narrowing 

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/libAACdec/include \
        $(LOCAL_PATH)/libAACenc/include \
        $(LOCAL_PATH)/libPCMutils/include \
        $(LOCAL_PATH)/libFDK/include \
        $(LOCAL_PATH)/libSYS/include \
        $(LOCAL_PATH)/libMpegTPDec/include \
        $(LOCAL_PATH)/libMpegTPEnc/include \
        $(LOCAL_PATH)/libSBRdec/include \
        $(LOCAL_PATH)/libSBRenc/include \
        ${LOCAL_PATH}/libArithCoding/include \
        ${LOCAL_PATH}/libDRCdec/include \
        ${LOCAL_PATH}/libSACdec/include \
        ${LOCAL_PATH}/libSACenc/include

LOCAL_MODULE:= libFraunhoferAAC

include $(BUILD_STATIC_LIBRARY)
