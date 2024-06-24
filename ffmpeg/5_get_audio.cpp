extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
}
// how to ask for answer? 关键在于如何问问题？
// 前置条件，想要的结果，如有需要，加些细节
// ffmpeg version7 get audio as mp3 from video  by c++，give me all the code
int main() {
    AVFormatContext* formatContext = nullptr;
    int audioStreamIndex = -1;
    // 1. 打开输入文件
    if (avformat_open_input(&formatContext, "/Users/zack/Desktop/test.mp4", nullptr, nullptr) < 0) {
        return 1;  // 出现错误
    }
    // 2. 寻找音频流
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1) {
        return 1;  // 无法找到音频流
    }
    // 获取解码器
    AVCodecParameters* codecParameters = formatContext->streams[audioStreamIndex]->codecpar;
    AVCodec* codec = (AVCodec* )avcodec_find_decoder(codecParameters->codec_id);
    if (!codec) {
        return 1;  // 解码器未找到
    }
    // 创建编码器
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        return 1;  // 初始化编码器失败
    }
    // 打开编码器
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        return 1;  // 打开编码器失败
    }
    // 创建输出
    AVFormatContext* outputContext;
    if (avformat_alloc_output_context2(&outputContext, nullptr, nullptr, "output.aac") < 0) {
        return 1;  // 创建输出文件失败
    }
    // 创建输出流
    AVStream* outputStream = avformat_new_stream(outputContext, codec);
    if (outputStream == nullptr) {
        return 1;  // 创建输出流失败
    }
    // 注意这点很重要，复制输入流参数信息设置输出流参数
    if (avcodec_parameters_copy(outputStream->codecpar, codecParameters) < 0) {
        return 1;  // 复制解码器参数到输出流失败
    }
    // 打开音频文件并将文件头写入
    //在使用FFmpeg库时，通常在打开一个输出文件后需要写入文件头信息。
    //写入文件头信息的目的是为了在文件中包含必要的元数据和格式信息，以便后续解码和处理这个文件。
    //avformat_write_header函数的第二个参数是一个包含一些额外选项的字典。
    //这个参数通常用来指定一些特定的设置或配置，例如音频编码器的参数、视频编码器的参数等。
    //如果没有需要特殊配置的情况，可以将第二个参数设置为nullptr，表示不传递任何额外选项。 
    if (avio_open(&outputContext->pb, "output.aac", AVIO_FLAG_WRITE) < 0 || avformat_write_header(outputContext, nullptr) < 0) {
        return 1;  // 无法打开音频文件或写入文件头
    }
    AVPacket packet;
    av_init_packet(&packet);
    // 循环读包并写入音频文件
    while (av_read_frame(formatContext, &packet) == 0) {
        if (packet.stream_index == audioStreamIndex) {
            // packet.pts = packet.dts = AV_NOPTS_VALUE;
            packet.stream_index = 0;
            //和av_write_frame的区别在于在交错的数据中，
            //音频帧和视频帧会交错存储，以便在播放时能够按照正确的时间顺序呈现。
            av_interleaved_write_frame(outputContext, &packet);
        }
    }
    // 写入文件尾并关闭文件
    av_write_trailer(outputContext);
    avio_closep(&outputContext->pb);
    // 清理
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_free_context(outputContext);
    return 0;
}



