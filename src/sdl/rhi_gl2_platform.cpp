// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
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
#include "../doomstat.h" // mainwads
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
	case rhi::PipelineProgram::kCrtSharp:
		return "crtsharp";
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
	std::string shaderspk3 = "shaders.pk3";
	INT32 shaderwadnum = -1;
	for (INT32 wadnum = 0; wadnum <= mainwads; wadnum++)
	{
		std::string wadname = std::string(wadfiles[wadnum]->filename);
		if (wadname.find(shaderspk3) != std::string::npos)
		{
			shaderwadnum = wadnum;
			break;
		}
	}

	if (shaderwadnum < 0)
	{
		throw std::runtime_error("Unable to identify the shaders.pk3 wadnum");
	}

	UINT16 glsllist_lump_num = W_CheckNumForLongNamePwad(lumpname, shaderwadnum, 0);
	if (glsllist_lump_num == INT16_MAX)
	{
		throw std::runtime_error(fmt::format("Unable to find glsllist lump {}", lumpname));
	}

	std::string glsllist_lump_data;
	glsllist_lump_data.resize(W_LumpLengthPwad(shaderwadnum, glsllist_lump_num));
	W_ReadLumpPwad(shaderwadnum, glsllist_lump_num, glsllist_lump_data.data());

	std::istringstream glsllist(glsllist_lump_data);
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

		UINT16 source_lump_num = W_CheckNumForLongNamePwad(line.c_str(), shaderwadnum, 0);
		if (source_lump_num == INT16_MAX)
		{
			throw std::runtime_error(fmt::format("Unable to find glsl source lump lump {}", lumpname));
		}

		std::string source_lump;
		source_lump.resize(W_LumpLengthPwad(shaderwadnum, source_lump_num));
		W_ReadLumpPwad(shaderwadnum, source_lump_num, source_lump.data());

		sources.emplace_back(source_lump);
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
