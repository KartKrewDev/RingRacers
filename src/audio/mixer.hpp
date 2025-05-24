// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_MIXER_HPP__
#define __SRB2_AUDIO_MIXER_HPP__

#include <memory>
#include <vector>

#include <tcb/span.hpp>

#include "source.hpp"

namespace srb2::audio
{

template <size_t C>
class Mixer : public Source<C>
{
public:
	virtual std::size_t generate(tcb::span<Sample<C>> buffer) override final;

	virtual ~Mixer();

	void add_source(const std::shared_ptr<Source<C>>& source);

private:
	std::vector<std::shared_ptr<Source<C>>> sources_;
	std::vector<Sample<C>> buffer_;
};

extern template class Mixer<1>;
extern template class Mixer<2>;

} // namespace srb2::audio

#endif // __SRB2_AUDIO_MIXER_HPP__
