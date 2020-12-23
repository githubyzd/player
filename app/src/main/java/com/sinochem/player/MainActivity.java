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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface_view);
//        TextView info = findViewById(R.id.info);
//        info.setText(stringFromJNI());

        player = new JdPlayer();
        player.setSurfaceView(surfaceView);
        player.setDataSource("http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8");
        player.setOnPrepareListener(new JdPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {

                int i = 1;
                Log.e("aaaaa", "成功了");
            }
        });
    }


    public void start(View view) {
        player.prepare();
    }

}