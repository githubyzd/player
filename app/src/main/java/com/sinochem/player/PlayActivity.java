package com.sinochem.player;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class PlayActivity extends AppCompatActivity implements View.OnClickListener, SeekBar.OnSeekBarChangeListener {
    private SurfaceView surfaceView;
    private SeekBar seekBar;
    private JdPlayer player;
    private String TAG = "PlayActivity";
    public static String URL_KEY = "url_key";
    public static String TITLE_KEY = "title_key";
    private int progress;
    private boolean isTouch;
    private boolean isSeek;

    static {
        System.loadLibrary("native-lib");
    }

    private ProgressBar progressBar;
    private FrameLayout container;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager
                .LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_play);
        initView();
        initData();
    }

    private void initData() {
        Intent intent = getIntent();
        String title = intent.getStringExtra(TITLE_KEY);


        String url = intent.getStringExtra(URL_KEY);
        player.setDataSource(url);
        setTitle(title);

        if (url.startsWith("/storage")){
            setHeight();
        }
    }


    private void initView() {
        surfaceView = findViewById(R.id.surface_view);
        progressBar = findViewById(R.id.progress);
        container = findViewById(R.id.container);
        TextView info = findViewById(R.id.info);
        info.setText("播放器版本:" + stringFromJNI());
        findViewById(R.id.play).setOnClickListener(this);
        findViewById(R.id.pause).setOnClickListener(this);
        seekBar = findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(this);

        player = new JdPlayer();
        player.setSurfaceView(surfaceView);
        player.setOnPrepareListener(new JdPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                Log.d(TAG, "准备好了");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        progressBar.setVisibility(View.GONE);
                        Toast.makeText(PlayActivity.this, "开始播放", Toast.LENGTH_LONG).show();
                        int duration = player.getDuration();
                        if (duration != 0) {
                            seekBar.setVisibility(View.VISIBLE);
                        }
                    }
                });

                player.start();
            }

            @Override
            public void onProgress(int progress) {
                if (!isTouch) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            int duration = player.getDuration();
                            //如果是直播
                            if (duration != 0) {
                                if (isSeek) {
                                    isSeek = false;
                                    return;
                                }
                                //更新进度 计算比例
                                seekBar.setProgress(progress * 100 / duration);
                            }
                        }
                    });
                }
            }
        });
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.play:
                progressBar.setVisibility(View.VISIBLE);
                player.prepare();
                break;
            case R.id.pause:
                player.stop();
                break;
        }
    }


    @Override
    protected void onResume() {
        super.onResume();
        player.prepare();
    }

    @Override
    protected void onStop() {
        super.onStop();
        progressBar.setVisibility(View.VISIBLE);
        player.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
    }

    native String stringFromJNI();

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        isTouch = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        isSeek = true;
        isTouch = false;
        progress = player.getDuration() * seekBar.getProgress() / 100;
        //进度调整
        player.seek(progress);
    }

    private void setHeight() {
        LinearLayout.LayoutParams layoutParams = (LinearLayout.LayoutParams) container.getLayoutParams();
        layoutParams.height = 0;
        layoutParams.weight = 1;
        container.setLayoutParams(layoutParams);
    }

    //dip和px之间的转换
    public static int dp2px(Context context, float dipValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (dipValue * scale + 0.5f);
    }
}