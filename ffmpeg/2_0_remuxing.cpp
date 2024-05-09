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
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
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
    int ret,i; //ret是获取各种
    int stream_index = 0;
    int *streams_list = NULL;
    int number_of_streams = 0;
    int fragmented_mp4_options = 0; //如果加options，改为1

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
    





    







//goto end的时候走这部分    
end: 
    //释放资源
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
