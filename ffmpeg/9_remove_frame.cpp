#include<iostream>
#include<string>
#include <cstdlib>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: "<< argv[0] << " input_file output_file"<< std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    // 初始化FFmpeg
    // av_register_all();

    // 打开输入文件
    AVFormatContext *input_format_context = nullptr;
    if (avformat_open_input(&input_format_context, input_file, nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file"<< std::endl;
        return 1;
    }

    // 获取输入文件流信息
    if (avformat_find_stream_info(input_format_context, nullptr) < 0) {
        std::cerr << "Could not find stream information"<< std::endl;
        avformat_close_input(&input_format_context);
        return 1;
    }

    // 查找视频流
    int video_stream_index = av_find_best_stream(input_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index < 0) {
        std::cerr << "Could not find video stream"<< std::endl;
        avformat_close_input(&input_format_context);
        return 1;
    }

    AVCodecContext *codec_context = avcodec_alloc_context3(nullptr);
    if (!codec_context) {
        std::cerr << "Could not allocate codec context"<< std::endl;
        avformat_close_input(&input_format_context);
        return 1;
    }

    // 复制视频流参数到解码器上下文
    if (avcodec_parameters_to_context(codec_context, input_format_context->streams[video_stream_index]->codecpar) < 0) {
        std::cerr << "Could not copy codec parameters"<< std::endl;
        avcodec_free_context(&codec_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    // 打开解码器
    if (avcodec_open2(codec_context, nullptr, nullptr) < 0) {
        std::cerr << "Could not open codec"<< std::endl;
        avcodec_free_context(&codec_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    // 创建输出文件
    AVFormatContext *output_format_context = avformat_alloc_context();
    if (!output_format_context) {
        std::cerr << "Could not allocate output format context"<< std::endl;
        avcodec_free_context(&codec_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    // 设置输出格式
    const char *output_format_str = "mp4";
    if (av_guess_format(output_format_str, nullptr, nullptr) == nullptr) {
        std::cerr << "Unknown output format"<< std::endl;
        avformat_free_context(&output_format_context);
        avcodec_free_context(&codec_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    output_format_context->oformat = av_guess_format(output_format_str, nullptr, nullptr);

    // 添加视频流到输出文件
    AVStream *output_stream = avformat_new_stream(output_format_context, nullptr);
    if (!output_stream) {
        std::cerr << "Could not create output stream"<< std::endl;
        avformat_free_context(&output_format_context);
        avcodec_free_context(&codec_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    output_stream->id = input_format_context->streams[video_stream_index]->id;
    output_stream->time_base = input_format_context->streams[video_stream_index]->time_base;

    if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // 复制编解码器参数到输出流
    if (avcodec_parameters_from_context(output_stream->codecpar, codec_context) < 0) {
        std::cerr << "Could not copy codec parameters"<< std::endl;
        avcodec_free_context(&codec_context);
        avformat_free_context(&output_format_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    // 打开输出文件
    if (avio_open(&output_format_context->pb, output_file, AVIO_FLAG_WRITE) < 0) {
        std::cerr << "Could not open output file"<< std::endl;
        avcodec_free_context(&codec_context);
        avformat_free_context(&output_format_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    // 写入输出文件头
    if (avformat_write_header(output_format_context, nullptr) < 0) {
        std::cerr << "Could not write output file header"<< std::endl;
        avio_closep(&output_format_context->pb);
        avcodec_free_context(&codec_context);
        avformat_free_context(&output_format_context);
        avformat_close_input(&input_format_context);
        return 1;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    int frame_count = 0;
    while (av_read_frame(input_format_context, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            // 解码帧
            int ret = avcodec_send_packet(codec_context, &packet);
            if (ret < 0) {
                std::cerr << "Error sending a packet for decoding"<< std::endl;
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_context, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    std::cerr << "Error during decoding"<< std::endl;
                    break;
                }

                // 根据需求判断是否删除该帧
                if (frame_count % 2 == 0) {
                    // 删除当前帧
                    av_frame_unref(frame);
                }

                // 将处理后的帧重新编码并写入输出文件
                ret = avcodec_send_frame(codec_context, frame);
                if (ret < 0) {
                    std::cerr << "Error sending a frame for encoding"<< std::endl;
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_packet(codec_context, &packet);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    } else if (ret < 0) {
                        std::cerr << "Error during encoding"<< std::endl;
                        break;
                    }

                    // 写入编码后的数据包到输出文件
                    ret = av_write_frame(output_format_context, &packet);
                    if (ret < 0) {
                        std::cerr << "Error writing a packet"<< std::endl;
                        break;
                    }
                }

                frame_count++;
            }
        }

        // 释放数据包资源
        av_packet_unref(&packet);
    }

    // 释放所有资源
    av_packet_unref(&packet);
    avcodec_free_context(&codec_context);
    av_frame_free(&frame);
    avio_closep(&output_format_context->pb);
    avformat_free_context(&output_format_context);
    avformat_close_input(&input_format_context);

    return 0;
}