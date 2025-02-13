// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>

#include <fmt/format.h>
#include <tcb/span.hpp>

#include "../cxxutil.hpp"
#include "vp8.hpp"
#include "vpx_error.hpp"
#include "yuv420p.hpp"

using namespace srb2::media;

vpx_codec_iface_t* VP8Encoder::kCodec = vpx_codec_vp8_cx();

const vpx_codec_enc_cfg_t VP8Encoder::configure(const Config user)
{
	vpx_codec_enc_cfg_t cfg;
	vpx_codec_enc_config_default(kCodec, &cfg, 0);

	cfg.g_threads = options_.get<int>("threads");

	cfg.g_w = user.width;
	cfg.g_h = user.height;

	cfg.g_bit_depth = VPX_BITS_8;
	cfg.g_input_bit_depth = 8;

	cfg.g_timebase.num = 1;
	cfg.g_timebase.den = user.frame_rate;

	cfg.g_pass = VPX_RC_ONE_PASS;
	cfg.rc_end_usage = static_cast<vpx_rc_mode>(options_.get<int>("quality_mode"));
	cfg.kf_mode = VPX_KF_AUTO;

	cfg.rc_target_bitrate = options_.get<int>("target_bitrate");
	cfg.rc_min_quantizer = options_.get<int>("min_q");
	cfg.rc_max_quantizer = options_.get<int>("max_q");

	// Keyframe spacing, in number of frames.
	// kf_max_dist should be low enough to allow scrubbing.

	int kf_max = options_.get<int>("kf_max");

	if (kf_max == static_cast<int>(KeyFrameOption::kAuto))
	{
		// Automatically pick a good rate
		kf_max = (user.frame_rate / 2); // every .5s
	}

	cfg.kf_min_dist = options_.get<int>("kf_min");
	cfg.kf_max_dist = kf_max;

	return cfg;
}

VP8Encoder::VP8Encoder(Config config) : ctx_(config), img_(config.width, config.height), frame_rate_(config.frame_rate)
{
	SRB2_ASSERT(config.buffer_method == VideoFrame::BufferMethod::kEncoderAllocatedRGBA8888);

	control<int>(VP8E_SET_CPUUSED, "cpu_used");
	control<int>(VP8E_SET_CQ_LEVEL, "cq_level");
	control<int>(VP8E_SET_SHARPNESS, "sharpness");
	control<int>(VP8E_SET_TOKEN_PARTITIONS, "token_parts");

	auto plane = [this](int k, int ycs = 0)
	{
		using T = uint8_t;
		auto view = tcb::span<T>(reinterpret_cast<T*>(img_->planes[k]), img_->stride[k] * (img_->h >> ycs));

		return VideoFrame::Buffer {view, static_cast<std::size_t>(img_->stride[k])};
	};

	frame_ = std::make_unique<YUV420pFrame>(
		0,
		plane(VPX_PLANE_Y),
		plane(VPX_PLANE_U, img_->y_chroma_shift),
		plane(VPX_PLANE_V, img_->y_chroma_shift),
		rgba_buffer_
	);
}

VP8Encoder::CtxWrapper::CtxWrapper(const Config user)
{
	const vpx_codec_enc_cfg_t cfg = configure(user);

	if (vpx_codec_enc_init(&ctx_, kCodec, &cfg, 0) != VPX_CODEC_OK)
	{
		throw std::invalid_argument(fmt::format("vpx_codec_enc_init: {}", VpxError(ctx_)));
	}
}

VP8Encoder::CtxWrapper::~CtxWrapper()
{
	vpx_codec_destroy(&ctx_);
}

VP8Encoder::ImgWrapper::ImgWrapper(int width, int height)
{
	if (vpx_img_alloc(&img_, VPX_IMG_FMT_I420, width, height, YUV420pFrame::kAlignment) == nullptr)
	{
		throw std::runtime_error("vpx_img_alloc");
	}
}

VP8Encoder::ImgWrapper::~ImgWrapper()
{
	vpx_img_free(&img_);
}

VideoFrame::instance_t VP8Encoder::new_frame(int width, int height, int pts)
{
	SRB2_ASSERT(frame_ != nullptr);

	if (rgba_buffer_.resize(width, height))
	{
		// If there was a resize, the aspect ratio may not
		// match. When the frame is scaled later, it will be
		// "fit" into the target aspect ratio, leaving some
		// empty space around the scaled image. (See
		// VP8Encoder::encode)
		//
		// Set whole scaled buffer to black now so the empty
		// space appears as "black bars".
		rgba_scaled_buffer_.erase();
	}

	frame_->reset(pts, rgba_buffer_);

	return std::move(frame_);
}

void VP8Encoder::encode(VideoFrame::instance_t frame)
{
	{
		using T = YUV420pFrame;

		SRB2_ASSERT(frame_ == nullptr);
		SRB2_ASSERT(dynamic_cast<T*>(frame.get()) != nullptr);

		frame_ = std::unique_ptr<T>(static_cast<T*>(frame.release()));
	}

	// This frame must be scaled to match encoder configuration
	if (frame_->width() != width() || frame_->height() != height())
	{
		rgba_scaled_buffer_.resize(width(), height());
		frame_->scale(rgba_scaled_buffer_);
	}
	else
	{
		rgba_scaled_buffer_.release();
	}

	frame_->convert();

	if (vpx_codec_encode(ctx_, img_, frame_->pts(), 1, 0, deadline_) != VPX_CODEC_OK)
	{
		throw std::invalid_argument(fmt::format("VP8Encoder::encode: vpx_codec_encode: {}", VpxError(ctx_)));
	}

	process();
}

void VP8Encoder::flush()
{
	do
	{
		if (vpx_codec_encode(ctx_, nullptr, 0, 0, 0, 0) != VPX_CODEC_OK)
		{
			throw std::invalid_argument(fmt::format("VP8Encoder::flush: vpx_codec_encode: {}", VpxError(ctx_)));
		}
	} while (process());
}

bool VP8Encoder::process()
{
	bool output = false;

	vpx_codec_iter_t iter = NULL;
	const vpx_codec_cx_pkt_t* pkt;

	while ((pkt = vpx_codec_get_cx_data(ctx_, &iter)))
	{
		output = true;

		if (pkt->kind != VPX_CODEC_CX_FRAME_PKT)
		{
			continue;
		}

		auto& frame = pkt->data.frame;

		{
			const std::lock_guard _(frame_count_mutex_);

			duration_ = frame.pts + frame.duration;
			frame_count_++;
		}

		const float ts = frame.pts / static_cast<float>(frame_rate());

		using T = const std::byte;
		tcb::span<T> p(reinterpret_cast<T*>(frame.buf), frame.sz);

		write_frame(p, std::chrono::duration<float>(ts), (frame.flags & VPX_FRAME_IS_KEY));
	}

	return output;
}

template <typename T>
void VP8Encoder::control(vp8e_enc_control_id id, const char* option)
{
	auto value = options_.get<T>(option);

	if (vpx_codec_control_(ctx_, id, value) != VPX_CODEC_OK)
	{
		throw std::invalid_argument(fmt::format("vpx_codec_control: {}, {}={}", VpxError(ctx_), option, value));
	}
}

VideoEncoder::FrameCount VP8Encoder::frame_count() const
{
	const std::lock_guard _(frame_count_mutex_);

	return {frame_count_, std::chrono::duration<float>(duration_ / static_cast<float>(frame_rate()))};
}
