//
// Created by admin on 2020/12/23.
//
#include "JavaCallHelper.h"
#include "macro.h"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance) {
    this->vm = vm;
    this->env = env;
    //一旦涉及到jobject 跨方法  跨线程  就需要创建全局引用
    this->instance = env->NewGlobalRef(instance);

    jclass clazz = env->GetObjectClass(instance);
    onErrorId = env->GetMethodID(clazz, "onError", "(I)V");
    onPrepareId = env->GetMethodID(clazz, "onPrepare", "()V");
}

JavaCallHelper::~JavaCallHelper() {
    env->DeleteGlobalRef(instance);
}

void JavaCallHelper::onError(int thread, int errorCode) {
    //主线程
    if (thread == THREAD_MAIN) {
        env->CallVoidMethod(instance, onErrorId, errorCode);
    } else {
        //子线程
        JNIEnv *env;
        //获得属于我这一个线程的jnienv
        vm->AttachCurrentThread(&env, 0);
        env->CallVoidMethod(instance, onErrorId, errorCode);
        vm->DetachCurrentThread();
    }
}

void JavaCallHelper::onPrepare(int thread) {
    if (thread == THREAD_MAIN) {
        env->CallVoidMethod(instance, onPrepareId);
    } else {
        JNIEnv *env;
        vm->AttachCurrentThread(&env, 0);
        env->CallVoidMethod(instance, onPrepareId);
        vm->DetachCurrentThread();
    }
}