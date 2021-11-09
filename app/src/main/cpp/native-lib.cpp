#include <jni.h>
#include <string>
#include "native_log.h"
#include "NFfmpeg.h"
#include "CallJava.h"
#include "Playstatus.h"


// 用来在子线程中回调给java层。
JavaVM *javaVm = NULL;
// 主要用来解码
NFfmpeg *fFmpeg = NULL;
// 用来回调给java的工具类
CallJava *callJava = NULL;

Playstatus *playstatus;
extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;
    javaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {

        return result;
    }
    return JNI_VERSION_1_4;

}

extern "C"
JNIEXPORT void JNICALL
Java_com_nzy_ffmpegplayer_FFmpegPlayer_native_1prepareAsync(JNIEnv *env, jobject thiz,
                                                            jstring _source) {
    const char *source = env->GetStringUTFChars(_source, NULL);

    if (fFmpeg) {
        fFmpeg = NULL;
        delete fFmpeg;
    }
    // 初始化 回调java类
    if (!callJava) {
        callJava = new CallJava(javaVm, env, &thiz);
    }
    // 初始化 播放状态
    playstatus = new Playstatus();
    // 初始化 NFfmpeg
    LOGE("文件地址 %s", source);
    fFmpeg = new NFfmpeg(callJava, playstatus, source);
    // 调用 NFfmpeg 的prepare
    fFmpeg->prepare();
    // 释放
    env->ReleaseStringUTFChars(_source, source);


}
extern "C"
JNIEXPORT void JNICALL
Java_com_nzy_ffmpegplayer_FFmpegPlayer_native_1start(JNIEnv *env, jobject thiz) {
    if (fFmpeg) {
        fFmpeg->start();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_nzy_ffmpegplayer_FFmpegPlayer_native_1pause(JNIEnv *env, jobject thiz) {
    if (fFmpeg) {
        fFmpeg->pause();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_nzy_ffmpegplayer_FFmpegPlayer_native_1resume(JNIEnv *env, jobject thiz) {
    if (fFmpeg) {
        fFmpeg->resume();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_nzy_ffmpegplayer_FFmpegPlayer_native_1stop(JNIEnv *env, jobject thiz) {
    if (fFmpeg) {

        // 正在退出 只调用一次
        if (fFmpeg != NULL) {
            fFmpeg->stop();
            delete (fFmpeg);
            if (callJava != NULL) {
                delete (callJava);
                callJava = NULL;
            }
            if (playstatus != NULL) {
                delete (playstatus);
                playstatus = NULL;
            }
        }
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_nzy_ffmpegplayer_FFmpegPlayer_native_1speed(JNIEnv *env, jobject thiz, jfloat speed) {
    if (fFmpeg) {
        fFmpeg->setSpeed(speed);
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_nzy_ffmpegplayer_FFmpegPlayer_native_1seek(JNIEnv *env, jobject thiz, jint position) {
    if (fFmpeg) {
        fFmpeg->seek(position);
    }
}