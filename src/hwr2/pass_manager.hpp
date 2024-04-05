// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
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

class PassManager final : public Pass
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
	PassManager(const PassManager&);
	PassManager(PassManager&&) = delete;
	PassManager& operator=(const PassManager&);
	PassManager& operator=(PassManager&&) = delete;

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	void insert(const std::string& name, std::shared_ptr<Pass> pass);
	void insert(const std::string& name, std::function<void(PassManager&, rhi::Rhi&)> prepass_func);
	void insert(
		const std::string& name,
		std::function<void(PassManager&, rhi::Rhi&)> prepass_func,
		std::function<void(PassManager&, rhi::Rhi&)> postpass_func
	);
	std::weak_ptr<Pass> for_name(const std::string& name);
	void set_pass_enabled(const std::string& name, bool enabled);

	void render(rhi::Rhi& rhi);
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_MANAGER_HPP__
