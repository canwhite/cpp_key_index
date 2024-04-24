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

    //分配一个AVFormatContext。这是一个包含了流（视频，音频，字幕等）信息的结构。
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //视频文件的路径。
    auto videoPath = "/Users/zack/Desktop/test.mp4";

    // 打开输入流，并读取文件头。文件头包含了所有流的信息。
    // &pFormatCtx 是一个指向AVFormatContext的指针的指针。这是因为函数可以改变pFormatCtx的值（可以被重新分配或者释放）。
    // 这是C语言中修改指针值的通常方式。在C++中，你也可以使用引用，但是FFmpeg的API是用C编写的。
    if(avformat_open_input(&pFormatCtx, videoPath, NULL ,NULL) != 0){
        //todo
        printf("Cannot open input file\n");
        return -1;
    }

    // 视频流的索引。这个值在后面的循环中被设置。
    int video_stream_index = -1;

    //循环只选择了第一个流作为视频流，这可能不总是正确的
    //TODO,思考是不是这里获取的有问题
    for(int i = 0 ; i < pFormatCtx->nb_streams; i++){
        video_stream_index = i;
        break;
    }

    // 如果没有找到视频流，表明文件中没有视频。
    if (video_stream_index == -1) {
        printf("Cannot find video stream\n");
        return -1;
    }

    //获取解码器参数。这里streams[video_stream_index]->codecpar可能含有解码器需要的额外信息，比如帧率、时间基准等。
    AVCodecParameters *pCodecPar =  pFormatCtx->streams[video_stream_index]->codecpar;
    int pixel_format_int = pCodecPar->format;

    //获取像素格式
    AVPixelFormat pixel_format = static_cast<AVPixelFormat>(pixel_format_int);
    printf("--pix %d /n",pixel_format); //这里输出为-1，有问题


    // 找到对应的解码器。
    // codec_id 是解码器的标识符，对每种解码器来说都是唯一的。
    const AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);

    // 如果没有找到解码器。
    if (pCodec == NULL) {
        printf("Cannot find codec\n");
        return -1;
    }

    // 分配一个解码器上下文。这个上下文在解码过程中，用来保存解码器的状态。
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);

    // 如果没能分配上下文。
    if (pCodecCtx == NULL) {
        printf("Cannot allocate memory for codec context\n");
        return -1;
    }

    // 把参数从AVCodecParameters复制到AVCodecContext。这是因为解码函数需要AVCodecContext。
    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0) {
        printf("Failed to copy codec parameters to codec context\n");
        return -1;
    }


    //TODO, 如何将pCodecCtx解码器上下文的内容，转成YUV图像
    // 分配一个AVFrame结构体，用于存储未解码的图像帧
    AVFrame* pFrameYUV = av_frame_alloc();
    // 获取所提供的图像格式和大小所需的内存大小（以字节为单位）
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
    // 分配一块内存（以字节为单位）
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    // 根据提供的参数设置data和linesize数组
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

    printf("---width%d \n",pCodecCtx->width);
    printf("---height%d \n",pCodecCtx->width);
    printf("---scrFormat%d \n",pCodecCtx->pix_fmt); //这里像素格式输出为-1，明显有问题呀
    printf("---tWidth%d",pCodecCtx->width);
    // 原始数据
    // scrW: 原始格式宽度
    // scrH: 原始格式高度
    // scrFormat: 原始数据格式

    // 目标数据
    // dstW: 目标格式宽度
    // dstH: 目标格式高度
    // dstFormat: 目标数据格式
    //这个函数将返回一个SwsContext结构的指针，该结构包含了执行图像尺寸和像素格式转换所需要的所有信息。
    // struct SwsContext* sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    



    //TODO，再然后就是下一章编码了




    return 0;
}
