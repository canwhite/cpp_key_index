extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <iostream>
using namespace std;

/**
current question
这样设置了swr
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        return AVERROR(ENOMEM);
    }

    av_opt_set_chlayout(swr_ctx, "in_ch_layout",  &dec_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate",     dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", dec_ctx->sample_fmt, 0);

    av_opt_set_chlayout(swr_ctx, "out_ch_layout",  &enc_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate",    enc_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", enc_ctx->sample_fmt, 0);

但是最终编译结果Input channel layout "" is invalid or unsupported.
Failed to initialize the resampling context，这是什么原因额
*/
//todo, 这里还有问题，先pass了
int main(int argc, char *argv[]) {
    const char *input_filename = "/Users/zack/Desktop/test.mp4";
    const char *output_filename = "output.mp3";

    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *dec_ctx = NULL, *enc_ctx = NULL;
    SwrContext *swr_ctx = NULL;
    AVStream *audio_stream = NULL;
    int audio_stream_idx = -1;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL, *swr_frame = NULL;
    int ret;

    // av_register_all();

    // Open input file
    if ((ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL)) < 0) {
        fprintf(stderr, "Could not open input file '%s'\n", input_filename);
        return ret;
    }

    // Retrieve stream information
    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return ret;
    }

    // Find the audio stream
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            audio_stream = fmt_ctx->streams[i];
            break;
        }
    }

    if (audio_stream_idx == -1) {
        fprintf(stderr, "Could not find audio stream in the input, aborting\n");
        return -1;
    }

    // Find decoder for the audio stream
    const AVCodec *dec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!dec) {
        fprintf(stderr, "Failed to find audio codec\n");
        return AVERROR(EINVAL);
    }

    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx) {
        fprintf(stderr, "Failed to allocate the audio codec context\n");
        return AVERROR(ENOMEM);
    }

    if ((ret = avcodec_parameters_to_context(dec_ctx, audio_stream->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy audio codec parameters to decoder context\n");
        return ret;
    }

    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        fprintf(stderr, "Failed to open audio codec\n");
        return ret;
    }

    // Find the MP3 encoder
    const AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!enc) {
        fprintf(stderr, "Necessary encoder not found\n");
        return AVERROR_INVALIDDATA;
    }

    enc_ctx = avcodec_alloc_context3(enc);
    if (!enc_ctx) {
        fprintf(stderr, "Failed to allocate the encoder context\n");
        return AVERROR(ENOMEM);
    }

    //TODO，主要问题还是在于设置channel layout的方式有问题
    enc_ctx->bit_rate = 128000;
    enc_ctx->sample_fmt = enc->sample_fmts[0];
    enc_ctx->sample_rate = dec_ctx->sample_rate;
    enc_ctx->ch_layout = dec_ctx->ch_layout;
    
    // enc_ctx->ch_layout.channels = dec_ctx->ch_layout.nb_channels;

    if ((ret = avcodec_open2(enc_ctx, enc, NULL)) < 0) {
        fprintf(stderr, "Cannot open audio encoder\n");
        return ret;
    }

    // Initialize SwrContext
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        return AVERROR(ENOMEM);
    }

    // av_opt_set_chlayout(swr_ctx, "in_ch_layout",  &dec_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate",     dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", dec_ctx->sample_fmt, 0);

    av_opt_set_chlayout(swr_ctx, "out_ch_layout",  &enc_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate",    enc_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", enc_ctx->sample_fmt, 0);

    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        return ret;
    }

    // Open output file
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Could not open output file '%s'\n", output_filename);
        return AVERROR(errno);
    }

    // Allocate frames and packet
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate AVPacket\n");
        return AVERROR(ENOMEM);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate AVFrame\n");
        return AVERROR(ENOMEM);
    }

    swr_frame = av_frame_alloc();
    if (!swr_frame) {
        fprintf(stderr, "Could not allocate SWR AVFrame\n");
        return AVERROR(ENOMEM);
    }

    swr_frame->ch_layout = enc_ctx->ch_layout;
    swr_frame->sample_rate = enc_ctx->sample_rate;
    swr_frame->format = enc_ctx->sample_fmt;
    swr_frame->nb_samples = enc_ctx->frame_size;
    cout << swr_frame->ch_layout.nb_channels << endl;

    if ((ret = av_frame_get_buffer(swr_frame, 0)) < 0) {
        fprintf(stderr, "Could not allocate output frame samples\n");
        return ret;
    }

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == audio_stream_idx) {
            if ((ret = avcodec_send_packet(dec_ctx, pkt)) < 0) {
                fprintf(stderr, "Error while sending a packet to the decoder\n");
                return ret;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error while receiving a frame from the decoder\n");
                    return ret;
                }

                if ((ret = swr_convert_frame(swr_ctx, swr_frame, frame)) < 0) {
                    fprintf(stderr, "Error while converting\n");
                    return ret;
                }

                if ((ret = avcodec_send_frame(enc_ctx, swr_frame)) < 0) {
                    fprintf(stderr, "Error sending the frame to the encoder\n");
                    return ret;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_packet(enc_ctx, pkt);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        fprintf(stderr, "Error encoding audio frame\n");
                        return ret;
                    }

                    fwrite(pkt->data, 1, pkt->size, output_file);
                    av_packet_unref(pkt);
                }
            }
        }
        av_packet_unref(pkt);
    }

    // Flush the encoder
    avcodec_send_frame(enc_ctx, NULL);
    while (avcodec_receive_packet(enc_ctx, pkt) >= 0) {
        fwrite(pkt->data, 1, pkt->size, output_file);
        av_packet_unref(pkt);
    }

    // Cleanup
    fclose(output_file);
    av_frame_free(&swr_frame);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr_ctx);
    avcodec_free_context(&enc_ctx);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
