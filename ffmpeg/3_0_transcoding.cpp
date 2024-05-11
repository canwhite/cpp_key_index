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

struct StreamingContext
{
    AVFormatContext *avfc;
    AVCodec *video_avc;
    AVCodec *audio_avc;
    AVStream *video_avs;
    AVStream *audio_avs;
    AVCodecContext *video_avcc;
    AVCodecContext *audio_avcc;
    int video_index;
    int audio_index;
    char *filename;
};

int open_media(){
    
}

//todo，接着往下走
int main(){
    /*
    * H264 -> H265
    * Audio -> remuxed (untouched)
    * MP4 - MP4
    */
    //c语言的结构体初始化，可以使用= {0}或者= {}来初始化一个结构体，这样所有成员都会初始化为0
    StreamingParams sp = {0};
    sp.copy_audio = 1;
    sp.copy_video = 0;
    //注意C++11并不允许将字符串字面值赋值给非const类型的字符指针。
    sp.video_codec = "libx265";
    sp.codec_priv_key = "x265-params";
    sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

    /*
    * H264 -> H264 (fixed gop)
    * Audio -> remuxed (untouched)
    * MP4 - MP4
    */
    //StreamingParams sp = {0};
    //sp.copy_audio = 1;
    //sp.copy_video = 0;
    //sp.video_codec = "libx264";
    //sp.codec_priv_key = "x264-params";
    //sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";

    /*
    * H264 -> H264 (fixed gop)
    * Audio -> remuxed (untouched)
    * MP4 - fragmented MP4
    */
    //StreamingParams sp = {0};
    //sp.copy_audio = 1;
    //sp.copy_video = 0;
    //sp.video_codec = "libx264";
    //sp.codec_priv_key = "x264-params";
    //sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
    //sp.muxer_opt_key = "movflags";
    //sp.muxer_opt_value = "frag_keyframe+empty_moov+delay_moov+default_base_moof";

    /*
    * H264 -> H264 (fixed gop)
    * Audio -> AAC
    * MP4 - MPEG-TS
    */
    //StreamingParams sp = {0};
    //sp.copy_audio = 0;
    //sp.copy_video = 0;
    //sp.video_codec = "libx264";
    //sp.codec_priv_key = "x264-params";
    //sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
    //sp.audio_codec = "aac";
    //sp.output_extension = ".ts";

    /*
    * H264 -> VP9
    * Audio -> Vorbis
    * MP4 - WebM
    */
    //StreamingParams sp = {0};
    //sp.copy_audio = 0;
    //sp.copy_video = 0;
    //sp.video_codec = "libvpx-vp9";
    //sp.audio_codec = "libvorbis";
    //sp.output_extension = ".webm";

    //calloc
    //第一个参数是n即个数，
    //第二个参数是size，
    //注意这几个alloc都是返回void *, 所以需要一个强转

    //之前还没怎么用过StreamingContext的，这里正好用一下
    StreamingContext *decoder = (StreamingContext*) calloc(1,sizeof(StreamingContext));
    //C++ 11 warning
    decoder->filename = IN_FLIENAME;

    StreamingContext *encoder = (StreamingContext*) calloc(1,sizeof(StreamingContext));
    decoder->filename = OUT_FILENAME;

    //拼接，如果需要生成新的类型的话
    if (sp.output_extension)
        //第一个是源字符串，第二个是往后拼接的部分
        strcat(encoder->filename, sp.output_extension);
    
    // if (open_media(decoder->filename, &decoder->avfc)) return -1;


    






    

    return 0;
}