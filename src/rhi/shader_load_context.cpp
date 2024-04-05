// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "shader_load_context.hpp"

#include <fmt/core.h>

using namespace srb2;
using namespace rhi;

ShaderLoadContext::ShaderLoadContext() = default;

void ShaderLoadContext::set_version(std::string_view version)
{
	version_ = fmt::format("#version {}\n", version);
}

void ShaderLoadContext::add_source(const std::string& source)
{
	sources_.push_back(source);
}

void ShaderLoadContext::add_source(std::string&& source)
{
	sources_.push_back(std::move(source));
}

void ShaderLoadContext::define(std::string_view name)
{
	defines_.append(fmt::format("#define {}\n", name));
}

std::vector<const char*> ShaderLoadContext::get_sources_array()
{
	std::vector<const char*> ret;

	ret.push_back(version_.c_str());
	ret.push_back(defines_.c_str());
	for (auto& source : sources_)
	{
		ret.push_back(source.c_str());
	}

	return ret;
}
