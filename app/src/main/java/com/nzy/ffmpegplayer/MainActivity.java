package com.nzy.ffmpegplayer;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import java.io.File;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "zhiyang";
    private boolean seek = false;
    private TextView tvTime;
    private PlayerGlSurfaceView glSurfaceView;
    private SeekBar seekBar;
    private FFmpegPlayer mPlayer;
    private int position;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Utils.copyAssets(this, "hashiqi.mp4");
        Utils.copyAssets(this, "Dance.mp3");
        tvTime = findViewById(R.id.tv_time);
        glSurfaceView = findViewById(R.id.surfaceview);
        seekBar = findViewById(R.id.seekbar);
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                position = progress * mPlayer.duration/ 100;
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                seek = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                mPlayer.seek(position);
                seek = false;
            }
        });

    }

    public void begin(View view) {
        File hashiqi = new File(getFilesDir(), "hashiqi.mp4");
        mPlayer = new FFmpegPlayer();
        mPlayer.setGlSurfaceView(glSurfaceView);
        mPlayer.setDataSource(hashiqi.getAbsolutePath());
        mPlayer.prepareAsync();
        mPlayer.setOnPreparedListener(new FFmpegPlayer.OnPreparedListener() {
            @Override
            public void onPrepared() {
                Log.i(TAG, "Java收到准备完毕了");
                mPlayer.start();
            }

            @Override
            public void onCurrentTime(int currentTime, int totalTime) {
                Log.i(TAG, "Java收到准备完毕了 " + Thread.currentThread().getName());
                if (!seek && totalTime > 0) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            seekBar.setProgress(currentTime * 100 / totalTime);
                            tvTime.setText(Utils.secdsToDateFormat(currentTime)
                                    + "/" + Utils.secdsToDateFormat(totalTime));
                        }
                    });

                }
            }
        });
    }


    public void pause(View view) {

        mPlayer.pause();

    }

    public void resume(View view) {
        mPlayer.resume();
    }


    public void stop(View view) {
        mPlayer.stop();
    }


    public void next(View view) {
        //wlPlayer.playNext("/mnt/shared/Other/testvideo/楚乔传第一集.mp4");
    }

    public void speed1(View view) {
        mPlayer.setSpeed(1.5f);

    }

    public void speed2(View view) {
        mPlayer.setSpeed(2.0f);
    }
}