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
    

    return 0;
}
