//
// Created by nzy on 11/8/21.
//
#include "jni.h"
#include "native_log.h"
#ifndef FFMPEGPLAYER_CALLJAVA_H
#define FFMPEGPLAYER_CALLJAVA_H

#define MAIN_THREAD 0
#define CHILD_THREAD 1


class CallJava {
private:
    JavaVM *javaVm;
    JNIEnv *env;
    jobject job;
    jmethodID jmid_parpared;
    jmethodID jmid_timeinfo;
    jmethodID jmid_load;
    jmethodID jmid_renderyuv;
public:
    CallJava(JavaVM *javaVm, JNIEnv *env, jobject *jobject);

    void onCallParpared(int threadId);

    virtual ~CallJava();

    void onCallTimeInfo(int threadId, double currentTime, int totalTime);

    void onCallLoad(int threadId, bool load);

    void onCallRenderYUV(int threadId,int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv);
};


#endif //FFMPEGPLAYER_CALLJAVA_H
