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
}

JdFFmpeg::~JdFFmpeg() {
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
        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }

    //2.查找媒体中的音视频留（给context里的 streams等成员赋值）
    ret = avformat_find_stream_info(formatContext, 0);
    //小于0  失败
    if (ret < 0) {
        LOGE("查找流失败:%s", av_err2str(ret));
        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        return;
    }

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
            callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }

        //2.获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(dec);
        if (context == NULL) {
            LOGE("创建解码器上下文失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }

        //3.设置上线文内的一些参数（context->width）
        //context->width = codecParameters->width;
        //context->height = codecParameters->height;
        ret = avcodec_parameters_to_context(context, codecpar);
        //失败
        if (ret < 0) {
            LOGE("设置解码器上下文参数失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }

        //4.打开解码器
        ret = avcodec_open2(context, dec, 0);
        if (ret != 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            return;
        }
        //音频
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, context);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoChannel = new VideoChannel(i, context);
            videoChannel->setRenderFrameCallback(callback);
        }
    }

    //没有音视频（很少见）
    if (!audioChannel && !videoChannel) {
        LOGE("没有音视频");
        callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        return;
    }

    //准备完了  通知java
    callHelper->onPrepare(THREAD_CHILD);
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
    if (audioChannel){
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
        } else {

        }
    }
    LOGI("Method end---> JdFFmpeg _start(");
}

void JdFFmpeg::setRenderFrameCallback(RenderFrameCallback callback) {
    this->callback = callback;
}
