// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_RHI_GL2_RHI_HPP__
#define __SRB2_RHI_GL2_RHI_HPP__

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../rhi.hpp"

namespace srb2::rhi
{

struct Gl2FramebufferKey
{
	Handle<Texture> color;
	std::optional<Handle<Renderbuffer>> depth_stencil;

	bool operator==(const Gl2FramebufferKey& rhs) const noexcept
	{
		return color == rhs.color && depth_stencil == rhs.depth_stencil;
	}

	bool operator!=(const Gl2FramebufferKey& rhs) const noexcept { return !(*this == rhs); }
};

} // namespace srb2::rhi

// To make sure the compiler selects the struct specialization of std::hash for Gl2FramebufferKey,
// we need to split the namespace declarations _before_ the instantiation of std::unordered_map.

template <>
struct std::hash<srb2::rhi::Gl2FramebufferKey>
{
	std::size_t operator()(const srb2::rhi::Gl2FramebufferKey& key) const
	{
		std::size_t color_hash = std::hash<srb2::rhi::Handle<srb2::rhi::Texture>>()(key.color);
		std::size_t depth_stencil_hash = 0;
		if (key.depth_stencil)
		{
			depth_stencil_hash = std::hash<srb2::rhi::Handle<srb2::rhi::Renderbuffer>>()(*key.depth_stencil);
		}
		return color_hash ^ (depth_stencil_hash << 1);
	}
};

struct GladGLContext;

namespace srb2::rhi
{

typedef void (*GlProc)(void);
typedef GlProc (*GlLoadFunc)(const char* name);

/// @brief Platform-specific implementation details for the GL2 backend.
struct Gl2Platform
{
	virtual ~Gl2Platform();

	virtual void present() = 0;
	virtual std::tuple<std::vector<std::string>, std::vector<std::string>> find_shader_sources(PipelineProgram program) = 0;
	virtual Rect get_default_framebuffer_dimensions() = 0;
};

struct Gl2Texture : public rhi::Texture
{
	uint32_t texture;
	rhi::TextureDesc desc;
};

struct Gl2Buffer : public rhi::Buffer
{
	uint32_t buffer;
	rhi::BufferDesc desc;
};

struct Gl2RenderPass : public rhi::RenderPass
{
	rhi::RenderPassDesc desc;
};

struct Gl2Renderbuffer : public rhi::Renderbuffer
{
	uint32_t renderbuffer;
	rhi::RenderbufferDesc desc;
};

struct Gl2UniformSet : public rhi::UniformSet
{
	std::vector<rhi::UniformVariant> uniforms;
};

struct Gl2BindingSet : public rhi::BindingSet
{
	std::vector<rhi::VertexAttributeBufferBinding> vertex_buffer_bindings;
	std::unordered_map<rhi::SamplerName, uint32_t> textures {4};
};

struct Gl2Pipeline : public rhi::Pipeline
{
	uint32_t vertex_shader = 0;
	uint32_t fragment_shader = 0;
	uint32_t program = 0;
	std::unordered_map<rhi::VertexAttributeName, uint32_t> attrib_locations {2};
	std::unordered_map<rhi::UniformName, uint32_t> uniform_locations {2};
	std::unordered_map<rhi::SamplerName, uint32_t> sampler_locations {2};
	rhi::PipelineDesc desc;
};

struct Gl2ActiveUniform
{
	uint32_t type;
	uint32_t location;
};

class Gl2Rhi final : public Rhi
{
	std::unique_ptr<Gl2Platform> platform_;

	std::unique_ptr<GladGLContext> gl_;

	Slab<Gl2RenderPass> render_pass_slab_;
	Slab<Gl2Texture> texture_slab_;
	Slab<Gl2Buffer> buffer_slab_;
	Slab<Gl2Renderbuffer> renderbuffer_slab_;
	Slab<Gl2Pipeline> pipeline_slab_;
	Slab<Gl2UniformSet> uniform_set_slab_;
	Slab<Gl2BindingSet> binding_set_slab_;

	Handle<Buffer> current_index_buffer_;

	std::unordered_map<Gl2FramebufferKey, uint32_t> framebuffers_ {16};

	struct DefaultRenderPassState
	{
	};
	using RenderPassState = std::variant<DefaultRenderPassState, RenderPassBeginInfo>;
	std::optional<RenderPassState> current_render_pass_;
	std::optional<Handle<Pipeline>> current_pipeline_;
	PrimitiveType current_primitive_type_ = PrimitiveType::kPoints;
	uint32_t index_buffer_offset_ = 0;

	uint8_t stencil_front_reference_ = 0;
	uint8_t stencil_front_compare_mask_ = 0xFF;
	uint8_t stencil_front_write_mask_ = 0xFF;
	uint8_t stencil_back_reference_ = 0;
	uint8_t stencil_back_compare_mask_ = 0xFF;
	uint8_t stencil_back_write_mask_ = 0xFF;

public:
	Gl2Rhi(std::unique_ptr<Gl2Platform>&& platform, GlLoadFunc load_func);
	virtual ~Gl2Rhi();

	virtual Handle<RenderPass> create_render_pass(const RenderPassDesc& desc) override;
	virtual void destroy_render_pass(Handle<RenderPass> handle) override;
	virtual Handle<Pipeline> create_pipeline(const PipelineDesc& desc) override;
	virtual void destroy_pipeline(Handle<Pipeline> handle) override;

	virtual Handle<Texture> create_texture(const TextureDesc& desc) override;
	virtual void destroy_texture(Handle<Texture> handle) override;
	virtual Handle<Buffer> create_buffer(const BufferDesc& desc) override;
	virtual void destroy_buffer(Handle<Buffer> handle) override;
	virtual Handle<Renderbuffer> create_renderbuffer(const RenderbufferDesc& desc) override;
	virtual void destroy_renderbuffer(Handle<Renderbuffer> handle) override;

	virtual TextureDetails get_texture_details(Handle<Texture> texture) override;
	virtual Rect get_renderbuffer_size(Handle<Renderbuffer> renderbuffer) override;
	virtual uint32_t get_buffer_size(Handle<Buffer> buffer) override;

	virtual void update_buffer(
		Handle<Buffer> buffer,
		uint32_t offset,
		tcb::span<const std::byte> data
	) override;
	virtual void update_texture(
		Handle<Texture> texture,
		Rect region,
		srb2::rhi::PixelFormat data_format,
		tcb::span<const std::byte> data
	) override;
	virtual void update_texture_settings(
		Handle<Texture> texture,
		TextureWrapMode u_wrap,
		TextureWrapMode v_wrap,
		TextureFilterMode min,
		TextureFilterMode mag
	) override;
	virtual Handle<UniformSet>
	create_uniform_set(const CreateUniformSetInfo& info) override;
	virtual Handle<BindingSet>
	create_binding_set(Handle<Pipeline> pipeline, const CreateBindingSetInfo& info)
		override;

	// Graphics context functions
	virtual void begin_default_render_pass(bool clear) override;
	virtual void begin_render_pass(const RenderPassBeginInfo& info) override;
	virtual void end_render_pass() override;
	virtual void bind_pipeline(Handle<Pipeline> pipeline) override;
	virtual void bind_uniform_set(uint32_t slot, Handle<UniformSet> set) override;
	virtual void bind_binding_set(Handle<BindingSet> set) override;
	virtual void bind_index_buffer(Handle<Buffer> buffer) override;
	virtual void set_scissor(const Rect& rect) override;
	virtual void set_viewport(const Rect& rect) override;
	virtual void draw(uint32_t vertex_count, uint32_t first_vertex) override;
	virtual void draw_indexed(uint32_t index_count, uint32_t first_index) override;
	virtual void
	read_pixels(const Rect& rect, PixelFormat format, tcb::span<std::byte> out) override;
	virtual void copy_framebuffer_to_texture(
		Handle<Texture> dst_tex,
		const Rect& dst_region,
		const Rect& src_region
	) override;
	virtual void set_stencil_reference(CullMode face, uint8_t reference) override;
	virtual void set_stencil_compare_mask(CullMode face, uint8_t mask) override;
	virtual void set_stencil_write_mask(CullMode face, uint8_t mask) override;

	virtual void present() override;

	virtual void finish() override;
};

} // namespace srb2::rhi

#endif // __SRB2_RHI_GL2_RHI_HPP__
