// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_SDL_RHI_GL2_PLATFORM_HPP__
#define __SRB2_SDL_RHI_GL2_PLATFORM_HPP__

#include <tuple>

#include "../core/string.h"
#include "../core/vector.hpp"
#include "../rhi/gl2/gl2_rhi.hpp"
#include "../rhi/rhi.hpp"

#include <SDL.h>

namespace srb2::rhi
{

struct SdlGl2Platform final : public Gl2Platform
{
	SDL_Window* window = nullptr;

	virtual ~SdlGl2Platform();

	virtual void present() override;
	virtual std::tuple<Vector<srb2::String>, Vector<srb2::String>>
	find_shader_sources(const char* name) override;
	virtual Rect get_default_framebuffer_dimensions() override;
};

} // namespace srb2::rhi

#endif // __SRB2_SDL_RHI_GL2_PLATFORM_HPP__
