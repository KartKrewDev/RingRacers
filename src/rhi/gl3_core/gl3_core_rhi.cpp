// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "gl3_core_rhi.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>
#include <glad/gl.h>

using namespace srb2;
using namespace rhi;

#ifndef NDEBUG
#define GL_ASSERT                                                                                                      \
	{                                                                                                                  \
		GLenum __err = gl_->GetError();                                                                                \
		if (__err != GL_NO_ERROR)                                                                                      \
		{                                                                                                              \
			I_Error("GL Error at %s %d: %d", __FILE__, __LINE__, __err);                                               \
		}                                                                                                              \
	}
#else
#define GL_ASSERT ;
#endif

namespace
{

constexpr GLenum map_pixel_format(rhi::PixelFormat format)
{
	switch (format)
	{
	case rhi::PixelFormat::kR8:
		return GL_R8;
	case rhi::PixelFormat::kRG8:
		return GL_RG8;
	case rhi::PixelFormat::kRGB8:
		return GL_RGB8;
	case rhi::PixelFormat::kRGBA8:
		return GL_RGBA8;
	case rhi::PixelFormat::kDepth16:
		return GL_DEPTH_COMPONENT16;
	case rhi::PixelFormat::kStencil8:
		return GL_STENCIL_INDEX8;
	default:
		return GL_ZERO;
	}
}

constexpr std::tuple<GLenum, GLenum, GLuint> map_pixel_data_format(rhi::PixelFormat format)
{
	GLenum layout = GL_ZERO;
	GLenum type = GL_ZERO;
	GLuint size = 0;
	switch (format)
	{
	case rhi::PixelFormat::kR8:
		layout = GL_RED;
		type = GL_UNSIGNED_BYTE;
		size = 1;
		break;
	case rhi::PixelFormat::kRG8:
		layout = GL_RG;
		type = GL_UNSIGNED_BYTE;
		size = 2;
		break;
	case rhi::PixelFormat::kRGB8:
		layout = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		size = 3;
		break;
	case rhi::PixelFormat::kRGBA8:
		layout = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		size = 4;
		break;
	default:
		break;
	}
	return std::tuple(layout, type, size);
}

constexpr GLenum map_texture_format(rhi::TextureFormat format)
{
	switch (format)
	{
	case rhi::TextureFormat::kRGBA:
		return GL_RGBA;
	case rhi::TextureFormat::kRGB:
		return GL_RGB;
	case rhi::TextureFormat::kLuminance:
		return GL_RED;
	case rhi::TextureFormat::kLuminanceAlpha:
		return GL_RG;
	default:
		return GL_ZERO;
	}
}

constexpr GLenum map_internal_texture_format(rhi::TextureFormat format)
{
	switch (format)
	{
	case rhi::TextureFormat::kRGBA:
		return GL_RGBA8;
	case rhi::TextureFormat::kRGB:
		return GL_RGB8;
	case rhi::TextureFormat::kLuminance:
		return GL_R8;
	case rhi::TextureFormat::kLuminanceAlpha:
		return GL_RG8;
	case rhi::TextureFormat::kDepth:
		return GL_DEPTH_COMPONENT24;
	default:
		return GL_ZERO;
	}
}

constexpr GLenum map_buffer_type(rhi::BufferType type)
{
	switch (type)
	{
	case rhi::BufferType::kVertexBuffer:
		return GL_ARRAY_BUFFER;
	case rhi::BufferType::kIndexBuffer:
		return GL_ELEMENT_ARRAY_BUFFER;
	default:
		return GL_ZERO;
	}
}

constexpr GLenum map_buffer_usage(rhi::BufferUsage usage)
{
	switch (usage)
	{
	case rhi::BufferUsage::kImmutable:
		return GL_STATIC_DRAW;
	case rhi::BufferUsage::kDynamic:
		return GL_DYNAMIC_DRAW;
	default:
		return GL_ZERO;
	}
}

constexpr GLenum map_compare_func(rhi::CompareFunc func)
{
	switch (func)
	{
	case rhi::CompareFunc::kNever:
		return GL_NEVER;
	case rhi::CompareFunc::kLess:
		return GL_LESS;
	case rhi::CompareFunc::kEqual:
		return GL_EQUAL;
	case rhi::CompareFunc::kLessEqual:
		return GL_LEQUAL;
	case rhi::CompareFunc::kGreater:
		return GL_GREATER;
	case rhi::CompareFunc::kNotEqual:
		return GL_NOTEQUAL;
	case rhi::CompareFunc::kGreaterEqual:
		return GL_GEQUAL;
	case rhi::CompareFunc::kAlways:
		return GL_ALWAYS;
	default:
		return GL_ZERO;
	}
}

constexpr GLenum map_blend_factor(rhi::BlendFactor factor)
{
	switch (factor)
	{
	case rhi::BlendFactor::kZero:
		return GL_ZERO;
	case rhi::BlendFactor::kOne:
		return GL_ONE;
	case rhi::BlendFactor::kSource:
		return GL_SRC_COLOR;
	case rhi::BlendFactor::kOneMinusSource:
		return GL_ONE_MINUS_SRC_COLOR;
	case rhi::BlendFactor::kSourceAlpha:
		return GL_SRC_ALPHA;
	case rhi::BlendFactor::kOneMinusSourceAlpha:
		return GL_ONE_MINUS_SRC_ALPHA;
	case rhi::BlendFactor::kDest:
		return GL_DST_COLOR;
	case rhi::BlendFactor::kOneMinusDest:
		return GL_ONE_MINUS_DST_COLOR;
	case rhi::BlendFactor::kDestAlpha:
		return GL_DST_ALPHA;
	case rhi::BlendFactor::kOneMinusDestAlpha:
		return GL_ONE_MINUS_DST_ALPHA;
	case rhi::BlendFactor::kConstant:
		return GL_CONSTANT_COLOR;
	case rhi::BlendFactor::kOneMinusConstant:
		return GL_ONE_MINUS_CONSTANT_COLOR;
	case rhi::BlendFactor::kConstantAlpha:
		return GL_CONSTANT_ALPHA;
	case rhi::BlendFactor::kOneMinusConstantAlpha:
		return GL_ONE_MINUS_CONSTANT_ALPHA;
	case rhi::BlendFactor::kSourceAlphaSaturated:
		return GL_SRC_ALPHA_SATURATE;
	default:
		return GL_ONE;
	}
}

constexpr GLenum map_blend_function(rhi::BlendFunction function)
{
	switch (function)
	{
	case rhi::BlendFunction::kAdd:
		return GL_FUNC_ADD;
	case rhi::BlendFunction::kSubtract:
		return GL_FUNC_SUBTRACT;
	case rhi::BlendFunction::kReverseSubtract:
		return GL_FUNC_REVERSE_SUBTRACT;
	default:
		return GL_FUNC_ADD;
	}
}

constexpr GLenum map_cull_mode(rhi::CullMode mode)
{
	switch (mode)
	{
	case rhi::CullMode::kNone:
		return GL_NONE;
	case rhi::CullMode::kFront:
		return GL_FRONT;
	case rhi::CullMode::kBack:
		return GL_BACK;
	default:
		return GL_NONE;
	}
}

constexpr GLenum map_winding(rhi::FaceWinding winding)
{
	switch (winding)
	{
	case rhi::FaceWinding::kCounterClockwise:
		return GL_CCW;
	case rhi::FaceWinding::kClockwise:
		return GL_CW;
	default:
		return GL_CCW;
	}
}

constexpr GLenum map_primitive_mode(rhi::PrimitiveType type)
{
	switch (type)
	{
	case rhi::PrimitiveType::kPoints:
		return GL_POINTS;
	case rhi::PrimitiveType::kLines:
		return GL_LINES;
	case rhi::PrimitiveType::kLineStrip:
		return GL_LINE_STRIP;
	case rhi::PrimitiveType::kTriangles:
		return GL_TRIANGLES;
	case rhi::PrimitiveType::kTriangleStrip:
		return GL_TRIANGLE_STRIP;
	case rhi::PrimitiveType::kTriangleFan:
		return GL_TRIANGLE_FAN;
	default:
		return GL_ZERO;
	}
}

constexpr const char* map_vertex_attribute_symbol_name(rhi::VertexAttributeName name)
{
	switch (name)
	{
	case rhi::VertexAttributeName::kPosition:
		return "a_position";
	case rhi::VertexAttributeName::kNormal:
		return "a_normal";
	case rhi::VertexAttributeName::kColor:
		return "a_color";
	case rhi::VertexAttributeName::kTexCoord0:
		return "a_texcoord0";
	case rhi::VertexAttributeName::kTexCoord1:
		return "a_texcoord1";
	default:
		return nullptr;
	}
}

constexpr const char* map_vertex_attribute_enable_define(rhi::VertexAttributeName name)
{
	switch (name)
	{
	case rhi::VertexAttributeName::kPosition:
		return "ENABLE_VA_POSITION";
	case rhi::VertexAttributeName::kNormal:
		return "ENABLE_VA_NORMAL";
	case rhi::VertexAttributeName::kColor:
		return "ENABLE_VA_COLOR";
	case rhi::VertexAttributeName::kTexCoord0:
		return "ENABLE_VA_TEXCOORD0";
	case rhi::VertexAttributeName::kTexCoord1:
		return "ENABLE_VA_TEXCOORD1";
	default:
		return nullptr;
	}
}

constexpr const char* map_uniform_attribute_symbol_name(rhi::UniformName name)
{
	switch (name)
	{
	case rhi::UniformName::kTime:
		return "u_time";
	case rhi::UniformName::kModelView:
		return "u_modelview";
	case rhi::UniformName::kProjection:
		return "u_projection";
	case rhi::UniformName::kTexCoord0Transform:
		return "u_texcoord0_transform";
	case rhi::UniformName::kSampler0IsIndexedAlpha:
		return "u_sampler0_is_indexed_alpha";
	case rhi::UniformName::kWipeColorizeMode:
		return "u_wipe_colorize_mode";
	case rhi::UniformName::kWipeEncoreSwizzle:
		return "u_wipe_encore_swizzle";
	default:
		return nullptr;
	}
}

constexpr const char* map_uniform_enable_define(rhi::UniformName name)
{
	switch (name)
	{
	case rhi::UniformName::kTime:
		return "ENABLE_U_TIME";
	case rhi::UniformName::kProjection:
		return "ENABLE_U_PROJECTION";
	case rhi::UniformName::kModelView:
		return "ENABLE_U_MODELVIEW";
	case rhi::UniformName::kTexCoord0Transform:
		return "ENABLE_U_TEXCOORD0_TRANSFORM";
	case rhi::UniformName::kSampler0IsIndexedAlpha:
		return "ENABLE_U_SAMPLER0_IS_INDEXED_ALPHA";
	case rhi::UniformName::kWipeColorizeMode:
		return "ENABLE_U_WIPE_COLORIZE_MODE";
	case rhi::UniformName::kWipeEncoreSwizzle:
		return "ENABLE_U_WIPE_ENCORE_SWIZZLE";
	default:
		return nullptr;
	}
}

constexpr const char* map_sampler_symbol_name(rhi::SamplerName name)
{
	switch (name)
	{
	case rhi::SamplerName::kSampler0:
		return "s_sampler0";
	case rhi::SamplerName::kSampler1:
		return "s_sampler1";
	case rhi::SamplerName::kSampler2:
		return "s_sampler2";
	case rhi::SamplerName::kSampler3:
		return "s_sampler3";
	default:
		return nullptr;
	}
}

constexpr const char* map_sampler_enable_define(rhi::SamplerName name)
{
	switch (name)
	{
	case rhi::SamplerName::kSampler0:
		return "ENABLE_S_SAMPLER0";
	case rhi::SamplerName::kSampler1:
		return "ENABLE_S_SAMPLER1";
	case rhi::SamplerName::kSampler2:
		return "ENABLE_S_SAMPLER2";
	case rhi::SamplerName::kSampler3:
		return "ENABLE_S_SAMPLER3";
	default:
		return nullptr;
	}
}

constexpr GLenum map_vertex_attribute_format(rhi::VertexAttributeFormat format)
{
	switch (format)
	{
	case rhi::VertexAttributeFormat::kFloat:
		return GL_FLOAT;
	case rhi::VertexAttributeFormat::kFloat2:
		return GL_FLOAT_VEC2;
	case rhi::VertexAttributeFormat::kFloat3:
		return GL_FLOAT_VEC3;
	case rhi::VertexAttributeFormat::kFloat4:
		return GL_FLOAT_VEC4;
	default:
		return GL_ZERO;
	}
}

constexpr GLenum map_vertex_attribute_type(rhi::VertexAttributeFormat format)
{
	switch (format)
	{
	case rhi::VertexAttributeFormat::kFloat:
		return GL_FLOAT;
	case rhi::VertexAttributeFormat::kFloat2:
		return GL_FLOAT;
	case rhi::VertexAttributeFormat::kFloat3:
		return GL_FLOAT;
	case rhi::VertexAttributeFormat::kFloat4:
		return GL_FLOAT;
	default:
		return GL_ZERO;
	}
}

constexpr GLint map_vertex_attribute_format_size(rhi::VertexAttributeFormat format)
{
	switch (format)
	{
	case rhi::VertexAttributeFormat::kFloat:
		return 1;
	case rhi::VertexAttributeFormat::kFloat2:
		return 2;
	case rhi::VertexAttributeFormat::kFloat3:
		return 3;
	case rhi::VertexAttributeFormat::kFloat4:
		return 4;
	default:
		return 0;
	}
}

constexpr GLenum map_uniform_format(rhi::UniformFormat format)
{
	switch (format)
	{
	case rhi::UniformFormat::kFloat:
		return GL_FLOAT;
	case rhi::UniformFormat::kFloat2:
		return GL_FLOAT_VEC2;
	case rhi::UniformFormat::kFloat3:
		return GL_FLOAT_VEC3;
	case rhi::UniformFormat::kFloat4:
		return GL_FLOAT_VEC4;
	case rhi::UniformFormat::kInt:
		return GL_INT;
	case rhi::UniformFormat::kInt2:
		return GL_INT_VEC2;
	case rhi::UniformFormat::kInt3:
		return GL_INT_VEC3;
	case rhi::UniformFormat::kInt4:
		return GL_INT_VEC4;
	case rhi::UniformFormat::kMat2:
		return GL_FLOAT_MAT2;
	case rhi::UniformFormat::kMat3:
		return GL_FLOAT_MAT3;
	case rhi::UniformFormat::kMat4:
		return GL_FLOAT_MAT4;
	default:
		return GL_ZERO;
	}
}

} // namespace

GlCorePlatform::~GlCorePlatform() = default;

GlCoreRhi::GlCoreRhi(std::unique_ptr<GlCorePlatform>&& platform, GlLoadFunc load_func) : platform_(std::move(platform))
{
	gl_ = std::make_unique<GladGLContext>();
	gladLoadGLContext(gl_.get(), load_func);
}

GlCoreRhi::~GlCoreRhi() = default;

rhi::Handle<rhi::RenderPass> GlCoreRhi::create_render_pass(const rhi::RenderPassDesc& desc)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	// GL has no formal render pass object
	GlCoreRenderPass pass;
	pass.desc = desc;
	return render_pass_slab_.insert(std::move(pass));
}

void GlCoreRhi::destroy_render_pass(rhi::Handle<rhi::RenderPass> handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	render_pass_slab_.remove(handle);
}

rhi::Handle<rhi::Texture> GlCoreRhi::create_texture(const rhi::TextureDesc& desc)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	GLenum internal_format = map_internal_texture_format(desc.format);
	SRB2_ASSERT(internal_format != GL_ZERO);
	GLenum format = GL_RGBA;
	if (desc.format == TextureFormat::kDepth)
	{
		format = GL_DEPTH_COMPONENT;
	}

	GLuint name = 0;
	gl_->GenTextures(1, &name);

	gl_->BindTexture(GL_TEXTURE_2D, name);

	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GL_ASSERT
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GL_ASSERT
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	GL_ASSERT
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	GL_ASSERT
	gl_->TexImage2D(GL_TEXTURE_2D, 0, internal_format, desc.width, desc.height, 0, format, GL_UNSIGNED_BYTE, nullptr);
	GL_ASSERT

	GlCoreTexture texture;
	texture.texture = name;
	texture.desc = desc;
	return texture_slab_.insert(std::move(texture));
}

void GlCoreRhi::destroy_texture(rhi::Handle<rhi::Texture> handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(texture_slab_.is_valid(handle) == true);
	GlCoreTexture casted = texture_slab_.remove(handle);
	GLuint name = casted.texture;
	disposal_.push_back([this, name] { gl_->DeleteTextures(1, &name); });
}

void GlCoreRhi::update_texture(
	Handle<TransferContext> ctx,
	Handle<Texture> texture,
	Rect region,
	srb2::rhi::PixelFormat data_format,
	tcb::span<const std::byte> data
)
{
	SRB2_ASSERT(graphics_context_active_ == false);
	SRB2_ASSERT(transfer_context_active_ == true);
	SRB2_ASSERT(ctx.generation() == transfer_context_generation_);

	if (data.empty())
	{
		return;
	}

	SRB2_ASSERT(texture_slab_.is_valid(texture) == true);
	auto& t = texture_slab_[texture];

	// Each row of pixels must be on the unpack alignment boundary.
	// This alignment is not user changeable until OpenGL 4.
	constexpr const int32_t kUnpackAlignment = 4;

	GLenum format = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;
	GLuint size = 0;
	std::tie(format, type, size) = map_pixel_data_format(data_format);
	SRB2_ASSERT(format != GL_ZERO && type != GL_ZERO);
	SRB2_ASSERT(map_texture_format(t.desc.format) == format);

	int32_t expected_row_span = (((size * region.w) + kUnpackAlignment - 1) / kUnpackAlignment) * kUnpackAlignment;
	SRB2_ASSERT(expected_row_span * region.h == data.size_bytes());
	SRB2_ASSERT(region.x + region.w <= t.desc.width && region.y + region.h <= t.desc.height);

	gl_->ActiveTexture(GL_TEXTURE0);
	GL_ASSERT
	gl_->BindTexture(GL_TEXTURE_2D, t.texture);
	GL_ASSERT
	gl_->TexSubImage2D(
		GL_TEXTURE_2D,
		0,
		region.x,
		region.y,
		region.w,
		region.h,
		format,
		type,
		reinterpret_cast<const void*>(data.data())
	);
	GL_ASSERT
}

rhi::Handle<rhi::Buffer> GlCoreRhi::create_buffer(const rhi::BufferDesc& desc)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	GLenum target = map_buffer_type(desc.type);
	SRB2_ASSERT(target != GL_ZERO);

	GLenum usage = map_buffer_usage(desc.usage);
	SRB2_ASSERT(usage != GL_ZERO);

	GLuint name = 0;
	gl_->GenBuffers(1, &name);
	GL_ASSERT

	gl_->BindBuffer(target, name);
	GL_ASSERT

	gl_->BufferData(target, desc.size, nullptr, usage);
	GL_ASSERT

	GlCoreBuffer buffer;
	buffer.buffer = name;
	buffer.desc = desc;
	return buffer_slab_.insert(std::move(buffer));
}

void GlCoreRhi::destroy_buffer(rhi::Handle<rhi::Buffer> handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(buffer_slab_.is_valid(handle) == true);
	SRB2_ASSERT(graphics_context_active_ == false);
	GlCoreBuffer casted = buffer_slab_.remove(handle);
	GLuint name = casted.buffer;

	disposal_.push_back([this, name] { gl_->DeleteBuffers(1, &name); });
}

void GlCoreRhi::update_buffer_contents(
	rhi::Handle<TransferContext> ctx,
	rhi::Handle<rhi::Buffer> handle,
	uint32_t offset,
	tcb::span<const std::byte> data
)
{
	SRB2_ASSERT(graphics_context_active_ == false);
	SRB2_ASSERT(transfer_context_active_ == true);
	SRB2_ASSERT(ctx.generation() == transfer_context_generation_);

	if (data.empty())
	{
		return;
	}

	SRB2_ASSERT(buffer_slab_.is_valid(handle) == true);
	auto& b = buffer_slab_[handle];

	SRB2_ASSERT(offset < b.desc.size && offset + data.size() <= b.desc.size);

	GLenum target = GL_ZERO;
	switch (b.desc.type)
	{
	case rhi::BufferType::kVertexBuffer:
		target = GL_ARRAY_BUFFER;
		break;
	case rhi::BufferType::kIndexBuffer:
		target = GL_ELEMENT_ARRAY_BUFFER;
		break;
	}

	gl_->BindBuffer(target, b.buffer);
	GL_ASSERT
	gl_->BufferSubData(target, offset, data.size(), data.data());
	GL_ASSERT
}

rhi::Handle<rhi::UniformSet>
GlCoreRhi::create_uniform_set(rhi::Handle<rhi::TransferContext> ctx, const rhi::CreateUniformSetInfo& info)
{
	SRB2_ASSERT(graphics_context_active_ == false);
	SRB2_ASSERT(transfer_context_active_ == true);
	SRB2_ASSERT(ctx.generation() == transfer_context_generation_);

	GlCoreUniformSet uniform_set;

	for (auto& uniform : info.uniforms)
	{
		uniform_set.uniforms.push_back(uniform);
	}

	return uniform_set_slab_.insert(std::move(uniform_set));
}

rhi::Handle<rhi::BindingSet> GlCoreRhi::create_binding_set(
	rhi::Handle<rhi::TransferContext> ctx,
	Handle<Pipeline> pipeline,
	const rhi::CreateBindingSetInfo& info
)
{
	SRB2_ASSERT(graphics_context_active_ == false);
	SRB2_ASSERT(transfer_context_active_ == true);
	SRB2_ASSERT(ctx.generation() == transfer_context_generation_);

	SRB2_ASSERT(pipeline_slab_.is_valid(pipeline) == true);
	auto& pl = pipeline_slab_[pipeline];

	SRB2_ASSERT(info.vertex_buffers.size() == pl.desc.vertex_input.buffer_layouts.size());

	GlCoreBindingSet binding_set;

	GLuint vao = 0;
	gl_->GenVertexArrays(1, &vao);
	GL_ASSERT
	gl_->BindVertexArray(vao);
	GL_ASSERT

	for (auto& attr_layout : pl.desc.vertex_input.attr_layouts)
	{
		SRB2_ASSERT(buffer_slab_.is_valid(info.vertex_buffers[attr_layout.buffer_index].vertex_buffer));
		auto& buf = buffer_slab_[info.vertex_buffers[attr_layout.buffer_index].vertex_buffer];
		SRB2_ASSERT(buf.desc.type == rhi::BufferType::kVertexBuffer);

		auto& buffer_layout = pl.desc.vertex_input.buffer_layouts[attr_layout.buffer_index];

		gl_->BindBuffer(GL_ARRAY_BUFFER, buf.buffer);
		GL_ASSERT

		GLuint attrib_location = pl.attrib_locations[attr_layout.name];
		VertexAttributeFormat vert_attr_format = rhi::vertex_attribute_format(attr_layout.name);
		GLenum vertex_attr_type = map_vertex_attribute_type(vert_attr_format);
		SRB2_ASSERT(vertex_attr_type != GL_ZERO);
		GLint vertex_attr_size = map_vertex_attribute_format_size(vert_attr_format);
		SRB2_ASSERT(vertex_attr_size != 0);
		uint32_t vertex_buffer_offset = 0; // TODO allow binding set to specify
		gl_->EnableVertexAttribArray(pl.attrib_locations[attr_layout.name]);
		GL_ASSERT
		gl_->VertexAttribPointer(
			attrib_location,
			vertex_attr_size,
			vertex_attr_type,
			GL_FALSE,
			buffer_layout.stride,
			reinterpret_cast<const void*>(vertex_buffer_offset + attr_layout.offset)
		);
		GL_ASSERT
	}

	binding_set.vao = vao;

	// Set textures
	for (size_t i = 0; i < info.sampler_textures.size(); i++)
	{
		auto& binding = info.sampler_textures[i];
		auto& sampler_name = pl.desc.sampler_input.enabled_samplers[i];
		SRB2_ASSERT(binding.name == sampler_name);

		SRB2_ASSERT(texture_slab_.is_valid(binding.texture));
		auto& tx = texture_slab_[binding.texture];
		binding_set.textures.insert({sampler_name, tx.texture});
	}

	return binding_set_slab_.insert(std::move(binding_set));
}

rhi::Handle<rhi::Renderbuffer> GlCoreRhi::create_renderbuffer(const rhi::RenderbufferDesc& desc)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	GLuint name = 0;
	gl_->GenRenderbuffers(1, &name);

	// Obtain storage up-front.
	gl_->BindRenderbuffer(GL_RENDERBUFFER, name);
	GL_ASSERT
	gl_->RenderbufferStorage(GL_RENDERBUFFER, map_pixel_format(desc.format), desc.width, desc.height);
	GL_ASSERT

	GlCoreRenderbuffer rb;
	rb.renderbuffer = name;
	return renderbuffer_slab_.insert(std::move(rb));
}

void GlCoreRhi::destroy_renderbuffer(rhi::Handle<rhi::Renderbuffer> handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(renderbuffer_slab_.is_valid(handle) == true);
	GlCoreRenderbuffer casted = renderbuffer_slab_.remove(handle);
	GLuint name = casted.renderbuffer;
	disposal_.push_back([this, name] { gl_->DeleteRenderbuffers(1, &name); });
}

rhi::Handle<rhi::Pipeline> GlCoreRhi::create_pipeline(const PipelineDesc& desc)
{
	SRB2_ASSERT(graphics_context_active_ == false);
	SRB2_ASSERT(platform_ != nullptr);
	// TODO assert compatibility of pipeline description with program using ProgramRequirements

	const rhi::ProgramRequirements& reqs = rhi::program_requirements_for_program(desc.program);

	GLuint vertex = 0;
	GLuint fragment = 0;
	GLuint program = 0;
	GlCorePipeline pipeline;

	auto [vert_src, frag_src] = platform_->find_shader_sources(desc.program);

	// TODO make use of multiple source files.
	// In Khronos Group's brilliance, #version is required to be the first directive,
	// but we need to insert #defines in-between.
	std::string vert_src_processed;
	std::string::size_type string_i = 0;
	do
	{
		std::string::size_type new_i = vert_src.find('\n', string_i);
		if (new_i == std::string::npos)
		{
			break;
		}
		std::string_view line_view(vert_src.c_str() + string_i, new_i - string_i + 1);
		vert_src_processed.append(line_view);
		if (line_view.rfind("#version ", 0) == 0)
		{
			for (auto& attribute : desc.vertex_input.attr_layouts)
			{
				for (auto const& require_attr : reqs.vertex_input.attributes)
				{
					if (require_attr.name == attribute.name && !require_attr.required)
					{
						vert_src_processed.append("#define ");
						vert_src_processed.append(map_vertex_attribute_enable_define(attribute.name));
						vert_src_processed.append("\n");
					}
				}
			}
			for (auto& uniform_group : desc.uniform_input.enabled_uniforms)
			{
				for (auto& uniform : uniform_group)
				{
					for (auto const& req_uni_group : reqs.uniforms.uniform_groups)
					{
						for (auto const& req_uni : req_uni_group)
						{
							if (req_uni.name == uniform && !req_uni.required)
							{
								vert_src_processed.append("#define ");
								vert_src_processed.append(map_uniform_enable_define(uniform));
								vert_src_processed.append("\n");
							}
						}
					}
				}
			}
		}
		string_i = new_i + 1;
	} while (string_i != std::string::npos);

	std::string frag_src_processed;
	string_i = 0;
	do
	{
		std::string::size_type new_i = frag_src.find('\n', string_i);
		if (new_i == std::string::npos)
		{
			break;
		}
		std::string_view line_view(frag_src.c_str() + string_i, new_i - string_i + 1);
		frag_src_processed.append(line_view);
		if (line_view.rfind("#version ", 0) == 0)
		{
			for (auto& sampler : desc.sampler_input.enabled_samplers)
			{
				for (auto const& require_sampler : reqs.samplers.samplers)
				{
					if (sampler == require_sampler.name && !require_sampler.required)
					{
						frag_src_processed.append("#define ");
						frag_src_processed.append(map_sampler_enable_define(sampler));
						frag_src_processed.append("\n");
					}
				}
			}
			for (auto& uniform_group : desc.uniform_input.enabled_uniforms)
			{
				for (auto& uniform : uniform_group)
				{
					for (auto const& req_uni_group : reqs.uniforms.uniform_groups)
					{
						for (auto const& req_uni : req_uni_group)
						{
							if (req_uni.name == uniform && !req_uni.required)
							{
								frag_src_processed.append("#define ");
								frag_src_processed.append(map_uniform_enable_define(uniform));
								frag_src_processed.append("\n");
							}
						}
					}
				}
			}
		}
		string_i = new_i + 1;
	} while (string_i != std::string::npos);

	const char* vert_src_arr[1] = {vert_src_processed.c_str()};
	const GLint vert_src_arr_lens[1] = {static_cast<GLint>(vert_src_processed.size())};
	const char* frag_src_arr[1] = {frag_src_processed.c_str()};
	const GLint frag_src_arr_lens[1] = {static_cast<GLint>(frag_src_processed.size())};

	vertex = gl_->CreateShader(GL_VERTEX_SHADER);
	gl_->ShaderSource(vertex, 1, vert_src_arr, vert_src_arr_lens);
	gl_->CompileShader(vertex);
	GLint is_compiled = 0;
	gl_->GetShaderiv(vertex, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		gl_->GetShaderiv(vertex, GL_INFO_LOG_LENGTH, &max_length);
		std::vector<GLchar> compile_error(max_length);
		gl_->GetShaderInfoLog(vertex, max_length, &max_length, compile_error.data());

		gl_->DeleteShader(vertex);
		throw std::runtime_error(fmt::format("Vertex shader compilation failed: {}", std::string(compile_error.data()))
		);
	}
	fragment = gl_->CreateShader(GL_FRAGMENT_SHADER);
	gl_->ShaderSource(fragment, 1, frag_src_arr, frag_src_arr_lens);
	gl_->CompileShader(fragment);
	gl_->GetShaderiv(vertex, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		gl_->GetShaderiv(fragment, GL_INFO_LOG_LENGTH, &max_length);
		std::vector<GLchar> compile_error(max_length);
		gl_->GetShaderInfoLog(fragment, max_length, &max_length, compile_error.data());

		gl_->DeleteShader(fragment);
		gl_->DeleteShader(vertex);
		throw std::runtime_error(
			fmt::format("Fragment shader compilation failed: {}", std::string(compile_error.data()))
		);
	}

	// Program link

	program = gl_->CreateProgram();
	gl_->AttachShader(program, vertex);
	gl_->AttachShader(program, fragment);
	gl_->LinkProgram(program);
	gl_->GetProgramiv(program, GL_LINK_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		gl_->GetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);
		std::vector<GLchar> link_error(max_length);
		gl_->GetProgramInfoLog(program, max_length, &max_length, link_error.data());

		gl_->DeleteProgram(program);
		gl_->DeleteShader(fragment);
		gl_->DeleteShader(vertex);
		throw std::runtime_error(fmt::format("Pipeline program link failed: {}", std::string(link_error.data())));
	}

	std::unordered_map<std::string, GlCoreActiveUniform> active_attributes;
	GLint active_attribute_total = -1;
	gl_->GetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active_attribute_total);
	if (active_attribute_total < 0)
	{
		gl_->DeleteProgram(program);
		gl_->DeleteShader(fragment);
		gl_->DeleteShader(vertex);
		throw std::runtime_error("Unable to retrieve program active attributes");
	}
	if (desc.vertex_input.attr_layouts.size() != static_cast<GLuint>(active_attribute_total))
	{
		gl_->DeleteProgram(program);
		gl_->DeleteShader(fragment);
		gl_->DeleteShader(vertex);
		throw std::runtime_error(fmt::format(
			"Pipeline's enabled attribute count does not match the linked program's total: {} vs {}",
			desc.vertex_input.attr_layouts.size(),
			static_cast<GLuint>(active_attribute_total)
		));
	}
	for (GLint i = 0; i < active_attribute_total; i++)
	{
		GLsizei name_len = 0;
		GLint size = 0;
		GLenum type = GL_ZERO;
		char name[256];
		gl_->GetActiveAttrib(program, i, 255, &name_len, &size, &type, name);
		GL_ASSERT
		GLint location = gl_->GetAttribLocation(program, name);
		GL_ASSERT
		active_attributes.insert({std::string(name), GlCoreActiveUniform {type, static_cast<GLuint>(location)}});
	}

	std::unordered_map<std::string, GlCoreActiveUniform> active_uniforms;
	size_t total_enabled_uniforms = 0;
	for (auto g = desc.uniform_input.enabled_uniforms.cbegin(); g != desc.uniform_input.enabled_uniforms.cend();
		 g = std::next(g))
	{
		total_enabled_uniforms += g->size();
	}
	GLint active_uniform_total = -1;
	gl_->GetProgramiv(program, GL_ACTIVE_UNIFORMS, &active_uniform_total);
	if (active_uniform_total < 0)
	{
		gl_->DeleteProgram(program);
		gl_->DeleteShader(fragment);
		gl_->DeleteShader(vertex);
		throw std::runtime_error("Unable to retrieve program active uniforms");
	}
	if (total_enabled_uniforms + desc.sampler_input.enabled_samplers.size() !=
		static_cast<GLuint>(active_uniform_total))
	{
		gl_->DeleteProgram(program);
		gl_->DeleteShader(fragment);
		gl_->DeleteShader(vertex);
		throw std::runtime_error(fmt::format(
			"Pipeline's enabled uniform count (uniforms + samplers) does not match the linked program's total: {} vs "
			"{}",
			total_enabled_uniforms + desc.sampler_input.enabled_samplers.size(),
			static_cast<GLuint>(active_uniform_total)
		));
	}
	for (GLint i = 0; i < active_uniform_total; i++)
	{
		GLsizei name_len = 0;
		GLint size = 0;
		GLenum type = GL_ZERO;
		char name[256];
		gl_->GetActiveUniform(program, i, 255, &name_len, &size, &type, name);
		GL_ASSERT
		GLint location = gl_->GetUniformLocation(program, name);
		GL_ASSERT
		active_uniforms.insert({std::string(name), GlCoreActiveUniform {type, static_cast<GLuint>(location)}});
	}

	for (auto& attr : desc.vertex_input.attr_layouts)
	{
		const char* symbol_name = map_vertex_attribute_symbol_name(attr.name);
		SRB2_ASSERT(symbol_name != nullptr);
		if (active_attributes.find(symbol_name) == active_attributes.end())
		{
			gl_->DeleteProgram(program);
			gl_->DeleteShader(fragment);
			gl_->DeleteShader(vertex);
			throw std::runtime_error("Enabled attribute not found in linked program");
		}
		auto& active_attr = active_attributes[symbol_name];
		auto expected_format = rhi::vertex_attribute_format(attr.name);
		auto expected_gl_type = map_vertex_attribute_format(expected_format);
		SRB2_ASSERT(expected_gl_type != GL_ZERO);
		if (expected_gl_type != active_attr.type)
		{
			gl_->DeleteProgram(program);
			gl_->DeleteShader(fragment);
			gl_->DeleteShader(vertex);
			throw std::runtime_error("Active attribute type does not match expected type");
		}

		pipeline.attrib_locations.insert({attr.name, active_attr.location});
	}

	for (auto group_itr = desc.uniform_input.enabled_uniforms.cbegin();
		 group_itr != desc.uniform_input.enabled_uniforms.cend();
		 group_itr = std::next(group_itr))
	{
		auto& group = *group_itr;
		for (auto itr = group.cbegin(); itr != group.cend(); itr = std::next(itr))
		{
			auto& uniform = *itr;
			const char* symbol_name = map_uniform_attribute_symbol_name(uniform);
			SRB2_ASSERT(symbol_name != nullptr);
			if (active_uniforms.find(symbol_name) == active_uniforms.end())
			{
				gl_->DeleteProgram(program);
				gl_->DeleteShader(fragment);
				gl_->DeleteShader(vertex);
				throw std::runtime_error("Enabled uniform not found in linked program");
			}
			auto& active_uniform = active_uniforms[symbol_name];
			auto expected_format = rhi::uniform_format(uniform);
			auto expected_gl_type = map_uniform_format(expected_format);
			SRB2_ASSERT(expected_gl_type != GL_ZERO);
			if (expected_gl_type != active_uniform.type)
			{
				gl_->DeleteProgram(program);
				gl_->DeleteShader(fragment);
				gl_->DeleteShader(vertex);
				throw std::runtime_error("Active uniform type does not match expected type");
			}
			SRB2_ASSERT(pipeline.uniform_locations.find(uniform) == pipeline.uniform_locations.end());
			pipeline.uniform_locations.insert({uniform, active_uniform.location});
		}
	}

	for (auto& sampler : desc.sampler_input.enabled_samplers)
	{
		const char* symbol_name = map_sampler_symbol_name(sampler);
		SRB2_ASSERT(symbol_name != nullptr);
		if (active_uniforms.find(symbol_name) == active_uniforms.end())
		{
			gl_->DeleteProgram(program);
			gl_->DeleteShader(fragment);
			gl_->DeleteShader(vertex);
			throw std::runtime_error("Enabled sampler not found in linked program");
		}
		auto& active_sampler = active_uniforms[symbol_name];
		if (active_sampler.type != GL_SAMPLER_2D)
		{
			gl_->DeleteProgram(program);
			gl_->DeleteShader(fragment);
			gl_->DeleteShader(vertex);
			throw std::runtime_error("Active sampler type does not match expected type");
		}

		pipeline.sampler_locations.insert({sampler, active_sampler.location});
	}

	pipeline.desc = desc;
	pipeline.vertex_shader = vertex;
	pipeline.fragment_shader = fragment;
	pipeline.program = program;

	return pipeline_slab_.insert(std::move(pipeline));
}

void GlCoreRhi::destroy_pipeline(rhi::Handle<rhi::Pipeline> handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(pipeline_slab_.is_valid(handle) == true);
	GlCorePipeline casted = pipeline_slab_.remove(handle);
	GLuint vertex_shader = casted.vertex_shader;
	GLuint fragment_shader = casted.fragment_shader;
	GLuint program = casted.program;

	disposal_.push_back([this, fragment_shader] { gl_->DeleteShader(fragment_shader); });
	disposal_.push_back([this, vertex_shader] { gl_->DeleteShader(vertex_shader); });
	disposal_.push_back([this, program] { gl_->DeleteProgram(program); });
}

rhi::Handle<rhi::GraphicsContext> GlCoreRhi::begin_graphics()
{
	SRB2_ASSERT(graphics_context_active_ == false);
	graphics_context_active_ = true;
	return rhi::Handle<rhi::GraphicsContext>(0, graphics_context_generation_);
}

void GlCoreRhi::end_graphics(rhi::Handle<rhi::GraphicsContext> handle)
{
	SRB2_ASSERT(graphics_context_active_ == true);
	SRB2_ASSERT(current_pipeline_.has_value() == false && current_render_pass_.has_value() == false);
	graphics_context_generation_ += 1;
	graphics_context_active_ = false;
	gl_->Flush();
	GL_ASSERT
}

rhi::Handle<rhi::TransferContext> GlCoreRhi::begin_transfer()
{
	SRB2_ASSERT(graphics_context_active_ == false);
	SRB2_ASSERT(transfer_context_active_ == false);

	transfer_context_generation_ += 1;
	transfer_context_active_ = true;

	return rhi::Handle<rhi::TransferContext>(0, transfer_context_generation_);
}

void GlCoreRhi::end_transfer(rhi::Handle<rhi::TransferContext> ctx)
{
	SRB2_ASSERT(graphics_context_active_ == false);
	SRB2_ASSERT(transfer_context_active_ == true);

	transfer_context_active_ = false;
}

void GlCoreRhi::present()
{
	SRB2_ASSERT(platform_ != nullptr);
	SRB2_ASSERT(graphics_context_active_ == false);

	platform_->present();
}

void GlCoreRhi::begin_default_render_pass(Handle<GraphicsContext> ctx, bool clear)
{
	SRB2_ASSERT(platform_ != nullptr);
	SRB2_ASSERT(graphics_context_active_ == true);
	SRB2_ASSERT(current_render_pass_.has_value() == false);

	const Rect fb_rect = platform_->get_default_framebuffer_dimensions();

	gl_->BindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_ASSERT
	gl_->Disable(GL_SCISSOR_TEST);
	GL_ASSERT
	gl_->Viewport(0, 0, fb_rect.w, fb_rect.h);
	GL_ASSERT

	if (clear)
	{
		gl_->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		gl_->ClearDepth(1.0f);
		gl_->ClearStencil(0);
		gl_->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GL_ASSERT
	}

	current_render_pass_ = GlCoreRhi::DefaultRenderPassState {};
}

void GlCoreRhi::begin_render_pass(Handle<GraphicsContext> ctx, const RenderPassBeginInfo& info)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == false);

	SRB2_ASSERT(render_pass_slab_.is_valid(info.render_pass) == true);
	auto& rp = render_pass_slab_[info.render_pass];
	SRB2_ASSERT(rp.desc.depth_format.has_value() == info.depth_attachment.has_value());

	auto fb_itr = framebuffers_.find(GlCoreFramebufferKey {info.color_attachment, info.depth_attachment});
	if (fb_itr == framebuffers_.end())
	{
		// Create a new framebuffer for this color-depth pair
		GLuint fb_name;
		gl_->GenFramebuffers(1, &fb_name);
		GL_ASSERT
		gl_->BindFramebuffer(GL_FRAMEBUFFER, fb_name);
		GL_ASSERT
		fb_itr = framebuffers_
					 .insert(
						 {GlCoreFramebufferKey {info.color_attachment, info.depth_attachment},
						  static_cast<uint32_t>(fb_name)}
					 )
					 .first;

		GLuint attachment = GL_COLOR_ATTACHMENT0;
		auto visitor = srb2::Overload {
			[&, this](const Handle<Texture>& handle)
			{
				SRB2_ASSERT(texture_slab_.is_valid(handle));
				auto& texture = texture_slab_[handle];
				gl_->FramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.texture, 0);
				GL_ASSERT
			},
			[&, this](const Handle<Renderbuffer>& handle)
			{
				SRB2_ASSERT(renderbuffer_slab_.is_valid(handle));
				auto& renderbuffer = renderbuffer_slab_[handle];
				gl_->FramebufferRenderbuffer(
					GL_FRAMEBUFFER,
					attachment,
					GL_RENDERBUFFER,
					renderbuffer.renderbuffer
				);
				GL_ASSERT
			}};
		std::visit(visitor, info.color_attachment);
		if (info.depth_attachment)
		{
			attachment = GL_DEPTH_ATTACHMENT;
			std::visit(visitor, *info.depth_attachment);
		}
	}
	auto& fb = *fb_itr;
	gl_->BindFramebuffer(GL_FRAMEBUFFER, fb.second);
	GL_ASSERT
	gl_->Disable(GL_SCISSOR_TEST);
	GL_ASSERT

	if (rp.desc.load_op == rhi::AttachmentLoadOp::kClear)
	{
		gl_->ClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);
		gl_->ClearDepth(1.f);
		gl_->ClearStencil(0);
		gl_->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		GL_ASSERT
	}

	current_render_pass_ = info;
}

void GlCoreRhi::end_render_pass(Handle<GraphicsContext> ctx)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true);

	current_pipeline_ = std::nullopt;
	current_render_pass_ = std::nullopt;
}

void GlCoreRhi::bind_pipeline(Handle<GraphicsContext> ctx, Handle<Pipeline> pipeline)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(pipeline) == true);
	auto& pl = pipeline_slab_[pipeline];
	auto& desc = pl.desc;

	gl_->UseProgram(pl.program);
	GL_ASSERT

	gl_->Disable(GL_SCISSOR_TEST);
	GL_ASSERT

	if (desc.depth_attachment)
	{
		gl_->Enable(GL_DEPTH_TEST);
		GL_ASSERT
		GLenum depth_func = map_compare_func(desc.depth_attachment->func);
		SRB2_ASSERT(depth_func != GL_ZERO);
		gl_->DepthFunc(depth_func);
		GL_ASSERT
		gl_->DepthMask(desc.depth_attachment->write ? GL_TRUE : GL_FALSE);
		GL_ASSERT
	}
	else
	{
		gl_->Disable(GL_DEPTH_TEST);
		GL_ASSERT
	}

	if (desc.color_attachment.blend)
	{
		rhi::BlendDesc& bl = *desc.color_attachment.blend;
		gl_->Enable(GL_BLEND);
		GL_ASSERT
		gl_->BlendFuncSeparate(
			map_blend_factor(bl.source_factor_color),
			map_blend_factor(bl.dest_factor_color),
			map_blend_factor(bl.source_factor_alpha),
			map_blend_factor(bl.dest_factor_alpha)
		);
		GL_ASSERT
		gl_->BlendEquationSeparate(map_blend_function(bl.color_function), map_blend_function(bl.alpha_function));
		GL_ASSERT
		gl_->BlendColor(desc.blend_color.r, desc.blend_color.g, desc.blend_color.b, desc.blend_color.a);
		GL_ASSERT
	}
	else
	{
		gl_->Disable(GL_BLEND);
	}

	gl_->ColorMask(
		desc.color_attachment.color_mask.r ? GL_TRUE : GL_FALSE,
		desc.color_attachment.color_mask.g ? GL_TRUE : GL_FALSE,
		desc.color_attachment.color_mask.b ? GL_TRUE : GL_FALSE,
		desc.color_attachment.color_mask.a ? GL_TRUE : GL_FALSE
	);
	GL_ASSERT

	GLenum cull_face = map_cull_mode(desc.cull);
	if (cull_face == GL_NONE)
	{
		gl_->Disable(GL_CULL_FACE);
		GL_ASSERT
	}
	else
	{
		gl_->Enable(GL_CULL_FACE);
		GL_ASSERT
		gl_->CullFace(cull_face);
		GL_ASSERT
	}
	gl_->FrontFace(map_winding(desc.winding));
	GL_ASSERT

	current_pipeline_ = pipeline;
	current_primitive_type_ = desc.primitive;
}

void GlCoreRhi::bind_uniform_set(Handle<GraphicsContext> ctx, uint32_t slot, Handle<UniformSet> set)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(*current_pipeline_));
	auto& pl = pipeline_slab_[*current_pipeline_];

	SRB2_ASSERT(uniform_set_slab_.is_valid(set));
	auto& us = uniform_set_slab_[set];

	auto& uniform_input = pl.desc.uniform_input;
	SRB2_ASSERT(slot < uniform_input.enabled_uniforms.size());
	SRB2_ASSERT(us.uniforms.size() == uniform_input.enabled_uniforms[slot].size());

	// Assert compatibility of uniform set with pipeline's set slot
	for (size_t i = 0; i < us.uniforms.size(); i++)
	{
		SRB2_ASSERT(
			rhi::uniform_format(uniform_input.enabled_uniforms[slot][i]) == rhi::uniform_variant_format(us.uniforms[i])
		);
	}

	// Apply uniforms
	// TODO use Uniform Buffer Objects to optimize this.
	// We don't really *need* to, though, probably...
	// Also, we know that any given uniform name is uniquely present in a single uniform group asserted during pipeline
	// compilation. This is an RHI requirement to support backends that don't have UBOs.
	for (size_t i = 0; i < us.uniforms.size(); i++)
	{
		auto& uniform_name = uniform_input.enabled_uniforms[slot][i];
		auto& update_data = us.uniforms[i];
		SRB2_ASSERT(pl.uniform_locations.find(uniform_name) != pl.uniform_locations.end());
		GLuint pipeline_uniform = pl.uniform_locations[uniform_name];

		auto visitor = srb2::Overload {
			[&](const float& value)
			{
				gl_->Uniform1f(pipeline_uniform, value);
				GL_ASSERT
			},
			[&](const std::array<float, 2>& value)
			{
				gl_->Uniform2f(pipeline_uniform, value[0], value[1]);
				GL_ASSERT
			},
			[&](const std::array<float, 3>& value)
			{
				gl_->Uniform3f(pipeline_uniform, value[0], value[1], value[2]);
				GL_ASSERT
			},
			[&](const std::array<float, 4>& value)
			{
				gl_->Uniform4f(pipeline_uniform, value[0], value[1], value[2], value[3]);
				GL_ASSERT
			},
			[&](const int32_t& value)
			{
				gl_->Uniform1i(pipeline_uniform, value);
				GL_ASSERT
			},
			[&](const std::array<int32_t, 2>& value)
			{
				gl_->Uniform2i(pipeline_uniform, value[0], value[1]);
				GL_ASSERT
			},
			[&](const std::array<int32_t, 3>& value)
			{
				gl_->Uniform3i(pipeline_uniform, value[0], value[1], value[2]);
				GL_ASSERT
			},
			[&](const std::array<int32_t, 4>& value)
			{
				gl_->Uniform4i(pipeline_uniform, value[0], value[1], value[2], value[3]);
				GL_ASSERT
			},
			[&](const std::array<std::array<float, 2>, 2>& value)
			{
				gl_->UniformMatrix2fv(pipeline_uniform, 1, false, reinterpret_cast<const GLfloat*>(&value));
				GL_ASSERT
			},
			[&](const std::array<std::array<float, 3>, 3>& value)
			{
				gl_->UniformMatrix3fv(pipeline_uniform, 1, false, reinterpret_cast<const GLfloat*>(&value));
				GL_ASSERT
			},
			[&](const std::array<std::array<float, 4>, 4>& value)
			{
				gl_->UniformMatrix4fv(pipeline_uniform, 1, false, reinterpret_cast<const GLfloat*>(&value));
				GL_ASSERT
			},
		};
		std::visit(visitor, update_data);
	}
}

void GlCoreRhi::bind_binding_set(Handle<GraphicsContext> ctx, Handle<BindingSet> set)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(*current_pipeline_));
	auto& pl = pipeline_slab_[*current_pipeline_];

	SRB2_ASSERT(binding_set_slab_.is_valid(set));
	auto& bs = binding_set_slab_[set];

	SRB2_ASSERT(bs.textures.size() == pl.desc.sampler_input.enabled_samplers.size());

	// Bind the vertex array for drawing
	// TODO assert compatibility of binding set. The VAO's attributes must match the pipeline.
	gl_->BindVertexArray(bs.vao);

	// Index buffer is part of VAO.
	gl_->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Bind the samplers to the uniforms
	for (auto& texture_binding : bs.textures)
	{
		auto sampler_name = texture_binding.first;
		GLuint texture_gl_name = texture_binding.second;
		GLuint sampler_uniform_loc = pl.sampler_locations[sampler_name];
		GLenum active_texture = GL_TEXTURE0;
		GLuint uniform_value = 0;
		switch (sampler_name)
		{
		case rhi::SamplerName::kSampler0:
			active_texture = GL_TEXTURE0;
			uniform_value = 0;
			break;
		case rhi::SamplerName::kSampler1:
			active_texture = GL_TEXTURE0 + 1;
			uniform_value = 1;
			break;
		case rhi::SamplerName::kSampler2:
			active_texture = GL_TEXTURE0 + 2;
			uniform_value = 2;
			break;
		case rhi::SamplerName::kSampler3:
			active_texture = GL_TEXTURE0 + 3;
			uniform_value = 3;
			break;
		}
		gl_->ActiveTexture(active_texture);
		GL_ASSERT
		gl_->BindTexture(GL_TEXTURE_2D, texture_gl_name);
		GL_ASSERT
		gl_->Uniform1i(sampler_uniform_loc, uniform_value);
		GL_ASSERT
	}
}

void GlCoreRhi::bind_index_buffer(Handle<GraphicsContext> ctx, Handle<Buffer> buffer)
{
	SRB2_ASSERT(transfer_context_active_ == false);
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	SRB2_ASSERT(buffer_slab_.is_valid(buffer));
	auto& ib = buffer_slab_[buffer];

	SRB2_ASSERT(ib.desc.type == rhi::BufferType::kIndexBuffer);

	current_index_buffer_ = buffer;

	gl_->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.buffer);
}

void GlCoreRhi::set_scissor(Handle<GraphicsContext> ctx, const Rect& rect)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	gl_->Enable(GL_SCISSOR_TEST);
	gl_->Scissor(rect.x, rect.y, rect.w, rect.h);
}

void GlCoreRhi::set_viewport(Handle<GraphicsContext> ctx, const Rect& rect)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	gl_->Viewport(rect.x, rect.y, rect.w, rect.h);
}

void GlCoreRhi::draw(Handle<GraphicsContext> ctx, uint32_t vertex_count, uint32_t first_vertex)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	gl_->DrawArrays(map_primitive_mode(current_primitive_type_), first_vertex, vertex_count);
	GL_ASSERT
}

void GlCoreRhi::draw_indexed(Handle<GraphicsContext> ctx, uint32_t index_count, uint32_t first_index)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());

	SRB2_ASSERT(current_index_buffer_ != kNullHandle);
#ifndef NDEBUG
	{
		auto& ib = buffer_slab_[current_index_buffer_];
		SRB2_ASSERT((index_count + first_index) * 2 + index_buffer_offset_ <= ib.desc.size);
	}
#endif

	gl_->DrawElements(
		map_primitive_mode(current_primitive_type_),
		index_count,
		GL_UNSIGNED_SHORT,
		(const void*)((size_t)first_index * 2 + index_buffer_offset_)
	);
	GL_ASSERT
}

void GlCoreRhi::read_pixels(Handle<GraphicsContext> ctx, const Rect& rect, PixelFormat format, tcb::span<std::byte> out)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value());

	std::tuple<GLenum, GLenum, GLuint> gl_format = map_pixel_data_format(format);
	GLenum layout = std::get<0>(gl_format);
	GLenum type = std::get<1>(gl_format);
	GLint size = std::get<2>(gl_format);

	SRB2_ASSERT(out.size_bytes() == rect.w * rect.h * size);

	gl_->ReadPixels(rect.x, rect.y, rect.w, rect.h, layout, type, out.data());
}

void GlCoreRhi::finish()
{
	SRB2_ASSERT(graphics_context_active_ == false);

	for (auto it = binding_set_slab_.cbegin(); it != binding_set_slab_.cend(); it++)
	{
		gl_->BindVertexArray(0);
		GL_ASSERT
		GLuint vao = reinterpret_cast<const GlCoreBindingSet&>(*it).vao;
		gl_->DeleteVertexArrays(1, &vao);
		GL_ASSERT
	}
	binding_set_slab_.clear();
	uniform_set_slab_.clear();

	// I sure hope creating FBOs isn't costly on the driver!
	for (auto& fbset : framebuffers_)
	{
		gl_->DeleteFramebuffers(1, &fbset.second);
	}
	framebuffers_.clear();

	for (auto it = disposal_.begin(); it != disposal_.end(); it++)
	{
		(*it)();
	}

	disposal_.clear();
	GL_ASSERT
}
