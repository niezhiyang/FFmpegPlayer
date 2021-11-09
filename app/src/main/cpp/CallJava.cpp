//
// Created by nzy on 11/8/21.
//

#include "CallJava.h"


CallJava::CallJava(JavaVM *javaVm, JNIEnv *env, jobject *jobject) {
    this->javaVm = javaVm;
    this->env = env;
    this->job = *jobject;
    this->job = env->NewGlobalRef(job);
    // 找到 FFmpegPlayer 这个class
    jclass pJclass = env->GetObjectClass(job);
    // 找到准备成功这个方法id
    jmid_parpared = env->GetMethodID(pJclass, "onCallPrepared", "()V");

    // 找到回调 当前播放时间 以及 总时间的回调方法
    jmid_timeinfo = env->GetMethodID(pJclass, "onCallTimeInfo", "(II)V");

    // 网络不加，也就是视频quene 里面没有数据，正在加载中
    jmid_load = env->GetMethodID(pJclass, "onCallLoad", "(Z)V");

    // 回调解码完之后的YUV数据
    jmid_renderyuv = env->GetMethodID(pJclass, "onCallRenderYUV", "(II[B[B[B)V");

}

void CallJava::onCallParpared(int threadId) {


    if (threadId == MAIN_THREAD) {
        // 反射回调这个方法
        env->CallVoidMethod(job, jmid_parpared);
    } else if (threadId == CHILD_THREAD) {
        JNIEnv *jniEnv;
        // 因为 C 层 线程之间是隔离的
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("子线程回调出错了");
            return;
        }
        // 反射回调这个方法
        jniEnv->CallVoidMethod(job, jmid_parpared);
        javaVm->DetachCurrentThread();
    }
}

CallJava::~CallJava() {
    if (job) {
        env->DeleteGlobalRef(job);
    }

}

void CallJava::onCallTimeInfo(int threadId, double currentTime, int totalTime) {

    if (threadId == MAIN_THREAD) {
        // 反射回调这个方法
        env->CallVoidMethod(job, jmid_timeinfo, currentTime, totalTime);
    } else if (threadId == CHILD_THREAD) {
        JNIEnv *jniEnv;
        // 因为 C 层 线程之间是隔离的
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("子线程回调出错了");
            return;
        }
        // 反射回调这个方法
        jniEnv->CallVoidMethod(job, jmid_timeinfo);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallLoad(int threadId, bool load) {

    if (threadId == MAIN_THREAD) {
        env->CallVoidMethod(job, jmid_load, load);
    } else if (threadId == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("子线程回调出错了");
            return;
        }
        jniEnv->CallVoidMethod(job, jmid_load, load);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallRenderYUV(int threadId, int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {
    if (threadId == MAIN_THREAD) {
        jbyteArray y = env->NewByteArray(width * height);
        env->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

        jbyteArray u = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

        jbyteArray v = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));

        env->CallVoidMethod(job, jmid_renderyuv, width, height, y, u, v);

        env->DeleteLocalRef(y);
        env->DeleteLocalRef(u);
        env->DeleteLocalRef(v);
    } else if (threadId == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("子线程回调出错了");
            return;
        }
        jbyteArray y = jniEnv->NewByteArray(width * height);
        jniEnv->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

        jbyteArray u = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

        jbyteArray v = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));

        jniEnv->CallVoidMethod(job, jmid_renderyuv, width, height, y, u, v);

        jniEnv->DeleteLocalRef(y);
        jniEnv->DeleteLocalRef(u);
        jniEnv->DeleteLocalRef(v);
        javaVm->DetachCurrentThread();
    }
}
