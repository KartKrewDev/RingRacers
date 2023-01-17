// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__
#define __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__

#include "../rhi/gl3_core/gl3_core_rhi.hpp"
#include "../rhi/rhi.hpp"

#include <SDL.h>

namespace srb2::rhi
{

struct SdlGlCorePlatform final : public GlCorePlatform
{
	SDL_Window* window = nullptr;

	virtual ~SdlGlCorePlatform();

	virtual void present() override;
	virtual std::tuple<std::string, std::string> find_shader_sources(PipelineProgram program) override;
	virtual Rect get_default_framebuffer_dimensions() override;
};

} // namespace srb2::rhi

#endif // __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__
