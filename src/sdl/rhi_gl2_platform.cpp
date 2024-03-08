// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "rhi_gl2_platform.hpp"

#include <array>
#include <sstream>

#include <SDL.h>
#include <fmt/core.h>

#include "../cxxutil.hpp"
#include "../w_wad.h"
#include "../z_zone.h"

using namespace srb2;
using namespace srb2::rhi;

SdlGl2Platform::~SdlGl2Platform() = default;

void SdlGl2Platform::present()
{
	SRB2_ASSERT(window != nullptr);
	SRB2_ASSERT(SDL_GetWindowID(window) != 0);

	SDL_GL_SwapWindow(window);
}

static constexpr const char* pipeline_lump_slug(rhi::PipelineProgram program)
{
	switch (program)
	{
	case rhi::PipelineProgram::kUnshaded:
		return "unshaded";
	case rhi::PipelineProgram::kUnshadedPaletted:
		return "unshadedpaletted";
	case rhi::PipelineProgram::kPostprocessWipe:
		return "postprocesswipe";
	case rhi::PipelineProgram::kPostimg:
		return "postimg";
	case rhi::PipelineProgram::kSharpBilinear:
		return "sharpbilinear";
	case rhi::PipelineProgram::kCrt:
		return "crt";
	default:
		return "";
	}
}

static std::array<std::string, 2> glsllist_lump_names(rhi::PipelineProgram program)
{
	const char* pipeline_slug = pipeline_lump_slug(program);

	std::string vertex_list_name = fmt::format("rhi_glsllist_{}_vertex", pipeline_slug);
	std::string fragment_list_name = fmt::format("rhi_glsllist_{}_fragment", pipeline_slug);

	return {std::move(vertex_list_name), std::move(fragment_list_name)};
}

static std::vector<std::string> get_sources_from_glsllist_lump(const char* lumpname)
{
	lumpnum_t glsllist_lump_num = W_GetNumForLongName(lumpname);
	void* glsllist_lump = W_CacheLumpNum(glsllist_lump_num, PU_CACHE);
	size_t glsllist_lump_length = W_LumpLength(glsllist_lump_num);

	std::istringstream glsllist(std::string(static_cast<const char*>(glsllist_lump), glsllist_lump_length));
	std::vector<std::string> sources;
	for (std::string line; std::getline(glsllist, line); )
	{
		if (line.empty())
		{
			continue;
		}

		if (line[0] == '#')
		{
			continue;
		}

		if (line.back() == '\r')
		{
			line.pop_back();
		}

		lumpnum_t source_lump_num = W_GetNumForLongName(line.c_str());
		void* source_lump = W_CacheLumpNum(source_lump_num, PU_CACHE);
		size_t source_lump_length = W_LumpLength(source_lump_num);

		sources.emplace_back(static_cast<const char*>(source_lump), source_lump_length);
	}

	return sources;
}

std::tuple<std::vector<std::string>, std::vector<std::string>>
SdlGl2Platform::find_shader_sources(rhi::PipelineProgram program)
{
	std::array<std::string, 2> glsllist_names = glsllist_lump_names(program);

	std::vector<std::string> vertex_sources = get_sources_from_glsllist_lump(glsllist_names[0].c_str());
	std::vector<std::string> fragment_sources = get_sources_from_glsllist_lump(glsllist_names[1].c_str());

	return std::make_tuple(std::move(vertex_sources), std::move(fragment_sources));
}

rhi::Rect SdlGl2Platform::get_default_framebuffer_dimensions()
{
	SRB2_ASSERT(window != nullptr);
	int w;
	int h;
	SDL_GL_GetDrawableSize(window, &w, &h);
	return {0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
}
