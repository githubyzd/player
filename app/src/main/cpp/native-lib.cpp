#include <string>
#include "JdFFmpeg.h"
#include "JavaCallHelper.h"
#include <android/native_window_jni.h>
#include "macro.h"
#include "librtmp/rtmp.h"
#include <jni.h>
#include "VideoPushChannel.h"

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
    RTMP_Alloc();
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
        ffmpeg =0;
    }
    if (helper) {
        DELETE(helper);
    }
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

//push
SafeQueue<RTMPPacket *> packets;
VideoPushChannel *videoChannel = 0;
int isStart = 0;
pthread_t pid;

int readyPushing = 0;
uint32_t start_time;

void releasePackets(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        delete packet;
        packet = 0;
    }
}

void callback(RTMPPacket *packet) {
    if (packet) {
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        packets.push(packet);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_live_LivePusher_native_1init(JNIEnv *env, jobject instance) {
    //准备一个Video编码器的工具类 ：进行编码
    videoChannel = new VideoPushChannel;
    videoChannel->setVideoCallback(callback);
    //准备一个队列,打包好的数据 放入队列，在线程中统一的取出数据再发送给服务器
    packets.setReleaseCallbackPush(releasePackets);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_live_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject instance,
                                                                 jint width, jint height, jint fps,
                                                                 jint bitrate) {

    if (videoChannel) {
        videoChannel->setVideoEncInfo(width, height, fps, bitrate);
    }

}

void *start(void *args) {
    char *url = static_cast<char *>(args);
    RTMP *rtmp = 0;
    do {
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            LOGE("rtmp创建失败");
            break;
        }
        RTMP_Init(rtmp);
        //设置超时时间 5s
        rtmp->Link.timeout = 5;
        int ret = RTMP_SetupURL(rtmp, url);
        if (!ret) {
            LOGE("rtmp设置地址失败:%s", url);
            break;
        }
        //开启输出模式
        RTMP_EnableWrite(rtmp);
        ret = RTMP_Connect(rtmp, 0);
        if (!ret) {
            LOGE("rtmp连接地址失败:%s", url);
            break;
        }
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            LOGE("rtmp连接流失败:%s", url);
            break;
        }

        //准备好了 可以开始推流了
        readyPushing = 1;
        //记录一个开始推流的时间
        start_time = RTMP_GetTime();
        packets.setWork(1);
        RTMPPacket *packet = 0;
        //循环从队列取包 然后发送
        while (isStart) {
            packets.pop(packet);
            if (!isStart) {
                break;
            }
            if (!packet) {
                continue;
            }
            // 给rtmp的流id
            packet->m_nInfoField2 = rtmp->m_stream_id;
            //发送包 1:加入队列发送
            ret = RTMP_SendPacket(rtmp, packet, 1);
            releasePackets(packet);
            if (!ret) {
                LOGE("发送数据失败");
                break;
            }
        }
        releasePackets(packet);
    } while (0);
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete url;
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_live_LivePusher_native_1start(JNIEnv *env, jobject instance,
                                                       jstring path_) {
    if (isStart) {
        return;
    }
    const char *path = env->GetStringUTFChars(path_, 0);
    char *url = new char[strlen(path) + 1];
    strcpy(url, path);
    isStart = 1;
    //启动线程
    pthread_create(&pid, 0, start, url);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_live_LivePusher_native_1pushVideo(JNIEnv *env, jobject instance,
                                                           jbyteArray data_) {
    if (!videoChannel || !readyPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    videoChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_live_LivePusher_native_1stop(JNIEnv *env, jobject thiz) {
    // TODO: implement native_stop()
}extern "C"
JNIEXPORT void JNICALL
Java_com_sinochem_player_live_LivePusher_native_1release(JNIEnv *env, jobject thiz) {
    // TODO: implement native_release()
}