#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include <stdexcept>
#include <cstdlib>
#include<cstring>
#include<cmath>
#include<algorithm>
#include <iomanip>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

using namespace std;

const char* input_video = "input.mp4";
const char* output_audio = "output.mp3";

int main() {
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVCodec* codec = nullptr;
    AVPacket packet;
    SwrContext* swr_ctx = nullptr;
    FILE* fptr = nullptr;

    // 初始化FFmpeg库
    // av_register_all();

    // 打开输入视频文件
    if (avformat_open_input(&format_ctx, input_video, nullptr, nullptr) != 0) {
        throw runtime_error("Could not open input video file");
    }

    // 获取流信息
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        throw runtime_error("Could not find stream information");
    }

    // 查找音频流索引
    int audio_stream_index = -1;
    for (unsigned int i = 0; i< format_ctx->nb_streams; ++i) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index == -1) {
        throw runtime_error("Could not find audio stream");
    }

    // 获取音频解码器
    AVCodecParameters* codec_params = format_ctx->streams[audio_stream_index]->codecpar;
    codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        throw runtime_error("Unsupported audio codec");
    }

    // 初始化解码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        throw runtime_error("Could not allocate codec context");
    }

    // 复制解码器参数到上下文
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        throw runtime_error("Could not copy codec parameters to context");
    }

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        throw runtime_error("Could not open codec");
    }

    // 创建重采样上下文
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        throw runtime_error("Could not create resampling context");
    }

    // 设置重采样参数
    av_opt_set_int(swr_ctx, "in_channel_layout", codec_ctx->channels, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_MONO, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swr_ctx, "out_sample_rate", 44100, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, AV_OPT_SEARCH_CHILDREN);

    // 初始化重采样上下文
    if (swr_init(swr_ctx) < 0) {
        throw runtime_error("Could not initialize resampling context");
    }

    // 打开输出文件
    fptr = fopen(output_audio, "wb");
    if (!fptr) {
        throw runtime_error("Could not open output file");
    }

    // 循环读取并解码音频数据
    while (true) {
        int ret = av_read_frame(format_ctx, &packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw runtime_error("Error reading frame from input video");
        }

        if (packet.stream_index == audio_stream_index) {
            // 解码音频帧
            int len = avcodec_send_packet(codec_ctx, &packet);
            if (len < 0) {
                throw runtime_error("Error sending packet for decoding");
            }

            while (len >= 0) {
                len = avcodec_receive_frame(codec_ctx, &packet);
                if (len == AVERROR(EAGAIN) || len == AVERROR_EOF) {
                    break;
                } else if (len < 0) {
                    throw runtime_error("Error receiving decoded frame");
                }

                // 将解码后的音频数据从AVFrame转换为PCM格式
                int data_size = av_samples_get_buffer_size(nullptr, 1, packet.duration, AV_SAMPLE_FMT_S16, 1);
                vector<int16_t> pcm_data(data_size / sizeof(int16_t));
                int linesize = av_samples_get_line_size(nullptr, 1, AV_SAMPLE_FMT_S16, 1);
                av_samples_fill_arrays(pcm_data.data(), nullptr, 1, linesize, AV_SAMPLE_FMT_S16, 1);
                swr_convert(swr_ctx, pcm_data.data(), data_size / sizeof(int16_t), nullptr, 0);

                // 将PCM数据写入输出文件
                fwrite(pcm_data.data(), 1, data_size, fptr);
            }

            // 释放已解码的音频帧
            av_packet_unref(&packet);
        } else {
            // 如果是非音频流，则跳过
            av_packet_unref(&packet);
        }
    }

    // 关闭输出文件
    fclose(fptr);

    // 释放资源
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}