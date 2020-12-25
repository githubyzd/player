package com.sinochem.player;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

public class PlayActivity extends AppCompatActivity implements View.OnClickListener {
    private SurfaceView surfaceView;
    private JdPlayer player;
    private String TAG = "PlayActivity";
    public static String URL_KEY = "url_key";
    public static String TITLE_KEY = "title_key";

    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_play);
        initView();
        initData();
    }

    private void initData() {
        Intent intent = getIntent();
        String title = intent.getStringExtra(TITLE_KEY);
        setTitle(title);
        String url = intent.getStringExtra(URL_KEY);
        player.setDataSource(url);
    }

    private void initView() {
        surfaceView = findViewById(R.id.surface_view);
        TextView info = findViewById(R.id.info);
        info.setText("播放器版本:" + stringFromJNI());
        findViewById(R.id.play).setOnClickListener(this);
        findViewById(R.id.pause).setOnClickListener(this);

        player = new JdPlayer();
        player.setSurfaceView(surfaceView);
        player.setOnPrepareListener(() -> {
            Log.d(TAG, "准备好了");
            player.start();
        });
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.play:
                player.prepare();
                break;
            case R.id.pause:
                break;
        }
    }


    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
    }

    public void start(View view) {
        player.prepare();
    }

    native String stringFromJNI();
}