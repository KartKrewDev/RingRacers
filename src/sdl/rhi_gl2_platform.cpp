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
#include <string>
#include <sstream>

#include <SDL.h>

#include "../core/string.h"
#include "../core/vector.hpp"
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

static std::array<srb2::String, 2> glsllist_lump_names(const char* name)
{
	srb2::String vertex_list_name = fmt::format("rhi_glsllist_{}_vertex.txt", name);
	srb2::String fragment_list_name = fmt::format("rhi_glsllist_{}_fragment.txt", name);

	return {std::move(vertex_list_name), std::move(fragment_list_name)};
}

static srb2::Vector<srb2::String> get_sources_from_glsllist_lump(const char* lumpname)
{
	size_t buffer_size;
	if (!W_ReadShader(lumpname, &buffer_size, nullptr))
	{
		throw std::runtime_error(fmt::format("Unable to find glsllist lump {}", lumpname));
	}
	srb2::String glsllist_lump_data;
	glsllist_lump_data.resize(buffer_size);
	if (!W_ReadShader(lumpname, &buffer_size, glsllist_lump_data.data()))
	{
		throw std::runtime_error(fmt::format("Unable to read glsllist lump {}", lumpname));
	}

	std::istringstream glsllist(glsllist_lump_data);
	srb2::Vector<srb2::String> sources;
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
		srb2::String source_lump;
		source_lump.resize(source_lump_size);
		if (!W_ReadShader(line.c_str(), &source_lump_size, source_lump.data()))
		{
			throw std::runtime_error(fmt::format("Unable to read glsl source lump lump {}", line));
		}

		sources.emplace_back(source_lump);
	}

	return sources;
}

std::tuple<srb2::Vector<srb2::String>, srb2::Vector<srb2::String>>
SdlGl2Platform::find_shader_sources(const char* name)
{
	std::array<srb2::String, 2> glsllist_names = glsllist_lump_names(name);

	srb2::Vector<srb2::String> vertex_sources = get_sources_from_glsllist_lump(glsllist_names[0].c_str());
	srb2::Vector<srb2::String> fragment_sources = get_sources_from_glsllist_lump(glsllist_names[1].c_str());

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
