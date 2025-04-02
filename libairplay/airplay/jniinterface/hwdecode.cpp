#include <string>
#include <stdint.h>
#include <vector>
#include <android/log.h>
#include <pthread.h>
#include <sys/param.h>
#include <jni.h>
#include "hwdecode.h"
#include "JNIEnvPtr.h"

#define MODULE_NAME "hwdecode"
#include <logging_macros.h>


JavaVM *HwDecoder::javaVM = NULL;
jobject HwDecoder::jobj = NULL;
ringbuffer_t *HwDecoder::rbuf = NULL;
jmethodID HwDecoder::jmid_streamclosed = NULL;
jmethodID HwDecoder::jmid_streamopen = NULL;


void HwDecoder::reset() {
    if (rbuf) {
        ringbuffer_reset(rbuf);
    }
}


JNIEXPORT void JNICALL
HwDecoder::initNative(JNIEnv *env, jobject thiz) {
    LOGE("HwDecoder::initNative");
    if (jobj) {
        env->DeleteGlobalRef(jobj);
    }

    jobj = env->NewGlobalRef(thiz);
}

static int8_t *pBuffer = NULL;
static jbyteArray buffer = NULL;

void HwDecoder::write264Stream(uint8_t *data, int size, uint64_t timestamp) {
    if (!rbuf) {
        return;
    }
 
    ringbuffer_put(rbuf, (const char *)data, size);
}

JNIEXPORT jint JNICALL
HwDecoder::read264Stream(JNIEnv *env, jobject clazz, jlong pos, jbyteArray buf, jint offset, jint size) {
    if (!rbuf) {
         return -1;
    }

    if (ringbuffer_is_empty(rbuf))
        return 0;

    if (pBuffer == NULL) {
        buffer = buf;
        pBuffer = env->GetByteArrayElements(buf, NULL);  // fixme:  memory leak
    }

    int len = ringbuffer_get(rbuf, (char *)(pBuffer), size );

    return len;
}

void HwDecoder::start(bool init) {
    LOGE("HwDecoder::start");

    if (jobj && init) {
        JNIEnvPtr env(javaVM);
        env->CallVoidMethod(jobj, jmid_streamopen);
    }
 
    if (!rbuf) {
        rbuf = ringbuffer_create(1 << 23);  //8MB
    }
    reset();
    pBuffer = NULL;
    buffer = NULL;
}

void HwDecoder::stop() {
    LOGE("HwDecoder::stop");
    reset();

    if (jobj) {
        JNIEnvPtr env(javaVM);
        env->CallVoidMethod(jobj, jmid_streamclosed);
    }
    //env->DeleteGlobalRef(jobj);

    if (rbuf) {
        ringbuffer_destroy(rbuf);
        rbuf = NULL;
    }
 
}

JNIEXPORT jint JNICALL
HwDecoder::readableSize(JNIEnv *env, jobject clazz, jint size) {
    return 0;
}

JNIEXPORT void JNICALL
HwDecoder::closeStream(JNIEnv *env, jobject clazz) {
}

void HwDecoder::init(JavaVM *vm, JNIEnv *env) {
    LOGE("HwDecoder::init");
    javaVM = vm;

    static JNINativeMethod methods[] = {
            {"read264Stream", "(J[BII)I", (void *) read264Stream},
            {"readableSize",  "(I)I",     (void *) readableSize},
            {"closeStream",   "()V",      (void *) closeStream},
            {"initNative",    "()V",      (void *) initNative},
    };

    jclass clazz = env->FindClass( "com/aircast/source/AirplayMirrorSource");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof((methods)[0])) < 0) {
        LOGE("RegisterNatives  error");
    }

    //jni 需调用的java函数
    jmid_streamclosed = env->GetMethodID(clazz, "streamClosed", "()V");
    jmid_streamopen = env->GetMethodID(clazz, "streamOpened", "()V");
}

void HwDecoder::uninit() {
    LOGE("HwDecoder::uninit");
}
