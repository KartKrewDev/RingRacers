// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__
#define __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__

#include "../rhi/gles2/gles2_rhi.hpp"
#include "../rhi/rhi.hpp"

#include <SDL.h>

namespace srb2::rhi
{

struct SdlGles2Platform final : public Gles2Platform
{
	SDL_Window* window = nullptr;

	virtual ~SdlGles2Platform();

	virtual void present() override;
	virtual std::tuple<std::string, std::string> find_shader_sources(PipelineProgram program) override;
	virtual Rect get_default_framebuffer_dimensions() override;
};

} // namespace srb2::rhi

#endif // __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__
