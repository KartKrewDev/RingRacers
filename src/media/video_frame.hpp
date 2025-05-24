// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_VIDEO_FRAME_HPP__
#define __SRB2_MEDIA_VIDEO_FRAME_HPP__

#include <cstddef>
#include <cstdint>
#include <memory>

#include <tcb/span.hpp>

namespace srb2::media
{

class VideoFrame
{
public:
	using instance_t = std::unique_ptr<VideoFrame>;

	enum class BufferMethod
	{
		// Returns an already allocated buffer for each
		// frame. See VideoFrame::rgba_buffer(). The encoder
		// completely manages allocating this buffer.
		kEncoderAllocatedRGBA8888,
	};

	struct Buffer
	{
		tcb::span<uint8_t> plane;
		std::size_t row_stride; // size of each row
	};

	virtual int width() const = 0;
	virtual int height() const = 0;

	int pts() const { return pts_; }

	// Returns a buffer that should be
	// filled with RGBA pixels.
	//
	// This method may only be used if
	// the encoder was configured with
	// BufferMethod::kEncoderAllocatedRGBA8888.
	virtual const Buffer& rgba_buffer() const = 0;

protected:
	VideoFrame(int pts) : pts_(pts) {}

private:
	int pts_;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_VIDEO_FRAME_HPP__
