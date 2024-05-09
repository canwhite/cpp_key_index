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
            streams_list[i] = -1;
            continue;
        }
        //标记一下是第几个stream
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
