#include <string>
#include "JdFFmpeg.h"
#include "JavaCallHelper.h"
#include <android/native_window_jni.h>
#include "macro.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

JavaVM *javaVm = 0;
JdFFmpeg *ffmpeg = 0;
ANativeWindow *window = 0;
JavaCallHelper *helper = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int JNI_OnLoad(JavaVM *vm, void *r) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_sinochem_player_PlayActivity_stringFromJNI(JNIEnv *env, jobject thiz) {
    std::string hello = av_version_info();
    return env->NewStringUTF(hello.c_str());
}

//画画
void render(uint8_t *data, int lineszie, int w, int h) {
    LOGI("Method start---> native-lib render");
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, w,
                                     h,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //填充rgb数据给dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    // stride：一行多少个数据（RGBA） *4
    int dst_linesize = window_buffer.stride * 4;
    //一行一行的拷贝
    for (int i = 0; i < window_buffer.height; ++i) {
        //memcpy(dst_data , data, dst_linesize);
        memcpy(dst_data + i * dst_linesize, data + i * lineszie, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
    LOGI("Method end---> native-lib render");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_JdPlayer_native_1prepare(JNIEnv *env, jobject instance,
                                                  jstring dataSource_) {
    const char *dataSource = env->GetStringUTFChars(dataSource_, 0);
    LOGI("Method start---> native-lib prepare");
    //创建播放器
    helper = new JavaCallHelper(javaVm, env, instance);
    ffmpeg = new JdFFmpeg(helper, dataSource);
    ffmpeg->setRenderFrameCallback(render);
    ffmpeg->prepare();
    env->ReleaseStringUTFChars(dataSource_, dataSource);
    LOGI("Method end---> native-lib prepare");
}


extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_JdPlayer_native_1start(JNIEnv *env, jobject instance) {
    LOGI("Method start---> native-lib start");
    if (ffmpeg) {
        ffmpeg->start();
    }
    LOGI("Method end---> native-lib start");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_JdPlayer_native_1setSurface(JNIEnv *env, jobject instance,
                                                     jobject surface) {
    LOGI("Method start---> native-lib setSurface");
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
    LOGI("Method end---> native-lib setSurface");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_JdPlayer_native_1stop(JNIEnv *env, jobject thiz) {
    if (ffmpeg) {
        ffmpeg->stop();
    }
    DELETE(helper);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_JdPlayer_native_1release(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_sinochem_player_JdPlayer_native_1getDuration(JNIEnv *env, jobject thiz) {
    if (ffmpeg) {
        return ffmpeg->getDuration();
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_JdPlayer_native_1seek(JNIEnv *env, jobject thiz, jint progress) {

    if (ffmpeg) {
        ffmpeg->seek(progress);
    }

}