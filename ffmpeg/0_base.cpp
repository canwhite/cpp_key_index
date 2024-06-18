/**

Pre：
概念：
1）视频-目光所见

如果以一定的频率播放一组图片(比如每秒24张图片)，人将会产生视觉暂留现象。 
概括来讲，视频的本质就是: 以给定频率播放的一系列图片/帧.


2）音频-耳朵所听

尽管一个没有声音的视频也可以表达很多感受和情绪，但加入音频会带来更多的体验乐趣。
声音是指以压力波形式通过空气或其他介质（例如气体、液体或者固体）传播的振动。
PS:
在数字音频系统中，麦克风将声音转换为模拟电信号，
然后通常使用脉冲编码调制（PCM）的模数转换器（ADC）将模拟信号转换为数字信号。


3）编解码-压缩数据
PS:
CODEC是用于压缩或解压缩数字音频/视频的硬件或软件。 它提供将原始（未压缩的）数字音频/视频与压缩格式相互转换的能力。
如果我们选择打包数百万张图片来生成一个视频文件，那么该文件的大小将会非常惊人。让我们来计算一下：

假如我们创建一个 1080x1920 (高x宽)的视频，每个像素占用 3 bytes 对颜色进行编码
(或使用 24 bit 真色彩, 这可以提供 16,777,216 种不同的颜色)，
每秒 24 帧，视频时长为 30 分钟。

tis = 30 * 60 // 时长(秒)
fps = 24 // 每秒帧数
toppf = 1080 * 1920 // 每帧所有的像素点
cpp = 3 // 每个像素的大小(bytes)

required_storage = tis * fps * toppf * cpp
计算结果显示，此视频需要大约 250.28G 的存储空间或 1.19Gbps 的带宽。
这就是我们为什么需要使用 CODEC 的原因。


4）容器-整合音视频
容器或者封装格式描述了不同的数据元素和元数据是如何在计算机文件中共存的
单个这样的文件包含所有的流（主要是音频和视频），并提供同步和通用元数据，比如标题、分辨率等等。
一般我们可以通过文件的后缀来判断文件格式：比如 video.webm 通常是一个使用 webm 容器格式的视频
C webm      C mp4
V vp9       V h264
A opus      A aac



FFmpeg是一个非常强大的视频处理库，它包括以下几个主要部分：
1. `ffmpeg`：
这是一个强大的转码工具，可以用来转换音频和视频格式，也可以用来提取音频和视频流。

2. `ffserver`：
这是一个HTTP和RTSP媒体服务器。可以用来直播流或者通过HTTP分发实时广播。

3. `ffprobe`：这是一个方便的工具，可以用来分析多媒体内容，
例如，获取音视频的参数（比如，持续时间，格式，编码参数等等）。


4. `ffplay`：
它是一个简单的多媒体播放器，基于SDL和FFmpeg库构造。


关于内部的一些工具：

1. libavcodec： 
这是一个编解码库。它包含了所有FFmpeg支持的编解码器。
每种编码器都处理一种特定的媒体类型。
除了基本的编解码功能外，该库还提供了包括解复用、封装、解码、编码等一系列处理器。

2. libavformat： 
这是一个多媒体容器解复用库。
它提供了DTS支持、流选择、容器解复用、容器封装和元数据访问功能。
在处理媒体文件时，这个库会解析文件头信息，获取流的信息。
它赋予FFmpeg处理各种音视频容器格式的能力。
-- PS：什么事头文件信息
在多媒体处理中，提到的“头文件信息”通常指的是媒体文件的元数据（Metadata）。
元数据包括了和音频、视频流相关的重要信息，例如编码格式, 比特率，帧率，持续时间，分辨率等等。
这些信息都会在解码和播放媒体文件的过程中被用到。
当我们使用类似libavformat这样的库来解析媒体文件时，
首先它会读取和解析这些“头文件信息”来了解如何进行后续的解码工作。

3. libswscale： 
这个库主要是用于处理图片格式转换和拉伸等功能。
在实际场景中很常用，比如视频画面的缩放，格式转换等。

4. libavutil： 
是FFmpeg开发框架的基础库。
它包括一些公共的工具函数，
如：数学处理、字符串操作、内存管理、媒体数据结构等实用工具，方便开发者调用。


5. libavfilter：
这个库提供了强大的对音频和视频进行过滤处理的功能。
它支持各种过滤操作，例如裁剪、缩放、旋转、颜色转换，甚至一些复杂的操作，
例如混合脚本、镜头校正等。


6. libavresample：
这个库用来进行音频重采样、重混合和采样格式转换。
然而它已经被FFmpeg社区标记为过时，并建议使用libswresample代替。

7. libswresample：
与libavresample相比，这个库有更多的功能并且更加稳定。
它用于混合、处理和转换音频数据。


8. libpostproc：
这是一个视频后处理库，提供基于内容分析的过滤器，如去方块噪声、去隔行扫描等。

9. libavdevice: 
这个库提供了对设备相关的编解码器和封装/解封装器的访问，
它可以用于与多种设备进行交互，例如数字电视和摄像机等。

*/
#include <iostream>
using namespace std; 
extern "C" {
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
}

static void logging(const char *fmt, ...);
static void logging(const char *fmt, ...){
    va_list args; 
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args ); 
    fprintf( stderr, "\n" ); 
}
int main(){
    /*
    公式：
    AVFormatContext 是多媒体文件格式的抽象（例如：MKV，MP4，Webm，TS）。 
    AVStream 代表给定格式的数据类型（例如：音频，视频，字幕，元数据）。
    AVPacket 是从 AVStream 获得的压缩数据的切片，
    可由 AVCodec（例如av1，h264，vp9，hevc）解码，
    从而生成称为 AVFrame 的原始数据。

    //这是一个基础结构
    while(av_read_frame(pFormatCtx, pPacket) >= 0)
    {
        if(pPacket->stream_index == audioStream)
        {
            avcodec_send_packet(pAudioCodecContext, pPacket);
            avcodec_receive_frame(pAudioCodecContext, pFrame);


            // 创建一个新的帧，用于存储编码后的数据
            AVFrame* pOutFrame = av_frame_alloc();
            pOutFrame->nb_samples     = pFrame->nb_samples;
            pOutFrame->format         = pFrame->format;
            pOutFrame->ch_layout = pFrame->ch_layout;

            AVPacket* pOutPacket = av_packet_alloc();

            // 将解码后的帧复制到新帧中
            av_frame_copy(pOutFrame, pFrame);
            // 使用编码器将新帧编码为MP3格式
            avcodec_send_frame(pOutAudioCodecContext, pOutFrame);
            avcodec_receive_packet(pOutAudioCodecContext, pOutPacket);


            // 将编码后的数据写入文件
            fwrite(pOutPacket->data, 1, pOutPacket->size, audioFile);
            av_frame_unref(pOutFrame);
            av_packet_unref(pPacket);
        }
        av_packet_unref(pPacket);
    }


    */
    logging("注释里主要是一些基础概念");

    /**
    音视频分析
    1）首先，你需要在命令行中定位到你的音频文件所在的位置。

    2）然后，键入下面的命令，这将输出你的音频文件的基础信息：
    ffprobe -i yourfile.mp3  //PS：probe是调查打探的意思

    3）仔细阅读输出的信息。你会看到一些关于音频流的基本信息，包括编码格式，采样率，通道数量等等。

    */



    return 0;
}