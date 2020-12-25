package com.sinochem.player;

import android.view.Surface;
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
        native_setSurface(holder.getSurface());
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
        native_start();
    }

    /**
     * 停止播放
     */
    public void stop() {
        native_stop();
    }

    public void release() {
        holder.removeCallback(this);
        native_release();
    }

    public int getDuration() {
        return native_getDuration();
    }

    public void seek(int progress) {
        new Thread() {
            @Override
            public void run() {
                native_seek(progress);
            }
        }.start();
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

    native void native_start();

    native void native_stop();

    native void native_release();

    native void native_setSurface(Surface surface);

    private native int native_getDuration();

    private native void native_seek(int progress);
}
