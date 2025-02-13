// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_HPP__
#define __SRB2_HWR2_PASS_HPP__

#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

/// @brief A rendering pass which performs logic during each phase of a frame render.
/// During rendering, all registered Pass's individual stages will be run together.
class Pass
{
public:
	virtual ~Pass();

	/// @brief Perform rendering logic and create necessary GPU resources.
	/// @param rhi
	virtual void prepass(rhi::Rhi& rhi) = 0;

	/// @brief Upload contents for needed GPU resources. Passes must implement but this will be removed soon.
	/// @param rhi
	/// @param ctx
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) = 0;

	/// @brief Issue draw calls.
	/// @param rhi
	/// @param ctx
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) = 0;

	/// @brief Cleanup GPU resources. Transient resources should be cleaned up here.
	/// @param rhi
	virtual void postpass(rhi::Rhi& rhi) = 0;
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_HPP__
