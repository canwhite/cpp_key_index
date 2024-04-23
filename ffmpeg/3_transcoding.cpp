#include <iostream>
#include <future>
#include <vector>
#include <map>
#include <algorithm> //find等方法
#include <memory>
#include <filesystem>
using namespace std; //可以使用标准库里的符号和方法
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}


int main (){


    auto inputFilePath = "/Users/zack/Desktop/test.mp4";
    auto outputUrl = "out.mp4";

    AVFormatContext *inputFormatContext = avformat_alloc_context();
    avformat_open_input(&inputFormatContext, inputFilePath, NULL, NULL);
   

    if (avformat_open_input(&inputFormatContext, inputFilePath, NULL, NULL) < 0) {
        fprintf(stderr, "Failed to open input file %s\n", inputFilePath);
        return -1;
    }
    // Try to read and decode a few frames to fill in any gaps in the stream info in the file header
    if (avformat_find_stream_info(inputFormatContext, NULL) < 0) {
        fprintf(stderr, "Failed to find stream info\n");
        return -1;
    }

    // Print some information about the file to aid debugging
    av_dump_format(inputFormatContext, 0, inputFilePath, 0);

    // Check that the file contains a video stream
    AVStream *videoStream = NULL;
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = inputFormatContext->streams[i];
            break;
        }
    }
    if (videoStream == NULL) {
        fprintf(stderr, "Failed to find a video stream\n");
        return -1;
    }


    AVFormatContext *outputFormatContext = avformat_alloc_context();

    // Allocate output file struct and copy file context over from the input file
    if (avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, outputUrl) < 0) {
        fprintf(stderr, "Failed to allocate output context\n");
        return -1;
    }

    // For each input file stream, create a new stream in the output file context and copy over the codec parameters
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++) {
        AVStream *outputStream = avformat_new_stream(outputFormatContext, NULL);
        if (!outputStream) {
            fprintf(stderr, "Failed to allocate output stream\n");
            return -1;
        }

        AVStream *inputStream = inputFormatContext->streams[i];
        if (avcodec_parameters_copy(outputStream->codecpar, inputStream->codecpar) < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            return -1;
        }
    }

    // Print some information about the output file format to aid debugging
    av_dump_format(outputFormatContext, 0, outputUrl, 1);

    // If the output file already exists, attempt to open it, setting up the IO context for writing
    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, outputUrl, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Failed to open output file '%s'", outputUrl);
            return -1;
        }
    }

    // Write the output file header
    if (avformat_write_header(outputFormatContext, NULL) < 0) {
        fprintf(stderr, "Failed to write output file header\n");
        return -1;
    }

    // Copy across the file data, one packet at a time
    AVPacket pkt;
    while (1) {
        AVStream *inputStream, *outputStream;

        // Get the next frame from the input file
        if (av_read_frame(inputFormatContext, &pkt) < 0) break;
        // log_packet(inputFormatContext, &pkt, "in ");

        // Get the relevant input and output streams
        inputStream  = inputFormatContext->streams[pkt.stream_index];
        outputStream = outputFormatContext->streams[pkt.stream_index];

        // Ensure timing properties pts, dts, and duration are scaled appropriately for the output time base
        pkt.pts = av_rescale_q_rnd(pkt.pts, inputStream->time_base, outputStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, inputStream->time_base, outputStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, inputStream->time_base, outputStream->time_base);
        pkt.pos = -1; // Unknown byte position in the stream
        // log_packet(outputFormatContext, &pkt, "out");

        // Write the frame to the output file
        if (av_interleaved_write_frame(outputFormatContext, &pkt) < 0) {
            fprintf(stderr, "Failed to write packet to output file\n");
            break;
        }

        // Finally, free the packet struct used for copying
        av_packet_unref(&pkt);
    }
    std::cout << std::endl;

    // Flush any buffered packets and finalise the output file
    av_write_trailer(outputFormatContext);

    avformat_close_input(&inputFormatContext);
    avformat_free_context(inputFormatContext);
    avformat_free_context(outputFormatContext);
    // avcodec_free_context(&videoCodecContext);
    // avcodec_free_context(&encoderContext);
    // sws_freeContext(swsContext);
    // av_frame_free(&frame);

    return 0;
}


