// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "gme_player.hpp"

using namespace srb2;
using namespace srb2::audio;

template <size_t C>
GmePlayer<C>::GmePlayer(Gme&& gme) : gme_(std::forward<Gme>(gme)), buf_() {
}

template <size_t C>
GmePlayer<C>::GmePlayer(GmePlayer<C>&& rhs) noexcept = default;

template <size_t C>
GmePlayer<C>& GmePlayer<C>::operator=(GmePlayer<C>&& rhs) noexcept = default;

template <size_t C>
GmePlayer<C>::~GmePlayer() = default;

template <size_t C>
std::size_t GmePlayer<C>::generate(tcb::span<Sample<C>> buffer) {
	buf_.clear();
	buf_.resize(buffer.size() * 2);

	std::size_t read = gme_.get_samples(tcb::make_span(buf_));
	buf_.resize(read);
	std::size_t new_samples = std::min((read / 2), buffer.size());
	for (std::size_t i = 0; i < new_samples; i++) {
		if constexpr (C == 1) {
			buffer[i].amplitudes[0] = (buf_[i * 2] / 32768.f + buf_[i * 2 + 1] / 32768.f) / 2.f;
		} else if constexpr (C == 2) {
			buffer[i].amplitudes[0] = buf_[i * 2] / 32768.f;
			buffer[i].amplitudes[1] = buf_[i * 2 + 1] / 32768.f;
		}
	}
	return new_samples;
}

template <size_t C>
void GmePlayer<C>::seek(float position_seconds) {
	gme_.seek(static_cast<std::size_t>(position_seconds * 44100.f));
}

template <size_t C>
void GmePlayer<C>::reset() {
	gme_.seek(0);
}

template <size_t C>
std::optional<float> GmePlayer<C>::duration_seconds() const {
	return gme_.duration_seconds();
}

template <size_t C>
std::optional<float> GmePlayer<C>::loop_point_seconds() const {
	return gme_.loop_point_seconds();
}

template <size_t C>
float GmePlayer<C>::position_seconds() const {
	return gme_.position_seconds();
}

template class srb2::audio::GmePlayer<1>;
template class srb2::audio::GmePlayer<2>;
