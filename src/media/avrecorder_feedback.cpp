// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <filesystem>
#include <sstream>

#include <fmt/format.h>

#include "../core/string.h"
#include "../cxxutil.hpp"
#include "avrecorder_impl.hpp"

#include "../v_video.h"

using namespace srb2::media;

using Impl = AVRecorder::Impl;

namespace
{

constexpr float kMb = 1024.f * 1024.f;

}; // namespace

void Impl::container_dtor_handler(const MediaContainer& container) const
{
	// Note that because this method is called from
	// container_'s destructor, any member variables declared
	// after Impl::container_ should not be accessed by now
	// (since they would have already destructed).

	if (max_size_ && container.size() > *max_size_)
	{
		const srb2::String line = srb2::format(
			"Video size has exceeded limit {} > {} ({}%)."
			" This should not happen, please report this bug.\n",
			container.size(),
			*max_size_,
			100.f * (*max_size_ / static_cast<float>(container.size()))
		);

		CONS_Alert(CONS_WARNING, "%s\n", line.c_str());
	}

	std::ostringstream msg;

	msg << "Video saved: " << std::filesystem::path(container.file_name()).filename().string()
		<< fmt::format(" ({:.2f}", container.size() / kMb);

	if (max_size_)
	{
		msg << fmt::format("/{:.2f}", *max_size_ / kMb);
	}

	msg << fmt::format(" MB, {:.1f}", container.duration().count());

	if (max_duration_config_)
	{
		msg << fmt::format("/{:.1f}", max_duration_config_->count());
	}

	msg << " seconds)";

	CONS_Printf("%s\n", msg.str().c_str());
}

void AVRecorder::print_configuration() const
{
	if (impl_->audio_encoder_)
	{
		const auto& a = *impl_->audio_encoder_;

		CONS_Printf("Audio: %s %dch %d Hz\n", a.name(), a.channels(), a.sample_rate());
	}

	if (impl_->video_encoder_)
	{
		const auto& v = *impl_->video_encoder_;

		CONS_Printf(
			"Video: %s %dx%d %d fps %d threads\n",
			v.name(),
			v.width(),
			v.height(),
			v.frame_rate(),
			v.thread_count()
		);
	}
}

void AVRecorder::draw_statistics() const
{
	SRB2_ASSERT(impl_->video_encoder_ != nullptr);

	auto draw = [](int x, const srb2::String text, int32_t flags = 0)
	{
		V_DrawThinString(
			x,
			190,
			(V_SNAPTOBOTTOM | V_SNAPTORIGHT) | flags,
			text.c_str()
		);
	};

	const float fps = impl_->video_frame_rate_avg_;
	const float size = impl_->container_->size();

	const int32_t fps_color = [&]
	{
		const int cap = impl_->video_encoder_->frame_rate();

		// red when dropped below 60% of the target
		if (fps > 0.f && fps < (0.6f * cap))
		{
			return V_REDMAP;
		}

		return 0;
	}();

	const int32_t mb_color = [&]
	{
		if (!impl_->max_size_)
		{
			return 0;
		}

		const std::size_t cap = *impl_->max_size_;

		// yellow when within 1 MB of the limit
		if (size >= (cap - kMb))
		{
			return V_YELLOWMAP;
		}

		return 0;
	}();

	draw(200, srb2::format("{:.0f}", fps), fps_color);
	draw(230, srb2::format("{:.1f}s", impl_->container_->duration().count()));
	draw(260, srb2::format("{:.1f} MB", size / kMb), mb_color);
}
