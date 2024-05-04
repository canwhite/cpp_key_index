#include <iostream>
#include <future>
#include <vector>
#include <map>
#include <algorithm> //find等方法
#include <memory>
using namespace std; //可以使用标准库里的符号和方法
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

/**  
1）为什么要编码？

编码是一个在数字媒体工作流中至关重要的步骤，它的目的是将原始的视频（或音频）数据转换为特定格式的文件，这个文件可以在各种设备上播放，例如电视、网络、移动设备等。
FFmpeg进行编码的原因有以下几个：
文件大小：未编码的视频文件非常大，很难进行存储和传输。编码通过压缩算法将文件大小降低，使其更易于存储和传输。
兼容性：不同的设备和应用支持不同的文件格式。通过编码，你可以将视频转换为目标设备或应用支持的格式。
优化质量和带宽：编码可以在保持视觉质量的同时减少带宽需求，对于网络传输来说特别重要。

2）编码得到的结果有什么用？
编码的结果通常是一定格式的视频、音频文件或流，可以用于以下的场景：
保存和存档：编码后的文件大小更小，更适合保存和存档。
在线播放：编码后的文件更易于被流媒体服务器接受和分发，用户可以直接在线播放。
文件共享：编码后的文件可以更方便地通过互联网、移动网络等进行共享。
因此，无论是在创建、编辑、分享或播放媒体内容时，编码都扮演了重要的角色。
 */ 


int main (){


    //输入和输出文件的地址
    const char* inputFileName = "/Users/zack/Desktop/cpp_key_index/ffmpeg/input.yuv";
    const char* outputFileName = "output.avi";

    FILE* inputFile = fopen(inputFileName, "rb");
    if (!inputFile) {
        std::cerr << "Error opening input file." << std::endl;
        return -1;
    }

    // Create AVFormatContext for output AVI file
    AVFormatContext* outputFormatContext = NULL;
    avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, "output.avi");
    if (!outputFormatContext) {
        std::cerr << "Error creating output format context." << std::endl;
        return -1;
    }

    // Create AVFormatContext
    AVFormatContext* formatContext = avformat_alloc_context();

    // Add video stream
    AVStream* videoStream = avformat_new_stream(formatContext, NULL);
    if (!videoStream) {
        std::cerr << "Error creating video stream." << std::endl;
        return -1;
    }

    // Set codec parameters using AVCodecParameters
    AVCodecParameters* codecParams = videoStream->codecpar;
    codecParams->codec_id = AV_CODEC_ID_MPEG4;
    codecParams->codec_type = AVMEDIA_TYPE_VIDEO;
    codecParams->width = 1920;
    codecParams->height = 1080;
    codecParams->format = AV_PIX_FMT_YUV420P;
    codecParams->codec_tag = 0;
    cout << codecParams->codec_id << endl;

    // Open video codec for output
    const AVCodec* codec = avcodec_find_encoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "Codec not found." << std::endl;
        return -1;
    }

    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Error allocating codec context." << std::endl;
        return -1;
    }

    // Set codec parameters for codec context
    if (avcodec_parameters_to_context(codecContext, codecParams) < 0) {
        std::cerr << "Failed to copy codec parameters to codec context." << std::endl;
        return -1;
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        std::cerr << "Error opening codec." << std::endl;
        return -1;
    }

    // Write header for output file
    if (avformat_write_header(outputFormatContext, NULL) < 0) {
        std::cerr << "Error writing header." << std::endl;
        return -1;
    }

    // Read YUV frames, encode, and write to output file
    AVPacket pkt;
    pkt.data = NULL;
    pkt.size = 0;

    


    // // Write trailer for output file
    // av_write_trailer(outputFormatContext);

    // // Cleanup
    // avcodec_free_context(&codecContext);
    // avformat_free_context(outputFormatContext);
    // fclose(inputFile);






}










    

