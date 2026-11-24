#include "FFmpegEncoder.h"
#include <iostream>


/// <summary>
/// 创建输出上下文和编码器
/// </summary>
/// <returns></returns>
bool FFmpegEncoder::open(int width,int height, const char* filePath) {

	// 1 创建输出格式上下文
	int result = avformat_alloc_output_context2(&f_ctx_, nullptr, nullptr, filePath);
	if (result < 0) {
		ErrorUtil::printError(result);
		return false;
	}

	// 1.1 打开输出文件 需要自己手动打开 AVFMT_NOFILE：本地文件类型，如果是rtsp等流媒体，不需要avio_open
	if (!(f_ctx_->oformat->flags & AVFMT_NOFILE)) {

		result = avio_open(&f_ctx_->pb, filePath, AVIO_FLAG_WRITE);
		if (result < 0) {
			ErrorUtil::printError(result);
			avformat_free_context(f_ctx_);
			
			return false;
		}
	}

	// 2 编码器层
	// 2.1 创建编码器
	const AVCodec* v_codec = avcodec_find_encoder(AV_CODEC_ID_H265);
	vc_ctx_ = avcodec_alloc_context3(v_codec);
	if (!vc_ctx_) {
	
		std::cerr << "AV_CODEC_ID_H265 context created error" << std::endl;
		avio_closep(&f_ctx_->pb);
		avformat_free_context(f_ctx_);
		return false;
	}

	vc_ctx_->width = width;
	vc_ctx_->height = height;
	vc_ctx_->time_base = { 1,30 };
	vc_ctx_->pix_fmt = AVPixelFormat::AV_PIX_FMT_YUV420P;
	// H26X文件的pps和sps vps数据
	if (f_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
		f_ctx_->flags |= AVFMT_GLOBALHEADER;
	}

	// 2.2 开启编码器
	result = avcodec_open2(vc_ctx_, v_codec, nullptr);
	if (result < 0) {
		avcodec_free_context(&vc_ctx_);
		avio_closep(&f_ctx_->pb);
		avformat_free_context(f_ctx_);
		ErrorUtil::printError(result);
		return false;
	}

	//1.2 创建媒体流,他由formatctx管理,但为了方便使用，提升为成员变量。Stream的参数和编码器耦合
	AVStream* v_stream_ = avformat_new_stream(f_ctx_, v_codec);
	if (!v_stream_) {

		std::cerr << "创建媒体流失败" << std::endl;
		avcodec_free_context(&vc_ctx_);
		avio_closep(&f_ctx_->pb);
		avformat_free_context(f_ctx_);
		return false;
	}
	v_stream_->time_base = vc_ctx_->time_base;
	result = avcodec_parameters_from_context(v_stream_->codecpar, vc_ctx_);
	if (result < 0) {
		ErrorUtil::printError(result);
		avcodec_free_context(&vc_ctx_);
		avio_closep(&f_ctx_->pb);
		avformat_free_context(f_ctx_);
		return false;
	}

	// 1.3 写入文件头， 头-中间-尾
	result = avformat_write_header(f_ctx_, nullptr);
	if (result < 0) {
		ErrorUtil::printError(result);
		avcodec_free_context(&vc_ctx_);
		avio_closep(&f_ctx_->pb);
		avformat_free_context(f_ctx_);

		return false;
	}
	return true;
}

/// <summary>
/// 输入每一帧进行编码
/// </summary>
/// <returns></returns>
bool FFmpegEncoder::sendFrame(AVFrame* frame) {

	if (!f_ctx_ || !vc_ctx_ || !v_stream_) {
		return false;
	}

	// 2.3 发送到编码器
	int result = avcodec_send_frame(vc_ctx_, frame);
	if (result < 0) {
		ErrorUtil::printError(result);
		return false;
	}

	// 2.4 读取编码结果并写入
	AVPacket* packet = nullptr;
	while (avcodec_receive_packet(vc_ctx_,packet) == 0)
	{
		// 1.4 写入中间数据
		// 缩放时间基数
		av_packet_rescale_ts(packet, vc_ctx_->time_base, v_stream_->time_base);

		// 指定轨道
		packet->stream_index = v_stream_->index;

		result = av_interleaved_write_frame(f_ctx_, packet);
		if (result < 0) {
			ErrorUtil::printError(result);
		}

		av_packet_unref(packet);

	}

	av_packet_free(&packet);
	return true;
}

/// <summary>
/// 清空缓冲区
/// </summary>
/// <returns></returns>
bool FFmpegEncoder::flush() {

	avcodec_send_frame(vc_ctx_,nullptr);
	AVPacket* packet = nullptr;
	while (avcodec_receive_packet(vc_ctx_,packet) == 0)
	{
		av_packet_rescale_ts(packet,vc_ctx_->time_base,v_stream_->time_base);

		packet->stream_index = v_stream_->index;

		av_interleaved_write_frame(f_ctx_,packet);

	} 
	int result = av_write_trailer(f_ctx_);
	if (result < 0) {
		ErrorUtil::printError(result);
		return false;
	}
	return true;
}

/// <summary>
/// 停止并释放资源
/// </summary>
/// <returns></returns>
bool FFmpegEncoder::stop() {
	if (f_ctx_)
	{
		if (!(f_ctx_->oformat->flags & AVFMT_NOFILE))
		{
			avio_closep(&f_ctx_->pb);
		}
		avformat_free_context(f_ctx_);
		f_ctx_ = nullptr;
	}
	if (vc_ctx_)
	{
		avcodec_free_context(&vc_ctx_); // 指针的指针，直接将指针置空
	}
	v_stream_ = nullptr;
	return true;
}