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
1）为什么要编码？

编码是一个在数字媒体工作流中至关重要的步骤，它的目的是将原始的视频（或音频）数据转换为特定格式的文件，这个文件可以在各种设备上播放，例如电视、网络、移动设备等。
FFmpeg进行编码的原因有以下几个：
文件大小：未编码的视频文件非常大，很难进行存储和传输。编码通过压缩算法将文件大小降低，使其更易于存储和传输。
兼容性：不同的设备和应用支持不同的文件格式。通过编码，你可以将视频转换为目标设备或应用支持的格式。
优化质量和带宽：编码可以在保持视觉质量的同时减少带宽需求，对于网络传输来说特别重要。

2）编码得到的结果有什么用？
编码的结果通常是一定格式的视频、音频文件或流，可以用于以下的场景：
保存和存档：编码后的文件大小更小，更适合保存和存档。
在线播放：编码后的文件更易于被流媒体服务器接受和分发，用户可以直接在线播放。
文件共享：编码后的文件可以更方便地通过互联网、移动网络等进行共享。
因此，无论是在创建、编辑、分享或播放媒体内容时，编码都扮演了重要的角色。
 */ 

int main (){
    //TODO.如何将yuv格式的视频转化为avi

    return 0;
}
