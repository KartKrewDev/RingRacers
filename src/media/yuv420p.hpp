// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_YUV420P_HPP__
#define __SRB2_MEDIA_YUV420P_HPP__

#include <cstdint>

#include "../core/vector.hpp"
#include "video_frame.hpp"

namespace srb2::media
{

class YUV420pFrame : public VideoFrame
{
public:
	// 32-byte aligned for AVX optimizations (see libyuv)
	static constexpr int kAlignment = 32;

	class BufferRGBA : public VideoFrame::Buffer
	{
	public:
		bool resize(int width, int height); // true if resized

		void erase(); // fills with black
		void release();

		int width() const { return width_; }
		int height() const { return height_; }

	private:
		int width_ = 0;
		int height_ = 0;

		srb2::Vector<uint8_t> vec_;
	};

	YUV420pFrame(int pts, Buffer y, Buffer u, Buffer v, const BufferRGBA& rgba);

	virtual ~YUV420pFrame();

	// Simply resets PTS and RGBA buffer while keeping YUV
	// buffers intact.
	void reset(int pts, const BufferRGBA& rgba) { *this = YUV420pFrame(pts, y_, u_, v_, rgba); }

	// Converts RGBA buffer to YUV planes.
	void convert() const;

	// Scales the existing buffer into a new one. This new
	// buffer replaces the existing one.
	void scale(const BufferRGBA& rgba);

	virtual int width() const override { return rgba_->width(); }
	virtual int height() const override { return rgba_->height(); }

	virtual const Buffer& rgba_buffer() const override;

private:
	Buffer y_, u_, v_;
	const BufferRGBA* rgba_;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_YUV420P_HPP__
