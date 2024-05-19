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

//4）视频和音频编码器
//主要是发现，打开，复制参数从context
int prepare_video_encoder(StreamingContext *sc,AVCodecContext *decoder_ctx, AVRational input_framerate,StreamingParams sp){
    //新建流，赋予avs，第二个参数是codec，现在没有指定
    sc->video_avs = avformat_new_stream(sc->avfc,NULL);
    //avc获取
    sc->video_avc = (AVCodec *)avcodec_find_encoder_by_name(sp.video_codec);
    if(!sc->video_avc){
        logging("could not find the proper codec"); 
        return -1;
    }
    //avcc获取,需要借助avc
    sc->video_avcc = avcodec_alloc_context3(sc->video_avc);
    if(!sc->video_avcc){
        logging("could not allocated memory for codec context"); 
        return -1;
    }

    //设置视频编码器的配置选项，
    //这里是设置H.264编码器的预设参数，让编码器工作在"fast"模式下。
    //参数1: sc->video_avcc->priv_data是要设置的对象，priv_data是指向编译器私有数据的指针
    //参数2: 设置属性名，preset表示预设
    //参数3: 设置属性值
    //参数4: 最后一个是搜寻字段的标志，一般是0

    av_opt_set(sc->video_avcc->priv_data, "preset", "fast", 0);

    //编码参数
    if (sp.codec_priv_key && sp.codec_priv_value)
        av_opt_set(sc->video_avcc->priv_data, sp.codec_priv_key, sp.codec_priv_value, 0);

    sc->video_avcc->height = decoder_ctx->height; //from decoder ctx
    sc->video_avcc->width = decoder_ctx->width; //from decoder ctx 
    sc->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio; //from decoder ctx


    if (sc->video_avc->pix_fmts)
        sc->video_avcc->pix_fmt = sc->video_avc->pix_fmts[0];
    else
        sc->video_avcc->pix_fmt = decoder_ctx->pix_fmt; //from decoder ctx


    //控制码率
    sc->video_avcc->bit_rate = 2 * 1000 * 1000;
    sc->video_avcc->rc_buffer_size = 4 * 1000 * 1000;
    sc->video_avcc->rc_max_rate = 2 * 1000 * 1000;
    sc->video_avcc->rc_min_rate = 2.5 * 1000 * 1000;

    //时间基数
    sc->video_avcc->time_base = av_inv_q(input_framerate);
    sc->video_avs->time_base = sc->video_avcc->time_base;

    if (avcodec_open2(sc->video_avcc, sc->video_avc, NULL) < 0) {
        logging("could not open the codec"); 
        return -1;
    }
    //主要就是为了设置这些参数，如果是copy的话，就直接复制参数给avs了
    avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);

    return 0;
}

int prepare_audio_encoder(StreamingContext *sc,int sample_rate, StreamingParams sp){
    //->是直接读取地址数据
    sc->audio_avs = avformat_new_stream(sc->avfc, NULL);
    //发现编码器
    sc->audio_avc = (AVCodec * )avcodec_find_encoder_by_name(sp.audio_codec);
    if (!sc->audio_avc) {
        logging("could not find the proper codec"); 
        return -1;
    }
    //然后搞一个ctx
    sc->audio_avcc = avcodec_alloc_context3(sc->video_avc);
    if (!sc->audio_avcc) {
        logging("could not allocated memory for codec context"); 
        return -1;
    }

    //设置一些属性
    int OUTPUT_CHANNELS = 2;
    int OUTPUT_BIT_RATE = 196000;

    //这两个7.0没有了，我这边先去掉了
    // sc->audio_avcc->channels       = OUTPUT_CHANNELS;
    // sc->audio_avcc->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);

    sc->audio_avcc->sample_rate    = sample_rate;
    sc->audio_avcc->sample_fmt     = sc->audio_avc->sample_fmts[0];
    sc->audio_avcc->bit_rate       = OUTPUT_BIT_RATE;
    sc->audio_avcc->time_base      = (AVRational){1, sample_rate};

    sc->audio_avcc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    sc->audio_avs->time_base = sc->audio_avcc->time_base;

    //打开，参数复制
    if (avcodec_open2(sc->audio_avcc, sc->audio_avc, NULL) < 0) {
        logging("could not open the codec"); 
        return -1;
    }
    avcodec_parameters_from_context(sc->audio_avs->codecpar, sc->audio_avcc);

    return 0;
}

//5）copy
//fc, avs and decoder params 
int prepare_copy(AVFormatContext *avfc, AVStream **avs, AVCodecParameters *decoder_par){
    *avs = avformat_new_stream(avfc, NULL);
    //直接复制参数给avs，
    avcodec_parameters_copy((*avs)->codecpar, decoder_par);
    return 0;
}

//续copy，如果走copy路线，走remux
//这里的AVRational是timebase的意思，区别于帧率
int remux(AVPacket **pkt, AVFormatContext **avfc,AVRational decoder_tb, AVRational encoder_tb){
    av_packet_rescale_ts(*pkt, decoder_tb, encoder_tb);
    if (av_interleaved_write_frame(*avfc, *pkt) < 0) { 
        logging("error while copying stream packet"); 
        return -1; 
    }
    return 0;
}


int encode_video(StreamingContext *decoder, StreamingContext *encoder , AVFrame *input_frame){
    if(input_frame) input_frame-> pict_type = AV_PICTURE_TYPE_NONE;
    //创建一个空的packet，接下来整体流程就和之前相反
    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {
        logging("could not allocate memory for output packet"); 
        return -1;
    }
    //然后就是和decoder时候相反的步骤
    int response = avcodec_send_frame(encoder->video_avcc,input_frame);

    while (response >= 0)
    {
        response = avcodec_receive_packet(encoder->video_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving packet from encoder: %s", av_err2str(response));
            return -1;
        }
        output_packet->stream_index = decoder->video_index;
        //总体是用时间基数，除以平均帧率
        output_packet->duration = encoder->video_avs->time_base.den / encoder->video_avs->time_base.num / decoder->video_avs->avg_frame_rate.num * decoder->video_avs->avg_frame_rate.den;

        // "av_packet_rescale_ts"是ffmpeg库中的一个函数，其主要功能是对AVPacket中的时间戳进行重缩放。
        // 在ffmpeg中，每个流都有自己的时间基。因此，当我们需要从一个流复制数据到另一个流时，
        // 可能需要对时间戳进行重缩放以匹配目标流的时间基。
        av_packet_rescale_ts(output_packet, decoder->video_avs->time_base, encoder->video_avs->time_base);

        //"av_interleaved_write_frame"
        //主要的作用是将一个媒体帧（AVPacket）写入到输出媒体上下文（AVFormatContext）中。
        //interleaved是插入的意思
        response = av_interleaved_write_frame(encoder->avfc, output_packet);
        if (response != 0) { 
            logging("Error %d while receiving packet from decoder: %s", response, av_err2str(response)); 
            return -1;
        }
    }

    //unref和free的区别是，unref是用于减少引用计数的，当减少到0就会释放，free是释放内存的
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);

    return 0;
}

int encode_audio(StreamingContext *decoder, StreamingContext *encoder, AVFrame *input_frame) {
    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {
        logging("could not allocate memory for output packet"); 
        return -1;
    }
    int response = avcodec_send_frame(encoder->audio_avcc, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(encoder->audio_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving packet from encoder: %s", av_err2str(response));
            return -1;
        }

        output_packet->stream_index = decoder->audio_index;

        av_packet_rescale_ts(output_packet, decoder->audio_avs->time_base, encoder->audio_avs->time_base);
        response = av_interleaved_write_frame(encoder->avfc, output_packet);
        if (response != 0) { 
            logging("Error %d while receiving packet from decoder: %s", response, av_err2str(response)); 
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}




//6)transcode video and audio
int transcode_video(StreamingContext * decoder,StreamingContext * encoder,AVPacket *input_packet, AVFrame *input_frame ){

    int response = avcodec_send_packet(decoder->video_avcc,input_packet);
    if(response < 0){
        logging("Error while sending packet to decoder: %s", av_err2str(response));
        return response;
    }

    while (response >= 0){
        response = avcodec_receive_frame(decoder->video_avcc, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving frame from decoder: %s", av_err2str(response));
            return response;
        }
        //如果是成功的状态
        if(response >= 0){
            //上边是解码，这里就需要编码了，基本上是解码的反顺序
            if(encode_video(decoder,encoder,input_frame)){
                return -1;
            }
        }
        av_frame_unref(input_frame);
    }
    return 0;
}

int transcode_audio(StreamingContext *decoder, StreamingContext *encoder, AVPacket *input_packet, AVFrame *input_frame) {
    int response = avcodec_send_packet(decoder->audio_avcc, input_packet);
    if (response < 0) {
        logging("Error while sending packet to decoder: %s", av_err2str(response)); 
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(decoder->audio_avcc, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving frame from decoder: %s", av_err2str(response));
            return response;
        }

        if (response >= 0) {
            if (encode_audio(decoder, encoder, input_frame)) 
                return -1;
        }
        av_frame_unref(input_frame);
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
    if(prepare_decoder(decoder)) {
        return -1;
    }
        
    //搞一个输出的fmt ctx，赋值的地方是encoder的avfc
    avformat_alloc_output_context2(&encoder->avfc, NULL, NULL, encoder->filename);
    //判断是否给encoder->avfc赋过值了
    if(!encoder->avfc){
        logging("could not allocate memory for output format");
        return -1;
    }


    //如果不是copy的时候就需要编码
    //a.关于视频--encoder和复制
    if(!sp.copy_video){
        //AVRational是有理数的意思，一般来表示帧率和时间基准
        //av_guess_frame_rate函数用于猜测给定的视频流的帧率。
        //它会尝试获取最准确的帧率信息，方法是正确地解析和返回包含在文件的元数据中的帧率。
        AVRational input_framerate = av_guess_frame_rate(decoder->avfc, decoder->video_avs, NULL);
        //如果不是copy，就使用视频编码器，里边的一些值需要我们自己填写
        prepare_video_encoder(encoder, decoder->video_avcc, input_framerate, sp);

    }else{
        //注意一些概念的理解：
        //av-> audio  video，  f -> format , c->Codec ｜ Context ,  s-> Stream
        if (prepare_copy(encoder->avfc, &encoder->video_avs, decoder->video_avs->codecpar)) {
            return -1;
        }
    }

    //b.关于音频--encoder和复制
    if(!sp.copy_audio){
        if (prepare_audio_encoder(encoder, decoder->audio_avcc->sample_rate, sp)) {
            return -1;
        }
    }else{
        if (prepare_copy(encoder->avfc, &encoder->audio_avs, decoder->audio_avs->codecpar)) {
            return -1;
        }
    }
    
    //通过设置 AV_CODEC_FLAG_GLOBAL_HEADER 来告诉编码器可以使用这个全局头信息，
    if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER)
        encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //最终打开输出文件写入文件头。
    if (!(encoder->avfc->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&encoder->avfc->pb, encoder->filename, AVIO_FLAG_WRITE) < 0) {
            logging("could not open the output file");
            return -1;
        }
    }

    AVDictionary* muxer_opts = NULL;

    if (sp.muxer_opt_key && sp.muxer_opt_value) {
        av_dict_set(&muxer_opts, sp.muxer_opt_key, sp.muxer_opt_value, 0);
    }

    if (avformat_write_header(encoder->avfc, &muxer_opts) < 0) {
        logging("an error occurred when opening output file"); 
        return -1;
    }

    //初始化packet和frame
    AVFrame *input_frame = av_frame_alloc();
    if (!input_frame) {
        logging("failed to allocated memory for AVFrame"); 
        return -1;
    }

    AVPacket *input_packet = av_packet_alloc();
    if (!input_packet) {
        logging("failed to allocated memory for AVPacket"); 
        return -1;
    }

    //av_read_frame实际上是一帧帧的读取packet
    while (av_read_frame(decoder->avfc, input_packet) >= 0)
    {
        //如果是视频流.packet上取的idx
        if(decoder -> avfc -> streams[input_packet->stream_index]-> codecpar-> codec_type == AVMEDIA_TYPE_VIDEO){
            //如果不是copy走转码
            if (!sp.copy_video) {
                // TODO: refactor to be generic for audio and video (receiving a function pointer to the differences)
                if (transcode_video(decoder, encoder, input_packet, input_frame)) 
                    return -1;
                av_packet_unref(input_packet);
            }
            //否则走remux
            else{  
                if(remux(&input_packet, &encoder->avfc, decoder->video_avs->time_base, encoder->video_avs->time_base))
                    return -1;
            }

        }
        //如果是音频
        else if(decoder->avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            //如果不是copy走转码
            if (!sp.copy_audio) {
                if (transcode_audio(decoder, encoder, input_packet, input_frame)) 
                    return -1;
                av_packet_unref(input_packet);

            }
            //否则走remux
            else{
                 if (remux(&input_packet, &encoder->avfc, decoder->audio_avs->time_base, encoder->audio_avs->time_base)) 
                    return -1;
            }
        }
        //如果都不是
        else{
            logging("ignoring all non video or audio packets");
        }

    }
    // TODO: should I also flush the audio encoder?
    if (encode_video(decoder, encoder, NULL)) return -1;

    av_write_trailer(encoder->avfc);

    //清理掉dictionary
    if (muxer_opts != NULL) {
        av_dict_free(&muxer_opts);
        muxer_opts = NULL;
    }

    //frame和packet置空
    if (input_frame != NULL) {
        av_frame_free(&input_frame);
        input_frame = NULL;
    }

    if (input_packet != NULL) {
        av_packet_free(&input_packet);
        input_packet = NULL;
    }

    
    avformat_close_input(&decoder->avfc);

    avformat_free_context(decoder->avfc); 
    decoder->avfc = NULL;

    avformat_free_context(encoder->avfc); 
    encoder->avfc = NULL;

    
    avcodec_free_context(&decoder->video_avcc); 
    decoder->video_avcc = NULL;
    avcodec_free_context(&decoder->audio_avcc); 
    decoder->audio_avcc = NULL;

    free(decoder); 
    decoder = NULL;
    free(encoder); 
    encoder = NULL;
    



    return 0;
}