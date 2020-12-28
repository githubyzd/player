//
// Created by admin on 2020/12/23.
//

#ifndef PLAYER_BASECHANNEL_H
#define PLAYER_BASECHANNEL_H

#include "safe_queue.h"
#include "JavaCallHelper.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

class BaseChannel {
public:
    BaseChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                AVRational time_base) :
            id(id), callHelper(javaCallHelper), avCodecContext(avCodecContext),
            time_base(time_base) {
        frames.setReleaseCallback(releaseAvFrame);
        packets.setReleaseCallback(releaseAvPacket);
    }

    //virtual
    virtual ~BaseChannel() {
        frames.clear();
        packets.clear();
        if (avCodecContext) {
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
    }

    /**
    * 释放 AVPacket
    * @param packet
    */
    static void releaseAvPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            //为什么用指针的指针？
            // 指针的指针能够修改传递进来的指针的指向
            *packet = 0;
        }
    }

    static void releaseAvFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            *frame = 0;
        }
    }

    virtual void play() = 0;


    void clear() {
        packets.clear();
        frames.clear();
    }

    void stopWork() {
        packets.setWork(0);
        frames.setWork(0);
    }

    void startWork() {
        packets.setWork(1);
        frames.setWork(1);
    }

    int id;
    AVCodecContext *avCodecContext;
    bool isPlaying;
    //编码数据包队列
    SafeQueue<AVPacket *> packets;
    //解码数据包队列
    SafeQueue<AVFrame *> frames;

    AVRational time_base;
    JavaCallHelper *callHelper;


public:
    double clock;
    int queueSize = 100;
};


#endif //PLAYER_BASECHANNEL_H
