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


int main(){

    //=====================1.初始化=======================
    StreamingParams sp = {0};
    //1--音频remuxing 
    sp.copy_audio = 1;
    //2--视频转码
    sp.copy_video = 0;
    //注意C++11并不允许将字符串字面值赋值给非const类型的字符指针。
    sp.video_codec = "libx265";
    sp.codec_priv_key = "x265-params";
    sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

    //decoder和encoder作为后续流转的核心，是两个全局对象，
    //无论是calloc，还是malloc，都需要一个强转,calloc的第一个参数是分配的个数，第二个是size
    StreamingContext* decoder_all_info = (StreamingContext*) calloc(1,sizeof(StreamingContext));
    //C++ 11 warning, ISO C++11 does not allow conversion from string literal to 'char *' 
    decoder_all_info->filename = (char *)IN_FLIENAME;

    StreamingContext* encoder_all_info = (StreamingContext*) calloc(1,sizeof(StreamingContext));
    encoder_all_info->filename = (char *)OUT_FILENAME;

    //拼接，如果需要生成新的类型的话
    if (sp.output_extension)
        //第一个是源字符串，第二个是往后拼接的部分
        strcat(encoder_all_info->filename, sp.output_extension);
    
    //=======================2.打开媒体文件========================
    //对应位置先分配内存
    decoder_all_info->avfc = avformat_alloc_context();
    //打开媒体文件
    if(avformat_open_input(&decoder_all_info->avfc, decoder_all_info->filename, NULL, NULL) != 0){
        logging("failed to open input file %s", decoder_all_info->filename); 
        return -1;
    }
    //赋予值
    if (avformat_find_stream_info(decoder_all_info->avfc, NULL) < 0) {
        logging("failed to get stream info"); 
        return -1;
    }

    //=====================3.为decoder做一些准备工作，主要是读取流信息=================================
    for (int i = 0; i < decoder_all_info->avfc->nb_streams; i++) {
        //如果是视频
        if(decoder_all_info ->avfc -> streams[i] ->codecpar -> codec_type == AVMEDIA_TYPE_VIDEO){
            //得到对应流，和对应流index
            decoder_all_info->video_avs =  decoder_all_info-> avfc->streams[i];
            decoder_all_info->video_index = i;

            decoder_all_info->video_avc = (AVCodec*)avcodec_find_decoder(decoder_all_info->video_avs->codecpar->codec_id);
            //创建codec ctx，创造ctx需要avc 
            decoder_all_info->video_avcc = avcodec_alloc_context3(decoder_all_info->video_avc);
            if (! decoder_all_info->video_avcc) {
                logging("failed to alloc memory for codec context"); 
                return -1;
            }
            //关键步骤，复制参数到ctx,
            //--从这里可以看出stream里的参数和AVCodecContext里的参数基本一致
            if(avcodec_parameters_to_context(
                decoder_all_info->video_avcc, 
                decoder_all_info->video_avs->codecpar) < 0){
                logging("failed to fill codec context"); 
                return -1;
            }
            //打开解码器
            if (avcodec_open2(decoder_all_info->video_avcc, decoder_all_info->video_avc, NULL) < 0) {
                logging("failed to open codec"); 
                return -1;
            }
            break;
        
        }
        //如果是音频
        else if(decoder_all_info ->avfc -> streams[i] ->codecpar -> codec_type == AVMEDIA_TYPE_AUDIO){
            // 得到对应流和对应流index
            decoder_all_info->audio_avs = decoder_all_info->avfc->streams[i];
            decoder_all_info->audio_index = i;

            decoder_all_info->audio_avc = (AVCodec*)avcodec_find_decoder(decoder_all_info->audio_avs->codecpar->codec_id);
            //创建codec ctx，创造ctx需要avc 
            decoder_all_info->audio_avcc = avcodec_alloc_context3(decoder_all_info->audio_avc);
            if (!decoder_all_info->audio_avcc) {
                logging("failed to alloc memory for codec context"); 
                return -1;
            }
            //关键步骤，复制参数到ctx
            if(avcodec_parameters_to_context(decoder_all_info->audio_avcc, decoder_all_info->audio_avs->codecpar) < 0){
                logging("failed to fill codec context"); 
                return -1;
            }
            //打开解码器
            if (avcodec_open2(decoder_all_info->audio_avcc, decoder_all_info->audio_avc, NULL) < 0) {
                logging("failed to open codec"); 
                return -1;
            }

            break;
        }else{
            //如果是其他
            logging("skipping streams other than audio and video");
            return -1;
        }
    }

    //=====================4.输出文件初始化=================================
    //一个输出的fmt ctx，赋值的地方是encoder的avfc
    avformat_alloc_output_context2(&encoder_all_info->avfc, NULL, NULL, encoder_all_info->filename);
    //判断是否给encoder->avfc赋过值了
    if(!encoder_all_info->avfc){
        logging("could not allocate memory for output format");
        return -1;
    }

    //=====================5.为encoder做一些准备工作，主要是配置encoder参数=================================
    //a.关于视频--encoder和参数的一些设定
    




    
    






    


    return 0;
}