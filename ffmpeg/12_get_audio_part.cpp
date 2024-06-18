#include <iostream>
#include "config.h"
extern "C" {
#include <libavutil/log.h>
#include <libavformat/avformat.h>
}
// aac每帧开头都要填写对应的格式信息
void adts_header(char *szAdtsHeader, int dataLen){


    int audio_object_type = 2;
    int sampling_frequency_index = 7;
    int channel_config = 2;

    int adtsLen = dataLen + 7;

    //给buf赋值，给了7个字节的空间
    szAdtsHeader[0] = 0xff;         //syncword:0xfff                          高8bits
    szAdtsHeader[1] = 0xf0;         //syncword:0xfff                          低4bits
    szAdtsHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    szAdtsHeader[1] |= (0 << 1);    //Layer:0                                 2bits
    szAdtsHeader[1] |= 1;           //protection abent:1                     1bit

    szAdtsHeader[2] = (audio_object_type - 1)<<6;            //profile:audio_object_type - 1                      2bits
    szAdtsHeader[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    szAdtsHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
    szAdtsHeader[2] |= (channel_config & 0x04)>>2;           //channel configuration:channel_config               高1bit

    szAdtsHeader[3] = (channel_config & 0x03)<<6;     //channel configuration:channel_config      低2bits
    szAdtsHeader[3] |= (0 << 5);                      //original：0                               1bit
    szAdtsHeader[3] |= (0 << 4);                      //home：0                                   1bit
    szAdtsHeader[3] |= (0 << 3);                      //copyright id bit：0                       1bit
    szAdtsHeader[3] |= (0 << 2);                      //copyright id start：0                     1bit
    szAdtsHeader[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits

    szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    szAdtsHeader[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    szAdtsHeader[6] = 0xfc;
}
/*
 * 从多媒体文件中抽取媒体信息
 * */

int main(int argc, char *argv[]) {



    AVFormatContext *fmt_ctx = NULL;

    //设置log级别
    av_log_set_level(AV_LOG_INFO);

    //输入和输出文件名称
    const char *input_filename = IN_FLIENAME;
    const char *output_filename =  "test.aac";

    // 1. 读取多媒体文件
    int ret = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "can't open file.\n");
        return -1;
    }

    //  write audio data to AAC file
    FILE *dst_fd = fopen(output_filename, "wb");
    if (dst_fd == NULL) {
        av_log(NULL, AV_LOG_ERROR, "open dst_fd failed.\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    
    //打印输入文件信息，第一个参数fmt_ctx是输入文件上下文，
    //第二个参数是流索引，
    //第三个参数是流名称，
    //第四个参数是打印信息级别,0表示打印所有信息，1表示打印错误信息，2表示打印警告信息，3表示打印调试信息。
    av_dump_format(fmt_ctx, 0, input_filename, 0);

    // 2. get stream, 这个方法实际上是对之前搜选方法的简化，返回满足条件的index
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "ret = %d\n", ret);
        avformat_close_input(&fmt_ctx);
        fclose(dst_fd);
        return -1;
    }

    int audio_index = -1;
    audio_index = ret;
    AVPacket pkt;
    /*Initialize optional fields of a packet with default values.*/
    av_init_packet(&pkt);

    int len = -1;
    /*保存原始数据，播放时需要添加AAC的音频格式说明的头*/
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == audio_index) {
            /*每帧开头都要写*/
            char adts_header_buf[7];
            //添加AAC的音频格式说明的头
            adts_header(adts_header_buf, pkt.size);
            //size_t fwrite(const void *__ptr, size_t __size, size_t __nitems, FILE *__stream)
            fwrite(adts_header_buf, 1, 7, dst_fd);
            //写入具体的内容，最后一个是输出文件,直接在pkt层面就操作了
            len = fwrite(pkt.data, 1, pkt.size, dst_fd);
            if (len != pkt.size) {
                av_log(NULL, AV_LOG_ERROR, "waning, length is not equl size of pkt.\n");
                return -1;
            }
        }
        /*Wipe the packet.*/
        av_packet_unref(&pkt);
    }


    /*Close an opened input AVFormatContext*/
    avformat_close_input(&fmt_ctx);
    if (dst_fd != NULL)
        fclose(dst_fd);

    return 0;
}

