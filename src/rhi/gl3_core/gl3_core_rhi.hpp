// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_RHI_GLES2_RHI_HPP__
#define __SRB2_RHI_GLES2_RHI_HPP__

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

struct GlCoreFramebufferKey
{
	TextureOrRenderbuffer color;
	std::optional<TextureOrRenderbuffer> depth;

	bool operator==(const GlCoreFramebufferKey& rhs) const noexcept { return color == rhs.color && depth == rhs.depth; }

	bool operator!=(const GlCoreFramebufferKey& rhs) const noexcept { return !(*this == rhs); }
};

} // namespace srb2::rhi

// To make sure the compiler selects the struct specialization of std::hash for GlCoreFramebufferKey,
// we need to split the namespace declarations _before_ the instantiation of std::unordered_map.

template <>
struct std::hash<srb2::rhi::GlCoreFramebufferKey>
{
	std::size_t operator()(const srb2::rhi::GlCoreFramebufferKey& key) const
	{
		struct GetHandleHashVisitor
		{
			uint32_t operator()(const srb2::rhi::Handle<srb2::rhi::Texture>& handle) const noexcept
			{
				return std::hash<srb2::rhi::Handle<srb2::rhi::Texture>>()(handle);
			}
			uint32_t operator()(const srb2::rhi::Handle<srb2::rhi::Renderbuffer>& handle) const noexcept
			{
				return std::hash<srb2::rhi::Handle<srb2::rhi::Renderbuffer>>()(handle);
			}
		};
		std::size_t color_hash = std::visit(GetHandleHashVisitor {}, key.color);
		std::size_t depth_hash = 0;
		if (key.depth)
		{
			depth_hash = std::visit(GetHandleHashVisitor {}, *key.depth);
		}
		return color_hash ^ (depth_hash << 1);
	}
};

struct GladGLContext;

namespace srb2::rhi
{

typedef void (*GlProc)(void);
typedef GlProc (*GlLoadFunc)(const char* name);

/// @brief Platform-specific implementation details for the GLES2 backend.
struct GlCorePlatform
{
	virtual ~GlCorePlatform();

	virtual void present() = 0;
	virtual std::tuple<std::string, std::string> find_shader_sources(PipelineProgram program) = 0;
	virtual Rect get_default_framebuffer_dimensions() = 0;
};

struct GlCoreTexture : public rhi::Texture
{
	uint32_t texture;
	rhi::TextureDesc desc;
};

struct GlCoreBuffer : public rhi::Buffer
{
	uint32_t buffer;
	rhi::BufferDesc desc;
};

struct GlCoreRenderPass : public rhi::RenderPass
{
	rhi::RenderPassDesc desc;
};

struct GlCoreRenderbuffer : public rhi::Renderbuffer
{
	uint32_t renderbuffer;
};

struct GlCoreUniformSet : public rhi::UniformSet
{
	std::vector<rhi::UniformVariant> uniforms;
};

struct GlCoreBindingSet : public rhi::BindingSet
{
	uint32_t vao;
	std::unordered_map<rhi::SamplerName, uint32_t> textures {4};
};

struct GlCorePipeline : public rhi::Pipeline
{
	uint32_t vertex_shader = 0;
	uint32_t fragment_shader = 0;
	uint32_t program = 0;
	std::unordered_map<rhi::VertexAttributeName, uint32_t> attrib_locations {2};
	std::unordered_map<rhi::UniformName, uint32_t> uniform_locations {2};
	std::unordered_map<rhi::SamplerName, uint32_t> sampler_locations {2};
	rhi::PipelineDesc desc;
};

struct GlCoreGraphicsContext : public rhi::GraphicsContext
{
};

struct GlCoreTransferContext : public rhi::TransferContext
{
};

struct GlCoreActiveUniform
{
	uint32_t type;
	uint32_t location;
};

class GlCoreRhi final : public Rhi
{
	std::unique_ptr<GlCorePlatform> platform_;

	std::unique_ptr<GladGLContext> gl_;

	Slab<GlCoreRenderPass> render_pass_slab_;
	Slab<GlCoreTexture> texture_slab_;
	Slab<GlCoreBuffer> buffer_slab_;
	Slab<GlCoreRenderbuffer> renderbuffer_slab_;
	Slab<GlCorePipeline> pipeline_slab_;
	Slab<GlCoreUniformSet> uniform_set_slab_;
	Slab<GlCoreBindingSet> binding_set_slab_;

	Handle<Buffer> current_index_buffer_;

	std::unordered_map<GlCoreFramebufferKey, uint32_t> framebuffers_ {16};

	struct DefaultRenderPassState
	{
	};
	using RenderPassState = std::variant<DefaultRenderPassState, RenderPassBeginInfo>;
	std::optional<RenderPassState> current_render_pass_;
	std::optional<Handle<Pipeline>> current_pipeline_;
	PrimitiveType current_primitive_type_ = PrimitiveType::kPoints;
	bool graphics_context_active_ = false;
	bool transfer_context_active_ = false;
	uint32_t graphics_context_generation_ = 0;
	uint32_t index_buffer_offset_ = 0;
	uint32_t transfer_context_generation_ = 0;

	std::vector<std::function<void()>> disposal_;

public:
	GlCoreRhi(std::unique_ptr<GlCorePlatform>&& platform, GlLoadFunc load_func);
	virtual ~GlCoreRhi();

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

	virtual Handle<TransferContext> begin_transfer() override;
	virtual void end_transfer(Handle<TransferContext> handle) override;

	virtual void update_buffer_contents(
		Handle<TransferContext> ctx,
		Handle<Buffer> buffer,
		uint32_t offset,
		tcb::span<const std::byte> data
	) override;
	virtual void update_texture(
		Handle<TransferContext> ctx,
		Handle<Texture> texture,
		Rect region,
		srb2::rhi::PixelFormat data_format,
		tcb::span<const std::byte> data
	) override;
	virtual Handle<UniformSet>
	create_uniform_set(Handle<TransferContext> ctx, const CreateUniformSetInfo& info) override;
	virtual Handle<BindingSet>
	create_binding_set(Handle<TransferContext> ctx, Handle<Pipeline> pipeline, const CreateBindingSetInfo& info)
		override;

	virtual Handle<GraphicsContext> begin_graphics() override;
	virtual void end_graphics(Handle<GraphicsContext> ctx) override;

	// Graphics context functions
	virtual void begin_default_render_pass(Handle<GraphicsContext> ctx, bool clear) override;
	virtual void begin_render_pass(Handle<GraphicsContext> ctx, const RenderPassBeginInfo& info) override;
	virtual void end_render_pass(Handle<GraphicsContext> ctx) override;
	virtual void bind_pipeline(Handle<GraphicsContext> ctx, Handle<Pipeline> pipeline) override;
	virtual void bind_uniform_set(Handle<GraphicsContext> ctx, uint32_t slot, Handle<UniformSet> set) override;
	virtual void bind_binding_set(Handle<GraphicsContext> ctx, Handle<BindingSet> set) override;
	virtual void bind_index_buffer(Handle<GraphicsContext> ctx, Handle<Buffer> buffer) override;
	virtual void set_scissor(Handle<GraphicsContext> ctx, const Rect& rect) override;
	virtual void set_viewport(Handle<GraphicsContext> ctx, const Rect& rect) override;
	virtual void draw(Handle<GraphicsContext> ctx, uint32_t vertex_count, uint32_t first_vertex) override;
	virtual void draw_indexed(Handle<GraphicsContext> ctx, uint32_t index_count, uint32_t first_index) override;
	virtual void read_pixels(Handle<GraphicsContext> ctx, const Rect& rect, tcb::span<std::byte> out) override;

	virtual void present() override;

	virtual void finish() override;
};

} // namespace srb2::rhi

#endif // __SRB2_RHI_GLES2_RHI_HPP__
