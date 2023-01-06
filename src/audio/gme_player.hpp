// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_GME_PLAYER_HPP__
#define __SRB2_AUDIO_GME_PLAYER_HPP__

#include <optional>

#include "gme.hpp"
#include "source.hpp"

namespace srb2::audio {

template <size_t C>
class GmePlayer : public Source<C> {
	Gme gme_;
	std::vector<short> buf_;

public:
	GmePlayer(Gme&& gme);
	GmePlayer(const GmePlayer<C>&) = delete;
	GmePlayer(GmePlayer<C>&& gme) noexcept;

	~GmePlayer();

	GmePlayer& operator=(const GmePlayer<C>&) = delete;
	GmePlayer& operator=(GmePlayer<C>&& rhs) noexcept;

	virtual std::size_t generate(tcb::span<Sample<C>> buffer) override;

	void seek(float position_seconds);

	std::optional<float> duration_seconds() const;
	std::optional<float> loop_point_seconds() const;
	float position_seconds() const;

	void reset();
};

extern template class GmePlayer<1>;
extern template class GmePlayer<2>;

} // namespace srb2::audio

#endif // __SRB2_AUDIO_GME_PLAYER_HPP__
