#include <iostream>
#include <future>
#include <vector>
#include <map>
#include <algorithm> //find等方法
#include <memory>
using namespace std; //可以使用标准库里的符号和方法
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

int stream_video(const char *output_url, const char *input_file) {
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
    if (avformat_alloc_output_context2(&output_format_ctx, NULL, "flv", output_url) < 0) {
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
    if (avio_open(&output_format_ctx->pb, output_url, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "无法打开输出文件\n");
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
        if (av_interleaved_write_frame(output_format_ctx, &pkt) < 0) {
            fprintf(stderr, "无法写入数据包\n");
            return -1;
        }
        av_packet_unref(&pkt);
    }

    // 写入文件尾
    av_write_trailer(output_format_ctx);

    // 释放资源
    avformat_close_input(&input_format_ctx);
    if (output_format_ctx && !(output_format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_close(output_format_ctx->pb);
    }
    avformat_free_context(output_format_ctx);

    return 0;
}



//等待验证
int main (){
    const char *output_url = "rtmp://your_streaming_server/your_stream_path";
    const char *input_file = "your_video_file.mp4";
    int result = stream_video(output_url, input_file);
    if (result == 0) {
        printf("推流成功\n");
    } else {
        printf("推流失败\n");
    }
    return result;
    
}
