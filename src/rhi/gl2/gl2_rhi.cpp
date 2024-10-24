// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "gl2_rhi.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include "../shader_load_context.hpp"

using namespace srb2;
using namespace rhi;

#ifndef NDEBUG
#define GL_ASSERT                                                                                                      \
	while (1)                                                                                                          \
	{                                                                                                                  \
		GLenum __err = gl_->GetError();                                                                                \
		if (__err != GL_NO_ERROR)                                                                                      \
		{                                                                                                              \
			I_Error("GL Error at %s %d: %d", __FILE__, __LINE__, __err);                                               \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			break;                                                                                                     \
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
		return GL_LUMINANCE8;
	case rhi::PixelFormat::kRG8:
		return GL_LUMINANCE8_ALPHA8;
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
		layout = GL_LUMINANCE;
		type = GL_UNSIGNED_BYTE;
		size = 1;
		break;
	case rhi::PixelFormat::kRG8:
		layout = GL_LUMINANCE_ALPHA;
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
		return GL_LUMINANCE;
	case rhi::TextureFormat::kLuminanceAlpha:
		return GL_LUMINANCE_ALPHA;
	default:
		return GL_ZERO;
	}
}

constexpr GLenum map_texture_wrap(rhi::TextureWrapMode wrap)
{
	switch (wrap)
	{
	case rhi::TextureWrapMode::kClamp:
		return GL_CLAMP_TO_EDGE;
	case rhi::TextureWrapMode::kRepeat:
		return GL_REPEAT;
	case rhi::TextureWrapMode::kMirroredRepeat:
		return GL_MIRRORED_REPEAT;
	default:
		return GL_REPEAT;
	}
}

constexpr GLenum map_texture_filter(rhi::TextureFilterMode filter)
{
	switch (filter)
	{
	case rhi::TextureFilterMode::kNearest:
		return GL_NEAREST;
	case rhi::TextureFilterMode::kLinear:
		return GL_LINEAR;
	default:
		return GL_NEAREST;
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
		return GL_LUMINANCE8;
	case rhi::TextureFormat::kLuminanceAlpha:
		return GL_LUMINANCE8_ALPHA8;
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
	case rhi::UniformName::kTexCoord0Min:
		return "u_texcoord0_min";
	case rhi::UniformName::kTexCoord0Max:
		return "u_texcoord0_max";
	case rhi::UniformName::kTexCoord1Transform:
		return "u_texcoord1_transform";
	case rhi::UniformName::kTexCoord1Min:
		return "u_texcoord1_min";
	case rhi::UniformName::kTexCoord1Max:
		return "u_texcoord1_max";
	case rhi::UniformName::kSampler0IsIndexedAlpha:
		return "u_sampler0_is_indexed_alpha";
	case rhi::UniformName::kSampler1IsIndexedAlpha:
		return "u_sampler1_is_indexed_alpha";
	case rhi::UniformName::kSampler2IsIndexedAlpha:
		return "u_sampler2_is_indexed_alpha";
	case rhi::UniformName::kSampler3IsIndexedAlpha:
		return "u_sampler3_is_indexed_alpha";
	case rhi::UniformName::kSampler0Size:
		return "u_sampler0_size";
	case rhi::UniformName::kSampler1Size:
		return "u_sampler1_size";
	case rhi::UniformName::kSampler2Size:
		return "u_sampler2_size";
	case rhi::UniformName::kSampler3Size:
		return "u_sampler3_size";
	case rhi::UniformName::kWipeColorizeMode:
		return "u_wipe_colorize_mode";
	case rhi::UniformName::kWipeEncoreSwizzle:
		return "u_wipe_encore_swizzle";
	case rhi::UniformName::kPostimgWater:
		return "u_postimg_water";
	case rhi::UniformName::kPostimgHeat:
		return "u_postimg_heat";
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
	case rhi::UniformName::kTexCoord0Min:
		return "ENABLE_U_TEXCOORD0_MIN";
	case rhi::UniformName::kTexCoord0Max:
		return "ENABLE_U_TEXCOORD0_MAX";
	case rhi::UniformName::kTexCoord1Transform:
		return "ENABLE_U_TEXCOORD1_TRANSFORM";
	case rhi::UniformName::kTexCoord1Min:
		return "ENABLE_U_TEXCOORD1_MIN";
	case rhi::UniformName::kTexCoord1Max:
		return "ENABLE_U_TEXCOORD1_MAX";
	case rhi::UniformName::kSampler0IsIndexedAlpha:
		return "ENABLE_U_SAMPLER0_IS_INDEXED_ALPHA";
	case rhi::UniformName::kSampler1IsIndexedAlpha:
		return "ENABLE_U_SAMPLER1_IS_INDEXED_ALPHA";
	case rhi::UniformName::kSampler2IsIndexedAlpha:
		return "ENABLE_U_SAMPLER2_IS_INDEXED_ALPHA";
	case rhi::UniformName::kSampler3IsIndexedAlpha:
		return "ENABLE_U_SAMPLER3_IS_INDEXED_ALPHA";
	case rhi::UniformName::kSampler0Size:
		return "ENABLE_U_SAMPLER0_SIZE";
	case rhi::UniformName::kSampler1Size:
		return "ENABLE_U_SAMPLER1_SIZE";
	case rhi::UniformName::kSampler2Size:
		return "ENABLE_U_SAMPLER2_SIZE";
	case rhi::UniformName::kSampler3Size:
		return "ENABLE_U_SAMPLER3_SIZE";
	case rhi::UniformName::kWipeColorizeMode:
		return "ENABLE_U_WIPE_COLORIZE_MODE";
	case rhi::UniformName::kWipeEncoreSwizzle:
		return "ENABLE_U_WIPE_ENCORE_SWIZZLE";
	case rhi::UniformName::kPostimgWater:
		return "ENABLE_U_POSTIMG_WATER";
	case rhi::UniformName::kPostimgHeat:
		return "ENABLE_U_POSTIMG_HEAT";
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

Gl2Platform::~Gl2Platform() = default;

Gl2Rhi::Gl2Rhi(std::unique_ptr<Gl2Platform>&& platform, GlLoadFunc load_func) : platform_(std::move(platform))
{
	gl_ = std::make_unique<GladGLContext>();
	gladLoadGLContext(gl_.get(), load_func);
}

Gl2Rhi::~Gl2Rhi() = default;

rhi::Handle<rhi::RenderPass> Gl2Rhi::create_render_pass(const rhi::RenderPassDesc& desc)
{
	// GL has no formal render pass object
	Gl2RenderPass pass;
	pass.desc = desc;
	return render_pass_slab_.insert(std::move(pass));
}

void Gl2Rhi::destroy_render_pass(rhi::Handle<rhi::RenderPass> handle)
{
	render_pass_slab_.remove(handle);
}

rhi::Handle<rhi::Texture> Gl2Rhi::create_texture(const rhi::TextureDesc& desc)
{
	GLenum internal_format = map_internal_texture_format(desc.format);
	SRB2_ASSERT(internal_format != GL_ZERO);
	GLenum format = GL_RGBA;

	GLuint name = 0;
	gl_->GenTextures(1, &name);

	gl_->BindTexture(GL_TEXTURE_2D, name);

	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, map_texture_filter(desc.min));
	GL_ASSERT;
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, map_texture_filter(desc.mag));
	GL_ASSERT;
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, map_texture_wrap(desc.u_wrap));
	GL_ASSERT;
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, map_texture_wrap(desc.v_wrap));
	GL_ASSERT;
	gl_->TexImage2D(GL_TEXTURE_2D, 0, internal_format, desc.width, desc.height, 0, format, GL_UNSIGNED_BYTE, nullptr);
	GL_ASSERT;

	Gl2Texture texture;
	texture.texture = name;
	texture.desc = desc;
	return texture_slab_.insert(std::move(texture));
}

void Gl2Rhi::destroy_texture(rhi::Handle<rhi::Texture> handle)
{
	SRB2_ASSERT(texture_slab_.is_valid(handle) == true);
	Gl2Texture casted = texture_slab_.remove(handle);
	GLuint name = casted.texture;
	gl_->DeleteTextures(1, &name);
	GL_ASSERT;
}

void Gl2Rhi::update_texture(
	Handle<Texture> texture,
	Rect region,
	srb2::rhi::PixelFormat data_format,
	tcb::span<const std::byte> data
)
{
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
	GL_ASSERT;
	gl_->BindTexture(GL_TEXTURE_2D, t.texture);
	GL_ASSERT;
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
	GL_ASSERT;
}

void Gl2Rhi::update_texture_settings(
	Handle<Texture> texture,
	TextureWrapMode u_wrap,
	TextureWrapMode v_wrap,
	TextureFilterMode min,
	TextureFilterMode mag
)
{
	SRB2_ASSERT(texture_slab_.is_valid(texture) == true);
	auto& t = texture_slab_[texture];

	gl_->ActiveTexture(GL_TEXTURE0);
	GL_ASSERT;
	gl_->BindTexture(GL_TEXTURE_2D, t.texture);
	GL_ASSERT;
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, map_texture_wrap(u_wrap));
	GL_ASSERT;
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, map_texture_wrap(v_wrap));
	GL_ASSERT;
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, map_texture_filter(min));
	GL_ASSERT;
	gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, map_texture_filter(mag));
	GL_ASSERT;
}

rhi::Handle<rhi::Buffer> Gl2Rhi::create_buffer(const rhi::BufferDesc& desc)
{
	GLenum target = map_buffer_type(desc.type);
	SRB2_ASSERT(target != GL_ZERO);

	GLenum usage = map_buffer_usage(desc.usage);
	SRB2_ASSERT(usage != GL_ZERO);

	GLuint name = 0;
	gl_->GenBuffers(1, &name);
	GL_ASSERT;

	gl_->BindBuffer(target, name);
	GL_ASSERT;

	gl_->BufferData(target, desc.size, nullptr, usage);
	GL_ASSERT;

	Gl2Buffer buffer;
	buffer.buffer = name;
	buffer.desc = desc;
	return buffer_slab_.insert(std::move(buffer));
}

void Gl2Rhi::destroy_buffer(rhi::Handle<rhi::Buffer> handle)
{
	SRB2_ASSERT(buffer_slab_.is_valid(handle) == true);
	Gl2Buffer casted = buffer_slab_.remove(handle);
	GLuint name = casted.buffer;

	gl_->DeleteBuffers(1, &name);
	GL_ASSERT;
}

void Gl2Rhi::update_buffer(
	rhi::Handle<rhi::Buffer> handle,
	uint32_t offset,
	tcb::span<const std::byte> data
)
{
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
	GL_ASSERT;
	gl_->BufferSubData(target, offset, data.size(), data.data());
	GL_ASSERT;
}

rhi::Handle<rhi::UniformSet>
Gl2Rhi::create_uniform_set(const rhi::CreateUniformSetInfo& info)
{
	Gl2UniformSet uniform_set;

	for (auto& uniform : info.uniforms)
	{
		uniform_set.uniforms.push_back(uniform);
	}

	return uniform_set_slab_.insert(std::move(uniform_set));
}

rhi::Handle<rhi::BindingSet> Gl2Rhi::create_binding_set(
	Handle<Pipeline> pipeline,
	const rhi::CreateBindingSetInfo& info
)
{
	SRB2_ASSERT(pipeline_slab_.is_valid(pipeline) == true);
	auto& pl = pipeline_slab_[pipeline];

	SRB2_ASSERT(info.vertex_buffers.size() == pl.desc.vertex_input.buffer_layouts.size());

	Gl2BindingSet binding_set;

	for (auto& vertex_buffer : info.vertex_buffers)
	{
		binding_set.vertex_buffer_bindings.push_back(vertex_buffer);
	}

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

rhi::Handle<rhi::Renderbuffer> Gl2Rhi::create_renderbuffer(const rhi::RenderbufferDesc& desc)
{
	GLuint name = 0;
	gl_->GenRenderbuffers(1, &name);

	// Obtain storage up-front.
	gl_->BindRenderbuffer(GL_RENDERBUFFER, name);
	GL_ASSERT;

	// For consistency, while RHI does not specify the bit size of the depth or stencil components,
	// nor if they are packed or separate, each backend should be expected to create a packed depth-stencil
	// D24S8 format image.
	// This is despite modern AMD apparently not supporting this format in hardware. It ensures the
	// depth behavior between backends is the same. We should not brush up against performance issues in practice.

	// - GL Core requires both D24S8 and D32FS8 format support.
	// - GL 2 via ARB_framebuffer_object requires D24S8. Backend must require this extension.
	// - GLES 2 via OES_packed_depth_stencil requires D24S8. Backend must require this extension.
	// - Vulkan requires **one of** D24S8 or D32FS8. The backend must decide which format to use based on caps.
	//   (Even if D32FS8 is available, D24S8 should be preferred)

	// For reference, D32FS8 at 4k requires 64 MiB of linear memory. D24S8 is 32 MiB.

	gl_->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, desc.width, desc.height);
	GL_ASSERT;

	Gl2Renderbuffer rb;
	rb.renderbuffer = name;
	rb.desc = desc;
	return renderbuffer_slab_.insert(std::move(rb));
}

void Gl2Rhi::destroy_renderbuffer(rhi::Handle<rhi::Renderbuffer> handle)
{
	SRB2_ASSERT(renderbuffer_slab_.is_valid(handle) == true);
	Gl2Renderbuffer casted = renderbuffer_slab_.remove(handle);
	GLuint name = casted.renderbuffer;
	gl_->DeleteRenderbuffers(1, &name);
	GL_ASSERT;
}

rhi::Handle<rhi::Pipeline> Gl2Rhi::create_pipeline(const PipelineDesc& desc)
{
	SRB2_ASSERT(platform_ != nullptr);
	// TODO assert compatibility of pipeline description with program using ProgramRequirements

	const rhi::ProgramRequirements& reqs = rhi::program_requirements_for_program(desc.program);

	GLuint vertex = 0;
	GLuint fragment = 0;
	GLuint program = 0;
	Gl2Pipeline pipeline;

	auto [vert_srcs, frag_srcs] = platform_->find_shader_sources(desc.program);

	// GL 2 note:
	// Do not explicitly set GLSL version. Unversioned sources are required to be treated as 110, but writing 110
	// breaks the AMD driver's program linker in a bizarre way.

	// Process vertex shader sources
	std::vector<const char*> vert_sources;
	ShaderLoadContext vert_ctx;
	vert_ctx.set_version("120");
	for (auto& attribute : desc.vertex_input.attr_layouts)
	{
		for (auto const& require_attr : reqs.vertex_input.attributes)
		{
			if (require_attr.name == attribute.name && !require_attr.required)
			{
				vert_ctx.define(map_vertex_attribute_enable_define(attribute.name));
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
						vert_ctx.define(map_uniform_enable_define(uniform));
					}
				}
			}
		}
	}
	for (auto& src : vert_srcs)
	{
		vert_ctx.add_source(std::move(src));
	}
	vert_sources = vert_ctx.get_sources_array();

	// Process vertex shader sources
	std::vector<const char*> frag_sources;
	ShaderLoadContext frag_ctx;
	frag_ctx.set_version("120");
	for (auto& sampler : desc.sampler_input.enabled_samplers)
	{
		for (auto const& require_sampler : reqs.samplers.samplers)
		{
			if (sampler == require_sampler.name && !require_sampler.required)
			{
				frag_ctx.define(map_sampler_enable_define(sampler));
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
						frag_ctx.define(map_uniform_enable_define(uniform));
					}
				}
			}
		}
	}
	for (auto& src : frag_srcs)
	{
		frag_ctx.add_source(std::move(src));
	}
	frag_sources = frag_ctx.get_sources_array();

	vertex = gl_->CreateShader(GL_VERTEX_SHADER);
	gl_->ShaderSource(vertex, vert_sources.size(), vert_sources.data(), NULL);
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
	gl_->ShaderSource(fragment, frag_sources.size(), frag_sources.data(), NULL);
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

	std::unordered_map<std::string, Gl2ActiveUniform> active_attributes;
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
		GL_ASSERT;
		GLint location = gl_->GetAttribLocation(program, name);
		GL_ASSERT;
		active_attributes.insert({std::string(name), Gl2ActiveUniform {type, static_cast<GLuint>(location)}});
	}

	std::unordered_map<std::string, Gl2ActiveUniform> active_uniforms;
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
		GL_ASSERT;
		GLint location = gl_->GetUniformLocation(program, name);
		GL_ASSERT;
		active_uniforms.insert({std::string(name), Gl2ActiveUniform {type, static_cast<GLuint>(location)}});
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

void Gl2Rhi::destroy_pipeline(rhi::Handle<rhi::Pipeline> handle)
{
	SRB2_ASSERT(pipeline_slab_.is_valid(handle) == true);
	Gl2Pipeline casted = pipeline_slab_.remove(handle);
	GLuint vertex_shader = casted.vertex_shader;
	GLuint fragment_shader = casted.fragment_shader;
	GLuint program = casted.program;

	gl_->DeleteProgram(program);
	GL_ASSERT;
	gl_->DeleteShader(vertex_shader);
	GL_ASSERT;
	gl_->DeleteShader(fragment_shader);
	GL_ASSERT;
}

void Gl2Rhi::present()
{
	SRB2_ASSERT(platform_ != nullptr);

	platform_->present();
}

void Gl2Rhi::begin_default_render_pass(bool clear)
{
	SRB2_ASSERT(platform_ != nullptr);
	SRB2_ASSERT(current_render_pass_.has_value() == false);

	const Rect fb_rect = platform_->get_default_framebuffer_dimensions();

	gl_->BindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_ASSERT;
	gl_->Disable(GL_SCISSOR_TEST);
	GL_ASSERT;
	gl_->Viewport(0, 0, fb_rect.w, fb_rect.h);
	GL_ASSERT;

	if (clear)
	{
		gl_->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		gl_->ClearDepth(1.0f);
		gl_->ClearStencil(0);
		gl_->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		GL_ASSERT;
	}

	current_render_pass_ = Gl2Rhi::DefaultRenderPassState {};
}

void Gl2Rhi::begin_render_pass(const RenderPassBeginInfo& info)
{
	SRB2_ASSERT(current_render_pass_.has_value() == false);

	SRB2_ASSERT(render_pass_slab_.is_valid(info.render_pass) == true);
	auto& rp = render_pass_slab_[info.render_pass];
	SRB2_ASSERT(rp.desc.use_depth_stencil == info.depth_stencil_attachment.has_value());

	auto fb_itr = framebuffers_.find(Gl2FramebufferKey {info.color_attachment, info.depth_stencil_attachment});
	if (fb_itr == framebuffers_.end())
	{
		// Create a new framebuffer for this color-depth pair
		GLuint fb_name;
		gl_->GenFramebuffers(1, &fb_name);
		GL_ASSERT;
		gl_->BindFramebuffer(GL_FRAMEBUFFER, fb_name);
		GL_ASSERT;
		fb_itr = framebuffers_
					 .insert(
						 {Gl2FramebufferKey {info.color_attachment, info.depth_stencil_attachment},
						  static_cast<uint32_t>(fb_name)}
					 )
					 .first;

		SRB2_ASSERT(texture_slab_.is_valid(info.color_attachment));
		auto& texture = texture_slab_[info.color_attachment];
		SRB2_ASSERT(texture.desc.format == TextureFormat::kRGBA);
		gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.texture, 0);
		GL_ASSERT;

		if (rp.desc.use_depth_stencil && info.depth_stencil_attachment.has_value())
		{
			SRB2_ASSERT(renderbuffer_slab_.is_valid(*info.depth_stencil_attachment));
			auto& renderbuffer = renderbuffer_slab_[*info.depth_stencil_attachment];
			gl_->FramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER,
				renderbuffer.renderbuffer
			);
			GL_ASSERT;
		}
	}
	auto& fb = *fb_itr;
	gl_->BindFramebuffer(GL_FRAMEBUFFER, fb.second);
	GL_ASSERT;
	gl_->Disable(GL_SCISSOR_TEST);
	GL_ASSERT;

	GLint clear_bits = 0;
	if (rp.desc.color_load_op == rhi::AttachmentLoadOp::kClear)
	{
		gl_->ClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);
		clear_bits |= GL_COLOR_BUFFER_BIT;
	}

	if (rp.desc.use_depth_stencil)
	{
		if (rp.desc.depth_load_op == rhi::AttachmentLoadOp::kClear)
		{
			gl_->ClearDepth(1.f);
			clear_bits |= GL_DEPTH_BUFFER_BIT;
		}
		if (rp.desc.stencil_load_op == rhi::AttachmentLoadOp::kClear)
		{
			gl_->ClearStencil(0);
			clear_bits |= GL_STENCIL_BUFFER_BIT;
		}
	}

	if (clear_bits != 0)
	{
		gl_->Clear(clear_bits);
		GL_ASSERT;
	}

	current_render_pass_ = info;
}

void Gl2Rhi::end_render_pass()
{
	SRB2_ASSERT(current_render_pass_.has_value() == true);

	current_pipeline_ = std::nullopt;
	current_render_pass_ = std::nullopt;
}

void Gl2Rhi::bind_pipeline(Handle<Pipeline> pipeline)
{
	SRB2_ASSERT(current_render_pass_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(pipeline) == true);
	auto& pl = pipeline_slab_[pipeline];
	auto& desc = pl.desc;

	gl_->UseProgram(pl.program);
	GL_ASSERT;

	gl_->Disable(GL_SCISSOR_TEST);
	GL_ASSERT;

	if (desc.depth_stencil_state)
	{
		if (desc.depth_stencil_state->depth_test)
		{
			gl_->Enable(GL_DEPTH_TEST);
			GL_ASSERT;
			GLenum depth_func = map_compare_func(desc.depth_stencil_state->depth_func);
			SRB2_ASSERT(depth_func != GL_ZERO);
			gl_->DepthFunc(depth_func);
			GL_ASSERT;
			gl_->DepthMask(desc.depth_stencil_state->depth_write ? GL_TRUE : GL_FALSE);
			GL_ASSERT;
		}
		else
		{
			gl_->Disable(GL_DEPTH_TEST);
			GL_ASSERT;
		}

		if (desc.depth_stencil_state->depth_write)
		{
			gl_->DepthMask(GL_TRUE);
			GL_ASSERT;
		}
		else
		{
			gl_->DepthMask(GL_FALSE);
			GL_ASSERT;
		}

		if (desc.depth_stencil_state->stencil_test)
		{
			gl_->Enable(GL_STENCIL_TEST);
			stencil_front_reference_ = 0;
			stencil_back_reference_ = 0;
			stencil_front_compare_mask_ = 0xFF;
			stencil_back_compare_mask_ = 0xFF;
			stencil_front_write_mask_ = 0xFF;
			stencil_back_write_mask_ = 0xFF;
			GL_ASSERT;

			gl_->StencilFuncSeparate(
				GL_FRONT,
				map_compare_func(desc.depth_stencil_state->front.stencil_compare),
				stencil_front_reference_,
				stencil_front_compare_mask_
			);
			GL_ASSERT;
			gl_->StencilFuncSeparate(
				GL_BACK,
				map_compare_func(desc.depth_stencil_state->back.stencil_compare),
				stencil_back_reference_,
				stencil_back_compare_mask_
			);
			GL_ASSERT;

			gl_->StencilMaskSeparate(GL_FRONT, stencil_front_write_mask_);
			GL_ASSERT;
			gl_->StencilMaskSeparate(GL_BACK, stencil_back_write_mask_);
			GL_ASSERT;
		}
		else
		{
			gl_->Disable(GL_STENCIL_TEST);
			GL_ASSERT;
		}
	}
	else
	{
		gl_->Disable(GL_DEPTH_TEST);
		GL_ASSERT;
		gl_->Disable(GL_STENCIL_TEST);
		GL_ASSERT;
		gl_->StencilMask(0);
		GL_ASSERT;
	}

	if (desc.color_state.blend)
	{
		rhi::BlendDesc& bl = *desc.color_state.blend;
		gl_->Enable(GL_BLEND);
		GL_ASSERT;
		gl_->BlendFuncSeparate(
			map_blend_factor(bl.source_factor_color),
			map_blend_factor(bl.dest_factor_color),
			map_blend_factor(bl.source_factor_alpha),
			map_blend_factor(bl.dest_factor_alpha)
		);
		GL_ASSERT;
		gl_->BlendEquationSeparate(map_blend_function(bl.color_function), map_blend_function(bl.alpha_function));
		GL_ASSERT;
		gl_->BlendColor(desc.blend_color.r, desc.blend_color.g, desc.blend_color.b, desc.blend_color.a);
		GL_ASSERT;
	}
	else
	{
		gl_->Disable(GL_BLEND);
	}

	gl_->ColorMask(
		desc.color_state.color_mask.r ? GL_TRUE : GL_FALSE,
		desc.color_state.color_mask.g ? GL_TRUE : GL_FALSE,
		desc.color_state.color_mask.b ? GL_TRUE : GL_FALSE,
		desc.color_state.color_mask.a ? GL_TRUE : GL_FALSE
	);
	GL_ASSERT;

	GLenum cull_face = map_cull_mode(desc.cull);
	if (cull_face == GL_NONE)
	{
		gl_->Disable(GL_CULL_FACE);
		GL_ASSERT;
	}
	else
	{
		gl_->Enable(GL_CULL_FACE);
		GL_ASSERT;
		gl_->CullFace(cull_face);
		GL_ASSERT;
	}
	gl_->FrontFace(map_winding(desc.winding));
	GL_ASSERT;

	current_pipeline_ = pipeline;
	current_primitive_type_ = desc.primitive;
}

void Gl2Rhi::bind_uniform_set(uint32_t slot, Handle<UniformSet> set)
{
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
				GL_ASSERT;
			},
			[&](const glm::vec2& value)
			{
				gl_->Uniform2f(pipeline_uniform, value.x, value.y);
				GL_ASSERT;
			},
			[&](const glm::vec3& value)
			{
				gl_->Uniform3f(pipeline_uniform, value.x, value.y, value.z);
				GL_ASSERT;
			},
			[&](const glm::vec4& value)
			{
				gl_->Uniform4f(pipeline_uniform, value.x, value.y, value.z, value.w);
				GL_ASSERT;
			},
			[&](const int32_t& value)
			{
				gl_->Uniform1i(pipeline_uniform, value);
				GL_ASSERT;
			},
			[&](const glm::ivec2& value)
			{
				gl_->Uniform2i(pipeline_uniform, value.x, value.y);
				GL_ASSERT;
			},
			[&](const glm::ivec3& value)
			{
				gl_->Uniform3i(pipeline_uniform, value.x, value.y, value.z);
				GL_ASSERT;
			},
			[&](const glm::ivec4& value)
			{
				gl_->Uniform4i(pipeline_uniform, value.x, value.y, value.z, value.w);
				GL_ASSERT;
			},
			[&](const glm::mat2& value)
			{
				gl_->UniformMatrix2fv(pipeline_uniform, 1, false, glm::value_ptr(value));
				GL_ASSERT;
			},
			[&](const glm::mat3& value)
			{
				gl_->UniformMatrix3fv(pipeline_uniform, 1, false, glm::value_ptr(value));
				GL_ASSERT;
			},
			[&](const glm::mat4& value)
			{
				gl_->UniformMatrix4fv(pipeline_uniform, 1, false, glm::value_ptr(value));
				GL_ASSERT;
			},
		};
		std::visit(visitor, update_data);
	}
}

void Gl2Rhi::bind_binding_set(Handle<BindingSet> set)
{
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(*current_pipeline_));
	auto& pl = pipeline_slab_[*current_pipeline_];

	SRB2_ASSERT(binding_set_slab_.is_valid(set));
	auto& bs = binding_set_slab_[set];

	SRB2_ASSERT(bs.textures.size() == pl.desc.sampler_input.enabled_samplers.size());

	// TODO only disable the vertex attributes of the previously bound pipeline (performance)
	for (GLuint i = 0; i < kMaxVertexAttributes; i++)
	{
		gl_->DisableVertexAttribArray(i);
	}

	// Update the vertex attributes with the new vertex buffer bindings.

	// OpenGL 2 does not require binding buffers to the pipeline the same way Vulkan does.
	// Instead, we need to find the pipeline vertex attributes which would be affected by
	// the changing set of vertex buffers, and reassign their Vertex Attribute Pointers.
	for (size_t i = 0; i < pl.desc.vertex_input.attr_layouts.size(); i++)
	{
		auto& attr_layout = pl.desc.vertex_input.attr_layouts[i];
		uint32_t attr_buffer_index = attr_layout.buffer_index;
		VertexAttributeName attr_name = attr_layout.name;

		auto& buffer_layout = pl.desc.vertex_input.buffer_layouts[attr_buffer_index];

		SRB2_ASSERT(pl.attrib_locations.find(attr_name) != pl.attrib_locations.end());
		auto gl_attr_location = pl.attrib_locations[pl.desc.vertex_input.attr_layouts[i].name];

		VertexAttributeFormat vert_attr_format = rhi::vertex_attribute_format(attr_name);
		GLenum vertex_attr_type = map_vertex_attribute_type(vert_attr_format);
		SRB2_ASSERT(vertex_attr_type != GL_ZERO);
		GLint vertex_attr_size = map_vertex_attribute_format_size(vert_attr_format);
		SRB2_ASSERT(vertex_attr_size != 0);

		uint32_t vertex_buffer_offset = 0;
		auto& vertex_binding = bs.vertex_buffer_bindings[attr_layout.buffer_index];
		rhi::Handle<rhi::Buffer> vertex_buffer_handle = vertex_binding.vertex_buffer;
		SRB2_ASSERT(buffer_slab_.is_valid(vertex_binding.vertex_buffer) == true);
		auto& buffer = *static_cast<Gl2Buffer*>(&buffer_slab_[vertex_buffer_handle]);
		SRB2_ASSERT(buffer.desc.type == rhi::BufferType::kVertexBuffer);

		gl_->BindBuffer(GL_ARRAY_BUFFER, buffer.buffer);
		gl_->EnableVertexAttribArray(gl_attr_location);
		gl_->VertexAttribPointer(
			gl_attr_location,
			vertex_attr_size,
			vertex_attr_type,
			GL_FALSE,
			buffer_layout.stride,
			reinterpret_cast<const void*>(vertex_buffer_offset + attr_layout.offset)
		);
	}

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
		GL_ASSERT;
		gl_->BindTexture(GL_TEXTURE_2D, texture_gl_name);
		GL_ASSERT;
		gl_->Uniform1i(sampler_uniform_loc, uniform_value);
		GL_ASSERT;
	}
}

void Gl2Rhi::bind_index_buffer(Handle<Buffer> buffer)
{
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	SRB2_ASSERT(buffer_slab_.is_valid(buffer));
	auto& ib = buffer_slab_[buffer];

	SRB2_ASSERT(ib.desc.type == rhi::BufferType::kIndexBuffer);

	current_index_buffer_ = buffer;

	gl_->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.buffer);
}

void Gl2Rhi::set_scissor(const Rect& rect)
{
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	gl_->Enable(GL_SCISSOR_TEST);
	gl_->Scissor(rect.x, rect.y, rect.w, rect.h);
}

void Gl2Rhi::set_viewport(const Rect& rect)
{
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	gl_->Viewport(rect.x, rect.y, rect.w, rect.h);
	GL_ASSERT;
}

void Gl2Rhi::draw(uint32_t vertex_count, uint32_t first_vertex)
{
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	gl_->DrawArrays(map_primitive_mode(current_primitive_type_), first_vertex, vertex_count);
	GL_ASSERT;
}

void Gl2Rhi::draw_indexed(uint32_t index_count, uint32_t first_index)
{
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
	GL_ASSERT;
}

void Gl2Rhi::read_pixels(const Rect& rect, PixelFormat format, tcb::span<std::byte> out)
{
	SRB2_ASSERT(current_render_pass_.has_value());

	std::tuple<GLenum, GLenum, GLuint> gl_format = map_pixel_data_format(format);
	GLenum layout = std::get<0>(gl_format);
	GLenum type = std::get<1>(gl_format);
	GLint size = std::get<2>(gl_format);

	// Pack alignment comes into play.
	uint32_t pack_stride = (rect.w * size + (kPixelRowPackAlignment - 1)) & ~(kPixelRowPackAlignment - 1);

	SRB2_ASSERT(out.size_bytes() == pack_stride * rect.h);

	bool is_back;
	Rect src_dim;
	auto render_pass_visitor = srb2::Overload {
		[&](const DefaultRenderPassState& state) {
			is_back = true;
			src_dim = platform_->get_default_framebuffer_dimensions();
		},
		[&](const RenderPassBeginInfo& state) {
			is_back = false;
			SRB2_ASSERT(texture_slab_.is_valid(state.color_attachment));
			auto& attach_tex = texture_slab_[state.color_attachment];
			src_dim = {0, 0, attach_tex.desc.width, attach_tex.desc.height};
		}
	};
	std::visit(render_pass_visitor, *current_render_pass_);

	SRB2_ASSERT(rect.x >= 0);
	SRB2_ASSERT(rect.y >= 0);
	SRB2_ASSERT(rect.x + rect.w <= src_dim.w);
	SRB2_ASSERT(rect.y + rect.h <= src_dim.h);

	GLenum read_buffer = is_back ? GL_BACK_LEFT : GL_COLOR_ATTACHMENT0;
	gl_->ReadBuffer(read_buffer);
	GL_ASSERT;

	gl_->ReadPixels(rect.x, rect.y, rect.w, rect.h, layout, type, out.data());
	GL_ASSERT;
}

void Gl2Rhi::set_stencil_reference(CullMode face, uint8_t reference)
{
	SRB2_ASSERT(face != CullMode::kNone);
	SRB2_ASSERT(current_render_pass_.has_value());
	SRB2_ASSERT(current_pipeline_.has_value());

	auto& pl = pipeline_slab_[*current_pipeline_];

	if (face == CullMode::kFront)
	{
		stencil_front_reference_ = reference;
		gl_->StencilFuncSeparate(
			GL_FRONT,
			map_compare_func(pl.desc.depth_stencil_state->front.stencil_compare),
			stencil_front_reference_,
			stencil_front_compare_mask_
		);
	}
	else if (face == CullMode::kBack)
	{
		stencil_back_reference_ = reference;
		gl_->StencilFuncSeparate(
			GL_BACK,
			map_compare_func(pl.desc.depth_stencil_state->back.stencil_compare),
			stencil_back_reference_,
			stencil_back_compare_mask_
		);
	}
}

void Gl2Rhi::set_stencil_compare_mask(CullMode face, uint8_t compare_mask)
{
	SRB2_ASSERT(face != CullMode::kNone);
	SRB2_ASSERT(current_render_pass_.has_value());
	SRB2_ASSERT(current_pipeline_.has_value());

	auto& pl = pipeline_slab_[*current_pipeline_];

	if (face == CullMode::kFront)
	{
		stencil_front_compare_mask_ = compare_mask;
		gl_->StencilFuncSeparate(
			GL_FRONT,
			map_compare_func(pl.desc.depth_stencil_state->front.stencil_compare),
			stencil_front_reference_,
			stencil_front_compare_mask_
		);
	}
	else if (face == CullMode::kBack)
	{
		stencil_back_compare_mask_ = compare_mask;
		gl_->StencilFuncSeparate(
			GL_BACK,
			map_compare_func(pl.desc.depth_stencil_state->back.stencil_compare),
			stencil_back_reference_,
			stencil_back_compare_mask_
		);
	}
}

void Gl2Rhi::set_stencil_write_mask(CullMode face, uint8_t write_mask)
{
	SRB2_ASSERT(face != CullMode::kNone);
	SRB2_ASSERT(current_render_pass_.has_value());
	SRB2_ASSERT(current_pipeline_.has_value());

	if (face == CullMode::kFront)
	{
		stencil_front_write_mask_ = write_mask;
		gl_->StencilMaskSeparate(GL_FRONT, stencil_front_write_mask_);
	}
	else if (face == CullMode::kBack)
	{
		stencil_back_write_mask_ = write_mask;
		gl_->StencilMaskSeparate(GL_BACK, stencil_back_write_mask_);
	}
}

TextureDetails Gl2Rhi::get_texture_details(Handle<Texture> texture)
{
	SRB2_ASSERT(texture_slab_.is_valid(texture));
	auto& t = texture_slab_[texture];

	TextureDetails ret {};
	ret.format = t.desc.format;
	ret.width = t.desc.width;
	ret.height = t.desc.height;

	return ret;
}

Rect Gl2Rhi::get_renderbuffer_size(Handle<Renderbuffer> renderbuffer)
{
	SRB2_ASSERT(renderbuffer_slab_.is_valid(renderbuffer));
	auto& rb = renderbuffer_slab_[renderbuffer];

	Rect ret {};
	ret.x = 0;
	ret.y = 0;
	ret.w = rb.desc.width;
	ret.h = rb.desc.height;

	return ret;
}

uint32_t Gl2Rhi::get_buffer_size(Handle<Buffer> buffer)
{
	SRB2_ASSERT(buffer_slab_.is_valid(buffer));
	auto& buf = buffer_slab_[buffer];

	return buf.desc.size;
}

void Gl2Rhi::finish()
{
	binding_set_slab_.clear();
	uniform_set_slab_.clear();

	// I sure hope creating FBOs isn't costly on the driver!
	for (auto& fbset : framebuffers_)
	{
		gl_->DeleteFramebuffers(1, (GLuint*)&fbset.second);
		GL_ASSERT;
	}
	framebuffers_.clear();
}

void Gl2Rhi::copy_framebuffer_to_texture(
	Handle<Texture> dst_tex,
	const Rect& dst_region,
	const Rect& src_region
)
{
	SRB2_ASSERT(current_render_pass_.has_value());
	SRB2_ASSERT(texture_slab_.is_valid(dst_tex));

	auto& tex = texture_slab_[dst_tex];
	SRB2_ASSERT(dst_region.w == src_region.w);
	SRB2_ASSERT(dst_region.h == src_region.h);
	SRB2_ASSERT(dst_region.x >= 0);
	SRB2_ASSERT(dst_region.y >= 0);
	SRB2_ASSERT(dst_region.x + dst_region.w <= tex.desc.width);
	SRB2_ASSERT(dst_region.y + dst_region.h <= tex.desc.height);

	bool is_back;
	Rect src_dim;
	auto render_pass_visitor = srb2::Overload {
		[&](const DefaultRenderPassState& state) {
			is_back = true;
			src_dim = platform_->get_default_framebuffer_dimensions();
		},
		[&](const RenderPassBeginInfo& state) {
			is_back = false;
			SRB2_ASSERT(texture_slab_.is_valid(state.color_attachment));
			auto& attach_tex = texture_slab_[state.color_attachment];
			src_dim = {0, 0, attach_tex.desc.width, attach_tex.desc.height};
		}
	};
	std::visit(render_pass_visitor, *current_render_pass_);

	SRB2_ASSERT(src_region.x >= 0);
	SRB2_ASSERT(src_region.y >= 0);
	SRB2_ASSERT(src_region.x + src_region.w <= src_dim.w);
	SRB2_ASSERT(src_region.y + src_region.h <= src_dim.h);

	GLenum read_buffer = is_back ? GL_BACK_LEFT : GL_COLOR_ATTACHMENT0;
	gl_->ReadBuffer(read_buffer);
	GL_ASSERT;

	gl_->BindTexture(GL_TEXTURE_2D, tex.texture);
	GL_ASSERT;
	gl_->CopyTexSubImage2D(GL_TEXTURE_2D, 0, dst_region.x, dst_region.y, src_region.x, src_region.y, dst_region.w, dst_region.h);
	GL_ASSERT;
}
