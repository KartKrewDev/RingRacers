// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_RHI_RHI_HPP__
#define __SRB2_RHI_RHI_HPP__

#include <cstdint>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <tcb/span.hpp>

#include "../core/static_vec.hpp"
#include "handle.hpp"

namespace srb2::rhi
{

struct Buffer
{
};

/// @brief Sampler image source or color image attachment.
struct Texture
{
};

struct Pipeline
{
};

struct RenderPass
{
};

/// @brief Depth-stencil image attachment.
struct Renderbuffer
{
};

enum class VertexAttributeFormat
{
	kFloat,
	kFloat2,
	kFloat3,
	kFloat4
};

enum class UniformFormat
{
	kFloat,
	kFloat2,
	kFloat3,
	kFloat4,
	kInt,
	kInt2,
	kInt3,
	kInt4,
	kMat2,
	kMat3,
	kMat4
};

enum class PixelFormat
{
	kR8,
	kRG8,
	kRGB8,
	kRGBA8,
	kDepth16,
	kStencil8
};

enum class TextureFormat
{
	kLuminance,
	kLuminanceAlpha,
	kRGB,
	kRGBA
};

enum class CompareFunc
{
	kNever,
	kLess,
	kEqual,
	kLessEqual,
	kGreater,
	kNotEqual,
	kGreaterEqual,
	kAlways
};

enum class BlendFactor
{
	kZero,
	kOne,
	kSource,
	kOneMinusSource,
	kSourceAlpha,
	kOneMinusSourceAlpha,
	kDest,
	kOneMinusDest,
	kDestAlpha,
	kOneMinusDestAlpha,
	kConstant,
	kOneMinusConstant,
	kConstantAlpha,
	kOneMinusConstantAlpha,
	kSourceAlphaSaturated
};

enum class BlendFunction
{
	kAdd,
	kSubtract,
	kReverseSubtract
};

enum class PrimitiveType
{
	kPoints,
	kLines,
	kLineStrip,
	kTriangles,
	kTriangleStrip,
	kTriangleFan
};

enum class CullMode
{
	kNone,
	kFront,
	kBack
};

enum class FaceWinding
{
	kCounterClockwise,
	kClockwise
};

enum class AttachmentLoadOp
{
	kLoad,
	kClear,
	kDontCare
};

enum class AttachmentStoreOp
{
	kStore,
	kDontCare
};

enum class PipelineProgram
{
	kUnshaded,
	kUnshadedPaletted,
	kPostprocessWipe
};

enum class BufferType
{
	kVertexBuffer,
	kIndexBuffer
};

enum class BufferUsage
{
	kImmutable,
	kDynamic
};

enum class VertexAttributeName
{
	kPosition,
	kNormal,
	kTexCoord0,
	kTexCoord1,
	kColor
};

enum class UniformName
{
	kTime,
	kModelView,
	kProjection,
	kTexCoord0Transform,
	kTexCoord1Transform,
	kSampler0IsIndexedAlpha,
	kSampler1IsIndexedAlpha,
	kSampler2IsIndexedAlpha,
	kSampler3IsIndexedAlpha,
	kSampler0Size,
	kSampler1Size,
	kSampler2Size,
	kSampler3Size,
	kWipeColorizeMode,
	kWipeEncoreSwizzle
};

enum class SamplerName
{
	kSampler0,
	kSampler1,
	kSampler2,
	kSampler3
};

struct Rect
{
	int32_t x;
	int32_t y;
	uint32_t w;
	uint32_t h;
};

constexpr const size_t kMaxVertexAttributes = 8;
constexpr const size_t kMaxSamplers = 4;

struct ProgramVertexInput
{
	VertexAttributeName name;
	VertexAttributeFormat type;
	bool required;
};

struct ProgramUniformInput
{
	UniformName name;
	bool required;
};

struct ProgramSamplerInput
{
	SamplerName name;
	bool required;
};

struct ProgramVertexInputRequirements
{
	srb2::StaticVec<ProgramVertexInput, kMaxVertexAttributes> attributes;
};

struct ProgramUniformRequirements
{
	srb2::StaticVec<srb2::StaticVec<ProgramUniformInput, 16>, 4> uniform_groups;
};

struct ProgramSamplerRequirements
{
	srb2::StaticVec<ProgramSamplerInput, kMaxSamplers> samplers;
};

struct ProgramRequirements
{
	ProgramVertexInputRequirements vertex_input;
	ProgramUniformRequirements uniforms;
	ProgramSamplerRequirements samplers;
};

extern const ProgramRequirements kProgramRequirementsUnshaded;
extern const ProgramRequirements kProgramRequirementsUnshadedPaletted;
extern const ProgramRequirements kProgramRequirementsPostprocessWipe;

const ProgramRequirements& program_requirements_for_program(PipelineProgram program) noexcept;

inline constexpr const VertexAttributeFormat vertex_attribute_format(VertexAttributeName name) noexcept
{
	switch (name)
	{
	case VertexAttributeName::kPosition:
		return VertexAttributeFormat::kFloat3;
	case VertexAttributeName::kNormal:
		return VertexAttributeFormat::kFloat3;
	case VertexAttributeName::kTexCoord0:
		return VertexAttributeFormat::kFloat2;
	case VertexAttributeName::kTexCoord1:
		return VertexAttributeFormat::kFloat2;
	case VertexAttributeName::kColor:
		return VertexAttributeFormat::kFloat4;
	default:
		return VertexAttributeFormat::kFloat;
	};
}

inline constexpr const UniformFormat uniform_format(UniformName name) noexcept
{
	switch (name)
	{
	case UniformName::kTime:
		return UniformFormat::kFloat;
	case UniformName::kModelView:
		return UniformFormat::kMat4;
	case UniformName::kProjection:
		return UniformFormat::kMat4;
	case UniformName::kTexCoord0Transform:
		return UniformFormat::kMat3;
	case UniformName::kTexCoord1Transform:
		return UniformFormat::kMat3;
	case UniformName::kSampler0IsIndexedAlpha:
		return UniformFormat::kInt;
	case UniformName::kSampler1IsIndexedAlpha:
		return UniformFormat::kInt;
	case UniformName::kSampler2IsIndexedAlpha:
		return UniformFormat::kInt;
	case UniformName::kSampler3IsIndexedAlpha:
		return UniformFormat::kInt;
	case UniformName::kSampler0Size:
		return UniformFormat::kFloat2;
	case UniformName::kSampler1Size:
		return UniformFormat::kFloat2;
	case UniformName::kSampler2Size:
		return UniformFormat::kFloat2;
	case UniformName::kSampler3Size:
		return UniformFormat::kFloat2;
	case UniformName::kWipeColorizeMode:
		return UniformFormat::kInt;
	case UniformName::kWipeEncoreSwizzle:
		return UniformFormat::kInt;
	default:
		return UniformFormat::kFloat;
	}
}

struct VertexBufferLayoutDesc
{
	uint32_t stride;
};

struct VertexAttributeLayoutDesc
{
	VertexAttributeName name;
	uint32_t buffer_index;
	uint32_t offset;
};

// constexpr const size_t kMaxVertexBufferBindings = 4;

struct VertexInputDesc
{
	srb2::StaticVec<VertexBufferLayoutDesc, 4> buffer_layouts;
	srb2::StaticVec<VertexAttributeLayoutDesc, 8> attr_layouts;
};

struct UniformInputDesc
{
	srb2::StaticVec<srb2::StaticVec<UniformName, 16>, 4> enabled_uniforms;
};

struct SamplerInputDesc
{
	std::vector<SamplerName> enabled_samplers;
};

struct ColorMask
{
	bool r;
	bool g;
	bool b;
	bool a;
};

struct BlendDesc
{
	BlendFactor source_factor_color;
	BlendFactor dest_factor_color;
	BlendFunction color_function;
	BlendFactor source_factor_alpha;
	BlendFactor dest_factor_alpha;
	BlendFunction alpha_function;
};

enum class StencilOp
{
	kKeep,
	kZero,
	kReplace,
	kIncrementClamp,
	kDecrementClamp,
	kInvert,
	kIncrementWrap,
	kDecrementWrap
};

struct PipelineStencilOpStateDesc
{
	StencilOp fail;
	StencilOp pass;
	StencilOp depth_fail;
	CompareFunc stencil_compare;
	uint32_t compare_mask;
	uint32_t write_mask;
	uint32_t reference;
};

struct PipelineDepthStencilStateDesc
{
	bool depth_test;
	bool depth_write;
	CompareFunc depth_func;
	bool stencil_test;
	PipelineStencilOpStateDesc front;
	PipelineStencilOpStateDesc back;
};

struct PipelineColorStateDesc
{
	std::optional<BlendDesc> blend;
	ColorMask color_mask;
};

struct PipelineDesc
{
	PipelineProgram program;
	VertexInputDesc vertex_input;
	UniformInputDesc uniform_input;
	SamplerInputDesc sampler_input;
	std::optional<PipelineDepthStencilStateDesc> depth_stencil_state;
	PipelineColorStateDesc color_state;
	PrimitiveType primitive;
	CullMode cull;
	FaceWinding winding;
	glm::vec4 blend_color;
};

struct RenderPassDesc
{
	bool use_depth_stencil;
	AttachmentLoadOp color_load_op;
	AttachmentStoreOp color_store_op;
	AttachmentLoadOp depth_load_op;
	AttachmentStoreOp depth_store_op;
	AttachmentLoadOp stencil_load_op;
	AttachmentStoreOp stencil_store_op;
};

struct RenderbufferDesc
{
	uint32_t width;
	uint32_t height;
};

enum class TextureWrapMode
{
	kRepeat,
	kMirroredRepeat,
	kClamp
};

struct TextureDesc
{
	TextureFormat format;
	uint32_t width;
	uint32_t height;
	TextureWrapMode u_wrap;
	TextureWrapMode v_wrap;
};

struct BufferDesc
{
	uint32_t size;
	BufferType type;
	BufferUsage usage;
};

struct RenderPassBeginInfo
{
	Handle<RenderPass> render_pass;
	Handle<Texture> color_attachment;
	std::optional<Handle<Renderbuffer>> depth_stencil_attachment;
	glm::vec4 clear_color;
};

using UniformVariant = std::variant<
	float,
	glm::vec2,
	glm::vec3,
	glm::vec4,

	int32_t,
	glm::ivec2,
	glm::ivec3,
	glm::ivec4,

	glm::mat2,
	glm::mat3,
	glm::mat4
>;

inline constexpr UniformFormat uniform_variant_format(const UniformVariant& variant)
{
	struct Visitor
	{
		UniformFormat operator()(const float&) const noexcept { return UniformFormat::kFloat; }
		UniformFormat operator()(const glm::vec2&) const noexcept { return UniformFormat::kFloat2; }
		UniformFormat operator()(const glm::vec3&) const noexcept { return UniformFormat::kFloat3; }
		UniformFormat operator()(const glm::vec4&) const noexcept { return UniformFormat::kFloat4; }
		UniformFormat operator()(const int32_t&) const noexcept { return UniformFormat::kInt; }
		UniformFormat operator()(const glm::ivec2&) const noexcept { return UniformFormat::kInt2; }
		UniformFormat operator()(const glm::ivec3&) const noexcept { return UniformFormat::kInt3; }
		UniformFormat operator()(const glm::ivec4&) const noexcept { return UniformFormat::kInt4; }
		UniformFormat operator()(const glm::mat2&) const noexcept { return UniformFormat::kMat2; }
		UniformFormat operator()(const glm::mat3&) const noexcept { return UniformFormat::kMat3; }
		UniformFormat operator()(const glm::mat4&) const noexcept { return UniformFormat::kMat4; }
	};
	return std::visit(Visitor {}, variant);
}

struct VertexAttributeBufferBinding
{
	uint32_t attribute_index;
	Handle<Buffer> vertex_buffer;
};

struct TextureBinding
{
	SamplerName name;
	Handle<Texture> texture;
};

struct CreateUniformSetInfo
{
	tcb::span<UniformVariant> uniforms;
};

struct CreateBindingSetInfo
{
	tcb::span<VertexAttributeBufferBinding> vertex_buffers;
	tcb::span<TextureBinding> sampler_textures;
};

struct UniformSet
{
};
struct BindingSet
{
};

struct TransferContext
{
};
struct GraphicsContext
{
};

struct TextureDetails
{
	uint32_t width;
	uint32_t height;
	TextureFormat format;
};

/// @brief The unpack alignment of a row span when uploading pixels to the device.
constexpr const std::size_t kPixelRowUnpackAlignment = 4;

/// @brief An active handle to a rendering device.
struct Rhi
{
	virtual ~Rhi();

	virtual Handle<RenderPass> create_render_pass(const RenderPassDesc& desc) = 0;
	virtual void destroy_render_pass(Handle<RenderPass> handle) = 0;
	virtual Handle<Pipeline> create_pipeline(const PipelineDesc& desc) = 0;
	virtual void destroy_pipeline(Handle<Pipeline> handle) = 0;

	virtual Handle<Texture> create_texture(const TextureDesc& desc) = 0;
	virtual void destroy_texture(Handle<Texture> handle) = 0;
	virtual Handle<Buffer> create_buffer(const BufferDesc& desc) = 0;
	virtual void destroy_buffer(Handle<Buffer> handle) = 0;
	virtual Handle<Renderbuffer> create_renderbuffer(const RenderbufferDesc& desc) = 0;
	virtual void destroy_renderbuffer(Handle<Renderbuffer> handle) = 0;

	virtual TextureDetails get_texture_details(Handle<Texture> texture) = 0;
	virtual Rect get_renderbuffer_size(Handle<Renderbuffer> renderbuffer) = 0;

	virtual Handle<TransferContext> begin_transfer() = 0;
	virtual void end_transfer(Handle<TransferContext> handle) = 0;

	// Transfer Context functions
	virtual void update_buffer(
		Handle<TransferContext> ctx,
		Handle<Buffer> buffer,
		uint32_t offset,
		tcb::span<const std::byte> data
	) = 0;
	virtual void update_texture(
		Handle<TransferContext> ctx,
		Handle<Texture> texture,
		Rect region,
		srb2::rhi::PixelFormat data_format,
		tcb::span<const std::byte> data
	) = 0;
	virtual Handle<UniformSet> create_uniform_set(Handle<TransferContext> ctx, const CreateUniformSetInfo& info) = 0;
	virtual Handle<BindingSet>
	create_binding_set(Handle<TransferContext> ctx, Handle<Pipeline> pipeline, const CreateBindingSetInfo& info) = 0;

	virtual Handle<GraphicsContext> begin_graphics() = 0;
	virtual void end_graphics(Handle<GraphicsContext> ctx) = 0;

	// Graphics context functions
	virtual void begin_default_render_pass(Handle<GraphicsContext> ctx, bool clear) = 0;
	virtual void begin_render_pass(Handle<GraphicsContext> ctx, const RenderPassBeginInfo& info) = 0;
	virtual void end_render_pass(Handle<GraphicsContext> ctx) = 0;
	virtual void bind_pipeline(Handle<GraphicsContext> ctx, Handle<Pipeline> pipeline) = 0;
	virtual void bind_uniform_set(Handle<GraphicsContext> ctx, uint32_t slot, Handle<UniformSet> set) = 0;
	virtual void bind_binding_set(Handle<GraphicsContext> ctx, Handle<BindingSet> set) = 0;
	virtual void bind_index_buffer(Handle<GraphicsContext> ctx, Handle<Buffer> buffer) = 0;
	virtual void set_scissor(Handle<GraphicsContext> ctx, const Rect& rect) = 0;
	virtual void set_viewport(Handle<GraphicsContext> ctx, const Rect& rect) = 0;
	virtual void draw(Handle<GraphicsContext> ctx, uint32_t vertex_count, uint32_t first_vertex) = 0;
	virtual void draw_indexed(Handle<GraphicsContext> ctx, uint32_t index_count, uint32_t first_index) = 0;
	virtual void
	read_pixels(Handle<GraphicsContext> ctx, const Rect& rect, PixelFormat format, tcb::span<std::byte> out) = 0;

	virtual void present() = 0;

	virtual void finish() = 0;
};

} // namespace srb2::rhi

#endif // __SRB2_RHI_RHI_HPP__
