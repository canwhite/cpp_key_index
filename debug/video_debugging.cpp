#include "video_debugging.h"
void logging(const char *fmt,...){
    va_list args;
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    fprintf( stderr, "\n" );
}
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt){
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    logging("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}
void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs){
    //
}
