//
// Created by admin on 2020/12/23.
//

#ifndef PLAYER_VIDEOCHANNEL_H
#define PLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"

extern "C" {
#include <libswscale/swscale.h>
};

typedef void (*RenderFrameCallback)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {
public:
    VideoChannel(int id, AVCodecContext *avCodecContext);

    ~VideoChannel();

    //解码  播放
    void play();

    void decode();

    void render();

    void setRenderFrameCallback(RenderFrameCallback callback);

private:
    pthread_t pid_decode;
    pthread_t pid_render;
    SafeQueue<AVFrame *> frames;
    SwsContext *swsContext = 0;
    RenderFrameCallback callback;
};


#endif //PLAYER_VIDEOCHANNEL_H
