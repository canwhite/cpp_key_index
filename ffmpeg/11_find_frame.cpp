#include<iostream>
#include<cstring>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

int main() {
    const char* filename = "input.mp4";
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVCodec* codec = nullptr;

    // 打开视频文件
    if (avformat_open_input(&format_ctx, filename, nullptr, nullptr) != 0) {
        std::cerr << "Could not open file "<< filename<< std::endl;
        return 1;
    }

    // 获取流信息
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream information"<< std::endl;
        avformat_close_input(&format_ctx);
        return 1;
    }

    // 查找视频流
    int video_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_index < 0) {
        std::cerr << "Could not find video stream"<< std::endl;
        avformat_close_input(&format_ctx);
        return 1;
    }

    AVStream* stream = format_ctx->streams[video_index];
    codec_ctx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(codec_ctx, stream->codecpar);

    // 打开解码器
    codec = avcodec_find_decoder_by_name("h264_cuvid");
    if (!codec) {
        std::cerr << "Codec h264_cuvid not found"<< std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return 1;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec"<< std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return 1;
    }

    int frame_number = 100; // 要查找的帧号
    AVRational time_base = stream->time_base;

    // 查找特定帧
    int64_t timestamp = av_rescale_q(frame_number, time_base, codec_ctx->time_base);
    if (av_seek_frame(format_ctx, video_index, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        std::cerr << "Could not seek to frame "<< frame_number<< std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return 1;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    // 读取特定帧
    while (av_read_frame(format_ctx, &packet) >= 0) {
        if (packet.stream_index == video_index) {
            AVFrame* frame = av_frame_alloc();
            int ret = avcodec_send_packet(codec_ctx, &packet);
            if (ret < 0) {
                std::cerr << "Error sending a packet for decoding"<< std::endl;
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    av_frame_unref(frame);
                    break;
                } else if (ret < 0) {
                    std::cerr << "Error during decoding"<< std::endl;
                    break;
                }

                // 处理帧数据...

                av_frame_unref(frame);
            }

            av_frame_free(&frame);
        }

        av_packet_unref(&packet);
    }

    // 释放资源
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}