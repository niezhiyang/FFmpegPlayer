
cmake_minimum_required(VERSION 3.10.2)
# 添加整个文件夹的.h 这是ffmpeg的
include_directories(include)

# 添加整个文件夹的.h 这是 soundtouch 的，用于处理 pcm 变速的
include_directories(soundtouch/include)
include_directories(soundtouch/SoundTouch)


project("ffmpegplayer")
# 把 CMakeLists.txt本目录下的所有的 cpp文件都 引入到 cpp_source 这个变量
#file(GLOB cpp_source *.cpp)
# 和下面一个道理，都可以直接放到 cpp_source 中
aux_source_directory(. cpp_source)

# 添加 soundtoucch 的所有cpp 到 SOURCE 变量中
aux_source_directory(./soundtouch/SoundTouch SOURCE)

# 相当于给下面的 target_link_libraries 配置了一个系统路径
#比如 log，会从系统的路径找，所以 NFfmpeg 中的so 也会从系统找，如果找不到就从这个路径找，
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}")
add_library(
        native-lib
        SHARED
        ${cpp_source}
        ${SOURCE}
)


find_library(
        log-lib

        log)


target_link_libraries(
        native-lib
        ${log-lib}
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
        OpenSLES
        # 系统 android.so 主要用Window.jni，如果不用NativeWindow,则不用添加
        android
)