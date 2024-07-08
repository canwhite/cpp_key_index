#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

int main(int argc, char *argv[]) {
    AVFormatContext *input_format_context = NULL, *output_format_context = NULL;
    AVCodecContext *input_codec_context = NULL, *output_codec_context = NULL;
    SwrContext *resample_context = NULL;
    AVStream *out_stream = NULL;
    const AVCodec *output_codec;
    int ret;

    // 打开输入文件
    if ((ret = avformat_open_input(&input_format_context, "/Users/zack/Desktop/test.mp4", NULL, NULL)) < 0) {
        fprintf(stderr, "Could not open input file\n");
        return 1;
    }

    // 查找流信息
    if ((ret = avformat_find_stream_info(input_format_context, NULL)) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return 1;
    }

    // 找到音频流
    int audio_stream_index = av_find_best_stream(input_format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audio_stream_index < 0) {
        fprintf(stderr, "Could not find audio stream\n");
        return 1;
    }

    // 创建输出格式上下文
    avformat_alloc_output_context2(&output_format_context, NULL, NULL, "output.mp3");
    if (!output_format_context) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        return 1;
    }

    // 找到MP3编码器
    output_codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!output_codec) {
        fprintf(stderr, "Could not find encoder for MP3\n");
        return 1;
    }

    // 创建输出流
    out_stream = avformat_new_stream(output_format_context, NULL);
    if (!out_stream) {
        fprintf(stderr, "Failed allocating output stream\n");
        ret = AVERROR_UNKNOWN;
        return 1;
    }

    output_codec_context = avcodec_alloc_context3(output_codec);
    if (!output_codec_context) {
        fprintf(stderr, "Could not allocate an encoding context\n");
        ret = AVERROR(ENOMEM);
        return 1;
    }

    // 设置输出参数
    output_codec_context->sample_fmt  = AV_SAMPLE_FMT_S16;
    output_codec_context->ch_layout = output_codec_context->ch_layout;
    output_codec_context->sample_rate    = 44100;
    // output_codec_context->channels       = 2;
    output_codec_context->bit_rate       = 128000;

    // 打开编码器
    if ((ret = avcodec_open2(output_codec_context, output_codec, NULL)) < 0) {
        fprintf(stderr, "Could not open output codec\n");
        return 1;
    }

    // 复制参数到输出流
    ret = avcodec_parameters_from_context(out_stream->codecpar, output_codec_context);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        return 1;
    }

    // 打开输出文件
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&output_format_context->pb, "output.mp3", AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file\n");
            return 1;
        }
    }

    // 写文件头
    ret = avformat_write_header(output_format_context, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return 1;
    }

    // 初始化重采样上下文
    resample_context = swr_alloc();
    if (!resample_context) {
        fprintf(stderr, "Could not allocate resample context\n");
        ret = AVERROR(ENOMEM);
        return 1;
    }

    // 设置重采样参数
    
    // av_opt_set_int(resample_context, "in_channel_layout", input_format_context->streams[audio_stream_index]->codecpar->channel_layout, 0);
    // av_opt_set_int(resample_context, "in_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(resample_context, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(resample_context, "in_sample_rate", input_format_context->streams[audio_stream_index]->codecpar->sample_rate, 0);
    av_opt_set_int(resample_context, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(resample_context, "in_sample_fmt", (enum AVSampleFormat)input_format_context->streams[audio_stream_index]->codecpar->format, 0);
    av_opt_set_sample_fmt(resample_context, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    // 初始化重采样上下文
    if ((ret = swr_init(resample_context)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        return 1;
    }

    // 读取输入文件并编码到输出文件
    AVPacket *input_packet = av_packet_alloc();
    AVFrame *input_frame = av_frame_alloc();
    AVFrame *output_frame = av_frame_alloc();
    AVPacket *output_packet = av_packet_alloc();

    output_frame->nb_samples = output_codec_context->frame_size;
    output_frame->format = output_codec_context->sample_fmt;
    output_frame->ch_layout = output_codec_context->ch_layout;

    ret = av_frame_get_buffer(output_frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate output frame samples\n");
        return 1;
    }

    while (av_read_frame(input_format_context, input_packet) >= 0) {
        if (input_packet->stream_index == audio_stream_index) {
            ret = avcodec_send_packet(input_codec_context, input_packet);
            if (ret < 0) {
                fprintf(stderr, "Error submitting the packet to the decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(input_codec_context, input_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    return 1;
                }

                // 重采样
                ret = swr_convert(resample_context,
                                  output_frame->data, output_frame->nb_samples,
                                  (const uint8_t **)input_frame->data, input_frame->nb_samples);
                if (ret < 0) {
                    fprintf(stderr, "Error while converting\n");
                    return 1;
                }
                output_frame->pts = av_rescale_q(input_frame->pts,
                                                 input_format_context->streams[audio_stream_index]->time_base,
                                                 output_codec_context->time_base);

                // 编码
                ret = avcodec_send_frame(output_codec_context, output_frame);
                if (ret < 0) {
                    fprintf(stderr, "Error sending the frame to the encoder\n");
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_packet(output_codec_context, output_packet);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during encoding\n");
                        return 1;
                    }

                    // 写入输出文件
                    ret = av_interleaved_write_frame(output_format_context, output_packet);
                    if (ret < 0) {
                        fprintf(stderr, "Error while writing output packet\n");
                        return 1;
                    }
                    av_packet_unref(output_packet);
                }
            }
        }
        av_packet_unref(input_packet);
    }

    // 写文件尾
    av_write_trailer(output_format_context);

    // 清理资源
    avformat_close_input(&input_format_context);
    if (output_format_context && !(output_format_context->oformat->flags & AVFMT_NOFILE))
        avio_closep(&output_format_context->pb);
    avformat_free_context(output_format_context);
    avcodec_free_context(&input_codec_context);
    avcodec_free_context(&output_codec_context);
    swr_free(&resample_context);
    av_frame_free(&input_frame);
    av_frame_free(&output_frame);
    av_packet_free(&input_packet);
    av_packet_free(&output_packet);

    return 0;
}