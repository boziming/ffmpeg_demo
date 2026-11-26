

#include <iostream>

#include "ErrorUtil.h"
#include "FFmpegEncoder.h"
#include "FFmpegDecoder.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
}
#pragma comment(lib, "avutil.lib")    // 工具类 日志、错误打印
#pragma comment(lib, "avformat.lib") // 格式化库
#pragma comment(lib, "avcodec.lib") // 编解码库



int main()
{
    // 解码本地文件
    FFmpegDecoder decoder;
    decoder.open("demo.mp4");

    // 编码读取的yuv文件
    FFmpegEncoder encoder;
    encoder.open(640, 480, "out.mp4");

    decoder.transfer([&encoder](AVFrame * frame){
        encoder.sendFrame(frame);
    });

    decoder.flush([&encoder](AVFrame* frame) {
        encoder.sendFrame(frame);
    });

    encoder.flush();

    encoder.stop();


    std::cout << "当前环境 FFmpeg 版本: " << av_version_info() << std::endl;
    return 0;
}


