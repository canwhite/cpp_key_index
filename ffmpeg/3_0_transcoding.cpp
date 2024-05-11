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
//1）打开media
int open_media(const char *in_filename, AVFormatContext **avfc){
    //先给ctx分配内存
    *avfc = avformat_alloc_context();

    //然后打开输入  
    if(avformat_open_input(avfc, in_filename, NULL, NULL) != 0){
        logging("failed to open input file %s", in_filename); 
        return -1;
    }
    //最后获取steam信息,这一步实际上是该fmt_ctx上
    if (avformat_find_stream_info(*avfc, NULL) < 0) {
        logging("failed to get stream info"); 
        return -1;
    }
    
    return 0;
}

//3）解码器的处理
//主要是通过从stream里拿到的id去创建avcodec和avcodec ctx
int fill_stream_info( AVStream *avs, AVCodec **avc, AVCodecContext **avcc){
    //会有一个问题，将const赋值给非const的问题，这里做个一个非const强转
    *avc = (AVCodec*)avcodec_find_decoder(avs->codecpar->codec_id);
    //创建codec ctx，创造ctx需要avc 
    *avcc = avcodec_alloc_context3(*avc);
    if (!*avcc) {
        logging("failed to alloc memory for codec context"); 
        return -1;
    }
    //关键步骤，复制参数到ctx
    if(avcodec_parameters_to_context(*avcc, avs->codecpar) < 0){
        logging("failed to fill codec context"); 
        return -1;
    }
    //打开解码器
    if (avcodec_open2(*avcc, *avc, NULL) < 0) {
        logging("failed to open codec"); 
        return -1;
    }
    return 0;
}


//2）从Stream里拿到相关参数，主要是拿到AVStream里边有各种参数信息
int prepare_decoder(StreamingContext *sc){
    //从sc中先取avfc，再取nb_streams
    for (int i = 0; i < sc->avfc->nb_streams; i++) {
        if(sc ->avfc -> streams[i] ->codecpar -> codec_type == AVMEDIA_TYPE_VIDEO){
            
            sc->video_avs = sc->avfc->streams[i];
            sc->video_index = i;
            //这里传入视频的 编解码器本身和其context
            if(fill_stream_info(sc->video_avs,&sc->video_avc, &sc->video_avcc)){
                return -1;
            }

        }else if(sc ->avfc -> streams[i] ->codecpar -> codec_type == AVMEDIA_TYPE_AUDIO){
            //如果是音频
            sc->audio_avs = sc->avfc->streams[i];
            sc->audio_index = i;
            //这里传入音频的编解码器本身和其context
            if(fill_stream_info(sc->audio_avs,&sc->audio_avc,&sc->audio_avcc)){
                return -1;
            }

        }else{
            //如果是其他
            logging("skipping streams other than audio and video");
        }
    }
    return 0;
}

int main(){
    /*
    * H264 -> H265
    * Audio -> remuxed (untouched)
    * MP4 - MP4
    */
    //设置一些全局参数
    //c语言的结构体初始化，可以使用= {0}或者= {}来初始化一个结构体，这样所有成员都会初始化为0
    StreamingParams sp = {0};
    sp.copy_audio = 1; //意思是音频是copy的
    sp.copy_video = 0; //视频需要转码
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

    //--decoder和encoder作为后续流转的核心
    //之前还没怎么用过StreamingContext的，这里正好用一下
    StreamingContext *decoder = (StreamingContext*) calloc(1,sizeof(StreamingContext));
    //C++ 11 warning, ISO C++11 does not allow conversion from string literal to 'char *' 
    decoder->filename = (char *)IN_FLIENAME;

    StreamingContext *encoder = (StreamingContext*) calloc(1,sizeof(StreamingContext));
    encoder->filename = (char *)OUT_FILENAME;

    //拼接，如果需要生成新的类型的话
    if (sp.output_extension)
        //第一个是源字符串，第二个是往后拼接的部分
        strcat(encoder->filename, sp.output_extension);
    
    //自实现方法，打开媒体文件,注意这里指针传值，因为avfc还没有初始化
    //对avfc的修改最后也会在decoder上
    if (open_media(decoder->filename, &decoder->avfc)){
        return -1;
    } 
    
    //拿到stream里的par，然后处理好解码器，复制好参数
    if(prepare_decoder(decoder)) return -1;

    //搞一个输出的fmt ctx，赋值的地方是encoder的avfc
    avformat_alloc_output_context2(&encoder->avfc, NULL, NULL, encoder->filename);
    //判断是否给encoder->avfc赋过值了
    if(!encoder->avfc){
        logging("could not allocate memory for output format");
        return -1;
    }
    



    //如果不是copy的时候就需要编码
    //a.关于视频
    if(!sp.copy_video){
        //todo, 这里需要准备编码器
        //av_guess_frame_rate函数用于猜测给定的视频流的帧率。
        //它会尝试获取最准确的帧率信息，方法是正确地解析和返回包含在文件的元数据中的帧率。
        AVRational input_framerate = av_guess_frame_rate(decoder->avfc, decoder->video_avs, NULL);
    }else{
        //todo
    }

    //b.关于音频
    if(!sp.copy_audio){
        //todo，这里需要准备编码器
    }else{
        //todo
    }
    




    return 0;
}