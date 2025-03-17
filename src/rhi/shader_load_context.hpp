// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_RHI_SHADER_LOAD_CONTEXT_HPP__
#define __SRB2_RHI_SHADER_LOAD_CONTEXT_HPP__

#include <string_view>

#include "../core/string.h"
#include "../core/vector.hpp"

namespace srb2::rhi
{

class ShaderLoadContext
{
	srb2::String version_;
	srb2::String defines_;
	srb2::Vector<srb2::String> sources_;

public:
	ShaderLoadContext();

	void set_version(std::string_view version);
	void add_source(const srb2::String& source);
	void add_source(srb2::String&& source);

	void define(std::string_view name);

	srb2::Vector<const char*> get_sources_array();
};

}; // namespace srb2::rhi

#endif // __SRB2_RHI_SHADER_LOAD_CONTEXT_HPP__
