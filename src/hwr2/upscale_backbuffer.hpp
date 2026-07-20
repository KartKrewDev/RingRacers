// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_HWR2_UPSCALE_BACKBUFFER_HPP
#define SRB2_HWR2_UPSCALE_BACKBUFFER_HPP

#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

class UpscaleBackbuffer
{
	rhi::Handle<rhi::Texture> color_;
	rhi::Handle<rhi::Renderbuffer> depth_;

public:
	UpscaleBackbuffer();
	UpscaleBackbuffer(const UpscaleBackbuffer&) = delete;
	UpscaleBackbuffer(UpscaleBackbuffer&&);
	~UpscaleBackbuffer();

	UpscaleBackbuffer& operator=(const UpscaleBackbuffer&) = delete;
	UpscaleBackbuffer& operator=(UpscaleBackbuffer&&);

	void begin_pass(rhi::Rhi& rhi);
	rhi::Handle<rhi::Texture> color() const noexcept { return color_; }
};

} // namespace srb2::hwr2

#endif // SRB2_HWR2_UPSCALE_BACKBUFFER_HPP
