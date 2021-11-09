//
// Created by nzy on 11/8/21.
//

#ifndef FFMPEGPLAYER_VIDEOCHANNEL_H
#define FFMPEGPLAYER_VIDEOCHANNEL_H

#include "SafeQueue.h"
#include "Playstatus.h"

#include "CallJava.h"
#include "AudioChannel.h"


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

class VideoChannel {
public:
    SafeQueue  *queue = NULL;
    int streamIndex = -1;
    AVCodecContext *avCodecContext = NULL;
    AVCodecParameters *codecpar = NULL;
    Playstatus *playstatus = NULL;
    CallJava *wlCallJava = NULL;
    pthread_mutex_t codecMutex;
    pthread_t thread_play;

    double clock = 0;
    //实时计算出来   主要与音频的差值
    // 因为 音频 的时间是固定的，但是视频解码的时间是不固定的
    // IPB 时间是不一样的，所以要以 音频为准
    double delayTime = 0;
    //  默认休眠时间   40ms  0.04s    帧率 25帧
    double defaultDelayTime = 0.04;
    AudioChannel *audio = NULL;
    AVRational time_base;
public:
    VideoChannel(Playstatus *playstatus, CallJava *wlCallJava);
    ~VideoChannel();
    void play();
    double getDelayTime(double diff);
    double getFrameDiffTime(AVFrame *avFrame);
};


#endif //FFMPEGPLAYER_VIDEOCHANNEL_H
