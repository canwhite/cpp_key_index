#include<iostream>
#include<string>
#include<vector>
#include <stdexcept>
#include <cstdlib>
#include<cstring>
#include<cmath>
#include<ctime>
#include<algorithm>
#include<iterator>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << "Usage: "<< argv[0] << " input_video output_video frame_image"<< endl;
        return 1;
    }

    string input_video = argv[1];
    string output_video = argv[2];
    string frame_image = argv[3];

    AVFormatContext *input_format_ctx = nullptr;
    AVCodecContext *input_codec_ctx = nullptr;
    AVFrame *input_frame = nullptr;

    AVFormatContext *output_format_ctx = nullptr;
    AVCodecContext *output_codec_ctx = nullptr;
    AVFrame *output_frame = nullptr;

    SwsContext *sws_ctx = nullptr;

    int ret = 0;

    // Register all formats and codecs
    av_register_all();

    // Open input video file
    if ((ret = avformat_open_input(&input_format_ctx, input_video.c_str(), nullptr, nullptr)) < 0) {
        cerr << "Could not open input video file: "<< input_video<< endl;
        goto end;
    }

    // Retrieve stream information
    if ((ret = avformat_find_stream_info(input_format_ctx, nullptr)) < 0) {
        cerr << "Could not retrieve stream information from input video file: "<< input_video<< endl;
        goto end;
    }

    // Find the first video stream
    int stream_idx = av_find_best_stream(input_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (stream_idx < 0) {
        cerr << "Could not find video stream in input video file: "<< input_video<< endl;
        goto end;
    }

    // Get a pointer to the codec context for the video stream
    input_codec_ctx = avcodec_alloc_context3(nullptr);
    if (!input_codec_ctx) {
        cerr << "Could not allocate video codec context"<< endl;
        ret = AVERROR(ENOMEM);
        goto end;
    }

    // Copy the stream parameters from the input format context to the codec context
    if ((ret = avcodec_parameters_to_context(input_codec_ctx, input_format_ctx->streams[stream_idx]->codecpar)) < 0) {
        cerr << "Could not copy stream parameters to codec context"<< endl;
        goto end;
    }

    // Find the decoder for the video stream
    AVCodec *codec = avcodec_find_decoder(input_codec_ctx->codec_id);
    if (!codec) {
        cerr << "Unsupported video codec: "<< input_codec_ctx->codec_id<< endl;
        ret = AVERROR(EINVAL);
        goto end;
    }

    // Open the decoder
    if ((ret = avcodec_open2(input_codec_ctx, codec, nullptr)) < 0) {
        cerr << "Could not open video decoder: "<< input_video<< endl;
        goto end;
    }

    // Allocate a video frame
    input_frame = av_frame_alloc();
    if (!input_frame) {
        cerr << "Could not allocate video frame"<< endl;
        ret = AVERROR(ENOMEM);
        goto end;
    }

    // Allocate an image for the frame
    uint8_t *frame_data = new uint8_t[AV_PIX_FMT_RGB24 * input_codec_ctx->height * input_codec_ctx->width];
    if (!frame_data) {
        cerr << "Could not allocate frame data"<< endl;
        ret = AVERROR(ENOMEM);
        goto end;
    }

    // Read a frame from the input video file
    if ((ret = av_read_frame(input_format_ctx, &input_packet)) < 0) {
        cerr << "Could not read frame from input video file: "<< input_video<< endl;
        goto end;
    }

    // Check if the packet belongs to the video stream
    if (input_packet.stream_index != stream_idx) {
        cerr << "Packet does not belong to the video stream"<< endl;
        ret = AVERROR_INVALIDDATA;
        goto end;
    }

    // Decode the frame
    ret = avcodec_send_packet(input_codec_ctx, &input_packet);
    if (ret < 0) {
        cerr << "Error sending a packet for decoding"<< endl;
        goto end;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(input_codec_ctx, input_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            cerr << "Error during decoding"<< endl;
            goto end;
        }

        // Convert the decoded frame to RGB24 format
        sws_ctx = sws_getContext(input_codec_ctx->width, input_codec_ctx->height, input_codec_ctx->pix_fmt,
                                 input_codec_ctx->width, input_codec_ctx->height, AV_PIX_FMT_RGB24,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx) {
            cerr << "Could not create scaling context"<< endl;
            ret = AVERROR(ENOMEM);
            goto end;
        }

        sws_scale(sws_ctx, input_frame->data, input_frame->linesize, 0, input_codec_ctx->height,
                  frame_data, input_frame->linesize);

        // Save the frame to a file
        FILE *frame_file = fopen(frame_image.c_str(), "wb");
        if (!frame_file) {
            cerr << "Could not open frame image file: "<< frame_image<< endl;
            ret = AVERROR(EIO);
            goto end;
        }

        fwrite(frame_data, 1, AV_PIX_FMT_RGB24 * input_codec_ctx->height * input_codec_ctx->width, frame_file);
        fclose(frame_file);
    }

end:
    // Free resources
    if (input_frame) {
        av_frame_free(&input_frame);
    }
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }
    if (input_codec_ctx) {
        avcodec_free_context(&input_codec_ctx);
    }
    if (input_format_ctx) {
        avformat_close_input(&input_format_ctx);
    }
    if (frame_data) {
        delete[] frame_data;
    }

    return ret;
}