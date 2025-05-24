// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "shader_load_context.hpp"

#include <fmt/core.h>

#include "../core/string.h"
#include "../core/vector.hpp"

using namespace srb2;
using namespace rhi;

ShaderLoadContext::ShaderLoadContext() = default;

void ShaderLoadContext::set_version(std::string_view version)
{
	version_ = srb2::format("#version {}\n", version);
}

void ShaderLoadContext::add_source(const srb2::String& source)
{
	sources_.push_back(source);
}

void ShaderLoadContext::add_source(srb2::String&& source)
{
	sources_.push_back(std::move(source));
}

void ShaderLoadContext::define(std::string_view name)
{
	defines_.append(srb2::format("#define {}\n", name));
}

srb2::Vector<const char*> ShaderLoadContext::get_sources_array()
{
	srb2::Vector<const char*> ret;

	ret.push_back(version_.c_str());
	ret.push_back(defines_.c_str());
	for (auto& source : sources_)
	{
		ret.push_back(source.c_str());
	}

	return ret;
}
