#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}
using namespace std;

int main(){

    auto inputFilePath = "/Users/zack/Desktop/test.mp4";
    auto outputUrl = "out.mp4";

    AVFormatContext *inputFormatContext = avformat_alloc_context();
    return 0;
}