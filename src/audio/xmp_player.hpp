// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_XMP_PLAYER_HPP__
#define __SRB2_AUDIO_XMP_PLAYER_HPP__

#include "source.hpp"
#include "xmp.hpp"

#include "../core/vector.hpp"

namespace srb2::audio
{

template <size_t C>
class XmpPlayer final : public Source<C>
{
	Xmp<C> xmp_;
	srb2::Vector<std::array<int16_t, C>> buf_;

public:
	XmpPlayer(Xmp<C>&& xmp);

	XmpPlayer(const XmpPlayer<C>&) = delete;
	XmpPlayer(XmpPlayer<C>&& rhs) noexcept;

	XmpPlayer<C>& operator=(const XmpPlayer<C>&) = delete;
	XmpPlayer<C>& operator=(XmpPlayer<C>&& rhs) noexcept;

	~XmpPlayer();

	virtual std::size_t generate(tcb::span<Sample<C>> buffer) override final;

	bool looping() { return xmp_.looping(); };
	void looping(bool looping) { xmp_.looping(looping); }
	void reset() { xmp_.reset(); }
	float duration_seconds() const;
	float position_seconds() const;
	void seek(float position_seconds);
};

extern template class XmpPlayer<1>;
extern template class XmpPlayer<2>;

} // namespace srb2::audio

#endif // __SRB2_AUDIO_XMP_PLAYER_HPP__
