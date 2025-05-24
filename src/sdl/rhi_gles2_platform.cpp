// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "rhi_gles2_platform.hpp"

#include <SDL.h>

#include "../cxxutil.hpp"
#include "../w_wad.h"
#include "../z_zone.h"

using namespace srb2;
using namespace srb2::rhi;

SdlGles2Platform::~SdlGles2Platform() = default;

void SdlGles2Platform::present()
{
	SRB2_ASSERT(window != nullptr);
	SRB2_ASSERT(SDL_GetWindowID(window) != 0);

	SDL_GL_SwapWindow(window);
}

std::tuple<std::string, std::string> SdlGles2Platform::find_shader_sources(rhi::PipelineProgram program)
{
	const char* vertex_lump_name = nullptr;
	const char* fragment_lump_name = nullptr;
	switch (program)
	{
	case rhi::PipelineProgram::kUnshaded:
		vertex_lump_name = "rhi_glsles_vertex_unshaded";
		fragment_lump_name = "rhi_glsles_fragment_unshaded";
		break;
	case rhi::PipelineProgram::kUnshadedPaletted:
		vertex_lump_name = "rhi_glsles_vertex_unshadedpaletted";
		fragment_lump_name = "rhi_glsles_fragment_unshadedpaletted";
		break;
	default:
		std::terminate();
	}

	lumpnum_t vertex_lump_num = W_GetNumForLongName(vertex_lump_name);
	lumpnum_t fragment_lump_num = W_GetNumForLongName(fragment_lump_name);
	size_t vertex_lump_length = W_LumpLength(vertex_lump_num);
	size_t fragment_lump_length = W_LumpLength(fragment_lump_num);
	void* vertex_lump = W_CacheLumpNum(vertex_lump_num, PU_CACHE);
	void* fragment_lump = W_CacheLumpNum(fragment_lump_num, PU_CACHE);

	std::string vertex_shader(static_cast<const char*>(vertex_lump), vertex_lump_length);
	std::string fragment_shader(static_cast<const char*>(fragment_lump), fragment_lump_length);

	return std::make_tuple(std::move(vertex_shader), std::move(fragment_shader));
}

rhi::Rect SdlGles2Platform::get_default_framebuffer_dimensions()
{
	SRB2_ASSERT(window != nullptr);
	int w;
	int h;
	SDL_GL_GetDrawableSize(window, &w, &h);
	return {0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
}
