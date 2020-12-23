package com.sinochem.player;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private SurfaceView surfaceView;
    private JdPlayer player;
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface_view);
        TextView info = findViewById(R.id.info);
        info.setText(stringFromJNI());

//        player = new JdPlayer();
//        player.setSurfaceView(surfaceView);
//        player.setDataSource("http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8");
    }


    public void start(View view) {
        player.prepare();
    }

    native String stringFromJNI();
}