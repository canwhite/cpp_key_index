#include <iostream>
#include <future>
#include <vector>
#include <map>
#include <algorithm> //find等方法
#include <memory>
#include <filesystem>
//注意这里使用了相对路径,因为我定义的是cpp所以在这里引入
#include "../debug/video_debugging.h" 
#include "config.h"
using namespace std; //可以使用标准库里的符号和方法
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libavutil/opt.h>
#include <string.h>
#include <inttypes.h>
}

struct StreamingParams
{
    char copy_video;
    char copy_audio;
    char *output_extension;
    const char *muxer_opt_key;
    const char *muxer_opt_value;
    const char *video_codec;
    const char *audio_codec;
    const char *codec_priv_key;
    const char *codec_priv_value;
};
//包含muxing和transcoding的所有内容

