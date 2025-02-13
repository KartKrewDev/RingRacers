// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_VIDEO_ENCODER_HPP__
#define __SRB2_MEDIA_VIDEO_ENCODER_HPP__

#include "encoder.hpp"
#include "video_frame.hpp"

namespace srb2::media
{

class VideoEncoder : virtual public MediaEncoder
{
public:
	struct Config
	{
		int width;
		int height;
		int frame_rate;
		VideoFrame::BufferMethod buffer_method;
	};

	struct FrameCount
	{
		// Number of real frames, not counting frame skips.
		int frames;

		time_unit_t duration;
	};

	// VideoFrame::width() and VideoFrame::height() should be
	// used on the returned frame.
	virtual VideoFrame::instance_t new_frame(int width, int height, int pts) = 0;

	virtual void encode(VideoFrame::instance_t frame) = 0;

	virtual int width() const = 0;
	virtual int height() const = 0;
	virtual int frame_rate() const = 0;

	// Reports the number of threads used, if the encoder is
	// multithreaded.
	virtual int thread_count() const = 0;

	// Number of frames fully encoded so far.
	virtual FrameCount frame_count() const = 0;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_VIDEO_ENCODER_HPP__
