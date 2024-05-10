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
    //
}
void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs){
    //
}
