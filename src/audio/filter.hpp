// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_FILTER_HPP__
#define __SRB2_AUDIO_FILTER_HPP__

#include <cstddef>
#include <memory>
#include <vector>

#include <tcb/span.hpp>

#include "source.hpp"

namespace srb2::audio
{

template <size_t IC, size_t OC>
class Filter : public Source<OC>
{
public:
	virtual std::size_t generate(tcb::span<Sample<OC>> buffer) override;

	void bind(const std::shared_ptr<Source<IC>>& input);

	virtual std::size_t filter(tcb::span<Sample<IC>> input_buffer, tcb::span<Sample<OC>> buffer) = 0;

	virtual ~Filter();

private:
	std::shared_ptr<Source<IC>> input_;
	std::vector<Sample<IC>> input_buffer_;
};

extern template class Filter<1, 1>;
extern template class Filter<1, 2>;
extern template class Filter<2, 1>;
extern template class Filter<2, 2>;

} // namespace srb2::audio

#endif // __SRB2_AUDIO_FILTER_HPP__
