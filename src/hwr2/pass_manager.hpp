// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_MANAGER_HPP__
#define __SRB2_HWR2_PASS_MANAGER_HPP__

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../rhi/rhi.hpp"
#include "pass.hpp"

namespace srb2::hwr2
{

class PassManager
{
	struct PassManagerEntry
	{
		std::string name;
		std::shared_ptr<Pass> pass;
		bool enabled;
	};

	std::unordered_map<std::string, std::size_t> pass_by_name_;
	std::vector<PassManagerEntry> passes_;

public:
	PassManager();
	PassManager(const PassManager&) = delete;
	PassManager(PassManager&&) = delete;
	PassManager& operator=(const PassManager&) = delete;
	PassManager& operator=(PassManager&&) = delete;

	void insert(const std::string& name, std::shared_ptr<Pass> pass);
	void insert(const std::string& name, std::function<void(PassManager&, rhi::Rhi&)> prepass_func);
	void insert(
		const std::string& name,
		std::function<void(PassManager&, rhi::Rhi&)> prepass_func,
		std::function<void(PassManager&, rhi::Rhi&)> postpass_func
	);
	std::weak_ptr<Pass> for_name(const std::string& name);
	void set_pass_enabled(const std::string& name, bool enabled);

	void render(rhi::Rhi& rhi) const;
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_MANAGER_HPP__
