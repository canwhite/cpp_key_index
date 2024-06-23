extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/time.h>
}
int main() {


    const char* input_url = "/Users/zack/Desktop/test.mp4";
    const char* out_url = "rtmp://127.0.0.1/live/livestream";

    
    avformat_network_init();
    AVFormatContext* inputFormatContext = nullptr;
    if (avformat_open_input(&inputFormatContext, input_url, 0, 0) < 0) {
        printf("Could not open input file.");
        return -1;
    }
    if (avformat_find_stream_info(inputFormatContext, 0) < 0) {
        printf("Failed to retrieve input stream information");
        return -1;
    }
    //Alloc an avformat context
    AVFormatContext* outputFormatContext = nullptr;
    avformat_alloc_output_context2(&outputFormatContext, nullptr, "flv", out_url); //RTMP
    //Iterate input streams and add them to output context info
    for (int i = 0; i < inputFormatContext->nb_streams; i++) {
        AVStream *stream = avformat_new_stream(outputFormatContext, nullptr);
        avcodec_parameters_copy(stream->codecpar, inputFormatContext->streams[i]->codecpar);
        stream->codecpar->codec_tag = 0;
    }
    av_dump_format(outputFormatContext, 0, out_url, 1);
    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, out_url, AVIO_FLAG_WRITE) < 0) {
            printf("Could not open output URL");
            return -1;
        }
    }
    if (avformat_write_header(outputFormatContext, 0) < 0) {
        printf("Error occurred when opening output URL");
        return -1;
    }
    AVPacket pkg;
    av_init_packet(&pkg);
    while (av_read_frame(inputFormatContext, &pkg) >= 0) {
        av_write_frame(outputFormatContext, &pkg);
        av_packet_unref(&pkg);
        //Sleep based on framerate to simulate a real streaming situation
        // av_usleep((unsigned int)(1000000 * (1.0 / av_q2d(inputFormatContext->streams[0]->r_frame_rate))));
    }
    av_write_trailer(outputFormatContext);
    //Cleaning
    avformat_close_input(&inputFormatContext);
    if (!outputFormatContext)
        return 0;
    if (outputFormatContext && !(outputFormatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outputFormatContext->pb);
    avformat_free_context(outputFormatContext);
    return 0;
}