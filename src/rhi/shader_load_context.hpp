// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_RHI_SHADER_LOAD_CONTEXT_HPP__
#define __SRB2_RHI_SHADER_LOAD_CONTEXT_HPP__

#include <string>
#include <string_view>
#include <vector>

namespace srb2::rhi
{

class ShaderLoadContext
{
	std::string version_;
	std::string defines_;
	std::vector<std::string> sources_;

public:
	ShaderLoadContext();

	void set_version(std::string_view version);
	void add_source(const std::string& source);
	void add_source(std::string&& source);

	void define(std::string_view name);

	std::vector<const char*> get_sources_array();
};

}; // namespace srb2::rhi

#endif // __SRB2_RHI_SHADER_LOAD_CONTEXT_HPP__
