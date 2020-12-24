package com.sinochem.player;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    private SurfaceView surfaceView;
    private JdPlayer player;
    private String TAG = "MainActivity";

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface_view);
        TextView info = findViewById(R.id.info);
        info.setText("播放器版本:" + stringFromJNI());

        player = new JdPlayer();
        player.setSurfaceView(surfaceView);
        player.setDataSource("rtmp://58.200.131.2:1935/livetv/cctv6");
        player.setOnPrepareListener(new JdPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                Log.d(TAG, "准备好了");
                player.start();
            }
        });
    }


    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
        player.stop();
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