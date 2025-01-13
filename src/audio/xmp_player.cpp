// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "xmp_player.hpp"

#include <cmath>

#include "../core/vector.hpp"

using namespace srb2;
using namespace srb2::audio;

template <size_t C>
XmpPlayer<C>::XmpPlayer(Xmp<C>&& xmp) : xmp_(std::move(xmp)), buf_()
{
}

template <size_t C>
XmpPlayer<C>::XmpPlayer(XmpPlayer&& rhs) noexcept = default;

template <size_t C>
XmpPlayer<C>& XmpPlayer<C>::operator=(XmpPlayer<C>&& rhs) noexcept = default;

template <size_t C>
XmpPlayer<C>::~XmpPlayer() = default;

template <size_t C>
std::size_t XmpPlayer<C>::generate(tcb::span<Sample<C>> buffer)
{
	buf_.resize(buffer.size());
	std::size_t read = xmp_.play_buffer(tcb::make_span(buf_));
	buf_.resize(read);
	std::size_t ret = std::min(buffer.size(), buf_.size());

	for (std::size_t i = 0; i < ret; i++)
	{
		for (std::size_t j = 0; j < C; j++)
		{
			buffer[i].amplitudes[j] = buf_[i][j] / 32768.f;
		}
	}

	return ret;
}

template <size_t C>
float XmpPlayer<C>::duration_seconds() const
{
	return xmp_.duration_seconds();
}

template <size_t C>
float XmpPlayer<C>::position_seconds() const
{
	return xmp_.position_seconds();
}

template <size_t C>
void XmpPlayer<C>::seek(float position_seconds)
{
	xmp_.seek(static_cast<int>(std::round(position_seconds * 1000.f)));
}

template class srb2::audio::XmpPlayer<1>;
template class srb2::audio::XmpPlayer<2>;
