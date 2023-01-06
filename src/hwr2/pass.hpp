#ifndef __SRB2_HWR2_PASS_HPP__
#define __SRB2_HWR2_PASS_HPP__

#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

/// @brief A rendering pass which performs logic during each phase of a frame render.
/// During rendering, all registered Pass's individual stages will be run together.
struct Pass {
	virtual ~Pass();

	/// @brief Perform rendering logic and create necessary GPU resources.
	/// @param rhi
	virtual void prepass(rhi::Rhi& rhi) = 0;

	/// @brief Upload contents for needed GPU resources.
	/// @param rhi
	/// @param ctx
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) = 0;

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
