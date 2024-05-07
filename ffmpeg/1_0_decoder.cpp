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
#include <stdio.h>
//#include <stdarg.h>: 这个头文件主要用于处理变长参数列表。在你的代码中，函数定义中的fmt, ...（即等号后面的省略号）
//就意味着函数的参数数目是可变的。
//stdarg.h头文件中定义的宏和类型（比如va_list, va_start, va_end和vfprintf函数）用于操作这些可变的参数。
#include <stdarg.h>
//#include <stdlib.h>: 这个头文件是标准库头文件，含有C语言的基本建筑块，
//比如由于种种原因终止程序的函数exit、执行动态内存管理的函数malloc和free等。
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
}
// print out the steps and errors
// ...是可变参数的意思，
// 声明部分
static void logging(const char *fmt, ...);
// implement
static void logging(const char *fmt, ...){
    //首先声明了一个名为args的变量，类型为va_list。这是处理变长参数列表时使用的一种类型。
    va_list args; 
    //这行代码向标准错误(stderr)输出"LOG: "。
    fprintf( stderr, "LOG: " );
    //这行代码的作用是获取变长参数列表。函数参数fmt是一个指向字符常量的指针，它指定了传递给函数的变长参数的数量和类型。
    va_start( args, fmt );
    //这行代码将可变参数列表args按照fmt指定的格式输出到标准错误(stderr)。
    vfprintf( stderr, fmt, args );
    //这行代码的作用是结束对变长参数列表的访问，释放资源。
    va_end( args ); 
    //这行代码在错误信息后打印一个换行符，使得各个日志之间保持清晰和整齐。
    fprintf( stderr, "\n" ); 
}

// save a frame into a .pgm file
static void save_gray_frame(unsigned char *buf, int unit_size, int xsize, int ysize, char *filename);
// implement
static void save_gray_frame(unsigned char *buf, int unit_size, int xsize, int ysize, char *filename){
    FILE * f;
    int i;
    f = fopen(filename,"w");
    // writing the minimal required header for a pgm file format
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // writing line by line
    for (i = 0; i < ysize; i++)
        /** 
        size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
        ptr：这是指向要写入字节的指针。
        size：要写入的每个元素的字节大小。
        count：要写入 size 字节的元素的数量。
        stream：这是一个指向 FILE 对象的指针，这个对象指定了一个输出流。
        */
        fwrite(buf + i * unit_size, 1, xsize, f);

    fclose(f);

}


// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
// implement, core function
// 主要目的： 将原始数据包，通过解码器，解码为数据帧
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame){
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
    // 来把原始数据包（未解压的帧）发送给解码器
    int response = avcodec_send_packet(pCodecContext, pPacket);
    if(response < 0){
        //todo
        logging("Error while sending a packet to the decoder: %s", av_err2str(response));
        return response;
    }

    while (response >= 0 )
    {
        // Return decoded output data (into a frame) from a decoder
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
        // 从解码器接受原始数据帧（解压后的帧），上边是send，这里是receive
        response = avcodec_receive_frame(pCodecContext, pFrame);

        //如果报错
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break; //跳出while循环
        }else{
            logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
            return response;
        }

        //如果没有报错
        //可以输出 frame 编号、PTS、DTS、frame 类型等其他信息。
        if(response >= 0 ){
            // printf(
            //     "Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d]",
            //     av_get_picture_type_char(pFrame->pict_type),
            //     pCodecContext->frame_number,
            //     pFrame->pts,
            //     pFrame->pkt_dts,
            //     pFrame->key_frame,
            //     pFrame->coded_picture_number,
            //     pFrame->display_picture_number
            // );
            char frame_filename[1024];
            //snprintf是一个用于格式化字符串并将其写入字符数组的函数，它来源于C标准库中的stdio.h。
            //*str 是指向将被写入的字符数组的指针。
            //size 是你想要写入的字符数组的最大长度，包括结尾的空字符（'\0'）。
            //*format 是一个格式字符串，它定义了接下来的可变参数如何转换成字符串。
            //最后一个 ... 是可变参数，它们将按照 *format 字符串所指定的格式被格式化并写入 *str。
            // snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
            // Check if the frame is a planar YUV 4:2:0, 12bpp
            // That is the format of the provided .mp4 file
            // RGB formats will definitely not give a gray image
            // Other YUV image may do so, but untested, so give a warning
            if (pFrame->format != AV_PIX_FMT_YUV420P)
            {
                logging("Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB");
            }
            // save a grayscale frame into a .pgm file
            // 因为我们只需要灰色，所以取data[0],
            // linesize[0]通常表示的就是Y平面(亮度分量)的行大小
            // 接着是frame的宽和高，以及要写入的文件名
            save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
        }

    }
    return 0;
}



int main(){

    logging("initializing all the containers, codecs and protocols.");


    return 0;
}