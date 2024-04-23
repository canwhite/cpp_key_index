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

    //
    if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) != 0) {
        printf("Cannot open input file\n");
        return -1;
    }

    int video_stream_index = -1;

    for(int i = 0 ; i < pFormatCtx->nb_streams; i++){
        video_stream_index = i;
        break;
    }

    if (video_stream_index == -1) {
        printf("Cannot find video stream\n");
        return -1;
    }

    AVCodecParameters *pCodecPar =  pFormatCtx->streams[video_stream_index]->codecpar;
    const AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (pCodec == NULL) {
        printf("Cannot find codec\n");
        return -1;
    }
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        printf("Cannot allocate memory for codec context\n");
        return -1;
    }
    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0) {
        printf("Failed to copy codec parameters to codec context\n");
        return -1;
    }



    return 0;
}
