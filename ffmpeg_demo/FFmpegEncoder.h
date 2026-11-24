#pragma once
extern "C" {

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "ErrorUtil.h"

}
class FFmpegEncoder
{

public:
	/// <summary>
	/// 创建输出上下文和编码器
	/// </summary>
	/// <returns></returns>
	bool open(int width, int height, const char* filePath);

	/// <summary>
	/// 输入每一帧进行编码
	/// </summary>
	/// <returns></returns>
	bool sendFrame(AVFrame* frame);

	/// <summary>
	/// 清空缓冲区
	/// </summary>
	/// <returns></returns>
	bool flush();

	/// <summary>
	/// 停止并释放资源
	/// </summary>
	/// <returns></returns>
	bool stop();

private:
	AVFormatContext* f_ctx_ = nullptr;
	AVCodecContext* vc_ctx_ = nullptr;
	AVStream* v_stream_ = nullptr;

};

