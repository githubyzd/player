//
// Created by Administrator on 2018/9/5.
//

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include "VideoChannel.h"
#include "macro.h"

void *decode_task(void *args) {
    LOGI("Method start---> VideoChannel decode_task");
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->decode();
    LOGI("Method end---> VideoChannel decode_task");
    return 0;
}

void *render_task(void *args) {
    LOGI("Method start---> VideoChannel render_task");
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->render();
    LOGI("Method end---> VideoChannel render_task");
    return 0;
}

/**
 * 丢包 直到下一个关键帧
 * @param q
 */

void dropAvPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *packet = q.front();
        //如果不属于 I 帧
        if (packet->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releaseAvPacket(&packet);
            q.pop();
        } else {
            break;
        }
    }
}

/**
 * 丢已经解码的图片
 * @param q
 */
void dropAvFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAvFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                           AVRational time_base, int fps) :
        BaseChannel(id, javaCallHelper, avCodecContext, time_base) {
    this->fps = fps;
    //  用于 设置一个 同步操作 队列的一个函数指针
//    packets.setSyncHandle(dropAvPacket);
    frames.setSyncHandle(dropAvFrame);
}

VideoChannel::~VideoChannel() {
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;
}

void VideoChannel::play() {
    LOGI("Method start---> VideoChannel play");
    //设置为工作状态
    isPlaying = 1;
    frames.setWork(1);
    packets.setWork(1);
    //1、解码
    pthread_create(&pid_decode, 0, decode_task, this);
    //2、播放
    pthread_create(&pid_render, 0, render_task, this);
    LOGI("Method end---> VideoChannel play");
}

//解码
void VideoChannel::decode() {
    LOGI("Method start---> VideoChannel decode");
    AVPacket *packet = 0;
    while (isPlaying) {
        //取出一个数据包
        int ret = packets.pop(packet);
        if (!isPlaying) {
            break;
        }
        //取出失败
        if (!ret) {
            continue;
        }
        //把包丢给解码器
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(&packet);
        //重试
        if (ret != 0) {
            break;
        }
        //代表了一个图像 (将这个图像先输出来)
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取 解码后的数据包 AVFrame
        ret = avcodec_receive_frame(avCodecContext, frame);
        //需要更多的数据才能够进行解码
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        //播放本地文件  解码过快
        while (frames.size() > queueSize && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
        //再开一个线程 来播放 (流畅度)
        frames.push(frame);
    }
    releaseAvPacket(&packet);
    LOGI("Method end---> VideoChannel decode");
}

//播放
void VideoChannel::render() {
    LOGI("Method start---> VideoChannel render");
    //目标： RGBA
    swsContext = sws_getContext(
            avCodecContext->width, avCodecContext->height, avCodecContext->pix_fmt,
            avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);
    //每个画面 刷新的间隔 单位：秒
    double frame_delay = 1.0 / fps;
    AVFrame *frame = 0;
    //指针数组
    uint8_t *dst_data[4];
    int dst_linesize[4];
    av_image_alloc(dst_data, dst_linesize,
                   avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA, 1);

    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

#if 1
        /**
      *  seek需要注意的点：编码器中存在缓存
      *  100s 的图像,用户seek到第 50s 的位置
      *  音频是50s的音频，但是视频 你获得的是100s的视频
      */

        //显示时间戳 什么时候显示这个frame
        if ((clock = frame->best_effort_timestamp) == AV_NOPTS_VALUE) {
            clock = 0;
        }
        //pts 单位就是time_base
        //av_q2d转为双精度浮点数 乘以 pts 得到pts --- 显示时间:秒
        clock = clock * av_q2d(time_base);
        //frame->repeat_pict = 当解码时，这张图片需要要延迟多久显示
        //需要求出扩展延时：
        double repeat_pict = frame->repeat_pict;
        double extra_delay = repeat_pict / (2 * fps);


        // 真实需要的间隔时间
        double delay = extra_delay + frame_delay;
        if (clock == 0) {
            //正常播放
            av_usleep(delay * 1000000);
        } else {
            double audioClock = audioChannel ? audioChannel->clock : 0;
            double diff = fabs(clock - audioClock);
//            LOGE("当前和音频比较:%f - %f = %f", clock, audioClock, diff);
            //允许误差 diff > 0.04 &&
            if (audioChannel) {
                //如果视频比音频快，延迟差值播放，否则直接播放
                if (clock > audioClock) {
                    LOGE("视频快了：%lf", diff);
                    if (diff > 1) {
                        //差的太久了， 那只能慢慢赶 不然就是卡好久
                        av_usleep((delay * 2) * 1000000);
                    } else {
                        //差的不多，尝试一次赶上去
                        av_usleep((delay + diff) * 1000000);
                    }
                } else {
                    LOGE("音频快了：%lf", diff);
                    //音频比视频快
                    //视频慢了 0.05s 已经比较明显了 (丢帧)
                    if (diff > 1) {
                        //一种可能： 快进了(因为解码器中有缓存数据，这样获得的avframe就和seek的匹配了)
                    } else if (diff >= 0.05) {
                        releaseAvFrame(&frame);
                        //执行同步操作 删除到最近的key frame
                        frames.sync();
                        continue;
                    } else {
                        //不休眠 加快速度赶上去
                    }
                }
            } else {
                //正常播放
                av_usleep(delay * 1000000);
            }
        }

#endif
        //diff太大了不回调了
        if (callHelper && audioChannel) {
            callHelper->onProgress(THREAD_CHILD, clock);
        }
        //src_linesize: 表示每一行存放的 字节长度
        sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                  frame->linesize, 0,
                  avCodecContext->height,
                  dst_data,
                  dst_linesize);
        //回调出去进行播放
        callback(dst_data[0], dst_linesize[0], avCodecContext->width, avCodecContext->height);
        releaseAvFrame(&frame);
    }
    av_freep(&dst_data[0]);
    releaseAvFrame(&frame);
    isPlaying = 0;
    sws_freeContext(swsContext);
    swsContext = 0;
    LOGI("Method end---> VideoChannel render");
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback callback) {
    this->callback = callback;
}

void VideoChannel::stop() {
    LOGI("Method start---> VideoChannel stop");
    isPlaying = 0;
    callHelper = 0;
    frames.setWork(0);
    frames.setWork(0);
    pthread_join(pid_decode, 0);
    pthread_join(pid_render, 0);
    LOGI("Method end---> VideoChannel stop");
}