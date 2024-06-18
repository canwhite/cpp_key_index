#include<iostream>
#include<fstream>
#include "config.h"
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include "lame.h"
}
#include<algorithm> // 添加缺少的头文件
#include "../debug/video_debugging.h" 
                
using namespace std;

int main() {
    const char* input_video_path = (char*) IN_FLIENAME;
    const char* output_audio_path = "output_audio.mp3";


    AVFormatContext* format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, input_video_path, nullptr, nullptr) != 0) {
        std::cerr << "Could not open input video file."<< std::endl;
        return -1;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream information."<< std::endl;
        avformat_close_input(&format_ctx);
        return -1;
    }

    //还可以这样直接获取音频流索引
    int audio_stream_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        std::cerr << "Could not find audio stream in input video file."<< std::endl;
        avformat_close_input(&format_ctx);
        return -1;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(nullptr);
    if (!codec_ctx) {
        std::cerr << "Could not allocate audio codec context."<< std::endl;
        avformat_close_input(&format_ctx);
        return -1;
    }

    const AVCodec* codec = avcodec_find_decoder(format_ctx->streams[audio_stream_index]->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Unsupported audio codec."<< std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[audio_stream_index]->codecpar) < 0) {
        std::cerr << "Failed to copy audio parameters to codec context."<< std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Failed to open audio codec."<< std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    std::ofstream output_file(output_audio_path, std::ios::binary);
    if (!output_file) {
        std::cerr << "Could not open output audio file."<< std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }
    // lame_t lame = lame_init();
    // lame_set_in_samplerate(lame, sampleRate);
    // lame_set_num_channels(lame, channels);
    // lame_set_brate(lame, bitRate);  
    // lame_init_params(lame);

    AVPacket packet;
    AVFrame* decoded_frame = av_frame_alloc();
    while (av_read_frame(format_ctx, &packet) >= 0) {
        if (packet.stream_index == audio_stream_index) {
            int response = avcodec_send_packet(codec_ctx, &packet);
            if (response < 0) {
                std::cerr << "Error sending a packet for decoding."<< std::endl;
                break;
            }

            while (response >= 0) {

                response = avcodec_receive_frame(codec_ctx, decoded_frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error during decoding."<< std::endl;
                    break;
                }

                if (!decoded_frame) {
                    std::cerr << "Could not allocate memory for decoded audio frame."<< std::endl;
                    break;
                }
                int  channels = codec_ctx->ch_layout.nb_channels;
              
                
                int data_size = av_samples_get_buffer_size(nullptr,  channels, decoded_frame->nb_samples, codec_ctx->sample_fmt, 1);
                if (data_size <= 0) {
                    std::cerr << "Could not calculate buffer size for decoded audio samples."<< std::endl;
                    av_frame_free(&decoded_frame);
                    break;
                }
                if (data_size <= 0) {
                    std::cerr << "Could not calculate buffer size for decoded audio samples."<< std::endl;
                    av_frame_free(&decoded_frame);
                    break;
                }

                vector<uint8_t> audio_data(data_size);
                av_samples_fill_arrays(decoded_frame->extended_data, nullptr, audio_data.data(), channels, decoded_frame->nb_samples, codec_ctx->sample_fmt, 1);

                output_file.write((char*)audio_data.data(), data_size);

                av_frame_unref(decoded_frame);
            }
        }

        av_packet_unref(&packet);
    }

    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}