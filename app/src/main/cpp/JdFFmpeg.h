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

private:
    char *dataSource;
    pthread_t pid;
    AVFormatContext *formatContext;
    JavaCallHelper *callHelper;

    AudioChannel *audioChannel;
    VideoChannel *videoChannel;
};


#endif //PLAYER_JDFFMPEG_H
