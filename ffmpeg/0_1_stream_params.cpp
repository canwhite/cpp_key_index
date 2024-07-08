extern "C" {
#include <libavformat/avformat.h>
}

#include <iostream>
using namespace std;
/** 音视频流的相关信息
 *
音频参数和视频参数的区别
音频参数和视频参数有相似之处，但也有很多不同之处，因为它们涉及到不同的媒体类型。以下是它们的主要参数及其说明：

音频参数：
codec_id：编码器ID，用于指定使用哪种编码格式（如AAC、MP3）。
codec_tag：编码标签，用于进一步标识编码类型。
sample_rate：采样率，表示每秒采样的次数，通常为44100Hz或48000Hz。
channels：声道数，表示音频的声道数量，如单声道（1）或立体声（2）。
channel_layout：声道布局，表示声道的具体排列方式，如立体声、5.1声道等。
bit_rate：比特率，表示每秒传输的比特数，通常以kbps为单位。
format：采样格式，表示每个采样点的编码格式（如浮点、整数）。
frame_size：帧大小，表示每个音频帧包含的采样数。
profile：编码配置文件，用于指定特定的编码特性或质量级别。


视频参数：
codec_id：编码器ID，用于指定使用哪种编码格式（如H.264、VP9）。
codec_tag：编码标签，用于进一步标识编码类型。
width：视频宽度，表示视频帧的宽度（像素）。
height：视频高度，表示视频帧的高度（像素）。
bit_rate：比特率，表示每秒传输的比特数，通常以kbps为单位。
format：像素格式，表示每个像素的编码格式（如YUV、RGB）。
frame_rate：帧率，表示每秒显示的帧数，通常为24fps、30fps或60fps。
color_space：色彩空间，表示视频的色彩表示方式（如BT.601、BT.709）。
aspect_ratio：宽高比，表示视频帧的宽高比例。


相似之处：
codec_id 和 codec_tag 都用于标识编码类型。
bit_rate 都表示每秒传输的比特数。
format 表示音频的采样格式和视频的像素格式，都是数据表示方式的一种。


不同之处：
音频有sample_rate 和 channels 等特定参数，
而视频则有 width、height 和 frame_rate 等参数。
视频涉及更多的图像处理参数，如 color_space 和 aspect_ratio，而音频更多关注声道和采样率。
总的来说，尽管音频和视频参数在某些方面有相似之处，但由于它们分别处理的是不同类型的媒体数据，许多参数是特定于各自领域的。
*/



void print_audio_stream_info(const char* input_filename) {
    AVFormatContext* format_context = nullptr;
    AVStream* audio_stream = nullptr;
    int audio_stream_index = -1;
    

    // Open input file
    if (avformat_open_input(&format_context, input_filename, nullptr, nullptr) != 0) {
        cerr << "Could not open input file." << endl;
        return;
    }

    // Find stream info
    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        cerr << "Could not find stream info." << endl;
        return;
    }

    // Find audio stream
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        //获取音频相关
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            audio_stream = format_context->streams[i];
            break;
        }
        //获取视频相关
        

    }

    if (audio_stream_index == -1) {
        cerr << "Could not find audio stream." << endl;
        avformat_close_input(&format_context);
        return;
    }

    // Print codec parameters,音频部分
    AVCodecParameters* codecpar = audio_stream->codecpar;
    cout << "Audio Stream Codec Parameters:" << endl;
    cout << "Codec ID: " << codecpar->codec_id << endl;
    cout << "Codec Tag: " << codecpar->codec_tag << endl;
    cout << "Sample Format: " << codecpar->format << endl;
    cout << "Sample Rate: " << codecpar->sample_rate << endl;
    // cout << "Ch Layout: " << codecpar->ch_layout << endl;
    cout << "Channels: " << codecpar->ch_layout.nb_channels << endl;
    cout << "Channel Layout: " << codecpar->ch_layout.order << endl;
    //然后如何获取
    cout << "Frame Size: " << codecpar->frame_size << endl;
    cout << "Bitrate: " << codecpar->bit_rate << endl;
    cout << "Profile: " << codecpar->profile << endl;


    //获取时间基stream和ctx里有
    /**
    使用场景：
    1）时间戳转化
    int64_t timestamp = av_rescale_q(packet->pts, stream->time_base, AV_TIME_BASE_Q);
    2）帧的时间标记
    每帧的显示时间戳（PTS）和解码时间戳（DTS）都基于流的时间基准。
    frame->pts = av_rescale_q(packet->dts, stream->time_base, codec_ctx->time_base);
    3）同步和编辑
    在同步音频和视频流，或进行剪辑和拼接时，
    需要使用 time_base 来确保不同流之间的时间对齐。
    */
    cout << "timebase: " << audio_stream -> time_base.num << ":"<< audio_stream ->time_base.den << endl;



    //视频部分也可以看下

    avformat_close_input(&format_context);
}

int main() {
    const char* input_filename = "/Users/zack/Desktop/test.mp4";
    print_audio_stream_info(input_filename);
    return 0;
}
