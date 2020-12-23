//
// Created by admin on 2020/12/23.
//

#ifndef PLAYER_JDFFMPEG_H
#define PLAYER_JDFFMPEG_H

#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"

extern "C" {
#include <libavformat/avformat.h>
};

class JdFFmpeg {
public:
    JdFFmpeg(JavaCallHelper *callHelper, const char *dataSource);

    ~JdFFmpeg();

    void prepare();

    void _prepare();

    void start();

    void _start();

    void setRenderFrameCallback(RenderFrameCallback callback);

private:
    char *dataSource;
    pthread_t pid;
    pthread_t pid_play;

    AVFormatContext *formatContext = 0;
    JavaCallHelper *callHelper;

    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    RenderFrameCallback callback;
    bool isPlaying;
};


#endif //PLAYER_JDFFMPEG_H
