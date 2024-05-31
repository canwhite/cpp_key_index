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
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}


int add_watermark(const char *input_file, const char *output_file, const char *watermark_file) {
    // 注册所有组件
    // av_register_all();
    // avformat_network_init();

    // 打开输入文件
    AVFormatContext *input_format_ctx = NULL;
    if (avformat_open_input(&input_format_ctx, input_file, NULL, NULL) != 0) {
        fprintf(stderr, "无法打开输入文件\n");
        return -1;
    }

    // 查找流信息
    if (avformat_find_stream_info(input_format_ctx, NULL) < 0) {
        fprintf(stderr, "无法找到流信息\n");
        return -1;
    }

    // 创建输出上下文
    AVFormatContext *output_format_ctx = NULL;
    if (avformat_alloc_output_context2(&output_format_ctx, NULL, NULL, output_file) < 0) {
        fprintf(stderr, "无法创建输出上下文\n");
        return -1;
    }

    // 复制流信息
    for (int i = 0; i< input_format_ctx->nb_streams; i++) {
        AVStream *in_stream = input_format_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(output_format_ctx, in_stream->codec->codec);
        if (!out_stream) {
            fprintf(stderr, "无法创建输出流\n");
            return -1;
        }
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            fprintf(stderr, "无法复制编解码器上下文\n");
            return -1;
        }
        out_stream->codec->codec_tag = 0;
    }

    // 打开输出文件
    if (avio_open(&output_format_ctx->pb, output_file, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "无法打开输出文件\n");
        return -1;
    }

    // 创建过滤器图
    AVFilterGraph *filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        fprintf(stderr, "无法创建过滤器图\n");
        return -1;
    }

    // 创建视频过滤器
    AVFilterContext *buffer_src_ctx = NULL;
    AVFilterContext *buffer_sink_ctx = NULL;
    AVFilter *buffer_src = avfilter_get_by_name("buffer");
    AVFilter *buffer_sink = avfilter_get_by_name("buffersink");
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *outputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSrcParameters *src_params = av_buffersrc_parameters_alloc();
    src_params->format = AV_PIX_FMT_YUV420P;
    src_params->width = input_format_ctx->streams[0]->codec->width;
    src_params->height = input_format_ctx->streams[0]->codec->height;
    src_params->time_base = input_format_ctx->streams[0]->codec->time_base;
    src_params->sample_aspect_ratio = input_format_ctx->streams[0]->codec->sample_aspect_ratio;
    src_params->frame_rate = input_format_ctx->streams[0]->codec->framerate;
    src_params->hw_frames_ctx = NULL;

    // 创建水印过滤器
    AVFilter *movie_filter = avfilter_get_by_name("movie");
    AVFilter *overlay_filter = avfilter_get_by_name("overlay");
    char movie_filter_args[256];
    snprintf(movie_filter_args, sizeof(movie_filter_args), "movie=%s [watermark]; [in][watermark] overlay=10:10 [out]", watermark_file);

    // 解析过滤器描述
    if (avfilter_graph_parse2(filter_graph, movie_filter_args, &inputs, &outputs) < 0) {
        fprintf(stderr, "无法解析过滤器描述\n");
        return -1;
    }

    // 配置过滤器图
    if (avfilter_graph_config(filter_graph, NULL) < 0) {
        fprintf(stderr, "无法配置过滤器图\n");
        return -1;
    }

    // 写入文件头
    if (avformat_write_header(output_format_ctx, NULL) < 0) {
        fprintf(stderr, "无法写入文件头\n");
        return -1;
    }

    // 读取并写入数据包
    AVPacket pkt;
    while (av_read_frame(input_format_ctx, &pkt) >= 0) {
        if (av_buffersrc_add_frame_flags(buffer_src_ctx, pkt.data, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
            fprintf(stderr, "无法将帧添加到缓冲区源\n");
            return -1;
        }
        if (av_buffersink_get_frame(buffer_sink_ctx, pkt.data) < 0) {
            fprintf(stderr, "无法从缓冲区接收器获取帧\n");
            return -1;
        }
        if (av_interleaved_write_frame(output_format_ctx, &pkt) < 0) {
            fprintf(stderr, "无法写入数据包\n");
            return -1;
        }
        av_packet_unref(&pkt);
    }

    // 写入文件尾
    av_write_trailer(output_format_ctx);

    // 释放资源
    avfilter_graph_free(&filter_graph);
    avformat_close_input(&input_format_ctx);
    if (output_format_ctx && !(output_format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_close(output_format_ctx->pb);
    }
    avformat_free_context(output_format_ctx);

    return 0;
}


//TODO，等待验证
int main(){

    const char *input_file = "your_video_file.mp4";
    const char *output_file = "your_video_file_with_watermark.mp4";
    const char *watermark_file = "your_watermark_image.png";
    int result = add_watermark(input_file, output_file, watermark_file);
    if (result == 0) {
        printf("添加水印成功\n");
    } else {
        printf("添加水印失败\n");
    }
    return result;
}
