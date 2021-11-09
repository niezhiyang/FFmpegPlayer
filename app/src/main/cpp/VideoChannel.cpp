//
// Created by nzy on 11/8/21.
//

#include "VideoChannel.h"



VideoChannel::VideoChannel(Playstatus *playstatus, CallJava *wlCallJava) {
    this->playstatus = playstatus;
    this->wlCallJava = wlCallJava;
    queue = new SafeQueue(playstatus);
    pthread_mutex_init(&codecMutex, NULL);
}

void *playVideo(void *data) {
    VideoChannel *video = static_cast<VideoChannel *>(data);
    //   音频是通过喇叭来做驱动的，视频只能循环轮训取数据
    while (video->playstatus != NULL && !video->playstatus->exit) {
//         解码 seek   puase   队列没有数据
        if (video->playstatus->seek) {
            av_usleep(1000 * 100);
            continue;
        }
        if (video->playstatus->pause) {
            av_usleep(1000 * 100);
            continue;
        }
        if (video->queue->getQueueSize() == 0) {
            //   还没解码出来，网络不佳  请慢慢等待  回调应用层
            if (!video->playstatus->load) {
                video->playstatus->load = true;
                video->wlCallJava->onCallLoad(CHILD_THREAD, true);
                av_usleep(1000 * 100);
                continue;
            }

        }

        AVPacket *avPacket = av_packet_alloc();
        // 取出来 h264 数据
        if (video->queue->getAvpacket(avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        //    视频解码 比较耗时  多线程环境
        pthread_mutex_lock(&video->codecMutex);
        //解码操作
        if (avcodec_send_packet(video->avCodecContext, avPacket) != 0) {
            //  失败了，就释放这一次的数据，这一帧吧
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&video->codecMutex);
            continue;
        }
        AVFrame *avFrame = av_frame_alloc();

        if (avcodec_receive_frame(video->avCodecContext, avFrame) != 0) {
            // 失败了，就释放这一次的数据，这一帧吧
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&video->codecMutex);
            continue;
        }
        //此时解码成功了  如果 之前是yuv420  ----》   opengl
        if (avFrame->format == AV_PIX_FMT_YUV420P) {

            double diff = video->getFrameDiffTime(avFrame);
//            通过diff 计算休眠时间
            av_usleep(video->getDelayTime(diff) * 1000000);
            video->wlCallJava->onCallRenderYUV(
                    CHILD_THREAD,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    avFrame->data[0],
                    avFrame->data[1],
                    avFrame->data[2]);
//            LOGE("当前视频是YUV420P格式");
        } else {
//            LOGE("当前视频不是YUV420P格式，转化成 YUV420");
            AVFrame *pFrameYUV420P = av_frame_alloc();
            int num = av_image_get_buffer_size(
                    AV_PIX_FMT_YUV420P,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    1);
            uint8_t *buffer = static_cast<uint8_t *>(av_malloc(num * sizeof(uint8_t)));
            av_image_fill_arrays(
                    pFrameYUV420P->data,
                    pFrameYUV420P->linesize,
                    buffer,
                    AV_PIX_FMT_YUV420P,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    1);
            SwsContext *sws_ctx = sws_getContext(
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    video->avCodecContext->pix_fmt,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    AV_PIX_FMT_YUV420P,
                    SWS_BICUBIC, NULL, NULL, NULL);

            if (!sws_ctx) {
                av_frame_free(&pFrameYUV420P);
                av_free(pFrameYUV420P);
                av_free(buffer);
                pthread_mutex_unlock(&video->codecMutex);
                continue;
            }
            sws_scale(
                    sws_ctx,
                    reinterpret_cast<const uint8_t *const *>(avFrame->data),
                    avFrame->linesize,
                    0,
                    avFrame->height,
                    pFrameYUV420P->data,
                    pFrameYUV420P->linesize);
            //渲染
            video->wlCallJava->onCallRenderYUV(
                    CHILD_THREAD,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    pFrameYUV420P->data[0],
                    pFrameYUV420P->data[1],
                    pFrameYUV420P->data[2]);

            av_frame_free(&pFrameYUV420P);
            av_free(pFrameYUV420P);
            av_free(buffer);
            sws_freeContext(sws_ctx);
        }
        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        pthread_mutex_unlock(&video->codecMutex);
    }
    pthread_exit(&video->thread_play);
}

void VideoChannel::play() {
//    子线程播放   解码
    pthread_create(&thread_play, NULL, playVideo, this);

}

double VideoChannel::getFrameDiffTime(AVFrame *avFrame) {
//    先获取视频时间戳  处理之后
    double pts = av_frame_get_best_effort_timestamp(avFrame);
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
//     1.001*40ms
//    pts=pts * time_base.num / time_base.den;
    pts *= av_q2d(time_base);

    if (pts > 0) {
        clock = pts;
    }
    // 计算当前 音视频的 差值
    double diff = audio->clock - clock;
    return diff;
}

// 33ms  -----》动态计算
double VideoChannel::getDelayTime(double diff) {

//    返回秒数 3ms 以内
//音频超越视频  3ms   1

//视频超越  音频3ms   2
    if (diff > 0.003) {
//        视频休眠时间
        delayTime = delayTime * 2 / 3;// * 3/2;
        if (delayTime < defaultDelayTime / 2) {
//            用户有所察觉
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) {
//视频超前    休眠时间 相比于以前大一些
        delayTime = delayTime * 3 / 2;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    }
//感觉的 视频加速
    if (diff >= 0.5) {
        delayTime = 0;
    } else if (diff <= -0.5) {
        delayTime = defaultDelayTime * 2;
    }
//    音频太快了   视频怎么赶也赶不上        视频队列全部清空   直接解析最新的 最新鲜的
    if (diff >= 10) {
        queue->clearAvpacket();
        delayTime = defaultDelayTime;
    }
//视频太快了  音频赶不上
    if (diff <= -10) {
        audio->queue->clearAvpacket();
        delayTime = defaultDelayTime;
        LOGE("====================>视频太快了");
    }
    return delayTime;
}

VideoChannel::~VideoChannel() {

}
