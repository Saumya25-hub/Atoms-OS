/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef JNI_H
#define JNI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t   jboolean;
typedef int8_t   jbyte;
typedef uint16_t jchar;
typedef int16_t  jshort;
typedef int32_t  jint;
typedef int64_t  jlong;
typedef float    jfloat;
typedef double   jdouble;
typedef jint     jsize;

#define JNI_FALSE 0
#define JNI_TRUE  1

#define JNI_OK           0
#define JNI_ERR         (-1)
#define JNI_EDETACHED   (-2)
#define JNI_EVERSION    (-3)
#define JNI_ENOMEM      (-4)
#define JNI_EEXIST      (-5)
#define JNI_EINVAL      (-6)

#define JNI_VERSION_1_1 0x00010001
#define JNI_VERSION_1_2 0x00010002
#define JNI_VERSION_1_4 0x00010004
#define JNI_VERSION_1_6 0x00010006
#define JNI_VERSION_1_8 0x00010008

struct _jobject;
typedef struct _jobject* jobject;
typedef jobject jclass;
typedef jobject jthrowable;
typedef jobject jstring;
typedef jobject jarray;
typedef jobject jobjectArray;

typedef struct {
    char *optionString;
    void *extraInfo;
} JavaVMOption;

typedef struct {
    jint version;
    jint nOptions;
    JavaVMOption *options;
    jboolean ignoreUnrecognized;
} JavaVMInitArgs;

struct JNIInvokeInterface_;
struct JNINativeInterface_;

typedef const struct JNIInvokeInterface_* JavaVM;
typedef const struct JNINativeInterface_* JNIEnv;

struct JNIInvokeInterface_ {
    void* reserved0;
    void* reserved1;
    void* reserved2;
    jint (*DestroyJavaVM)(JavaVM* vm);
    jint (*AttachCurrentThread)(JavaVM* vm, void** penv, void* args);
    jint (*DetachCurrentThread)(JavaVM* vm);
    jint (*GetEnv)(JavaVM* vm, void** penv, jint version);
    jint (*AttachCurrentThreadAsDaemon)(JavaVM* vm, void** penv, void* args);
};

struct JNINativeInterface_ {
    void* reserved0;
    void* reserved1;
    void* reserved2;
    void* reserved3;
    jint (*GetVersion)(JNIEnv* env);
    jclass (*FindClass)(JNIEnv* env, const char* name);
    jint (*Throw)(JNIEnv* env, jthrowable obj);
    jint (*ThrowNew)(JNIEnv* env, jclass clazz, const char* message);
    jthrowable (*ExceptionOccurred)(JNIEnv* env);
    void (*ExceptionDescribe)(JNIEnv* env);
    void (*ExceptionClear)(JNIEnv* env);
    void (*FatalError)(JNIEnv* env, const char* message);
};

jint JNI_CreateJavaVM(JavaVM** pvm, void** penv, void* args);
jint JNI_GetDefaultJavaVMInitArgs(void* args);
jint JNI_GetCreatedJavaVMs(JavaVM** vmBuf, jsize bufLen, jsize* nVMs);

#ifdef __cplusplus
}
#endif

#endif // JNI_H
