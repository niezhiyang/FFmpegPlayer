//
// Created by nzy on 11/8/21.
//

#ifndef FFMPEGPLAYER_AUDIOCHANNEL_H
#define FFMPEGPLAYER_AUDIOCHANNEL_H
#include "SoundTouch.h"
#include "SafeQueue.h"
#include "Playstatus.h"
#include "CallJava.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};
using namespace soundtouch;

class AudioChannel {

public:
    int streamIndex = -1;
    // 解码器上下文
    AVCodecContext *avCodecContext = NULL;
    // 流的参数信息，长宽  帧率呀 等等
    AVCodecParameters *codecpar = NULL;
    // 生产者消费者 queue 存放 每一帧的 AvPacket
    SafeQueue *queue = NULL;
    Playstatus *playstatus = NULL;

    // 播放音频的线程
    pthread_t thread_play;
    // 解码器的数据
    AVPacket *avPacket = NULL;
    // 解码后的数据
    AVFrame *avFrame = NULL;
    int ret = 0;
    uint8_t *buffer = NULL;
    int data_size = 0;
    int sample_rate = 0;
    float pitch = 1.0f;

    // 下面是配置 使用 OpenSLES 播放 音频的
    // 引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;
    SLVolumeItf pcmVolumePlay = NULL;
    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;



    int duration = 0;
    //时间单位
    AVRational time_base;
    //当前时间
    double now_time;
    //当前播放的时间    准确时间
    double clock;

    CallJava *callJava = NULL;
    double last_tiem; //上一次调用时间
    //立体声
    int mute = 2;
    SLMuteSoloItf  pcmMutePlay = NULL;
    int volumePercent = 100;

    // 倍速
    float speed = 1.0f;
    // 使用 SoundTouch 来 解决倍速问题
    SoundTouch *soundTouch = NULL;
    //新的缓冲区
    SAMPLETYPE  *sampleBuffer = NULL;
    uint8_t *out_buffer = NULL;
    //    波处理完了没
    bool finished = true;

    //   变速后新波的实际个数
    int nb = 0;
    int num = 0;
public:
    AudioChannel(Playstatus *playstatus, int sample_rate, CallJava *callJava);
    ~AudioChannel();

    void play();
    int resampleAudio(void **pcmbuf);

    void initOpenSLES();

    int getCurrentSampleRateForOpensles(int sample_rate);
    void onCallTimeInfo(int type, int curr, int total);
    void pause();

    void resume();
    void setMute(int mute);

    void setVolume(int percent);

    void setSpeed(float speed);

    int getSoundTouchData();

    void setPitch(float pitch);

    void release();
};


#endif //FFMPEGPLAYER_AUDIOCHANNEL_H
