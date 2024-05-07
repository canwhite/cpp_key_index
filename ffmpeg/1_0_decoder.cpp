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
        }else if(response < 0){
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
            //其实这行是给frame_filename按照固定格式赋值
            snprintf(frame_filename, sizeof(frame_filename), "%s-%lld.pgm", "frame", pCodecContext->frame_num);
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

    //我们首先为 AVFormatContext 分配内存，利用它可以获得相关格式（容器）的信息。
    //AVFormatContext保存来自格式(容器)的头信息。为该组件分配内存
    // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (!pFormatContext) {
        logging("ERROR could not allocate memory for Format Context");
        return -1;
    }
    // 打开文件
    const char* videoPath = "/Users/zack/Desktop/test.mp4";
    if (avformat_open_input(&pFormatContext, videoPath, NULL, NULL) != 0) {
        logging("ERROR could not open the file");
        return -1;
    }

    // now we have access to some information about our file
    // since we read its header we can say what format (container) it's
    // and some other information related to the format itself.
    logging("format %s, duration %lld us, bit_rate %lld", pFormatContext->iformat->name, pFormatContext->duration, pFormatContext->bit_rate);
    
    logging("finding stream info from format");

    //为了访问数据流，我们需要从媒体文件中读取数据。
    //需要利用函数 avformat_find_stream_info完成此步骤。
    //pFormatContext->nb_streams 将获取所有的流信息，并且通过 pFormatContext->streams[i] 获取到指定的 i 数据流
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if (avformat_find_stream_info(pFormatContext,  NULL) < 0) {
        logging("ERROR could not get the stream info");
        return -1;
    }

    // the component that knows how to encode and decode the stream
    // it's the codec (audio or video)
    // http://ffmpeg.org/doxygen/trunk/structAVCodec.html
    const AVCodec * pCodec = NULL;

    //针对每个流维护一个对应的 AVCodecParameters，该结构体描述了被编码流的各种属性。
    AVCodecParameters *pCodecParameters =  NULL;
    int video_stream_index = -1;

    //遍历数据流，输出其中的重要信息
    //注意这里能获取到nb_streams，是因为我们之前调用了方法avformat_find_stream_info
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        //针对每个流维护一个对应的 AVCodecParameters，该结构体描述了被编码流的各种属性
        AVCodecParameters *pLocalCodecParameters =  NULL;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

        logging("AVStream->time_base before open coded %d/%d", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
        logging("AVStream->r_frame_rate before open coded %d/%d", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
        logging("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
        logging("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

        logging("finding the proper decoder (CODEC)");
        const AVCodec *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec==NULL) {
            logging("ERROR unsupported codec!");
            // In this example if the codec is not found we just skip it
            continue; //continue，跳过这一帧
        }
        
        //对于视频，我们在找到第一个视频流（video_stream_index == -1，即我们还没找到视频流）的时候就保存索引和codec相关的信息。
        //这就意味着我们只关心第一个找到的视频流。对于音频，我们只是打印出相关的音频信息，并没有保存索引和codec信息
        if(pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
            //如果是视频,就给下述几个值赋值
            if(video_stream_index == -1){
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }

        }else{
            //如果是音频
            logging("Audio Codec: %d channels, sample rate %d", 
            // pLocalCodecParameters->channels, //好像没有这个参数，后续todo
            0, 
            pLocalCodecParameters->sample_rate);

        }

        // print its name, id and bitrate
        logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
    }

    if (video_stream_index == -1) {
        logging("File %s does not contain a video stream!", *videoPath);
        return -1;
    }

    //利用刚刚获取的 AVCodec 为 AVCodecContext 分配内存，它将维护解码/编码过程的上下文。 
    //然后需要使用 avcodec_parameters_to_context和被编码流的参数(AVCodecParameters) 来填充 AVCodecContext。
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext)
    {
        logging("failed to allocated memory for AVCodecContext");
        return -1;
    }

    //给codec context填充参数
    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
    {
        logging("failed to copy codec params to codec context");
        return -1;
    }

    //使用avcodec_open2打开编码器
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
    {
        logging("failed to open codec through avcodec_open2");
        return -1;
    }

    //初始化packet和frame
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        logging("failed to allocate memory for AVPacket");
        return -1;
    }
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
    {
        logging("failed to allocate memory for AVFrame");
        return -1;
    }

    int response = 0;
    int how_many_packets_to_process = 5;

    //使用函数 av_read_frame 读取帧数据来填充数据包。
    //是从输入中读，未解压的packet
    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        // if it's the video stream
        if (pPacket->stream_index == video_stream_index) {
            logging("AVPacket->pts %" PRId64, pPacket->pts);
            response = decode_packet(pPacket, pCodecContext, pFrame);
            if (response < 0)
                break;
            // stop it, otherwise we'll be saving hundreds of frames
            if (--how_many_packets_to_process <= 0) break;
        }
        // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
        av_packet_unref(pPacket);
    }

    
    logging("releasing all the resources");

    avformat_close_input(&pFormatContext);//释放
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);
    return 0;
}