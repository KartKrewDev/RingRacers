// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_CONTAINER_HPP__
#define __SRB2_MEDIA_CONTAINER_HPP__

#include <chrono>
#include <functional>
#include <memory>

#include "../core/string.h"
#include "audio_encoder.hpp"
#include "video_encoder.hpp"

namespace srb2::media
{

class MediaContainer
{
public:
	using dtor_cb_t = std::function<void(const MediaContainer&)>;
	using time_unit_t = std::chrono::duration<float>;

	struct Config
	{
		srb2::String file_name;
		dtor_cb_t destructor_callback;
	};

	virtual ~MediaContainer() = default;

	virtual std::unique_ptr<AudioEncoder> make_audio_encoder(AudioEncoder::Config config) = 0;
	virtual std::unique_ptr<VideoEncoder> make_video_encoder(VideoEncoder::Config config) = 0;

	virtual const char* name() const = 0;
	virtual const char* file_name() const = 0;

	// These are normally estimates. However, when called from
	// Config::destructor_callback, these are the exact final
	// values.
	virtual time_unit_t duration() const = 0;
	virtual std::size_t size() const = 0;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_CONTAINER_HPP__
