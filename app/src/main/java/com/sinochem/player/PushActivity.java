package com.sinochem.player;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;

import com.sinochem.player.live.LivePusher;

import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public class PushActivity extends AppCompatActivity implements View.OnClickListener {

    static {
        System.loadLibrary("native-lib");
    }

    private LivePusher livePusher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_push);
        initView();
        myRequetPermission();
    }

    private void initView() {
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        findViewById(R.id.switch_camera).setOnClickListener(this);
        findViewById(R.id.start_push).setOnClickListener(this);
        findViewById(R.id.stop_push).setOnClickListener(this);
        livePusher = new LivePusher(this, 800, 480, 800_000, 10, Camera.CameraInfo.CAMERA_FACING_BACK);
        //  设置摄像头预览的界面
        livePusher.setPreviewDisplay(surfaceView.getHolder());

    }

    private void myRequetPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE
                    , Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO}, 101);
        } else {
            Log.d("MainActivity", "您已经申请权限");
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.switch_camera:
                livePusher.switchCamera();
                break;
            case R.id.start_push:
                livePusher.startLive("rtmp://120.53.242.105/myapp/yueyue");
                break;
            case R.id.stop_push:
                livePusher.stopLive();
                break;
        }
    }
}