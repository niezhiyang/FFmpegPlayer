package com.nzy.ffmpegplayer;

/**
 * @author niezhiyang
 * since 11/8/21
 */
public class FFmpegPlayer {
    static {
        System.loadLibrary("native-lib");
    }

    private OnPreparedListener mOnPreparedListener;
    private String mSource;
    private PlayerGlSurfaceView mGlSurfaceView;
    public int duration;


    /**
     * 设置 PlayerGlSurfaceView ，用来播放视频
     *
     * @param glSurfaceView
     */
    public void setGlSurfaceView(PlayerGlSurfaceView glSurfaceView) {
        mGlSurfaceView = glSurfaceView;
    }


    public void setDataSource(String source) {
        mSource = source;
    }

    public void prepareAsync() {
        native_prepareAsync(mSource);
    }

    public void start() {
        native_start();
    }

    public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
        mOnPreparedListener = onPreparedListener;
    }


    private native void native_prepareAsync(String source);

    private native void native_start();

    private native void native_pause();

    private native void native_resume();

    private native void native_stop();

    private native void native_speed(float speed);
    private native void native_seek(int position);

    public void pause() {
        native_pause();
    }

    public void resume() {
        native_resume();
    }

    public void stop() {
        native_stop();
    }

    public void setSpeed(float speed) {
        native_speed(speed);
    }

    public void seek(int position) {
        native_seek(position);
    }


    public interface OnPreparedListener {
        /**
         * 准备完毕，可以调用start方法了
         */
        void onPrepared();

        /**
         * 回调当前播放的进度
         *
         * @param currentTime 当前时间
         * @param totalTime   总时间
         */
        void onCurrentTime(int currentTime, int totalTime);
    }

    /**
     * Native 层调用的，当FFmpeg 准备完毕的时候回调
     * 然后给用户层 ，准备完毕
     */
    public void onCallPrepared() {
        if (mOnPreparedListener != null) {
            mOnPreparedListener.onPrepared();
        }
    }

    /**
     * Native 层调用的，网络比较慢
     */
    public void onCallLoad(boolean load) {

    }

    /**
     * Native 层调用的，当FFmpeg 准备完毕的时候回调
     * 然后给用户层 ，准备完毕
     *
     * @param currentTime 当前时间
     * @param totalTime   总时间
     */
    public void onCallTimeInfo(int currentTime, int totalTime) {
        if (mOnPreparedListener == null) {
            duration = totalTime;
            mOnPreparedListener.onCurrentTime(currentTime, totalTime);
        }

    }

    /**
     * Native 层调用的，给咱们的YUV数据
     */
    public void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {

        if (mGlSurfaceView != null) {

            mGlSurfaceView.setYUVData(width, height, y, u, v);
        }


    }


}
