// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
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

struct Gles2FramebufferKey
{
	TextureOrRenderbuffer color;
	std::optional<TextureOrRenderbuffer> depth;

	bool operator==(const Gles2FramebufferKey& rhs) const noexcept { return color == rhs.color && depth == rhs.depth; }

	bool operator!=(const Gles2FramebufferKey& rhs) const noexcept { return !(*this == rhs); }
};

} // namespace srb2::rhi

// To make sure the compiler selects the struct specialization of std::hash for Gles2FramebufferKey,
// we need to split the namespace declarations _before_ the instantiation of std::unordered_map.

template <>
struct std::hash<srb2::rhi::Gles2FramebufferKey>
{
	std::size_t operator()(const srb2::rhi::Gles2FramebufferKey& key) const
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

namespace srb2::rhi
{

/// @brief Platform-specific implementation details for the GLES2 backend.
struct Gles2Platform
{
	virtual ~Gles2Platform();

	virtual void present() = 0;
	virtual std::tuple<std::string, std::string> find_shader_sources(PipelineProgram program) = 0;
	virtual Rect get_default_framebuffer_dimensions() = 0;
};

class Gles2Rhi final : public Rhi
{
	std::unique_ptr<Gles2Platform> platform_;

	Slab<RenderPass> render_pass_slab_;
	Slab<Texture> texture_slab_;
	Slab<Buffer> buffer_slab_;
	Slab<Renderbuffer> renderbuffer_slab_;
	Slab<Pipeline> pipeline_slab_;

	std::unordered_map<Gles2FramebufferKey, uint32_t> framebuffers_ {16};

	struct DefaultRenderPassState
	{
	};
	using RenderPassState = std::variant<DefaultRenderPassState, RenderPassBeginInfo>;
	std::optional<RenderPassState> current_render_pass_;
	std::optional<Handle<Pipeline>> current_pipeline_;
	PrimitiveType current_primitive_type_ = PrimitiveType::kPoints;
	bool graphics_context_active_ = false;
	uint32_t graphics_context_generation_ = 0;
	uint32_t index_buffer_offset_ = 0;

	std::vector<std::function<void()>> disposal_;

public:
	Gles2Rhi(std::unique_ptr<Gles2Platform>&& platform);
	virtual ~Gles2Rhi();

	virtual Handle<RenderPass> create_render_pass(const RenderPassDesc& desc) override;
	virtual void destroy_render_pass(Handle<RenderPass>&& handle) override;
	virtual Handle<Texture>
	create_texture(const TextureDesc& desc, srb2::rhi::PixelFormat data_format, tcb::span<const std::byte> data)
		override;
	virtual void destroy_texture(Handle<Texture>&& handle) override;
	virtual Handle<Buffer> create_buffer(const BufferDesc& desc, tcb::span<const std::byte> data) override;
	virtual void destroy_buffer(Handle<Buffer>&& handle) override;
	virtual Handle<Renderbuffer> create_renderbuffer(const RenderbufferDesc& desc) override;
	virtual void destroy_renderbuffer(Handle<Renderbuffer>&& handle) override;
	virtual Handle<Pipeline> create_pipeline(const PipelineDesc& desc) override;
	virtual void destroy_pipeline(Handle<Pipeline>&& handle) override;

	virtual void
	update_buffer_contents(Handle<Buffer> buffer, uint32_t offset, tcb::span<const std::byte> data) override;
	virtual void update_texture(
		Handle<Texture> texture,
		Rect region,
		srb2::rhi::PixelFormat data_format,
		tcb::span<const std::byte> data
	) override;

	virtual Handle<GraphicsContext> begin_graphics() override;
	virtual void end_graphics(Handle<GraphicsContext>&& ctx) override;

	// Graphics context functions
	virtual void push_default_render_pass(Handle<GraphicsContext> ctx) override;
	virtual void push_render_pass(Handle<GraphicsContext> ctx, const RenderPassBeginInfo& info) override;
	virtual void pop_render_pass(Handle<GraphicsContext> ctx) override;
	virtual void bind_pipeline(Handle<GraphicsContext> ctx, Handle<Pipeline> pipeline) override;
	virtual void update_bindings(Handle<GraphicsContext> ctx, const UpdateBindingsInfo& info) override;
	virtual void update_uniforms(Handle<GraphicsContext> ctx, tcb::span<UniformUpdateData> uniforms) override;
	virtual void set_scissor(Handle<GraphicsContext> ctx, const Rect& rect) override;
	virtual void set_viewport(Handle<GraphicsContext> ctx, const Rect& rect) override;
	virtual void draw(Handle<GraphicsContext> ctx, uint32_t vertex_count, uint32_t first_vertex) override;
	virtual void
	draw_indexed(Handle<GraphicsContext> ctx, uint32_t index_count, uint32_t first_index, uint32_t vertex_offset)
		override;

	virtual void present() override;

	virtual void finish() override;
};

typedef void (*Gles2Proc)(void);
typedef Gles2Proc (*Gles2LoadFunc)(const char* name);

void load_gles2(Gles2LoadFunc func);

} // namespace srb2::rhi

#endif // __SRB2_RHI_GLES2_RHI_HPP__
