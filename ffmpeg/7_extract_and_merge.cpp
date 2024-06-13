#include<iostream>
#include<string>
#include<vector>
#include<filesystem>
#include <stdexcept>
#include <cstdlib>
#include<algorithm>
#include<chrono>
#include<thread>

#ifdef _WIN32
#include<windows.h>
#else
#include<unistd.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

using namespace std;
using namespace std::filesystem;

constexpr auto INPUT_VIDEO_PATH = "input_video.mp4";
constexpr auto OUTPUT_VIDEO_PATH = "output_video.mp4";
constexpr auto FPS = 30;
constexpr auto VIDEO_WIDTH = 1920;
constexpr auto VIDEO_HEIGHT = 1080;

int main() {
    // 初始化FFmpeg
    av_register_all();

    // 打开输入视频文件
    AVFormatContext* input_format_context = nullptr;
    if (avformat_open_input(&input_format_context, INPUT_VIDEO_PATH, nullptr, nullptr) != 0) {
        throw runtime_error("Could not open input video file");
    }

    // 获取输入视频流信息
    if (avformat_find_stream_info(input_format_context, nullptr) < 0) {
        throw runtime_error("Could not find stream information");
    }

    // 查找视频流索引
    int video_stream_index = -1;
    for (unsigned int i = 0; i< input_format_context->nb_streams; ++i) {
        if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        throw runtime_error("Could not find video stream");
    }

    // 获取视频解码器上下文
    AVCodecContext* codec_context = avcodec_alloc_context3(nullptr);
    if (!codec_context) {
        throw runtime_error("Could not allocate video codec context");
    }

    // 复制输入视频流的解码器参数
    if (avcodec_parameters_to_context(codec_context, input_format_context->streams[video_stream_index]->codecpar) < 0) {
        throw runtime_error("Could not copy video codec parameters");
    }

    // 打开视频解码器
    if (avcodec_open2(codec_context, avcodec_find_decoder(codec_context->codec_id), nullptr) < 0) {
        throw runtime_error("Could not open video decoder");
    }

    // 创建转换上下文
    SwsContext* sws_context = sws_getContext(
        codec_context->width, codec_context->height, codec_context->pix_fmt,
        VIDEO_WIDTH, VIDEO_HEIGHT, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    if (!sws_context) {
        throw runtime_error("Could not create sws context");
    }

    // 创建输出视频文件
    AVFormatContext* output_format_context = avformat_alloc_context();
    if (!output_format_context) {
        throw runtime_error("Could not allocate output format context");
    }

    // 设置输出格式
    const char* output_format_str = "mp4";
    if (avformat_query_codec(avcodec_find_encoder(AV_CODEC_ID_H264), AV_CODEC_ID_MPEG4, 0) == 0) {
        output_format_str = "mp4";
    } else if (avformat_query_codec(avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO), AV_CODEC_ID_MPEG2VIDEO, 0) == 0) {
        output_format_str = "mpg";
    } else {
        throw runtime_error("Unsupported output video format");
    }
    if (avformat_alloc_output_context2(&output_format_context, nullptr, output_format_str, OUTPUT_VIDEO_PATH) < 0) {
        throw runtime_error("Could not allocate output format context");
    }

    // 添加视频流到输出文件
    AVStream* output_stream = avformat_new_stream(output_format_context, nullptr);
    if (!output_stream) {
        throw runtime_error("Could not create output video stream");
    }

    // 设置输出视频流参数
    output_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    output_stream->codecpar->width = VIDEO_WIDTH;
    output_stream->codecpar->height = VIDEO_HEIGHT;
    output_stream->time_base = { 1, FPS };

    // 写入输出文件头
    if (avformat_write_header(output_format_context, nullptr) < 0) {
        throw runtime_error("Could not write output file header");
    }

    // 创建重采样缓冲区
    uint8_t* yuv_buffer[3] = { nullptr, nullptr, nullptr };
    int yuv_linesize[3] = { 0, 0, 0 };
    yuv_buffer[0] = static_cast<uint8_t*>(av_malloc(VIDEO_WIDTH * VIDEO_HEIGHT));
    yuv_buffer[1] = yuv_buffer[0] + VIDEO_WIDTH * VIDEO_HEIGHT / 4;
    yuv_buffer[2] = yuv_buffer[1] + VIDEO_WIDTH * VIDEO_HEIGHT / 4;
    yuv_linesize[0] = VIDEO_WIDTH;
    yuv_linesize[1] = VIDEO_WIDTH / 2;
    yuv_linesize[2] = VIDEO_WIDTH / 2;

    // 读取输入视频帧
    AVPacket packet;
    while (av_read_frame(input_format_context, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            // 解码视频帧
            int ret = avcodec_send_packet(codec_context, &packet);
            if (ret < 0) {
                throw runtime_error("Error sending a packet for decoding");
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_context, codec_context->coded_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    throw runtime_error("Error during decoding");
                }

                // 缩放视频帧
                sws_scale(sws_context,
                    reinterpret_cast<const uint8_t* const*>(codec_context->coded_frame->data), codec_context->coded_frame->linesize, 0,
                    codec_context->height,
                    yuv_buffer, yuv_linesize
                );

                // 将YUV帧转换为RGB帧
                uint8_t* rgb_buffer = new uint8_t[VIDEO_WIDTH * VIDEO_HEIGHT * 3];
                int rgb_linesize = VIDEO_WIDTH * 3;
                libswscale::SwsVector sws_rgb_vec = { rgb_buffer, rgb_linesize, VIDEO_WIDTH, VIDEO_HEIGHT };
                sws_scale(sws_context, yuv_buffer, yuv_linesize, 0, codec_context->height, &sws_rgb_vec, nullptr);

                // 将RGB帧写入输出视频文件
                AVFrame* rgb_frame = av_frame_alloc();
                rgb_frame->format = AV_PIX_FMT_RGB24;
                rgb_frame->width = VIDEO_WIDTH;
                rgb_frame->height = VIDEO_HEIGHT;
                av_image_alloc(rgb_frame->data, rgb_frame->linesize, VIDEO_WIDTH, VIDEO_HEIGHT, AV_PIX_FMT_RGB24, 1);
                memcpy(rgb_frame->data[0], rgb_buffer, VIDEO_WIDTH * VIDEO_HEIGHT * 3);
                delete[] rgb_buffer;

                // 设置输出视频帧时间戳
                AVRational time_base = { 1, FPS };
                int64_t timestamp = av_rescale_q(codec_context->coded_frame->pts, codec_context->time_base, output_stream->time_base);
                rgb_frame->pts = timestamp;

                // 写入输出视频帧
                ret = av_write_frame(output_format_context, rgb_frame);
                if (ret < 0) {
                    throw runtime_error("Error writing a frame to output file");
                }

                // 释放资源
                av_frame_free(&rgb_frame);
            }
        }

        // 释放输入视频帧资源
        av_packet_unref(&packet);
    }

    // 释放资源
    av_frame_free(&codec_context->coded_frame);
    avcodec_free_context(&codec_context);
    sws_freeContext(sws_context);
    av_packet_unref(&packet);
    avformat_close_input(&input_format_context);
    avformat_free_context(output_format_context);
    av_freep(&yuv_buffer[0]);

    return 0;
}