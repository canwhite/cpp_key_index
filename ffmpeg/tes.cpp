extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/samplefmt.h>
    #include <libswresample/swresample.h>
}

int main(int argc, char *argv[]) {
    const char *input_filename = "input.mp4";
    const char *output_filename = "output.mp3";

    // av_register_all();

    AVFormatContext *input_format_context = nullptr;
    if (avformat_open_input(&input_format_context, input_filename, nullptr, nullptr) < 0) {
        fprintf(stderr, "Could not open input file '%s'\n", input_filename);
        return -1;
    }

    if (avformat_find_stream_info(input_format_context, nullptr) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information\n");
        return -1;
    }

    AVCodec *input_codec = nullptr;
    AVCodecContext *input_codec_context = nullptr;
    int audio_stream_index = -1;
    for (unsigned int i = 0; i < input_format_context->nb_streams; i++) {
        AVStream *stream = input_format_context->streams[i];
        AVCodecParameters *codec_params = stream->codecpar;
        AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
        if (codec && codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
            input_codec = codec;
            input_codec_context = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(input_codec_context, codec_params);
            avcodec_open2(input_codec_context, codec, nullptr);
            audio_stream_index = i;
            break;
        }
    }

    if (!input_codec_context) {
        fprintf(stderr, "Could not find audio stream in the input file\n");
        return -1;
    }

    AVFormatContext *output_format_context = nullptr;
    avformat_alloc_output_context2(&output_format_context, nullptr, nullptr, output_filename);
    if (!output_format_context) {
        fprintf(stderr, "Could not create output context\n");
        return -1;
    }

    AVStream *output_stream = avformat_new_stream(output_format_context, nullptr);
    if (!output_stream) {
        fprintf(stderr, "Failed allocating output stream\n");
        return -1;
    }

    AVCodec *output_codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!output_codec) {
        fprintf(stderr, "Necessary encoder not found\n");
        return -1;
    }

    AVCodecContext *output_codec_context = avcodec_alloc_context3(output_codec);
    output_codec_context->sample_rate = input_codec_context->sample_rate;
    output_codec_context->ch_layout = input_codec_context->ch_layout;
    output_codec_context->channels = av_get_channel_layout_nb_channels(output_codec_context->ch_layout);
    output_codec_context->sample_fmt = output_codec->sample_fmts[0];
    output_codec_context->bit_rate = 192000;

    if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
        output_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(output_codec_context, output_codec, nullptr) < 0) {
        fprintf(stderr, "Cannot open output codec context\n");
        return -1;
    }

    avcodec_parameters_from_context(output_stream->codecpar, output_codec_context);
    output_stream->time_base = (AVRational){1, output_codec_context->sample_rate};

    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open output file '%s'\n", output_filename);
            return -1;
        }
    }

    if (avformat_write_header(output_format_context, nullptr) < 0) {
        fprintf(stderr, "Error occurred when writing header\n");
        return -1;
    }

    SwrContext *swr_ctx = swr_alloc();
    av_opt_set_int(swr_ctx, "in_ch_layout", input_codec_context->ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", input_codec_context->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", input_codec_context->sample_fmt, 0);
    av_opt_set_int(swr_ctx, "out_ch_layout", output_codec_context->ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", output_codec_context->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", output_codec_context->sample_fmt, 0);
    swr_init(swr_ctx);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *resampled_frame = av_frame_alloc();
    resampled_frame->format = output_codec_context->sample_fmt;
    resampled_frame->ch_layout = output_codec_context->ch_layout;
    resampled_frame->sample_rate = output_codec_context->sample_rate;
    resampled_frame->nb_samples = output_codec_context->frame_size;
    av_frame_get_buffer(resampled_frame, 0);

    while (av_read_frame(input_format_context, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (avcodec_send_packet(input_codec_context, packet) >= 0) {
                while (avcodec_receive_frame(input_codec_context, frame) >= 0) {
                    swr_convert(swr_ctx, resampled_frame->data, resampled_frame->nb_samples,
                                (const uint8_t **)frame->data, frame->nb_samples);
                    resampled_frame->pts = av_rescale_q(frame->pts, input_format_context->streams[audio_stream_index]->time_base,
                                                        output_codec_context->time_base);
                    avcodec_send_frame(output_codec_context, resampled_frame);
                    while (avcodec_receive_packet(output_codec_context, packet) >= 0) {
                        av_interleaved_write_frame(output_format_context, packet);
                        av_packet_unref(packet);
                    }
                }
            }
        }
        av_packet_unref(packet);
    }

    av_write_trailer(output_format_context);
    av_frame_free(&frame);
    av_frame_free(&resampled_frame);
    av_packet_free(&packet);
    swr_free(&swr_ctx);
    avcodec_free_context(&input_codec_context);
    avcodec_free_context(&output_codec_context);
    avformat_close_input(&input_format_context);
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE))
        avio_closep(&output_format_context->pb);
    avformat_free_context(output_format_context);

    return 0;
}
