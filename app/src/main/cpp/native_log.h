//
// Created by nzy on 11/8/21.
//

#ifndef FFMPEGPLAYER_NATIVE_LOG_H
#define FFMPEGPLAYER_NATIVE_LOG_H


#include "android/log.h"

#define LOG_DEBUG true

#define TAG "zhiyang"

#define LOGD(...) if(LOG_DEBUG){\
__android_log_print(ANDROID_LOG_DEFAULT,TAG,__VA_ARGS__);\
}

#define LOGE(...) if(LOG_DEBUG){\
__android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__);\
}

#define LOGI(...) if(LOG_DEBUG){\
__android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__);\
}

#define LOGW(...) if(LOG_DEBUG){\
__android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__);\
}

#define LOGA(...) __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)


#endif //FFMPEGPLAYER_NATIVE_LOG_H
