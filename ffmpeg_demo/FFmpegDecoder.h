#pragma once

#include <functional>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

}
class FFmpegDecoder
{

public:
	/// <summary>
	/// 初始化
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	bool open(const char* filePath);

	/// <summary>
	/// 直接解码视频，输出frame
	/// </summary>
	/// <returns></returns>
	bool transfer(std::function<void(AVFrame* frame)> callback);

	/// <summary>
	/// 清空解码器缓冲区数据
	/// </summary>
	/// <returns></returns>
	bool flush(std::function<void(AVFrame* frame)> callback);

	/// <summary>
	/// 结束，清理上下文对象
	/// </summary>
	/// <returns></returns>
	bool stop();

private:

	AVFormatContext* m_f_ctx_ = nullptr;
	AVStream* m_v_stream_ = nullptr;
	AVCodecContext* m_vc_ctx_ = nullptr;


};

