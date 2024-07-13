#include<iostream>
#include<string>
#include<vector>
#include<filesystem>
#include <stdexcept>
#include <cstdlib>
#include<chrono>
#include<thread>
#include<cmath>
#include<algorithm>

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
#include <libavutil/opt.h>
}

using namespace std;
using namespace std::filesystem;

constexpr int BUF_SIZE = 4096;

struct VideoOptions {
    string input_file;
    string output_file;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    double bitrate = 0.0;
};

void print_help() {
    cout << "Usage: video_compressor [options] input_file output_file\n";
    cout << "Options:\n";
    cout << "  -w, --width       Set output video width (default: original width)\n";
    cout << "  -h, --height      Set output video height (default: original height)\n";
    cout << "  -f, --fps         Set output video frames per second (default: original fps)\n";
    cout << "  -b, --bitrate     Set output video bitrate (default: 1M)\n";
    cout << "  -h, --help        Show this help message\n";
}

bool parse_options(int argc, char *argv[], VideoOptions &opts) {
    int opt;
    while ((opt = getopt(argc, argv, "w:h:f:b:h")) != -1) {
        switch (opt) {
            case 'w':
                opts.width = stoi(optarg);
                break;
            case 'h':
                opts.height = stoi(optarg);
                break;
            case 'f':
                opts.fps = stod(optarg);
                break;
            case 'b':
                opts.bitrate = stod(optarg);
                break;
            case 'h':
                print_help();
                return false;
            default:
                cerr << "Invalid option: "<< opt<< endl;
                print_help();
                return false;
        }
    }

    if (optind + 2 != argc) {
        cerr << "Expected input and output file paths after options"<< endl;
        print_help();
        return false;
    }

    opts.input_file = argv[optind];
    opts.output_file = argv[optind + 1];

    return true;
}

void compress_video(const VideoOptions &opts) {
    AVFormatContext *format_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    AVCodec *codec = nullptr;

    // Open input video file
    if (avformat_open_input(&format_ctx, opts.input_file.c_str(), nullptr, nullptr) != 0) {
        throw runtime_error("Could not open input video file");
    }

    // Retrieve stream information
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        throw runtime_error("Could not find stream information");
    }

    // Find the first video stream
    int stream_idx = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (stream_idx < 0) {
        throw runtime_error("Could not find video stream in input file");
    }

    AVStream *stream = format_ctx->streams[stream_idx];

    // Get codec information
    AVCodecParameters *codec_params = stream->codecpar;
    codec = avcodec_find_decoder_by_name("libx264");
    if (!codec) {
        throw runtime_error("Unsupported codec");
    }

    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        throw runtime_error("Could not allocate video codec context");
    }

    // Copy codec parameters to codec context
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        throw runtime_error("Could not copy codec parameters to codec context");
    }

    // Set output video parameters
    if (opts.width > 0 && opts.height > 0) {
        codec_ctx->width = opts.width;
        codec_ctx->height = opts.height;
    } else {
        codec_ctx->width = codec_params->width;
        codec_ctx->height = codec_params->height;
    }

    if (opts.fps > 0.0) {
        codec_ctx->time_base = {1, static_cast<int>(round(1.0 / opts.fps))};
    } else {
        codec_ctx->time_base = stream->time_base;
    }

    // Set output video bitrate
    codec_ctx->bit_rate = static_cast<int>(round(opts.bitrate));

    // Enable output format conversion
    SwsContext *sws_ctx = sws_getContext(codec_params->width, codec_params->height, codec_params->pix_fmt,
                                         codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_ctx) {
        throw runtime_error("Could not create scaling context");
    }

    // Open output video file
    AVOutputFormat *output_format = av_guess_format("mp4", nullptr, nullptr);
    if (!output_format) {
        throw runtime_error("Could not determine output format");
    }

    AVStream *output_stream = avformat_new_stream(format_ctx, nullptr);
    if (!output_stream) {
        throw runtime_error("Could not create output stream");
    }

    output_stream->id = format_ctx->nb_streams - 1;
    output_stream->time_base = codec_ctx->time_base;

    if (avcodec_parameters_from_context(output_stream->codecpar, codec_ctx) < 0) {
        throw runtime_error("Could not copy codec parameters to output stream");
    }

    output_stream->codecpar->codec_id = output_format->video_codec;
    output_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;

    if (avformat_write_header(format_ctx, nullptr) < 0) {
        throw runtime_error("Error occurred when opening output file");
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        throw runtime_error("Could not allocate video packet");
    }

    // Read frames from input file and write to output file
    while (true) {
        int ret = av_read_frame(format_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw runtime_error("Error occurred when reading input frame");
        }

        if (packet->stream_index == stream_idx) {
            // Rescale pixel format if necessary
            if (packet->format != codec_ctx->pix_fmt) {
                AVFrame *frame = av_frame_alloc();
                if (!frame) {
                    throw runtime_error("Could not allocate video frame");
                }

                int ret = av_frame_ref(frame, packet->data);
                if (ret < 0) {
                    throw runtime_error("Could not reference video frame");
                }   

                ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_params->height,
                                codec_ctx->get_buffer(codec_ctx)->data, codec_ctx->get_buffer(codec_ctx)->linesize);
                if (ret < 0) {
                    throw runtime_error("Error occurred when scaling video frame");
                }

                av_frame_unref(frame);
            }

            // Encode video frame
            ret = avcodec_send_frame(codec_ctx, packet->data);
            if (ret < 0) {
                throw runtime_error("Error occurred when sending video frame to encoder");
            }

            while (true) {
                ret = avcodec_receive_packet(codec_ctx, packet);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    throw runtime_error("Error occurred when encoding video frame");
                }

                // Write compressed frame to output file
                ret = av_write_frame(format_ctx, packet);
                if (ret < 0) {
                    throw runtime_error("Error occurred when writing compressed frame to output file");
                }
            }
        }

        av_packet_unref(packet);
    }

    // Flush encoder and write trailer
    avcodec_send_frame(codec_ctx, nullptr);
    while (true) {
        ret = avcodec_receive_packet(codec_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw runtime_error("Error occurred when flushing encoder");
        }

        ret = av_write_frame(format_ctx, packet);
        if (ret < 0) {
            throw runtime_error("Error occurred when writing trailer to output file");
        }
    }

    av_write_trailer(format_ctx);

    // Free resources
    sws_freeContext(sws_ctx);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
}

int main(int argc, char *argv[]) {
    try {
        VideoOptions opts;
        if (!parse_options(argc, argv, opts)) {
            return 1;
        }

        compress_video(opts);
    } catch (const exception &e) {
        cerr << "Error: " << e.what()<< endl;
        return 1;
    }

    return 0;
}