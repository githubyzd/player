//
// Created by Administrator on 2018/9/26.
//

#ifndef PUSHER_AUDIOPUSHCHANNEL_H
#define PUSHER_AUDIOPUSHCHANNEL_H


#include <inttypes.h>
#include "librtmp/rtmp.h"
#include <sys/types.h>
#include <faac/faac.h>

class AudioPushChannel {
    typedef void (*AudioCallback)(RTMPPacket *packet);

public:
    AudioPushChannel();

    ~AudioPushChannel();

    void setAudioEncInfo(int samplesInHZ, int channels);

    void setAudioCallback(AudioCallback audioCallback);

    int getInputSamples();

    void encodeData(int8_t *data);

    RTMPPacket* getAudioTag();
private:
    AudioCallback audioCallback;
    int mChannels;
    faacEncHandle audioCodec = 0;
    u_long inputSamples;
    u_long maxOutputBytes;
    u_char *buffer = 0;
};


#endif //PUSHER_AUDIOPUSHCHANNEL_H
