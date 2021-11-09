package com.nzy.ffmpegplayer;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

/**
 * @author niezhiyang
 * since 11/8/21
 */
public class PlayerGlSurfaceView extends GLSurfaceView {
    private PlayRender mPlayRender;

    public PlayerGlSurfaceView(Context context) {
        super(context);
    }

    public PlayerGlSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        // 设置 OpenGl 的版本，一般都是 2
        setEGLContextClientVersion(2);
        mPlayRender = new PlayRender(context);
        // 给 GlSurfaceView 设置 render
        setRenderer(mPlayRender);
        // 刷新模式
        // 1. GLSurfaceView.RENDERMODE_CONTINUOUSLY 是随着系统的刷新去刷新，比如60Hz，
        // 2. GLSurfaceView.RENDERMODE_WHEN_DIRTY 自己手动刷新，必须调用 requestRender() 来刷新
        //    刷新时会走 PlayRender 的 onDrawFrame 方法
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    /**
     * 当 NFfmpeg 解码出来视频的每一帧都会来调用这里去刷新
     *
     * @param width
     * @param height
     * @param y
     * @param u
     * @param v
     */
    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (mPlayRender != null) {
            mPlayRender.setYUVRenderData(width, height, y, u, v);
            requestRender();
        }
    }

}
