//
// Created by admin on 2020/12/23.
//

#include <pthread.h>
#include "JdFFmpeg.h"
#include "macro.h"

void *task_prepare(void *args) {
    LOGI("Method start---> JdFFmpeg task_prepare");
    JdFFmpeg *fFmpeg = static_cast<JdFFmpeg *>(args);
    fFmpeg->_prepare();
    LOGI("Method end---> JdFFmpeg task_prepare");
    return 0;
}

JdFFmpeg::JdFFmpeg(JavaCallHelper *callHelper, const char *dataSource) {
    this->callHelper = callHelper;
    //防止dataSource参数指向的内存被释放
    //strlen 获得字符串的长度 不包括\0
    this->dataSource = new char[strlen(dataSource) + 1];
    strcpy(this->dataSource, dataSource);

    isPlaying = false;
    duration = 0;

    pthread_mutex_init(&seekMutex, 0);
}

JdFFmpeg::~JdFFmpeg() {
    pthread_mutex_destroy(&seekMutex);
    //释放
    DELETE(dataSource);
    DELETE(callHelper);
}

void JdFFmpeg::prepare() {
    //创建一个线程
    LOGI("Method start---> JdFFmpeg prepare");
    pthread_create(&pid, 0, task_prepare, this);
    LOGI("Method end---> JdFFmpeg prepare");
}

void JdFFmpeg::_prepare() {
    LOGI("Method start---> JdFFmpeg _prepare");
    //初始化网络  让ffmpeg能够使用网络
    avformat_network_init();
    //1.打开媒体地址（文件地址，直播地址）
    //AVFormatContext  包含了视频的信息（宽 高等）
    formatContext = 0;
    //文件路径不对   手机没网
    int ret = avformat_open_input(&formatContext, dataSource, 0, 0);
    //ret不为0表示  打开媒体失败
    if (ret != 0) {
        LOGE("打开媒体失败:%s", av_err2str(ret));
        if (callHelper) {
            callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    //2.查找媒体中的音视频留（给context里的 streams等成员赋值）
    ret = avformat_find_stream_info(formatContext, 0);
    //小于0  失败
    if (ret < 0) {
        LOGE("查找流失败:%s", av_err2str(ret));
        if (callHelper) {
            callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    //视频时长（单位：微秒us，转换为秒需要除以1000000）
    duration = formatContext->duration / 1000000;

    //nb_streams: 几个流（极端视频/音频）
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //可能代表是一个视频  也可能代表是一个音频
        AVStream *stream = formatContext->streams[i];
        //包含了  解码  这段流 的各种参数信息（宽  高 码率  帧率）
        AVCodecParameters *codecpar = stream->codecpar;

        //无论视频还是音频都需要干一些事情（获得解码器）
        //1. 通过 当前流 使用 编码方式，查找编码器
        AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
        if (dec == NULL) {
            LOGE("查找解码器失败:%s", av_err2str(ret));
            if (callHelper) {
                callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }

        //2.获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(dec);
        if (context == NULL) {
            LOGE("创建解码器上下文失败:%s", av_err2str(ret));
            if (callHelper) {
                callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        //3.设置上线文内的一些参数（context->width）
        //context->width = codecParameters->width;
        //context->height = codecParameters->height;
        ret = avcodec_parameters_to_context(context, codecpar);
        //失败
        if (ret < 0) {
            LOGE("设置解码器上下文参数失败:%s", av_err2str(ret));
            if (callHelper) {
                callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        //4.打开解码器
        ret = avcodec_open2(context, dec, 0);
        if (ret != 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            if (callHelper) {
                callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        // 单位
        AVRational time_base = stream->time_base;
        //音频
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, callHelper, context, time_base);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //帧率： 单位时间内 需要显示多少个图像
            AVRational frame_rate = stream->avg_frame_rate;
            int fps = av_q2d(frame_rate);
            videoChannel = new VideoChannel(i, callHelper, context, time_base, fps);
            videoChannel->setRenderFrameCallback(callback);
        }
    }

    //没有音视频（很少见）
    if (!audioChannel && !videoChannel) {
        LOGE("没有音视频");
        if (callHelper) {
            callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    //准备完了  通知java
    if (callHelper) {
        callHelper->onPrepare(THREAD_CHILD);
    }
    LOGI("Method end---> JdFFmpeg _prepare");
}

void *task_play(void *args) {
    LOGI("Method start---> JdFFmpeg task_play");
    JdFFmpeg *ffmpeg = static_cast<JdFFmpeg *>(args);
    ffmpeg->_start();
    LOGI("Method end---> JdFFmpeg task_play");
    return 0;
}


void JdFFmpeg::start() {
    LOGI("Method start---> JdFFmpeg start");
    //正在播放
    isPlaying = 1;
    if (videoChannel) {
        videoChannel->play();
    }
    //启动声音的解码与播放
    if (audioChannel) {
        videoChannel->setAudioChannel(audioChannel);
        audioChannel->play();
    }

    pthread_create(&pid_play, 0, task_play, this);
    LOGI("Method end---> JdFFmpeg start");
}

/**
 * 专门读取数据包
 */
void JdFFmpeg::_start() {
    LOGI("Method start---> JdFFmpeg _start(");
    //1.读取媒体数据包(音视频数据包)
    int ret;
    while (isPlaying) {
        //读取文件的时候没有网络请求，一下子读完了，可能导致oom
        //特别是读本地文件的时候 一下子就读完了
        if (audioChannel && audioChannel->packets.size() > 100) {
            //10ms
            av_usleep(1000 * 10);
            continue;
        }
        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(formatContext, packet);
        //=0 成功  其他 失败
        if (ret == 0) {
            //stream_index 这一个流的一个序号
            if (audioChannel && packet->stream_index == audioChannel->id) {
                audioChannel->packets.push(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->id) {
                videoChannel->packets.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            //读取完成  但是可能还没播放完
            if (audioChannel->packets.empty() && audioChannel->frames.empty()
                && videoChannel->packets.empty() && videoChannel->frames.empty()) {
                break;
            }
            //为什么这里要让它继续循环 而不是sleep
            //如果是做直播 ，可以sleep
            //如果要支持点播(播放本地文件） seek 后退
        } else {

        }
    }
    isPlaying = 0;
    audioChannel->stop();
    videoChannel->stop();
    LOGI("Method end---> JdFFmpeg _start(");
}

void JdFFmpeg::setRenderFrameCallback(RenderFrameCallback callback) {
    this->callback = callback;
}

void *aync_stop(void *args) {
    JdFFmpeg *ffmpeg = static_cast<JdFFmpeg *>(args);
    //   等待prepare结束
    pthread_join(ffmpeg->pid, 0);
    // 保证 start线程结束
    pthread_join(ffmpeg->pid_play, 0);
    DELETE(ffmpeg->videoChannel);
    DELETE(ffmpeg->audioChannel);
    // 这时候释放就不会出现问题了
    if (ffmpeg->formatContext) {
        //先关闭读取 (关闭fileintputstream)
        avformat_close_input(&ffmpeg->formatContext);
        avformat_free_context(ffmpeg->formatContext);
        ffmpeg->formatContext = 0;
    }
    DELETE(ffmpeg);
    return 0;
}

void JdFFmpeg::stop() {
    isPlaying = 0;
    callHelper = 0;
    if (audioChannel) {
        audioChannel->callHelper = 0;
    }
    if (videoChannel) {
        videoChannel->callHelper = 0;
    }
    // formatContext
    pthread_create(&pid_stop, 0, aync_stop, this);
}


void JdFFmpeg::seek(int progress) {
    //进去必须 在0- duration 范围之类
    if (progress < 0 || progress >= duration) {
        return;
    }
    if (!audioChannel && !videoChannel) {
        return;
    }
    if (!formatContext) {
        return;
    }

    isSeek = 1;
    pthread_mutex_lock(&seekMutex);
    //单位是 微妙
    int64_t seek = progress * 1000000;

    //seek到请求的时间 之前最近的关键帧
    // 只有从关键帧才能开始解码出完整图片
    av_seek_frame(formatContext, -1, seek, AVSEEK_FLAG_BACKWARD);

    // 音频、与视频队列中的数据 是不是就可以丢掉了？
    if (audioChannel) {
        //暂停队列
        audioChannel->stopWork();
        //可以清空缓存
//        avcodec_flush_buffers();
        audioChannel->clear();
        //启动队列
        audioChannel->startWork();
    }

    if (videoChannel) {
        videoChannel->stopWork();
        videoChannel->clear();
        videoChannel->startWork();
    }
    pthread_mutex_unlock(&seekMutex);
    isSeek = 0;
}