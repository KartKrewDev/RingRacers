// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "pass_manager.hpp"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

namespace
{

class LambdaPass final : public Pass
{
	PassManager* mgr_;
	std::function<void(PassManager&, rhi::Rhi&)> prepass_func_;
	std::function<void(PassManager&, rhi::Rhi&)> postpass_func_;

public:
	LambdaPass(PassManager* mgr, std::function<void(PassManager&, rhi::Rhi&)> prepass_func);
	LambdaPass(
		PassManager* mgr,
		std::function<void(PassManager&, rhi::Rhi&)> prepass_func,
		std::function<void(PassManager&, rhi::Rhi&)> postpass_func
	);
	virtual ~LambdaPass();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi) override;
	virtual void graphics(rhi::Rhi& rhi) override;
	virtual void postpass(rhi::Rhi& rhi) override;
};

} // namespace

LambdaPass::LambdaPass(PassManager* mgr, std::function<void(PassManager&, rhi::Rhi&)> prepass_func)
	: mgr_(mgr), prepass_func_(prepass_func)
{
}

LambdaPass::LambdaPass(
	PassManager* mgr,
	std::function<void(PassManager&, rhi::Rhi&)> prepass_func,
	std::function<void(PassManager&, rhi::Rhi&)> postpass_func
)
	: mgr_(mgr), prepass_func_(prepass_func), postpass_func_(postpass_func)
{
}

LambdaPass::~LambdaPass() = default;

void LambdaPass::prepass(Rhi& rhi)
{
	if (prepass_func_)
	{
		(prepass_func_)(*mgr_, rhi);
	}
}

void LambdaPass::transfer(Rhi&)
{
}

void LambdaPass::graphics(Rhi&)
{
}

void LambdaPass::postpass(Rhi& rhi)
{
	if (postpass_func_)
	{
		(postpass_func_)(*mgr_, rhi);
	}
}

PassManager::PassManager() = default;
PassManager::PassManager(const PassManager&) = default;
PassManager& PassManager::operator=(const PassManager&) = default;

void PassManager::insert(const std::string& name, std::shared_ptr<Pass> pass)
{
	SRB2_ASSERT(pass_by_name_.find(name) == pass_by_name_.end());

	std::size_t index = passes_.size();
	passes_.push_back(PassManagerEntry {name, pass, true});
	pass_by_name_.insert({name, index});
}

void PassManager::insert(const std::string& name, std::function<void(PassManager&, Rhi&)> prepass_func)
{
	insert(std::forward<const std::string>(name), std::make_shared<LambdaPass>(LambdaPass {this, prepass_func}));
}

void PassManager::insert(
	const std::string& name,
	std::function<void(PassManager&, Rhi&)> prepass_func,
	std::function<void(PassManager&, Rhi&)> postpass_func
)
{
	insert(
		std::forward<const std::string>(name),
		std::make_shared<LambdaPass>(LambdaPass {this, prepass_func, postpass_func})
	);
}

void PassManager::set_pass_enabled(const std::string& name, bool enabled)
{
	SRB2_ASSERT(pass_by_name_.find(name) != pass_by_name_.end());

	passes_[pass_by_name_[name]].enabled = enabled;
}

std::weak_ptr<Pass> PassManager::for_name(const std::string& name)
{
	auto itr = pass_by_name_.find(name);
	if (itr == pass_by_name_.end())
	{
		return std::weak_ptr<Pass>();
	}
	return passes_[itr->second].pass;
}

void PassManager::prepass(Rhi& rhi)
{
	for (auto& pass : passes_)
	{
		if (pass.enabled)
		{
			pass.pass->prepass(rhi);
		}
	}
}

void PassManager::transfer(Rhi& rhi)
{
	for (auto& pass : passes_)
	{
		if (pass.enabled)
		{
			pass.pass->transfer(rhi);
		}
	}
}

void PassManager::graphics(Rhi& rhi)
{
	for (auto& pass : passes_)
	{
		if (pass.enabled)
		{
			pass.pass->graphics(rhi);
		}
	}
}

void PassManager::postpass(Rhi& rhi)
{
	for (auto& pass : passes_)
	{
		if (pass.enabled)
		{
			pass.pass->postpass(rhi);
		}
	}
}

void PassManager::render(Rhi& rhi)
{
	if (passes_.empty())
	{
		return;
	}

	prepass(rhi);

	transfer(rhi);
	graphics(rhi);

	postpass(rhi);
}
