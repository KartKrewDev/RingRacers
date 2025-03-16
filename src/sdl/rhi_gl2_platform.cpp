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

static std::array<std::string, 2> glsllist_lump_names(const char* name)
{
	std::string vertex_list_name = fmt::format("rhi_glsllist_{}_vertex.txt", name);
	std::string fragment_list_name = fmt::format("rhi_glsllist_{}_fragment.txt", name);

	return {std::move(vertex_list_name), std::move(fragment_list_name)};
}

static std::vector<std::string> get_sources_from_glsllist_lump(const char* lumpname)
{
	size_t buffer_size;
	if (!W_ReadShader(lumpname, &buffer_size, nullptr))
	{
		throw std::runtime_error(fmt::format("Unable to find glsllist lump {}", lumpname));
	}
	std::string glsllist_lump_data;
	glsllist_lump_data.resize(buffer_size);
	if (!W_ReadShader(lumpname, &buffer_size, glsllist_lump_data.data()))
	{
		throw std::runtime_error(fmt::format("Unable to read glsllist lump {}", lumpname));
	}

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

		// Compat: entries not ending in .glsl should append, for new shader file lookup system
		size_t glsl_pos = line.find(".glsl");
		if (line.size() < 5 || glsl_pos == line.npos || glsl_pos != line.size() - 5)
		{
			line.append(".glsl");
		}

		size_t source_lump_size;
		if (!W_ReadShader(line.c_str(), &source_lump_size, nullptr))
		{
			throw std::runtime_error(fmt::format("Unable to find glsl source lump lump {}", line));
		}
		std::string source_lump;
		source_lump.resize(source_lump_size);
		if (!W_ReadShader(line.c_str(), &source_lump_size, source_lump.data()))
		{
			throw std::runtime_error(fmt::format("Unable to read glsl source lump lump {}", line));
		}

		sources.emplace_back(source_lump);
	}

	return sources;
}

std::tuple<std::vector<std::string>, std::vector<std::string>>
SdlGl2Platform::find_shader_sources(const char* name)
{
	std::array<std::string, 2> glsllist_names = glsllist_lump_names(name);

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
