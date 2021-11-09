//
// Created by nzy on 11/8/21.
//

#ifndef FFMPEGPLAYER_NFFMPEG_H
#define FFMPEGPLAYER_NFFMPEG_H


#include "CallJava.h"
#include "Playstatus.h"
#include "AudioChannel.h"
#include "VideoChannel.h"

extern "C"{
#include <libavformat/avformat.h>
};

/**
 * 主要是用来解码
 */
class NFfmpeg {
public:

    AVFormatContext *avFormatContext = NULL;
    pthread_t pthread;
    // 初始化 解码 的时候加锁
    pthread_mutex_t init_mutex;
    // 当 seek 的时候 加锁
    pthread_mutex_t seek_mutex;
    // 网络 或者是 文件路径
    const char *url;
    // 播放状态
    Playstatus *playstatus;
    // 给 java 端 回调
    CallJava *callJava;
    // 用来处理音频
    AudioChannel *audioChannel;
    // 用来处理视频
    VideoChannel *videoChannel;
    int duration = 0;
    bool exit = false;
public:
    NFfmpeg(CallJava *callJava, Playstatus *playstatus, const char *url);

    virtual ~NFfmpeg();

    void start();

    void prepare();

    void decode();


    int getCodecContext(AVCodecParameters *pParameters, AVCodecContext **pContext);

    void pause();

    void resume();

    void stop();

    void setSpeed(jfloat speed);

    void seek(jint position);
};


#endif //FFMPEGPLAYER_NFFMPEG_H
