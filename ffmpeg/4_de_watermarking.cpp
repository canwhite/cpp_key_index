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

int main(){
    //TODO, 需要引入filter
    return 0;
}

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

    printf("streamIndex: %d" ,videoStream);



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
        //这句代码是用于判断读取的帧是否属于我们关心的视频通道。
        //如果你的文件只有一个流，那么可能packet.stream_index和videoStream都是0，
        //但如果有多个流，那么packet.stream_index就会有不同的值。
        //PS：那什么叫有多个流？
        /** 
         * 在媒体文件中，一个“流”可以是一个视频轨道、一个音频轨道或者一个字幕轨道等。、
         * 比如，一个常见的MP4文件可能包括一个视频流和一个音频流。
         * 如果一个文件有多个音频流，可能是因为它包含了多种语言的声音轨道。
         * 因此，“有多个流”就是指一个文件中包含了多种不同类型的数据，每一种都被称为一个“流”
         */ 
        if (packet.stream_index == videoStream) {
            //将编码过的数据包（如音频或视频数据包）送入解码器进行解码的
            avcodec_send_packet(pCodecCtx, &packet);

            //avcodec_receive_frame(pCodecCtx, pFrame)函数尝试从解码器上下文pCodecCtx中获取已解码的帧，并将其存储在pFrame中，
            //== 0表示每次成功从解码器上下文中接收帧时，这行代码会继续循环直到所有帧都被提取出来。
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                // 分配YUV帧
                //在堆上分配一个新的AVFrame，并返回指向它的指针。这个帧将用于存储转换为YUV格式后的图像数据。
                AVFrame *pFrameYUV = av_frame_alloc();
                //计算存储该YUV图像所需的字节数
                int numBytes = av_image_get_buffer_size(
                    AV_PIX_FMT_YUV420P, // 一种像素格式
                    pCodecCtx->width,   // 图像的宽度
                    pCodecCtx->height,  // 图像的高度
                    1                   // 内存地址的对齐
                );
                //PS：关于buffer
                //"buffer" 在编程中通常是指一块可以临时存储数据的内存区域。您可以将其想象成一个临时的仓库
                //这行代码的意思是它在内存中动态的分配一块区域。
                uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));


                //Pre: 我们需要理解FFmpeg处理视频数据的方法。
                //视频数据中的每一帧都可以看作是一个图像。为了进行处理，FFmpeg需要将这些图像的数据存放在特定的数组中。
                //总结：这句是将buffer关联到pFrameYUV的帧数据中，并依照YUV420P的数据格式，用已经分配好的buffer来填充pFrameYUV->data
                av_image_fill_arrays(
                    //是一个指向图像数据的指针数组，每一种平面的数据存储在不同的地址中（例如在YUV420中，Y，U，V分别存储在不同的地址）
                    pFrameYUV->data,     //av_image_fill_arrays 使用buffer为 pFrameYUV->data中的每种平面分配地址。
                    pFrameYUV->linesize, //这个数组包含了每种平面的每一行的字节数。这个函数同时也会填充这个数组。
                    buffer,              //这是分配的内存空间
                    AV_PIX_FMT_YUV420P,  //指向要使用的数据结构
                    pCodecCtx->width,    //图像的宽度
                    pCodecCtx->height,   //图像的高度
                    1                    //对齐
                );

                // 将视频帧数据转换为YUV格式
                // 它是FFmpeg库中用来进行图像尺寸缩放和像素格式转换的函数
            

                sws_scale(
                    swsContext,         //这个对象中保存了一系列的参数和函数，为图像的缩放和像素格式的转换工作提供参数和方法。
                    pFrame->data,       //是源图像帧的数据缓存区
                    pFrame->linesize,   //源图像帧的每行像素的字节大小 
                    0,                  //指定了将要进行缩放的源图像的线范围,这里的0是从0开始的意思
                    pCodecCtx->height,  //代表了图像的高度
                    pFrameYUV->data,    //是目标图像数据缓存区
                    pFrameYUV->linesize //目标图像的行宽高
                );


                // 写入YUV数据到文件
                // 对于第一组循环，对于Y
                // 这部分是将图像的Y（亮度）部分写入文件，因为在YUV420P格式中，每个像素点都有一个Y值，所以Y的总数量等于图像的宽度乘以高度。
                // 这里的操作是通过循环图像的高，然后一行行的往上添加数据，YUV分别去添加就可以
                for (int i = 0; i < pCodecCtx->height; i++) {
                    fwrite(
                        //需要写入的数据的起始数据，
                        //pFrameYUV->data[0]是Y分量的数据的起始位置
                        //加i* pFrameYUV->linesize[0]表示从第i行的数据开始写入。
                        pFrameYUV->data[0] + i * pFrameYUV->linesize[0], 
                        1,                  //这里表示每个数据单元的大小，这里边表示一个字节
                        pCodecCtx->width,   //这是数据单元的数量，也就是每次写入的字节数，这里是图像的宽度
                        file                //写到这个文件下
                    );
                }

                // 对于第二组循环，这里是U
                // YUV420格式中，每4个Y共享1对UV，所以UV的尺寸是Y的1/2。这就是为什么你看到pCodecCtx->height / 2和pCodecCtx->width / 2。
                // 所以第三个参数需要除以2，其他的和上述的一致
                for (int i = 0; i < pCodecCtx->height / 2; i++) {
                    fwrite(
                        pFrameYUV->data[1] + i * pFrameYUV->linesize[1], 
                        1, 
                        pCodecCtx->width / 2, 
                        file
                    );
                }
                // 对于第三组循环，这里是V，同上
                /** 
                U和V的主要区别是？
                U和V都是色度分量，在YUV色彩空间中，Y指的是亮度，U和V代表的是色度和浓度，这是将彩色视频信号分为亮度和色度两部分的表示法。具体来说：
                U分量，代表的是颜色从蓝色至红色的范围，而不含亮度信息。如果全部的像素U分量都设为0，那么图像则会在绿色（Green）和红色（Red）的范围变化。
                V分量，代表的是颜色从绿色至紫色的范围，而不含亮度信息。如果全部像素V分量都设为0，那么图像则会在蓝色（Blue）和黄色（Yellow）的范围变化。
                所以在处理图像时，我们可以单独修改Y,U,V分量，来达到改变图像明亮度、色度的效果。
                */


                for (int i = 0; i < pCodecCtx->height / 2; i++) {
                    fwrite(
                        pFrameYUV->data[2] + i * pFrameYUV->linesize[2], 
                        1, 
                        pCodecCtx->width / 2, 
                        file
                    );
                }

                // 释放YUV帧资源
                av_frame_free(&pFrameYUV);
            }
        }
        av_packet_unref(&packet);
    }


    fclose(file);                       //关闭你在程序中打开的文件
    avformat_close_input(&pFormatCtx);  //这个函数会关闭输入流并且释放 pFormatCtx 所使用的所有内存
    avformat_free_context(pFormatCtx);  //此语句将释放 pFormatCtx 指向的 AVFormatContext
    avcodec_free_context(&pCodecCtx);   //这里使用 FFmpeg 释放了之前创建的编解码上下文。释放后，pCodecCtx的值将被设为 NULL
    sws_freeContext(swsContext);        //此命令释放了之前创建的 LIBSWSCALE 上下文，该上下文是用于图像的缩放和格式转换。
    av_frame_free(&pFrame);             //该语句释放了帧声明的结构体和存储过的 AVFrame 数据。在编解码操作完成后，你需要释放所有的帧。


    return 0;
}
