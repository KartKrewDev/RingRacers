// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "ogg_player.hpp"

#include <cmath>
#include <optional>
#include <utility>

using namespace srb2;
using namespace srb2::audio;

namespace
{

std::optional<std::size_t> find_loop_point(const Ogg& ogg)
{
	OggComment comment = ogg.comment();
	std::size_t rate = ogg.sample_rate();
	for (auto& comment : comment.comments)
	{
		if (comment.find("LOOPPOINT=") == 0)
		{
			std::string_view comment_view(comment);
			comment_view.remove_prefix(10);
			std::string copied {comment_view};

			try
			{
				int loop_point = std::stoi(copied);
				return loop_point;
			}
			catch (...)
			{
			}
		}

		if (comment.find("LOOPMS=") == 0)
		{
			std::string_view comment_view(comment);
			comment_view.remove_prefix(7);
			std::string copied {comment_view};

			try
			{
				int loop_ms = std::stoi(copied);
				int loop_point = std::round(static_cast<double>(rate) * (loop_ms / 1000.));

				return loop_point;
			}
			catch (...)
			{
			}
		}
	}

	return std::nullopt;
}

} // namespace

template <size_t C>
OggPlayer<C>::OggPlayer(Ogg&& ogg) noexcept
	: playing_(false), looping_(false), loop_point_(std::nullopt), ogg_(std::forward<Ogg>(ogg))
{
	loop_point_ = find_loop_point(ogg_);
}

template <size_t C>
OggPlayer<C>::OggPlayer(OggPlayer&& rhs) noexcept = default;

template <size_t C>
OggPlayer<C>& OggPlayer<C>::operator=(OggPlayer&& rhs) noexcept = default;

template <size_t C>
OggPlayer<C>::~OggPlayer() = default;

template <size_t C>
std::size_t OggPlayer<C>::generate(tcb::span<Sample<C>> buffer)
{
	if (!playing_)
		return 0;

	std::size_t total = 0;
	do
	{
		std::size_t read = ogg_.get_samples(buffer.subspan(total));
		total += read;

		if (read == 0 && !looping_)
		{
			playing_ = false;
			break;
		}

		if (read == 0 && loop_point_)
		{
			ogg_.seek(*loop_point_);
		}

		if (read == 0 && !loop_point_)
		{
			ogg_.seek(0);
		}
	} while (total < buffer.size());

	return total;
}

template <size_t C>
void OggPlayer<C>::seek(float position_seconds)
{
	ogg_.seek(static_cast<std::size_t>(position_seconds * sample_rate()));
}

template <size_t C>
void OggPlayer<C>::loop_point_seconds(float loop_point)
{
	std::size_t rate = sample_rate();
	loop_point = static_cast<std::size_t>(std::round(loop_point * rate));
}

template <size_t C>
void OggPlayer<C>::reset()
{
	ogg_.seek(0);
}

template <size_t C>
std::size_t OggPlayer<C>::sample_rate() const
{
	return ogg_.sample_rate();
}

template <size_t C>
float OggPlayer<C>::duration_seconds() const
{
	return ogg_.duration_seconds();
}

template <size_t C>
std::optional<float> OggPlayer<C>::loop_point_seconds() const
{
	if (!loop_point_)
		return std::nullopt;

	return *loop_point_ / static_cast<float>(sample_rate());
}

template <size_t C>
float OggPlayer<C>::position_seconds() const
{
	return ogg_.position_seconds();
}

template class srb2::audio::OggPlayer<1>;
template class srb2::audio::OggPlayer<2>;
