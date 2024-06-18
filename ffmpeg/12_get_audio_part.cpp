#include<iostream>
#include<string>
#include<vector>
#include <stdexcept>
#include <cstdlib>
#include<cstring>
#include<algorithm>
#include "config.h"
#ifdef _WIN32
#include<windows.h>
#else
#include<unistd.h>
#endif
#include "../debug/video_debugging.h" 
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include "lame.h" //.a文件正常引入了，然后就是使用的时候的一些问题了

}

using namespace std;

constexpr const char* kInputVideoPath = (char *)IN_FLIENAME;
constexpr const char* kOutputAudioPath = "output_audio.mp3";

int main() {
    // 初始化FFmpeg库
    lame_init();

    // 打开输入视频文件
    AVFormatContext* format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, kInputVideoPath, nullptr, nullptr) != 0) {
        throw runtime_error("Could not open input video file");
    }

    // 获取流信息
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        throw runtime_error("Could not find stream information");
    }

    // 查找视频流并获取解码器上下文
    int video_stream_index = -1;
    for (unsigned int i = 0; i< format_ctx->nb_streams; ++i) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        throw runtime_error("Could not find video stream");
    }

    const AVCodec* codec =  avcodec_find_decoder(format_ctx->streams[video_stream_index]->codecpar->codec_id);
    if (!codec) {
        throw runtime_error("Unsupported video codec");
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        throw runtime_error("Could not allocate video codec context");
    }

    if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[video_stream_index]->codecpar) < 0) {
        throw runtime_error("Could not copy video codec parameters to context");
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        throw runtime_error("Could not open video codec");
    }

    // 查找音频流并获取解码器上下文
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

    codec = avcodec_find_decoder(format_ctx->streams[audio_stream_index]->codecpar->codec_id);
    if (!codec) {
        throw runtime_error("Unsupported audio codec");
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        throw runtime_error("Could not allocate audio codec context");
    }

    if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[audio_stream_index]->codecpar) < 0) {
        throw runtime_error("Could not copy audio codec parameters to context");
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        throw runtime_error("Could not open audio codec");
    }

    // 创建AVFrame用于读取音频数据
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        throw runtime_error("Could not allocate audio frame");
    }

    // 打开输出MP3文件
    FILE* output_file = fopen(kOutputAudioPath, "wb");
    if (!output_file) {
        throw runtime_error("Could not open output audio file");
    }

    // 初始化LAME编码器
    lame_t lame = lame_init();
    if (!lame) {
        throw runtime_error("Could not initialize LAME encoder");
    }

    lame_set_in_samplerate(lame, codec_ctx->sample_rate);
    lame_set_out_samplerate(lame, 44100);
    // error: no member named 'channels' in 'AVCodecContext'
    //[build]   125 |     lame_set_num_channels(lame, codec_ctx->channels);
    // lame_set_num_channels(lame, codec_ctx->channels);
    lame_set_mode(lame, MONO);
    lame_set_brate(lame, 128);


    //创建pakage
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        logging("failed to allocate memory for AVPacket");
        return -1;
    }
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
    {
        logging("failed to allocate memory for AVFrame");
        return -1;
    }

    //音频的处理过程




    //TODO，这里的过程有问题
    // while (true) {

    //     //这里有问题
    //     int read_bytes = av_read_frame(format_ctx, &frame);
    //     if (read_bytes < 0) {
    //         break;
    //     }

    //     if (frame->stream_index == audio_stream_index) {
    //         int converted_bytes = sws_scale(sws_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0, codec_ctx->height, frame->data, frame->linesize);
    //         if (converted_bytes < 0) {
    //             throw runtime_error("Error during image scaling");
    //         }

    //         // 将PCM数据写入LAME编码器
    //         int mp3_bytes = lame_encode_buffer(lame, frame->data[0], converted_bytes, nullptr);
    //         if (mp3_bytes < 0) {
    //             throw runtime_error("Error during MP3 encoding");
    //         }

    //         // 将MP3数据写入输出文件
    //         fwrite(lame->lame_header, 1, lame->header_size, output_file);
    //         fwrite(lame->lame_buf, 1, mp3_bytes, output_file);
    //     }

    //     av_frame_unref(frame);
    // }

    // 关闭所有资源
    fclose(output_file);
    lame_close(lame);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    //TODO，这里释放资源有问题
    // sws_freeContext(sws_ctx);

    return 0;
}