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
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    
    if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) != 0)
        return -1;

    // 搜索流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1;

    // 查找视频流索引
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    // 没有找到视频流
    if (videoStream == -1)
        return -1;


    //从视频流中获取指定视频流（videoStream）的编解码器参数，
    AVCodecParameters *pCodecPar = pFormatCtx->streams[videoStream]->codecpar;

    //根据编解码器参数中的编解码器ID（codec_id）在FFmpeg的编解码器列表中查找对应的解码器
    const AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);


    //判断解码器是否成功找到
    if (pCodec == NULL) {
        printf("Cannot find codec\n");
        return -1;
    }

    // 分配一个解码器上下文
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);

    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0) {
        printf("Failed to copy codec parameters to codec context\n");
        return -1;
    }

    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Failed to open codec\n");
        return -1;
    }

    // 分配一个AVFrame结构体
    AVFrame *pFrame = av_frame_alloc();
    AVPacket packet;

    // 初始化像素格式转换上下文
    struct SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                                   pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                                   SWS_BICUBIC, NULL, NULL, NULL);
    // 打开输出文件
    FILE *file = fopen("output.yuv", "wb");
    if (!file) {
        printf("Failed to open output file\n");
        return -1;
    }

    // 读取视频帧并转换为YUV
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
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
