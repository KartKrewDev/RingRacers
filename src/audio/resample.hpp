// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_RESAMPLE_HPP__
#define __SRB2_AUDIO_RESAMPLE_HPP__

#include <cmath>
#include <memory>
#include <vector>

#include <tcb/span.hpp>

#include "source.hpp"

namespace srb2::audio
{

template <size_t C>
class Resampler : public Source<C>
{
public:
	Resampler(std::shared_ptr<Source<C>>&& source_, float ratio);
	Resampler(const Resampler<C>& r) = delete;
	Resampler(Resampler<C>&& r);
	virtual ~Resampler();

	virtual std::size_t generate(tcb::span<Sample<C>> buffer);

	void ratio(float new_ratio);

	Resampler& operator=(const Resampler<C>& r) = delete;
	Resampler& operator=(Resampler<C>&& r);

private:
	std::shared_ptr<Source<C>> source_;
	float ratio_ {1.f};
	std::vector<Sample<C>> buf_;
	Sample<C> last_;
	int pos_ {0};
	float pos_frac_ {0.f};

	void advance(float samples)
	{
		pos_frac_ += samples;
		float integer;
		std::modf(pos_frac_, &integer);
		pos_ += integer;
		pos_frac_ -= integer;
	}

	void refill();
};

extern template class Resampler<1>;
extern template class Resampler<2>;

} // namespace srb2::audio

#endif // __SRB2_AUDIO_RESAMPLE_HPP__
