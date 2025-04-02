#pragma once
#ifndef HWDECODE_H
#define HWDECODE_H
#include <stdint.h>
#include "ringbuffer.h"
 

class HwDecoder {
private:
    static JavaVM *javaVM ;
    static jobject jobj;
    static ringbuffer_t *rbuf;
 
    static jmethodID jmid_streamclosed;
  
    static jmethodID jmid_streamopen;

public:
    static void init(JavaVM *javaVM, JNIEnv *env);
    static void uninit ();
    static void start(bool init);
    static void reset();
    static void stop();
    static void write264Stream(uint8_t *data, int size, uint64_t timestamp);

    static JNIEXPORT void JNICALL
    initNative(JNIEnv *env, jobject thiz);

    static JNIEXPORT jint JNICALL
    read264Stream(JNIEnv *env, jobject clazz, jlong pos, jbyteArray buffer, jint offset, jint size);

    static JNIEXPORT jint JNICALL
    readableSize(JNIEnv *env, jobject clazz, jint size);

    static JNIEXPORT void JNICALL
    closeStream(JNIEnv *env, jobject clazz);
};

#endif
