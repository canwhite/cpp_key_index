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
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
/** 
1）为什么要视频解码？
视频解码是因为视频通常以压缩格式存储，以减少存储需求并方便网络传输。
压缩过程会将冗余数据（例如场景中不变的背景部分）去除，只保存与前一帧之间的差异。
这种压缩方式极大地减少了视频文件的大小，但也导致不能直接播放，
必须经过解码才能恢复为连续的画面进行观看。


2）解码得到的结果有什么用？
解码得到的结果是一帧帧的图像数据，这些数据可以被渲染设备
（如你的电脑或手机屏幕）直接使用，显示出连续的画面给用户观看。
此外，如果你需要对视频进行进一步的处理，如添加特效、进行剪辑等，
也需要首先将视频解码得到原始的图像数据
*/

/** 
什么是YUV
YUV是一种颜色编码方法，广泛应用于视频系统和图像处理领域。
在YUV编码中，“Y”表示亮度（也称为灰度或亮度），
而“U”和“V”则表示色度或色彩信息。
这种编码方式的一个主要优点是它可以有效地减少颜色数据的冗余，
从而实现图像和视频的高效压缩。
Y：代表的是亮度，取自英文word luminance的首字母，表示的是图像的明暗程度，只有Y分量也就成了黑白图像。
U：色彩宽度，它代表颜色从蓝色至红色部分的信息。
V：色彩高度，它表示的是颜色从绿色至紫色的部分的信息。
即使我们丢失色度信息(U和V)，我们根据亮度信息Y也能获取整体图像的轮廓，
所以一些只需要黑白图像的情况可以仅使用Y通道。
在许多视频编码方案中，也常采取更多的比特来编码Y分量，较少的比特来编码U和V分量，
从而平衡视觉效果和编码效率。
*/ 


int main(){

    //设置视频文件路径
    const char* videoPath = "/Users/zack/Desktop/test.mp4";

    // 分配一个AVFormatContext结构体
    // avformat_alloc_context()这个函数调用的目的就是在内存中创建一个这样的结构体，
    // 用于存储媒体文件的各种信息。
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    
    //avformat_open_input打开视频文件的函数，如果没问题就返回0
    if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) != 0){
         return -1;
    }

    // 搜索流信息,读取源码信息
    // 这个函数的返回值是错误码。如果找到流的信息，函数会返回非负的值，否则会返回负值。
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        return -1;
    }


    // 查找视频流索引
    int videoStream = -1;
    //pFormatCtx->nb_streams，是视频流的数量
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        //pFormatCtx->streams[i] 是指向第i个视频流的指针
        //->codecpar->codec_type 是该媒体流的编码类型
        //== AVMEDIA_TYPE_VIDEO 说明这个流是一个视频流
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    // 没有找到视频流
    if (videoStream == -1){
        return -1;
    }


    //从视频流中获取指定视频流（videoStream）的编解码器参数，
    AVCodecParameters *pCodecPar = pFormatCtx->streams[videoStream]->codecpar;

    //根据编解码器参数中的编解码器ID（codec_id）
    //在FFmpeg的编解码器列表中查找对应的解码器
    //pCodecPar->codec_id 
    //这部分的意思是获取 pCodecPar（一个指向 AVCodecParameters 的指针）所指向的结构的 codec_id 成员。
    //codec_id 一般是由一种称为解复用器（Demuxer）的软件分析来自于压缩音视频文件的数据流，
    //然后记录具体是何种编码算法进行压缩的。
    const AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);


    //判断解码器是否成功找到
    if (pCodec == NULL) {
        printf("Cannot find codec\n");
        return -1;
    }

    // 分配一个解码器上下文
    //avcodec_alloc_context3(pCodec) 是一个函数调用，
    //其功能是为给定的编解码器 pCodec 分配一个新的 AVCodecContext 并返回其指针。
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);


    //这个函数的作用是将 pCodecPar中的参数复制到 pCodecCtx（一个AVCodecContext 结构体的指针）中。
    //这个函数会返回一个整数值，大于或等于 0 表示复制成功，小于 0 则表示复制失败。
    /* 
    PS: 为什么要复制进去呢？
    在音视频编码过程中, AVCodecParameters是用来存储编码信息的结构体，
    比如视频的宽、高、帧率、像素格式等实际编解码需要的参数；
    而AVCodecContext是编解码器上下文，它包含一些更详细和更深层次的编码或解码信息。
    将AVCodecParameters中的参数复制至AVCodecContext中，其实是为了获取并使用这些编码参数。
    在实际的编解码操作中，AVCodecContext会被频繁访问和使用。
    调用avcodec_parameters_to_context函数可以使程序更高效地获取到必要的编码参数，
    比如视频帧的大小，编码类型等等，使得AVCodecContext对象在编解码过程中能够正确的获取和使用这些参数。
    */

    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0) {
        printf("Failed to copy codec parameters to codec context\n");
        return -1;
    }

    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Failed to open codec\n");
        return -1;
    }

    // 分配一个AVFrame结构体, 来自libavcodec
    // AVFrame *pFrame = av_frame_alloc();这行代码的作用是分配一个AVFrame类型的结构体。
    // AVFrame是用来存储解码后的原始数据（即未压缩数据，音频或视频帧）的结构体。
    // av_frame_alloc函数用于为AVFrame分配内存。    
    
    AVFrame *pFrame = av_frame_alloc();

    //AVPacket packet;
    //这行代码则是定义了一个AVPacket类型的变量packet。
    //AVPacket结构体用于存储压缩编码的数据（即编码后的数据，音频或视频帧）。
    //也就是说，当从文件或者网络上读取数据时，获取的数据会以AVPacket形式存在。
    //在执行解码操作后，该结构体中的数据将被转存到AVFrame结构体中。
    AVPacket packet;

    //--总结：
    //这两者共同完成了音视频数据的解码工作，
    //先通过AVPacket读取压缩数据，
    //再解码生成AVFrame存储解码后的原始数据

    //sws_getContext函数的主要功能是得到一个用于图像缩放和像素格式转换的上下文（struct SwsContext）。
    struct SwsContext *swsContext = sws_getContext(
        pCodecCtx->width,  // 源图像的宽度
        pCodecCtx->height, // 源图像的高度
        pCodecCtx->pix_fmt,// 源图像的像素格式
        pCodecCtx->width,  // 目标图像的宽度
        pCodecCtx->height, // 目标图像的高度
        AV_PIX_FMT_YUV420P,// 目标图像的像素格式，这里设定为 AV_PIX_FMT_YUV420P，即YUV420P格式。
        SWS_BICUBIC,       // 缩放算法， 使用的是双立方插值算法进行图像缩放。FFmpeg提供了多种插值算法，比如 SWS_BICUBIC, SWS_BILINEAR, SWS_FAST_BILINEAR,等等。
        //这三个NULL参数，分别对应param[2]，param[3]，param[4]，通常不需要设置，故设为NULL。
        NULL, 
        NULL, 
        NULL
    );
    
    // 打开输出文件
    // 使用 fopen 函数尝试打开一个叫做 "output.yuv" 的文件，
    // 以二进制写模式("wb")，w代表写，b代表二进制
    // 注意这个函数的返回值是一个 FILE 指针
    // 如果这个文件不存在，会新建这个文件
    FILE *file = fopen("output.yuv", "wb");
    if (!file) {
        printf("Failed to open output file\n");
        return -1;
    }

    // 读取视频帧并转换为YUV
    //av_read_frame(pFormatCtx, &packet) 是 ffmpeg 库的函数，用于从媒体文件中读取下一帧。其中：
    //pFormatCtx: 封装格式上下文，通常会包含媒体文件的信息（如流数、帧数等）。
    //&packet: 用于存储读取的帧数据的结构。
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        //TODO 
        if (packet.stream_index == videoStream) {
            avcodec_send_packet(pCodecCtx, &packet);
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {

                // 分配YUV帧
                AVFrame *pFrameYUV = av_frame_alloc();
                int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
                uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
                av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

                // 将视频帧数据转换为YUV格式
                sws_scale(swsContext, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);


                // 写入YUV数据到文件
                for (int i = 0; i < pCodecCtx->height; i++) {
                    fwrite(pFrameYUV->data[0] + i * pFrameYUV->linesize[0], 1, pCodecCtx->width, file);
                }
                for (int i = 0; i < pCodecCtx->height / 2; i++) {
                    fwrite(pFrameYUV->data[1] + i * pFrameYUV->linesize[1], 1, pCodecCtx->width / 2, file);
                }
                for (int i = 0; i < pCodecCtx->height / 2; i++) {
                    fwrite(pFrameYUV->data[2] + i * pFrameYUV->linesize[2], 1, pCodecCtx->width / 2, file);
                }

                // 释放YUV帧资源
                av_frame_free(&pFrameYUV);
            }
        }
        av_packet_unref(&packet);
    }

    // 关闭文件和释放资源
    fclose(file);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
    avcodec_free_context(&pCodecCtx);
    sws_freeContext(swsContext);
    av_frame_free(&pFrame);


    return 0;
}
