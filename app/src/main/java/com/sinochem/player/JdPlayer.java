package com.sinochem.player;

import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class JdPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("native-lib");
    }

    private String dataSource;
    private SurfaceHolder holder;
    private OnPrepareListener listener;

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */

    public void setSurfaceView(SurfaceView surfaceView) {
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    public void setOnPrepareListener(OnPrepareListener listener) {
        this.listener = listener;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    /**
     * 准备好 要播放的视频
     */
    public void prepare() {
        native_prepare(dataSource);
    }


    /**
     * 开始播放
     */
    public void start() {

    }

    /**
     * 停止播放
     */
    public void stop() {

    }

    public void release() {
        holder.removeCallback(this);
    }


    public interface OnPrepareListener {
        void onPrepare();
    }


    public void onError(int errorCode) {
        System.out.println("Java接到回调:" + errorCode);
    }


    public void onPrepare() {
        if (null != listener) {
            listener.onPrepare();
        }
    }


    native void native_prepare(String dataSource);

}
