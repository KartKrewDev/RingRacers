// RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
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
#include <string>
#include <vector>

#include <tcb/span.hpp>

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

		std::string file_name;

		std::optional<std::size_t> max_size; // file size limit
		std::optional<std::chrono::duration<float>> max_duration;

		std::optional<Audio> audio;
		std::optional<Video> video;
	};

	// Returns the canonical file extension minus the dot.
	// E.g. "webm" (not ".webm").
	static const char* file_extension();

	AVRecorder(Config config);
	~AVRecorder();

	void push_audio_samples(audio_buffer_t buffer);

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
