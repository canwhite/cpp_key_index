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
    int ret,i;
    int stream_index = 0;
    int *streams_list = NULL;
    int number_of_streams = 0;
    int fragmented_mp4_options = 0; //如果加options，改为1
    














    return 0;
}
