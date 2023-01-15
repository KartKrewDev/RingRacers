#ifndef __SRB2_HWR2_PASS_SOFTWARE_HPP__
#define __SRB2_HWR2_PASS_SOFTWARE_HPP__

#include <array>

#include "../rhi/rhi.hpp"
#include "pass.hpp"

namespace srb2::hwr2
{

class SoftwareBlitPass : public Pass
{
	rhi::Handle<rhi::Pipeline> pipeline_;
	rhi::Handle<rhi::Texture> screen_tex_;
	rhi::Handle<rhi::Texture> palette_tex_;
	rhi::Handle<rhi::Buffer> quad_vbo_;
	rhi::Handle<rhi::Buffer> quad_ibo_;
	std::array<rhi::Handle<rhi::UniformSet>, 2> uniform_sets_;
	rhi::Handle<rhi::BindingSet> binding_set_;

	uint32_t screen_tex_width_ = 0;
	uint32_t screen_tex_height_ = 0;
	bool quad_vbo_needs_upload_ = false;
	bool quad_ibo_needs_upload_ = false;

	void upload_screen(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx);
	void upload_palette(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx);

public:
	virtual ~SoftwareBlitPass();

	virtual void prepass(rhi::Rhi& rhi) override;

	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;

	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;

	virtual void postpass(rhi::Rhi& rhi) override;
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_SOFTWARE_HPP__
