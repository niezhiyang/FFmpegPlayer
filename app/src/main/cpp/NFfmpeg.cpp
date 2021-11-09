//
// Created by nzy on 11/8/21.
//

#include <pthread.h>
#include "NFfmpeg.h"
#include "AudioChannel.h"

extern "C" {
// 因为ffempg都是c写的，咱们的工程是cpp的，所以要加入 extern c，否则找不到，对应的使用方法
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "android/native_window_jni.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include <libswresample/swresample.h>

}

NFfmpeg::NFfmpeg(CallJava *callJava, Playstatus *playstatus, const char *url) {
    this->url = url;
    this->playstatus = playstatus;
    this->callJava = callJava;
    // 初始化seek锁
    pthread_mutex_init(&seek_mutex, NULL);
    // 初始化init锁
    pthread_mutex_init(&init_mutex, NULL);
}


void NFfmpeg::start() {
    audioChannel->play();
    videoChannel->play();
    videoChannel->audio = audioChannel;
    int count = 0;

    while (playstatus != NULL && !playstatus->exit) {
        // 因为 seek  是耗时 ,所以
        if (playstatus->seek) {
            continue;
        }
        // 放入队列，缓存 40个吧
        if (audioChannel->queue->getQueueSize() > 40) {
            continue;
        }
        // 解码 avPacket --> CPU(软解码) --> AvFrame(1080*1920) --> 临时 AvFrame(500*400用来适应surface) --> surface(500*400)
        AVPacket *avPacket = av_packet_alloc();
        if (av_read_frame(avFormatContext, avPacket) == 0) {
            if (avPacket->stream_index == audioChannel->streamIndex) {
                //解码操作 音频
                count++;
                audioChannel->queue->putAvpacket(avPacket);
            } else if (avPacket->stream_index == videoChannel->streamIndex) {
                // 视频
                videoChannel->queue->putAvpacket(avPacket);
            } else {
                // 一般走到这里，有字幕流的时候 再写
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);
            while (playstatus != NULL && !playstatus->exit) {
                if (audioChannel->queue->getQueueSize() > 0) {
                    continue;
                } else {
                    playstatus->exit = true;
                    break;
                }
            }
        }
    }

}

void *prepareDecode(void *nFfmpeg) {
    NFfmpeg *ffmpeg = static_cast<NFfmpeg *>(nFfmpeg);
    // 调用 decode 去解码
    ffmpeg->decode();
    pthread_exit(&(ffmpeg->pthread));


}

/**
 * 子线程去初始化
 */
void NFfmpeg::prepare() {
    // 初始化线程，把 this 传过去
    pthread_create(&pthread, 0, prepareDecode, this);
}

void NFfmpeg::decode() {
    // 上锁
    pthread_mutex_lock(&init_mutex);
    // 注册所有的组件，已经过时了，源码说没有必要注册了，已经测过了 可以不写
    avcodec_register_all();
    // 初始化网络模块 ，就可以直接播放 网上的视频，或者这 rtmp
    avformat_network_init();
    // 实例化上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();
    this->avFormatContext = avFormatContext;
    LOGE(" avFormatContext  --- %d this %d", avFormatContext, this->avFormatContext);
    // 打开一个文件 或者是 视频流
    int ret = avformat_open_input(&avFormatContext, url, NULL, NULL);
    if (ret != 0) {
        LOGE("打开文件失败 %s", url);
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    LOGI("打开文件成功 %s", url);

    // 查找文件的流信息
    int info = avformat_find_stream_info(avFormatContext, NULL);
    if (info < 0) {
        pthread_mutex_unlock(&init_mutex);
        LOGE("查找 stream  错误了 %s", url);
        return;
    }
    LOGI("查找stream  成功了");

    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        //得到音频流
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audioChannel) {
                audioChannel = NULL;
                delete audioChannel;
            }
            if (audioChannel == NULL) {
                audioChannel = new AudioChannel(playstatus,
                                                avFormatContext->streams[i]->codecpar->sample_rate,
                                                callJava);
                audioChannel->streamIndex = i;
                // native 层都是以 微妙为单位  微妙 转换成秒
                // 拿到总时长
                audioChannel->duration = avFormatContext->duration / AV_TIME_BASE;
                // 基本的时间单位(以秒为单位)
                audioChannel->time_base = avFormatContext->streams[i]->time_base;
                // 与此流相关的编解码器参数，设置 解码器上下文
                audioChannel->codecpar = avFormatContext->streams[i]->codecpar;
                LOGE("audioChannel11 %d", audioChannel->codecpar);
                duration = audioChannel->duration;
            }
        } else if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (videoChannel) {
                videoChannel = NULL;
                delete videoChannel;
            }
            if (videoChannel == NULL) {
                videoChannel = new VideoChannel(playstatus, callJava);
                videoChannel->streamIndex = i;
                videoChannel->codecpar = avFormatContext->streams[i]->codecpar;
                videoChannel->time_base = avFormatContext->streams[i]->time_base;
                int num = avFormatContext->streams[i]->avg_frame_rate.num;
                int den = avFormatContext->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0) {
                    int fps = num / den;//[25 / 1]
                    videoChannel->defaultDelayTime = 1.0 / fps;//秒
                }
            }
        }
    }


    if (videoChannel != NULL) {
        getCodecContext(videoChannel->codecpar, &videoChannel->avCodecContext);
    }
    if (audioChannel != NULL) {
        getCodecContext(audioChannel->codecpar, &audioChannel->avCodecContext);
    }
    LOGI("准备成功 回调给java层");
    // 准备成功，通知java层，可以调用start了
    callJava->onCallParpared(CHILD_THREAD);
    pthread_mutex_unlock(&init_mutex);
}

int NFfmpeg::getCodecContext(AVCodecParameters *codecpar, AVCodecContext **avCodecContext) {
    // 解码器
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    if (!dec) {
        LOGE(" 获取解码器AVCodec 失败");
        // 退出
        exit = true;
        // 释放初始化的锁
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    // 生成新的 AVCodecContext，解码器上下文
    *avCodecContext = avcodec_alloc_context3(dec);
    if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (!videoChannel->avCodecContext) {
            exit = true;
            pthread_mutex_unlock(&init_mutex);
            LOGE("视频 创建解码器 AVCodecContext 上下文失败");
            return -1;
        }

    } else {
        if (!audioChannel->avCodecContext) {
            exit = true;
            pthread_mutex_unlock(&init_mutex);
            LOGI("音频 创建解码器 AVCodecContext 上下文失败");
            return -1;
        }

    }


    // 根据编解码器的值填充编解码器上下文
    if (avcodec_parameters_to_context(*avCodecContext, codecpar) < 0) {
        LOGE("填充上下文失败");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avcodec_open2(*avCodecContext, dec, 0) != 0) {
        LOGE("打开流失败 strames");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        LOGI("打开音频流成功");
    } else {
        LOGI("打开视频流成功");
    }

    return 0;


}

NFfmpeg::~NFfmpeg() {
    pthread_mutex_destroy(&seek_mutex);
    pthread_mutex_destroy(&init_mutex);
}

void NFfmpeg::pause() {
    if (audioChannel) {
        audioChannel->pause();
    }
}

void NFfmpeg::resume() {
    if (audioChannel) {
        audioChannel->resume();
    }
}

void NFfmpeg::stop() {
    if (LOG_DEBUG) {
        LOGE("开始释放Ffmpe");
    }
    playstatus->exit = true;
//    队列        stop    exit
    int sleepCount = 0;
    pthread_mutex_lock(&init_mutex);
    while (!exit) {
        if (sleepCount > 1000) {
            exit = true;

        }
        LOGE("wait ffmpeg  exit %d", sleepCount);
        sleepCount++;
        av_usleep(1000 * 10);//暂停10毫秒
    }

    if (audioChannel != NULL) {
        audioChannel->release();
        delete (audioChannel);
        audioChannel = NULL;
    }

    if (LOG_DEBUG) {
        LOGE("释放 封装格式上下文");
    }
    if (avFormatContext != NULL) {
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 callJava");
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 playstatus");
    }
    if (playstatus != NULL) {
        playstatus = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

void NFfmpeg::setSpeed(float speed) {
    if (audioChannel) {
        // 因为视频追随音频 所以 直接管理音频即可
        audioChannel->setSpeed(speed);
    }

}

void NFfmpeg::seek(jint position) {
    if (duration <= 0) {
        return;
    }
    if (position >= 0 && position <= duration) {
        playstatus->seek = true;
        pthread_mutex_lock(&seek_mutex);
        int64_t rel = position * AV_TIME_BASE;
        LOGE("rel time %d", position);
        avformat_seek_file(avFormatContext, -1, INT64_MIN, rel, INT64_MAX, 0);
        if (audioChannel != NULL) {
            audioChannel->queue->clearAvpacket();
            audioChannel->clock = 0;
            audioChannel->last_tiem = 0;
            avcodec_flush_buffers(audioChannel->avCodecContext);
        }
        if (videoChannel != NULL) {
            videoChannel->queue->clearAvpacket();
            pthread_mutex_lock(&videoChannel->codecMutex);
            avcodec_flush_buffers(videoChannel->avCodecContext);
            pthread_mutex_unlock(&videoChannel->codecMutex);
        }
        pthread_mutex_unlock(&seek_mutex);
        playstatus->seek = false;
    }
}


