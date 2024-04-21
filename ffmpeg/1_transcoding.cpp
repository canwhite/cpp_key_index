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
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

int main (){

    // 打开输入视频文件
    AVFormatContext *inputFormatContext = avformat_alloc_context();
    avformat_open_input(&inputFormatContext, "/Users/zack/Desktop/test.mp4", NULL, NULL);
    avformat_find_stream_info(inputFormatContext, NULL);

    // 查找输入文件中的视频流
    int videoStreamIndex = -1;
    for (int i = 0; i < inputFormatContext->nb_streams; i++) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    // 初始化视频解码器
    const AVCodec *videoCodec = avcodec_find_decoder(inputFormatContext->streams[videoStreamIndex]->codecpar->codec_id);
    AVCodecContext *videoCodecContext = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(videoCodecContext, inputFormatContext->streams[videoStreamIndex]->codecpar);
    avcodec_open2(videoCodecContext, videoCodec, NULL);

    // 初始化视频编码器
    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext *encoderContext = avcodec_alloc_context3(encoder);
    
    
    // 配置编码器参数，如分辨率、比特率等
    // 打开输出文件以进行视频转码
    AVFormatContext *outputFormatContext;
    avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, "output.avi");
    AVStream *outputStream = avformat_new_stream(outputFormatContext, encoder);
    // 配置输出流参数
    SwsContext *swsContext = sws_getContext(videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt,
                                            encoderContext->width, encoderContext->height, encoderContext->pix_fmt,
                                            SWS_LANCZOS, NULL, NULL, NULL);
    // 转码视频帧
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    while (av_read_frame(inputFormatContext, &packet) >= 0) {
        if (packet.stream_index == videoStreamIndex) {
            // 解码视频帧
            avcodec_send_packet(videoCodecContext, &packet);
            avcodec_receive_frame(videoCodecContext, frame);

            // 如果需要，进行帧格式转换
            sws_scale(swsContext, frame->data, frame->linesize, 0, videoCodecContext->height, frame->data, frame->linesize);

            // 编码并写入转码后的帧
            avcodec_send_frame(encoderContext, frame);
            while (avcodec_receive_packet(encoderContext, &packet) == 0) {
                // 将数据包写入输出文件
                av_write_frame(outputFormatContext, &packet);
                av_packet_unref(&packet);
            }
        }
        av_packet_unref(&packet);
    }

    // 清理和释放资源
    avformat_close_input(&inputFormatContext);
    avformat_free_context(inputFormatContext);
    avcodec_free_context(&videoCodecContext);
    avcodec_free_context(&encoderContext);
    avformat_free_context(outputFormatContext);
    sws_freeContext(swsContext);
    av_frame_free(&frame);

    return 0;
}


