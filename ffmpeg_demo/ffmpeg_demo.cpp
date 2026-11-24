

#include <iostream>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include "ErrorUtil.h"
#include "FFmpegEncoder.h"
}
#pragma comment(lib, "avutil.lib")    // 工具类 日志、错误打印
#pragma comment(lib, "avformat.lib") // 格式化库
#pragma comment(lib, "avcodec.lib") // 编解码库



int main()
{

    // 1 视频的封装层，解析视频的参数
    AVFormatContext* f_ctx = nullptr;
    const char* inputPath = "demo.mp4";
    // 打开格式化文件
    int result = avformat_open_input(&f_ctx, "demo.mp4", nullptr, nullptr);
    if (result < 0) {
    
        std::cerr << "无法打开文件: " << std::endl;
        ErrorUtil::printError(result);
        return -1;
    }

    // 1.1 获取封装的视频流，解析视频中流的参数
    result = avformat_find_stream_info(f_ctx, nullptr);
    if (result < 0) {
    
        std::cerr << "获取流信息失败:" << std::endl;
        ErrorUtil::printError(result);
        avformat_close_input(&f_ctx);
        return -1;
    }

    // 1.2 获取视频流id
    int videoId = -1;
    for (int i = 0; i < f_ctx->nb_streams; i++) {

        if (f_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        
            videoId = i;
            break;
        }

    }

    if (videoId == -1) {
        std::cerr << "没有视频流" << std::endl;
        avformat_close_input(&f_ctx);
        return -1;
    }

    // 2 编解码层
    AVStream * v_stream = f_ctx->streams[videoId];
    
    // 2.1 查找指定类型解码器,使用当前视频流的对应参数 例如h264/h265的解码器
    const AVCodec* v_decoder = nullptr;
    AVCodecID codec_id = v_stream->codecpar->codec_id;
    v_decoder = avcodec_find_decoder(codec_id);
    if (!v_decoder) {
        std::cerr << "not found codec" << std::endl;
        avformat_close_input(&f_ctx);
        return -1;
    }

    // 2.2 创建解码器上下文
    AVCodecContext* vd_ctx;
    vd_ctx = avcodec_alloc_context3(v_decoder);
    // 配置解码器
    avcodec_parameters_to_context(vd_ctx, v_stream->codecpar);

    // 2.3 打开解码器
    result = avcodec_open2(vd_ctx, v_decoder, nullptr);
    if (result < 0) {
        ErrorUtil::printError(result);
        avcodec_free_context(&vd_ctx);
        avformat_close_input(&f_ctx);
        return -1;
    }

    // 2.4 解码器解码
    // 2.4.1 读取packet
    AVPacket* packet = av_packet_alloc(); // 解码输入
    AVFrame* frame = av_frame_alloc(); // 解码输出
    while (av_read_frame(f_ctx, packet) >= 0 ) {
        // 过滤音频包
        if (packet->stream_index == videoId) {
            // 2.4.2 编码器输入
           result = avcodec_send_packet(vd_ctx, packet);
           if (result < 0) {
               ErrorUtil::printError(result);
               // data引用计数减一，但是不销毁packet对象本身
               av_packet_unref(packet);
               continue;
           }
           // 调用时会自动unref一次frame
           while (avcodec_receive_frame(vd_ctx,frame) == 0) {
               // 3 视频帧处理
               std::cout << frame->pts << ":" << frame->width << "+" << frame->height << std::endl;
   
           }
        }
        av_packet_unref(packet);
    }

    // 2.5 输出解码器所有缓冲数据
    std::cout << "flush" << std::endl;
    avcodec_send_packet(vd_ctx,nullptr); // 输入空包表示输出所有
    while (avcodec_receive_frame(vd_ctx,frame) == 0) {
    
        // 3 视频帧处理
        std::cout << frame->pts << std::endl;
    
    }

    // 4 释放资源 释放对象本身，而不是缓冲区
    av_frame_free(&frame); // 释放frame对象
    av_packet_free(&packet);
    avcodec_free_context(&vd_ctx);
    avformat_close_input(&f_ctx);// 释放上下文，同时释放iocontext

    FFmpegEncoder encoder;
    result = encoder.open(854,480,"out.mp4");
    std::cout << "结果：" << (int)result << std::endl;

    std::cout << "当前环境 FFmpeg 版本: " << av_version_info() << std::endl;
    return 0;
}


