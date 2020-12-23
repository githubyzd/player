//
// Created by admin on 2020/12/23.
//

#ifndef PLAYER_AUDIOCHANNEL_H
#define PLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

class AudioChannel : public BaseChannel {
public:
    AudioChannel(int id, AVCodecContext *avCodecContext);

    void play();
};


#endif //PLAYER_AUDIOCHANNEL_H
