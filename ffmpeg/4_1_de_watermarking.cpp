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
#include "libavfilter/avfilter.h"
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

/*全局变量。这里主要是在之前重封装的基础上添加上水印*/
AVFilterGraph *filter_graph = nullptr; //最关键的过滤器结构体
AVFilterContext* filter_ctx = nullptr;//上下文
AVFilterInOut *outputs  = nullptr;
AVFilterInOut *inputs = nullptr;
AVFilterContext *buffersink_ctx = nullptr;
AVFilterContext *buffersrc_ctx = nullptr;

/** 实现一个初始化滤镜的方法 */
//https://blog.csdn.net/asdasfdgdhh/article/details/119533863
int initAVFilter(){
    //初始化结构体开始
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");     //输入，原始数据输入处
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");


    //初始化结束


    return 0;
}

int main(){
    initAVFilter();

    return 0;
}