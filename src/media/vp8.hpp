// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_VP8_HPP__
#define __SRB2_MEDIA_VP8_HPP__

#include <mutex>

#include <vpx/vp8cx.h>

#include "options.hpp"
#include "video_encoder.hpp"
#include "yuv420p.hpp"

namespace srb2::media
{

class VP8Encoder : public VideoEncoder
{
public:
	static const Options options_;

	VP8Encoder(VideoEncoder::Config config);

	virtual VideoFrame::instance_t new_frame(int width, int height, int pts) override final;

	virtual void encode(VideoFrame::instance_t frame) override final;
	virtual void flush() override final;

	virtual const char* name() const override final { return "VP8"; }
	virtual int width() const override final { return img_->w; }
	virtual int height() const override final { return img_->h; }
	virtual int frame_rate() const override final { return frame_rate_; }
	virtual int thread_count() const override final { return thread_count_; }

	virtual FrameCount frame_count() const override final;

private:
	class CtxWrapper
	{
	public:
		CtxWrapper(const Config config);
		~CtxWrapper();

		operator vpx_codec_ctx_t*() { return &ctx_; }
		operator vpx_codec_ctx_t&() { return ctx_; }

	private:
		vpx_codec_ctx_t ctx_;
	};

	class ImgWrapper
	{
	public:
		ImgWrapper(int width, int height);
		~ImgWrapper();

		operator vpx_image_t*() { return &img_; }
		vpx_image_t* operator->() { return &img_; }
		const vpx_image_t* operator->() const { return &img_; }

	private:
		vpx_image_t img_;
	};

	enum class KeyFrameOption : int
	{
	    kAuto = -1,
	};

	enum class DeadlineOption : int
	{
	    kInfinite = 0,
	};

	static vpx_codec_iface_t* kCodec;

	static const vpx_codec_enc_cfg_t configure(const Config config);

	CtxWrapper ctx_;
	ImgWrapper img_;

	const int frame_rate_;
	const int thread_count_ = options_.get<int>("threads");
	const int deadline_ = options_.get<int>("deadline");

	mutable std::recursive_mutex frame_count_mutex_;

	int duration_ = 0;
	int frame_count_ = 0;

	YUV420pFrame::BufferRGBA //
		rgba_buffer_,
		rgba_scaled_buffer_; // only allocated if input NEEDS scaling

	std::unique_ptr<YUV420pFrame> frame_;

	bool process();

	template <typename T> // T = option type
	void control(vp8e_enc_control_id id, const char* option);
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_VP8_HPP__
