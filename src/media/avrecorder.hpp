// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_AVRECORDER_HPP__
#define __SRB2_MEDIA_AVRECORDER_HPP__

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include <tcb/span.hpp>

#include "../core/string.h"
#include "../core/vector.hpp"
#include "../audio/sample.hpp"

namespace srb2::media
{

class AVRecorder
{
public:
	using audio_sample_t = srb2::audio::Sample<2>;
	using audio_buffer_t = tcb::span<const audio_sample_t>;

	class Impl;

	struct Config
	{
		struct Audio
		{
			int sample_rate;
		};

		struct Video
		{
			int width;
			int height;
			int frame_rate;
		};

		srb2::String file_name;

		std::optional<std::size_t> max_size; // file size limit
		std::optional<std::chrono::duration<float>> max_duration;

		std::optional<Audio> audio;
		std::optional<Video> video;
	};

	// TODO: remove once hwr2 twodee is finished
	struct StagingVideoFrame
	{
		using instance_t = std::unique_ptr<StagingVideoFrame>;

		srb2::Vector<uint8_t> screen;
		uint32_t width, height;
		int pts;

		StagingVideoFrame(uint32_t width_, uint32_t height_, int pts_) :
			screen(width_ * height_ * 3), width(width_), height(height_), pts(pts_)
		{
		}
	};

	// Returns the canonical file extension minus the dot.
	// E.g. "webm" (not ".webm").
	static const char* file_extension();

	AVRecorder(Config config);
	~AVRecorder();

	void print_configuration() const;
	void draw_statistics() const;

	void push_audio_samples(audio_buffer_t buffer);

	// May return nullptr in case called between units of
	// Config::frame_rate
	StagingVideoFrame::instance_t new_staging_video_frame(uint32_t width, uint32_t height);

	void push_staging_video_frame(StagingVideoFrame::instance_t frame);

	// Proper name of the container format.
	const char* format_name() const;

	// True if this instance has terminated. Continuing to use
	// this interface is useless and the object should be
	// destructed immediately.
	bool invalid() const;

private:
	std::unique_ptr<Impl> impl_;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_AVRECORDER_HPP__
