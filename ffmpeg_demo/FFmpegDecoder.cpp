#include "FFmpegDecoder.h"
#include "ErrorUtil.h"



bool FFmpegDecoder::open(const char* filePath) {

	// 1 foramt层，创建上下文，该函数会自动创建avioctx，不像输出一样需要手动open
	int result = avformat_open_input(&m_f_ctx_, filePath, nullptr, nullptr);
	if (result <0) {
		ErrorUtil::printError(result);
		return false;
	}

	// 1.1 获取所有流，并过滤视频流
	result = avformat_find_stream_info(m_f_ctx_, nullptr);
	if (result < 0) {
		ErrorUtil::printError(result);
		avformat_close_input(&m_f_ctx_);
		return false;
	}
	for (int i = 0; i < m_f_ctx_->nb_streams;i++) {
	
		if (m_f_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			m_v_stream_ = m_f_ctx_->streams[i];
			break;
		}
	}
	if (m_v_stream_ == nullptr) {
		avformat_close_input(&m_f_ctx_);
		return false;
	}




	// 2 创建解码器层 解码类型根据视频流类型创建
	const AVCodec* videoEncodec = avcodec_find_decoder(m_v_stream_->codecpar->codec_id);
	if (videoEncodec == nullptr) {
		avformat_close_input(&m_f_ctx_);
		return false;
	}

	// 2.1 创建解码器上下文
	m_vc_ctx_ = avcodec_alloc_context3(videoEncodec);
	if (m_vc_ctx_ == nullptr) {
		avformat_close_input(&m_f_ctx_);\
		return false;
	}

	// 2.2 配置解码器
	result = avcodec_parameters_to_context(m_vc_ctx_, m_v_stream_->codecpar);
	if (result < 0) {
		ErrorUtil::printError(result);
		avformat_close_input(&m_f_ctx_);
		avcodec_free_context(&m_vc_ctx_);
		return false;
	}

	// 2.3 打开解码器
	result = avcodec_open2(m_vc_ctx_, videoEncodec, nullptr);
	if (result < 0) {
		ErrorUtil::printError(result);
		avformat_close_input(&m_f_ctx_);
		avcodec_free_context(&m_vc_ctx_);
		return false;
	}

}


bool FFmpegDecoder::transfer(std::function<void(AVFrame* frame)> callback) {


	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	while (av_read_frame(m_f_ctx_,packet) == 0) {
		if (packet->stream_index == m_v_stream_->index) {
		
			int result = avcodec_send_packet(m_vc_ctx_, packet);
			if (result < 0) {
				ErrorUtil::printError(result);
				av_packet_unref(packet);
				continue;
			}

			while (avcodec_receive_frame(m_vc_ctx_, frame) == 0)
			{
				callback(frame);
			}
		}
		av_packet_unref(packet);
	
	}

	av_packet_free(&packet);
	av_frame_free(&frame);
	return true;
}

bool FFmpegDecoder::flush(std::function<void(AVFrame* frame)> callback) {
	int result = avcodec_send_packet(m_vc_ctx_, nullptr);
	if (result < 0) {
		ErrorUtil::printError(result);
		return false;
	}

	AVFrame* frame = av_frame_alloc();
	while (avcodec_receive_frame(m_vc_ctx_,frame) == 0)
	{
		callback(frame);
	}
	av_frame_free(&frame);

}

bool FFmpegDecoder::stop() {

	if (m_f_ctx_) {
		avformat_close_input(&m_f_ctx_);
	}
	if (m_vc_ctx_) {
		avcodec_free_context(&m_vc_ctx_);
	}
	m_v_stream_ = nullptr;

	return false;
}
