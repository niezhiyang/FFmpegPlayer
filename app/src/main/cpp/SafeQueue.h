//
// Created by yangw on 2018-3-6.
//

#ifndef MYMUSIC_WLQUEUE_H
#define MYMUSIC_WLQUEUE_H

#include "queue"
#include "pthread.h"
#include "Playstatus.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "include/libavcodec/packet.h"
};


class SafeQueue {

public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    Playstatus *playstatus = NULL;

public:

    SafeQueue(Playstatus *playstatus);
    ~SafeQueue();

    int putAvpacket(AVPacket *packet);
    int getAvpacket(AVPacket *packet);

    int getQueueSize();

    void clearAvpacket();




};


#endif //MYMUSIC_WLQUEUE_H
