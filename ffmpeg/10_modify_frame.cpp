#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include <stdexcept>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

//例如用第i帧置换第j帧
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: "<< argv[0] << " input_file output_file frame_index new_frame_data"<< std::endl;
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];
    int frame_index = std::stoi(argv[3]);
    std::vector<uint8_t> new_frame_data(/* new frame data size */);

    // Register all formats and codecs
    av_register_all();

    // Open input video file
    AVFormatContext* input_format_context = nullptr;
    if (avformat_open_input(&input_format_context, input_file, nullptr, nullptr) != 0) {
        throw std::runtime_error("Could not open input file");
    }

    // Get input video stream information
    AVStream* input_video_stream = nullptr;
    if (input_format_context->nb_streams > 0 && input_format_context->streams[0]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        input_video_stream = input_format_context->streams[0];
    } else {
        throw std::runtime_error("Input file does not contain a video stream");
    }

    AVCodecParameters* input_codec_parameters = input_video_stream->codecpar;
    AVCodec* input_codec = avcodec_find_decoder(input_codec_parameters->codec_id);
    if (!input_codec) {
        throw std::runtime_error("Unsupported codec");
    }

    AVCodecContext* input_codec_context = avcodec_alloc_context3(input_codec);
    if (!input_codec_context) {
        throw std::runtime_error("Could not allocate video codec context");
    }
    input_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    if (avcodec_parameters_to_context(input_codec_context, input_codec_parameters) < 0) {
        throw std::runtime_error("Could not copy codec parameters to codec context");
    }

    if (avcodec_open2(input_codec_context, input_codec, nullptr) < 0) {
        throw std::runtime_error("Could not open video codec");
    }

    // Create output video file
    AVFormatContext* output_format_context = nullptr;
    if (avformat_alloc_output_context2(&output_format_context, nullptr, nullptr, output_file) < 0) {
        throw std::runtime_error("Could not create output format context");
    }

    AVStream* output_video_stream = avformat_new_stream(output_format_context, nullptr);
    if (!output_video_stream) {
        throw std::runtime_error("Could not create output video stream");
    }

    output_video_stream->codecpar->codec_id = input_codec_parameters->codec_id;
    output_video_stream->codecpar->width = input_codec_parameters->width;
    output_video_stream->codecpar->height = input_codec_parameters->height;
    output_video_stream->codecpar->format = input_codec_parameters->format;
    output_video_stream->time_base = input_video_stream->time_base;

    if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
        output_video_stream->codecpar->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avio_open(&output_format_context->pb, output_file, AVIO_FLAG_WRITE) < 0) {
        throw std::runtime_error("Could not open output file");
    }

    if (avformat_write_header(output_format_context, nullptr) < 0) {
        throw std::runtime_error("Could not write output file header");
    }

    SwsContext* sws_context = sws_getContext(
        input_codec_parameters->width, input_codec_parameters->height, input_codec_parameters->format,
        input_codec_parameters->width, input_codec_parameters->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    if (!sws_context) {
        throw std::runtime_error("Could not create scaling context");
    }

    AVPacket input_packet;
    av_init_packet(&input_packet);
    input_packet.data = nullptr;
    input_packet.size = 0;

    AVFrame* input_frame = av_frame_alloc();
    AVFrame* output_frame = av_frame_alloc();
    if (!input_frame || !output_frame) {
        throw std::runtime_error("Could not allocate video frames");
    }

    while (true) {
        // Read input frame
        int ret = av_read_frame(input_format_context, &input_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw std::runtime_error("Error reading input frame");
        }

        if (input_packet.stream_index == input_video_stream->index) {
            // Decode input frame
            ret = avcodec_send_packet(input_codec_context, &input_packet);
            if (ret < 0) {
                throw std::runtime_error("Error sending input packet to codec");
            }

            while (true) {
                ret = avcodec_receive_frame(input_codec_context, input_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    throw std::runtime_error("Error receiving input frame from codec");
                }

                // Scale input frame to output frame size
                sws_scale(sws_context,
                    (const uint8_t* const*)input_frame->data, input_frame->linesize, 0,
                    input_codec_parameters->height,
                    output_frame->data, output_frame->linesize
                );

                // Replace pixels in output frame with new data
                // TODO: implement pixel replacement logic

                // Encode output frame to packet
                ret = avcodec_send_frame(output_video_stream->codec, output_frame);
                if (ret < 0) {
                    throw std::runtime_error("Error sending output frame to codec");
                }

                while (true) {
                    ret = avcodec_receive_packet(output_video_stream->codec, &input_packet);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    } else if (ret < 0) {
                        throw std::runtime_error("Error receiving output packet from codec");
                    }

                    // Write output packet to output file
                    ret = av_write_frame(output_format_context, &input_packet);
                    if (ret < 0) {
                        throw std::runtime_error("Error writing output packet to file");
                    }

                    av_packet_unref(&input_packet);
                }
            }

            // Free input frame
            av_frame_unref(input_frame);
        }

        // Free input packet
        av_packet_unref(&input_packet);
    }

    // Write output file trailer
    av_write_trailer(output_format_context);

    // Close output file
    avio_closep(&output_format_context->pb);

    // Free resources
    sws_freeContext(sws_context);
    av_frame_free(&input_frame);
    av_frame_free(&output_frame);
    avcodec_free_context(&input_codec_context);
    avformat_free_context(output_format_context);

    return 0;
}