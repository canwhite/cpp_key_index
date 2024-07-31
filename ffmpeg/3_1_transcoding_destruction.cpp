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

    //=====================5.为encoder做一些准备工作，主要是预配置encoder参数=================================
    //a.如果是视频转码，以下是预转码过程
    if(!sp.copy_video){
。

        /**
        时间基准 是一个表示单位时间的分数，通常定义为 AVRational 类型，格式为 time_base = num/den，
        其中 num 是分子，den 是分母。它表示每一帧的时间长度。例如，如果 time_base 是 1/30，
        则每一帧的持续时间为 1/30 秒。
         
        av_guess_frame_rate 返回一个 AVRational 类型的值，
        表示估计的帧率，通常用分数形式表示（例如，30/1 表示每秒30帧）

        帧率和时间基准是导数的关系撒

        假设你有以下数据：

        我们知道：
        time_base 为 1/30（即每帧持续 1/30 秒）。
        duration 为 3000000（即 3000000 个 time_base 单位）。
        有效帧数为 90。可以通过解析视频流的包或者使用流的元数据获得。

        计算总时长(秒): 
        总时长 = duration * time_base
        = 3000000 * (1/30)
        = 100000 秒 = 100 秒

        计算帧率:将有效帧数除以转化后的时长（秒）即可得到帧率。
        帧率 = 有效帧数 / 总时长
        = 90 / 100
        = 0.9 fps (每秒帧数)
        */
        AVRational input_framerate = av_guess_frame_rate(decoder_all_info->avfc, decoder_all_info->video_avs, NULL);

        encoder_all_info->video_avs = avformat_new_stream(encoder_all_info->avfc,NULL);
        //avc获取
        encoder_all_info->video_avc = (AVCodec *)avcodec_find_encoder_by_name(sp.video_codec);
        if(!encoder_all_info->video_avc){
            logging("could not find the proper codec"); 
            return -1;
        }

        //avcc获取,需要借助avc
        encoder_all_info->video_avcc = avcodec_alloc_context3(encoder_all_info->video_avc);
        if(!encoder_all_info->video_avcc){
            logging("could not allocated memory for codec context"); 
            return -1;
        }

        //1）---设置视频编码器的配置选项，这些值是自己设置
        av_opt_set(encoder_all_info->video_avcc->priv_data, "preset", "fast", 0);

        //编码参数
        if (sp.codec_priv_key && sp.codec_priv_value)
            av_opt_set(encoder_all_info->video_avcc->priv_data, sp.codec_priv_key, sp.codec_priv_value, 0);

        //控制码率
        encoder_all_info->video_avcc->bit_rate = 2 * 1000 * 1000;
        encoder_all_info->video_avcc->rc_buffer_size = 4 * 1000 * 1000;
        encoder_all_info->video_avcc->rc_max_rate = 2 * 1000 * 1000;
        encoder_all_info->video_avcc->rc_min_rate = 2.5 * 1000 * 1000;


        //2）---这些值是从decoder的ctx中拿，ctx中持有的是输入流的信息
        //宽
        encoder_all_info->video_avcc->height = decoder_all_info->video_avcc->height; 
        //高
        encoder_all_info->video_avcc->width = decoder_all_info->video_avcc->width; 
        //样本长宽比，aspect外观，方面
        encoder_all_info->video_avcc->sample_aspect_ratio = decoder_all_info->video_avcc->sample_aspect_ratio; 

        //
        if (encoder_all_info->video_avc->pix_fmts)
            encoder_all_info->video_avcc->pix_fmt = encoder_all_info->video_avc->pix_fmts[0];
        else
            encoder_all_info->video_avcc->pix_fmt = decoder_all_info->video_avcc->pix_fmt; //from decoder ctx

        //time_base  => avcc和avs
        encoder_all_info->video_avcc->time_base = av_inv_q(input_framerate);
        encoder_all_info->video_avs->time_base = encoder_all_info->video_avcc->time_base;

        if (avcodec_open2(encoder_all_info->video_avcc, encoder_all_info->video_avc, NULL) < 0) {
            logging("could not open the codec"); 
            return -1;
        }
        //主要就是为了设置这些参数，如果是copy的话，就直接复制参数给avs了
        avcodec_parameters_from_context(encoder_all_info->video_avs->codecpar, encoder_all_info->video_avcc);
        
    }else{  
        //copy就相对简单了，新建流，给新建流赋予参数
        encoder_all_info->video_avs =  avformat_new_stream(encoder_all_info->avfc, NULL);
        //直接复制参数给avs，
        avcodec_parameters_copy(encoder_all_info->video_avs->codecpar,decoder_all_info->video_avs->codecpar);

    }

    //音频也操作下
    if(!sp.copy_audio){
        //编译
    }else{
        //然后是copy部分
    }   





    


    return 0;
}