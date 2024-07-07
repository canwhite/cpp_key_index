extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}
#include "../debug/video_debugging.h" 
#include <iostream>
#include <fstream>

void extract_audio_to_mp3(const char* input_filename, const char* output_filename) {
    AVFormatContext* format_context = nullptr;
    AVCodecContext* codec_context = nullptr;
    AVStream* audio_stream = nullptr;
    SwrContext* swr_context = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    int audio_stream_index = -1;
    
    
    // Open input file
    if (avformat_open_input(&format_context, input_filename, nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file." << std::endl;
        return;
    }

    // Find stream info
    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        std::cerr << "Could not find stream info." << std::endl;
        return;
    }

    // Find audio stream
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            audio_stream = format_context->streams[i];
            break;
        }
    }

    if (audio_stream_index == -1) {
        std::cerr << "Could not find audio stream." << std::endl;
        return;
    }

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Could not find decoder." << std::endl;
        return;
    }

    // Allocate codec context
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        std::cerr << "Could not allocate codec context." << std::endl;
        return;
    }

    // Copy codec parameters to codec context
    if (avcodec_parameters_to_context(codec_context, audio_stream->codecpar) < 0) {
        std::cerr << "Could not copy codec parameters." << std::endl;
        return;
    }

    // Open codec
    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        std::cerr << "Could not open codec." << std::endl;
        return;
    }

    // Allocate frame and packet
    frame = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) {
        std::cerr << "Could not allocate frame or packet." << std::endl;
        return;
    }

    // Set up resampler
    // 这个参数是绕不过去了，需要学习一下
    //采用双声道，44100采样率，16位采样深度，fltp格式
    //采样格式转换和混合库


    swr_context = swr_alloc_set_opts2(
        nullptr, 
        AV_CH_LAYOUT_STEREO,  //这里有参数问题，todo，需要校验一下
        AV_SAMPLE_FMT_FLTP, 
        44100,
        codec_context->ch_layout, 
        codec_context->sample_fmt, 
        codec_context->sample_rate, 
        0, 
        nullptr
    );


    /* 
    SwrContext *swr = swr_alloc_set_opts(
        NULL,  // we're allocating a new context
        AV_CH_LAYOUT_STEREO,  // out_ch_layout
        AV_SAMPLE_FMT_S16,    // out_sample_fmt
        44100,                // out_sample_rate
        AV_CH_LAYOUT_5POINT1, // in_ch_layout
        AV_SAMPLE_FMT_FLTP,   // in_sample_fmt
        48000,                // in_sample_rate
        0,                    // log_offset
        NULL);                // log_ctx
    等同于
    SwrContext *swr = swr_alloc();
    av_opt_set_channel_layout(swr, "in_channel_layout",  AV_CH_LAYOUT_5POINT1, 0);
    av_opt_set_channel_layout(swr, "out_channel_layout", AV_CH_LAYOUT_STEREO,  0);
    av_opt_set_int(swr, "in_sample_rate",     48000,                0);
    av_opt_set_int(swr, "out_sample_rate",    44100,                0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  AV_SAMPLE_FMT_FLTP, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
    */
    

    //todo，我们需要看下流信息里的参数 ，audio_stream->codecpar
    //引入我们的logging方法


    /** 
     * 上述方法现在应该是改成这样了
    int swr_alloc_set_opts2(
        struct SwrContext **ps,
        const AVChannelLayout *out_ch_layout, 
        enum AVSampleFormat out_sample_fmt, 
        int out_sample_rate,
        const AVChannelLayout *in_ch_layout, 
        enum AVSampleFormat  in_sample_fmt, 
        int  in_sample_rate,
        nt log_offset, 
        void *log_ctx);
    */

    
    if (!swr_context || swr_init(swr_context) < 0) {
        std::cerr << "Could not allocate or initialize resampler." << std::endl;
        return;
    }

    // Open output file
    std::ofstream output_file(output_filename, std::ios::binary);
    if (!output_file.is_open()) {
        std::cerr << "Could not open output file." << std::endl;
        return;
    }

    // Reading packets from input file and writing to output
    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (avcodec_send_packet(codec_context, packet) >= 0) {
                while (avcodec_receive_frame(codec_context, frame) >= 0) {
                    // Convert samples to destination format using resampler
                    uint8_t* converted_data[2] = { nullptr };
                    int converted_linesize;
                    av_samples_alloc(converted_data, &converted_linesize, 2, frame->nb_samples, AV_SAMPLE_FMT_FLTP, 0);
                    swr_convert(swr_context, converted_data, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);

                    // Write converted data to output file
                    output_file.write(reinterpret_cast<char*>(converted_data[0]), frame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP) * 2);
                    av_freep(&converted_data[0]);
                }
            }
        }
        av_packet_unref(packet);
    }

    // Cleanup
    output_file.close();
    swr_free(&swr_context);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
}

int main() {
    const char* input_filename = "/Users/zack/Desktop/test.mp4";
    const char* output_filename = "output_audio.mp3";
    extract_audio_to_mp3(input_filename, output_filename);
    std::cout << "Audio extraction and conversion successful." << std::endl;
    return 0;
}
