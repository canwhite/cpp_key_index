#include <iostream>
#include <future>
#include <vector>
#include <map>
#include <algorithm> //find等方法
#include <memory>
#include <filesystem>
using namespace std; //可以使用标准库里的符号和方法
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

//转封装
//类似于下边命令行,令ffmpeg 跳过编解码过程:
//ffmpeg -i bunny_1080p_60fps.mp4 -c copy bunny_1080p_60fps.ts

int main(){

    //因为用到了goto，所以该初始化的都初始化
    AVFormatContext *input_format_context = NULL, *output_format_context = NULL;
    AVPacket packet;

    //然后文件名我们自己定义
    const char* in_filename = "/Users/zack/Desktop/test.mp4";
    const char* out_filename = "remuxed_small_bunny_1080p_60fps.ts";

    //继续定义其他可用的属性
    int ret; //ret是获取各种创建结果，最后再end统一判断
    int stream_index = 0;
    int *streams_list = NULL;
    int number_of_streams = 0;
    int fragmented_mp4_options = 0; //如果加options，改为1
    //注意将变量定义在前边。否则goto会报错，跳转绕过了变量初始化
    AVDictionary *opts = NULL;

    //打开输入文件并分配内存
    if((ret = avformat_open_input(&input_format_context, in_filename, NULL, NULL)) < 0){
        //fprintf格式化输出方法。参数1，输出到指定的流，参数2，格式，参数3，内容      
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    //获取流信息
    if ((ret = avformat_find_stream_info(input_format_context, NULL)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    //给输出文件分配内存
    avformat_alloc_output_context2(&output_format_context, NULL, NULL, out_filename);
    if(!output_format_context){
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    //重新封装视频、音频、字幕流，因此需要将用到的这些流存入一个数组中。
    number_of_streams = input_format_context->nb_streams;
    //这里是给空间
    streams_list = (int *)av_mallocz(number_of_streams * sizeof(*streams_list));
    if (!streams_list) {
        ret = AVERROR(ENOMEM);
        goto end;
    }


    //遍历所有流，然后利用 avformat_new_stream 为每一个流创建一个对应的输出流
    //PS：当前只需要针对视频、音频、字幕流进行处理。
    for (int i = 0; i < input_format_context->nb_streams; i++)
    {
        AVStream *out_stream;
        AVStream *in_stream = input_format_context->streams[i];
        //参数，如果codec_type不满足视频音频和字幕这几种情况就跳过
        AVCodecParameters *in_codecpar = in_stream->codecpar;
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            //如果不是以上类型，直接给到-1
            streams_list[i] = -1;
            continue;
        }
        //上边不满足条件stream_index就没有++了，也就是说这里是按顺序加的
        streams_list[i] = stream_index++;
        //利用 avformat_new_stream 为每一个流创建一个对应的输出流，最后的结果是挂载在output_format_context
        out_stream = avformat_new_stream(output_format_context, NULL);

        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        //然后将参数也copy过去
        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto end;
        }

    }

    // https://ffmpeg.org/doxygen/trunk/group__lavf__misc.html#gae2645941f2dc779c307eb6314fd39f10
    // av_dump_format。这个函数主要用于打印输入或者输出的媒体格式的详细信息，包括流、编码器、时长、比特率等等。
    // 参数1: 流格式信息的结构体 ， 
    // 参数2:流的索引值，希望处理的是AVFormatContext 中的第一个（索引为0）流。即第一个流（通常，索引0是视频流，索引1是音频流，当然这个也与文件的实际情况有关）
    // 参数3:传入的媒体文件的URL
    // 参数4:0是输出，1是输出
    av_dump_format(output_format_context, 0, out_filename, 1);
    
    //创建一个输出文件
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        //参数1:代表了一个指向AVIOContext类型的指针，这里我们将它作为输出，即写入的目的地。
        //参数3:最后一个参数是打开文件的模式，AVIO_FLAG_WRITE表示以写入模式打开文件。
        //整体的意思是：
        //打开名为out_filename的文件以便写入，
        //并将AVIOContext上下文指针指向这个新打开的文件，
        //如果打开成功，ret将被赋值为0
        ret = avio_open(&output_format_context->pb, out_filename, AVIO_FLAG_WRITE);
        //如果没有成功
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    //----使用选项 start----
    //ffmpeg -i non_fragmented.mp4 -movflags frag_keyframe+empty_moov+default_base_moof fragmented.mp4
    if (fragmented_mp4_options) {
        // https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE
        av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
    }
      
    // https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga18b7b10bb5b94c4842de18166bc677cb
    // 将options写入header
    ret = avformat_write_header(output_format_context, &opts);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }
    //----使用选项 end----


    //将输入流逐个数据包复制到输出流
    while (1)
    {
        AVStream *in_stream, *out_stream;
        //通过（av_read_frame）循环读取每一个数据包。对于每一数据包，我们都要重新计算 PTS 和 DTS
        //PTS是展示时间戳，DTS是解码时间戳
        //read frame是读取数据包的，获得的是压缩数据的切片
        ret = av_read_frame(input_format_context, &packet);
        if (ret < 0)
            break;
        //拿到输入流
        in_stream  = input_format_context->streams[packet.stream_index];
        //如果超过过了分配空间或者不在分配空间之内，释放packet
        if (packet.stream_index >= number_of_streams || streams_list[packet.stream_index] < 0) {
            av_packet_unref(&packet);
            continue;
        }
        //正常的视频不会报错，但是不正常的估计都报错了
        //TODO，好像也没有问题，这里就不是-1了，而是streams_list本身的自增排序序号，不过也个数是对照的
        packet.stream_index = streams_list[packet.stream_index];
        //从fmt ctx中取，这个之前已经赋过值了
        out_stream = output_format_context->streams[packet.stream_index];
        /* copy packet */
        //av_rescale_q_rnd 是一个用于重新缩放时间戳的函数，rnd 在这里是 "rounding" 的缩写，表示 "舍入"。
        //q-quotient，时间基的意思
        //rnd 是round的意思，参数用于指定在进行时间戳缩放时，采取的舍入策略。
        packet.pts = av_rescale_q_rnd(
            packet.pts, // 想要转化的时间戳
            in_stream->time_base, //源时间基
            out_stream->time_base, //目标时间基 
            (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)   //舍入规则                    //
        );
        packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base,(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
        // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
        packet.pos = -1; //pos是代表 "position"的缩写，这通常是指数据在文件中的位置。

        //通过 av_interleaved_write_frame 写入输出格式的上下文。
        ret = av_interleaved_write_frame(output_format_context, &packet);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        av_packet_unref(&packet);
    }
    //这个函数的作用是写入任何必要的文件尾信息，并且丢弃所有的编码缓冲区。
    //对于一些格式，例如MP4，它必须在文件的末尾写入一些元信息，这些信息包含了如何有效的播放该文件所需的关键信息。
    av_write_trailer(output_format_context);

//goto end的时候走这部分    
end: 
    avformat_close_input(&input_format_context);

    //close output
    if (output_format_context && !(output_format_context->oformat->flags & AVFMT_NOFILE))
        avio_closep(&output_format_context->pb);
    avformat_free_context(output_format_context);

    //释放流赎罪
    av_freep(&streams_list);

    // judge ret
    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
