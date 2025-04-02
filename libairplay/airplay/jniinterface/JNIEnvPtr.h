#pragma once

class JNIEnvPtr {
public:
    JNIEnvPtr(JavaVM *jvm) :  env_{nullptr}, need_detach_{false}, jvm_(jvm) {
        if (jvm->GetEnv((void**) &env_, JNI_VERSION_1_4) ==
            JNI_EDETACHED) {
            jvm->AttachCurrentThread(&env_, nullptr);
            need_detach_ = true;
        }
    }

    ~JNIEnvPtr() {
        if (need_detach_) {
            jvm_->DetachCurrentThread();
        }
    }

    JNIEnv* operator->() {
        return env_;
    }

private:
    JNIEnvPtr(const JNIEnvPtr&) = delete;
    JNIEnvPtr& operator=(const JNIEnvPtr&) = delete;

private:
    JNIEnv* env_;
    bool need_detach_;
    JavaVM *jvm_;
};

/*
C
#define GET_CLASS(clazz, str, b_globlal) do { \
    (clazz) = (*env)->FindClass(env, (str)); \
    if (!(clazz)) { \
        LOGE("FindClass(%s) failed", (str)); \
        return -1; \
    } \
    if (b_globlal) { \
        (clazz) = (jclass) (*env)->NewGlobalRef(env, (clazz)); \
        if (!(clazz)) { \
            LOGE("NewGlobalRef(%s) failed", (str)); \
            return -1; \
        } \
    } \
} while (0)

#define GET_ID(get, id, clazz, str, args) do { \
    (id) = (*env)->get(env, (clazz), (str), (args)); \
    if (!(id)) { \
        LOGE(#get"(%s) failed", (str)); \
        return -1; \
    } \
} while (0)
*/

//C++
#define GET_JCLASS(clazz, str, b_globlal) do { \
    (clazz) = (env)->FindClass((str)); \
    if (!(clazz)) { \
        LOGE("FindClass(%s) failed", (str)); \
        return ; \
    } \
    if (b_globlal) { \
        (clazz) = (jclass) (env)->NewGlobalRef((clazz)); \
        if (!(clazz)) { \
            LOGE("NewGlobalRef(%s) failed", (str)); \
            return ; \
        } \
    } \
} while (0)

#define GET_JID(get, id, clazz, str, args) do { \
    (id) = (env)->get((clazz), (str), (args)); \
    if (!(id)) { \
        LOGE(#get"(%s) failed", (str)); \
        return ; \
    } \
} while (0)
