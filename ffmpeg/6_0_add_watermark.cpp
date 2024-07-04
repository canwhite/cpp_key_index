extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

using namespace std;


int main(int argc, char* argv[]) {
    const char *input_filename = "/Users/zack/Desktop/test.mp4";
    const char *output_filename = "output.mp4";
    const char *watermark_filename = "/Users/zack/Desktop/bing@2x.png";



    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *dec_ctx = nullptr;
    AVCodecContext *enc_ctx = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;
    AVFilterContext *buffersink_ctx = nullptr;
    AVFilterGraph *filter_graph = nullptr;
    int video_stream_index = -1;

    if (avformat_open_input(&fmt_ctx, input_filename, nullptr, nullptr) < 0) {
        fprintf(stderr, "Could not open input file.\n");
        return -1;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information.\n");
        return -1;
    }

    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        fprintf(stderr, "Could not find video stream.\n");
        return -1;
    }

    const AVCodec *dec = avcodec_find_decoder(fmt_ctx->streams[video_stream_index]->codecpar->codec_id);
    dec_ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);



    if (avcodec_open2(dec_ctx, dec, nullptr) < 0) {
        fprintf(stderr, "Could not open codec.\n");
        return -1;
    }



    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        fprintf(stderr, "Could not allocate filter graph.\n");
        return -1;
    }

    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             fmt_ctx->streams[video_stream_index]->time_base.num, fmt_ctx->streams[video_stream_index]->time_base.den,
             dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);
         
    // AVRational frame_rate = dec_ctx->framerate;

    printf("Frame rate: %d/%d\n", 
    fmt_ctx->streams[video_stream_index]->time_base.num , 
    fmt_ctx->streams[video_stream_index]->time_base.den);


    if (avfilter_graph_create_filter(&buffersrc_ctx, avfilter_get_by_name("buffer"), "in",
                                     args, nullptr, filter_graph) < 0) {
        fprintf(stderr, "Could not create buffer source.\n");
        return -1;
    }

    if (avfilter_graph_create_filter(&buffersink_ctx, avfilter_get_by_name("buffersink"), "out",
                                     nullptr, nullptr, filter_graph) < 0) {
        fprintf(stderr, "Could not create buffer sink.\n");
        return -1;
    }

    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *outputs = avfilter_inout_alloc();

    if (!inputs || !outputs) {
        fprintf(stderr, "Could not allocate filter inputs/outputs.\n");
        return -1;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    char filter_descr[512];
    snprintf(filter_descr, sizeof(filter_descr),
             "movie=%s [wm]; [in][wm] overlay=10:10 [out]",
             watermark_filename);

    if (avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, nullptr) < 0) {
        fprintf(stderr, "Could not parse filter graph.\n");
        return -1;
    }

    if (avfilter_graph_config(filter_graph, nullptr) < 0) {
        fprintf(stderr, "Could not configure filter graph.\n");
        return -1;
    }

    // 设置输出文件
    AVFormatContext *ofmt_ctx = nullptr;
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, output_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context.\n");
        return -1;
    }

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, nullptr);
    if (!out_stream) {
        fprintf(stderr, "Failed allocating output stream.\n");
        return -1;
    }

    const AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_H264);
    enc_ctx = avcodec_alloc_context3(enc);
    enc_ctx->width = dec_ctx->width;
    enc_ctx->height = dec_ctx->height;
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    
    enc_ctx->time_base =  (AVRational){
        fmt_ctx->streams[video_stream_index]->time_base.num,
        fmt_ctx->streams[video_stream_index]->time_base.den
        }; // 设置适合的时间基准

    enc_ctx->bit_rate = 400000;
    enc_ctx->gop_size = 10; 
    enc_ctx->max_b_frames = 1;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(enc_ctx, enc, nullptr) < 0) {
        fprintf(stderr, "Could not open encoder.\n");
        return -1;
    }

    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    out_stream->time_base = enc_ctx->time_base;

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open output file.\n");
            return -1;
        }
    }

    if (avformat_write_header(ofmt_ctx, nullptr) < 0) {
        fprintf(stderr, "Error writing header.\n");
        return -1;
    }

    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();

    while (av_read_frame(fmt_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            if (avcodec_send_packet(dec_ctx, &packet) < 0) {
                fprintf(stderr, "Error sending packet.\n");
                continue;
            }

            while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
                if (av_buffersrc_add_frame(buffersrc_ctx, frame) < 0) {
                    fprintf(stderr, "Error adding frame to buffer source.\n");
                    continue;
                }

                while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0) {
                    filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
                    avcodec_send_frame(enc_ctx, filt_frame);

                    AVPacket enc_pkt;
                    av_init_packet(&enc_pkt);
                    enc_pkt.data = nullptr;
                    enc_pkt.size = 0;

                    if (avcodec_receive_packet(enc_ctx, &enc_pkt) == 0) {
                        av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base, out_stream->time_base);
                        enc_pkt.stream_index = out_stream->index;
                        av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
                        av_packet_unref(&enc_pkt);
                    }
                    av_frame_unref(filt_frame);
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }

    av_write_trailer(ofmt_ctx);

    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    avcodec_free_context(&dec_ctx);
    avcodec_free_context(&enc_ctx);
    avformat_close_input(&fmt_ctx);
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
    avfilter_graph_free(&filter_graph);

    return 0;
}
