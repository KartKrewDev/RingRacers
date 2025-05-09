// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
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
#include <utility>

#include <fmt/format.h>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include "../../core/vector.hpp"
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

constexpr GLenum map_stencil_op(rhi::StencilOp op)
{
	switch (op)
	{
	case rhi::StencilOp::kKeep:
		return GL_KEEP;
	case rhi::StencilOp::kZero:
		return GL_ZERO;
	case rhi::StencilOp::kReplace:
		return GL_REPLACE;
	case rhi::StencilOp::kIncrementClamp:
		return GL_INCR;
	case rhi::StencilOp::kIncrementWrap:
		return GL_INCR_WRAP;
	case rhi::StencilOp::kDecrementClamp:
		return GL_DECR;
	case rhi::StencilOp::kDecrementWrap:
		return GL_DECR_WRAP;
	case rhi::StencilOp::kInvert:
		return GL_INVERT;
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

rhi::Handle<rhi::Program> Gl2Rhi::create_program(const ProgramDesc& desc)
{
	SRB2_ASSERT(platform_ != nullptr);
	Gl2Program program;

	auto [vert_srcs, frag_srcs] = platform_->find_shader_sources(desc.name);

	// GL 2 note:
	// Do not explicitly set GLSL version. Unversioned sources are required to be treated as 110, but writing 110
	// breaks the AMD driver's program linker in a bizarre way.

	// Process shader sources
	srb2::Vector<const char*> vert_sources;
	srb2::Vector<const char*> frag_sources;
	ShaderLoadContext vert_ctx;
	ShaderLoadContext frag_ctx;
	vert_ctx.set_version("120");
	frag_ctx.set_version("120");
	for (auto def : desc.defines)
	{
		vert_ctx.define(def);
		frag_ctx.define(def);
	}
	for (auto& src : vert_srcs)
	{
		vert_ctx.add_source(std::move(src));
	}
	for (auto& src : frag_srcs)
	{
		frag_ctx.add_source(std::move(src));
	}
	vert_sources = vert_ctx.get_sources_array();
	frag_sources = frag_ctx.get_sources_array();

	GLint is_compiled = 0;

	program.vertex_shader = gl_->CreateShader(GL_VERTEX_SHADER);
	GL_ASSERT;
	gl_->ShaderSource(program.vertex_shader, vert_sources.size(), vert_sources.data(), nullptr);
	gl_->CompileShader(program.vertex_shader);
	gl_->GetShaderiv(program.vertex_shader, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		gl_->GetShaderiv(program.vertex_shader, GL_INFO_LOG_LENGTH, &max_length);
		srb2::Vector<GLchar> compile_error(max_length);
		gl_->GetShaderInfoLog(program.vertex_shader, max_length, &max_length, compile_error.data());

		gl_->DeleteShader(program.vertex_shader);
		throw std::runtime_error(fmt::format("Vertex shader compilation failed: {}", String(compile_error.data())));
	}

	program.fragment_shader = gl_->CreateShader(GL_FRAGMENT_SHADER);
	GL_ASSERT;
	gl_->ShaderSource(program.fragment_shader, frag_sources.size(), frag_sources.data(), nullptr);
	gl_->CompileShader(program.fragment_shader);
	gl_->GetShaderiv(program.fragment_shader, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		gl_->GetShaderiv(program.fragment_shader, GL_INFO_LOG_LENGTH, &max_length);
		srb2::Vector<GLchar> compile_error(max_length);
		gl_->GetShaderInfoLog(program.fragment_shader, max_length, &max_length, compile_error.data());

		gl_->DeleteShader(program.fragment_shader);
		throw std::runtime_error(fmt::format("Fragment shader compilation failed: {}", String(compile_error.data())));
	}

	program.program = gl_->CreateProgram();
	GL_ASSERT;

	gl_->AttachShader(program.program, program.vertex_shader);
	gl_->AttachShader(program.program, program.fragment_shader);
	gl_->LinkProgram(program.program);
	gl_->GetProgramiv(program.program, GL_LINK_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		gl_->GetProgramiv(program.program, GL_INFO_LOG_LENGTH, &max_length);
		srb2::Vector<GLchar> link_error(max_length);
		gl_->GetProgramInfoLog(program.program, max_length, &max_length, link_error.data());

		gl_->DeleteProgram(program.program);
		gl_->DeleteShader(program.fragment_shader);
		gl_->DeleteShader(program.vertex_shader);
		throw std::runtime_error(fmt::format("Pipeline program link failed: {}", String(link_error.data())));
	}

	// get attribute information
	GLint active_attribute_total = -1;
	gl_->GetProgramiv(program.program, GL_ACTIVE_ATTRIBUTES, &active_attribute_total);
	if (active_attribute_total < 0)
	{
		gl_->DeleteProgram(program.program);
		gl_->DeleteShader(program.fragment_shader);
		gl_->DeleteShader(program.vertex_shader);
		throw std::runtime_error("Unable to retrieve program active attributes");
	}
	for (GLint i = 0; i < active_attribute_total; i++)
	{
		GLsizei name_len = 0;
		GLint size = 0;
		GLenum type = GL_ZERO;
		char name[256];
		gl_->GetActiveAttrib(program.program, i, 255, &name_len, &size, &type, name);
		GL_ASSERT;
		GLint location = gl_->GetAttribLocation(program.program, name);
		GL_ASSERT;
		program.attrib_locations[String(name)] = location;
	}

	// get uniform information
	GLint active_uniform_total = -1;
	gl_->GetProgramiv(program.program, GL_ACTIVE_UNIFORMS, &active_uniform_total);
	if (active_uniform_total < 0)
	{
		gl_->DeleteProgram(program.program);
		gl_->DeleteShader(program.fragment_shader);
		gl_->DeleteShader(program.vertex_shader);
		throw std::runtime_error("Unable to retrieve program active uniforms");
	}
	for (GLint i = 0; i < active_uniform_total; i++)
	{
		GLsizei name_len = 0;
		GLint size = 0;
		GLenum type = GL_ZERO;
		char name[256];
		gl_->GetActiveUniform(program.program, i, 255, &name_len, &size, &type, name);
		GL_ASSERT;
		GLint location = gl_->GetUniformLocation(program.program, name);
		GL_ASSERT;
		program.uniform_locations[String(name)] = location;
	}

	Handle<Program> program_handle = program_slab_.insert(std::move(program));
	return program_handle;
}

void Gl2Rhi::destroy_program(Handle<Program> handle)
{
	SRB2_ASSERT(program_slab_.is_valid(handle) == true);
	Gl2Program casted = program_slab_.remove(handle);
	gl_->DeleteProgram(casted.program);
	GL_ASSERT;
	gl_->DeleteShader(casted.fragment_shader);
	GL_ASSERT;
	gl_->DeleteShader(casted.vertex_shader);
	GL_ASSERT;
}

void Gl2Rhi::present()
{
	SRB2_ASSERT(platform_ != nullptr);

	platform_->present();
}

void Gl2Rhi::apply_default_framebuffer(bool clear)
{
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
}

void Gl2Rhi::apply_framebuffer(const RenderPassBeginInfo& info, bool allow_clear)
{
	auto fb_itr = framebuffers_.find(Gl2FramebufferKey {info.color_attachment, info.depth_stencil_attachment});
	if (fb_itr == framebuffers_.end())
	{
		// Create a new framebuffer for this color-depth pair
		GLuint fb_name;
		gl_->GenFramebuffers(1, &fb_name);
		GL_ASSERT;
		gl_->BindFramebuffer(GL_FRAMEBUFFER, fb_name);
		GL_ASSERT;
		fb_itr = framebuffers_.insert({
			Gl2FramebufferKey {info.color_attachment, info.depth_stencil_attachment},
			static_cast<uint32_t>(fb_name)
		}).first;

		SRB2_ASSERT(texture_slab_.is_valid(info.color_attachment));
		auto& texture = texture_slab_[info.color_attachment];
		SRB2_ASSERT(texture.desc.format == TextureFormat::kRGBA);
		gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.texture, 0);
		GL_ASSERT;

		if (info.depth_stencil_attachment.has_value())
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

	GLint clear_bits = 0;
	if (info.color_load_op == rhi::AttachmentLoadOp::kClear)
	{
		gl_->ClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);
		clear_bits |= GL_COLOR_BUFFER_BIT;
	}

	if (info.depth_load_op == rhi::AttachmentLoadOp::kClear)
	{
		gl_->ClearDepth(1.f);
		clear_bits |= GL_DEPTH_BUFFER_BIT;
	}
	if (info.stencil_load_op == rhi::AttachmentLoadOp::kClear)
	{
		gl_->ClearStencil(0);
		clear_bits |= GL_STENCIL_BUFFER_BIT;
	}

	if (clear_bits != 0 && allow_clear)
	{
		gl_->Clear(clear_bits);
		GL_ASSERT;
	}
}

void Gl2Rhi::push_default_render_pass(bool clear)
{
	SRB2_ASSERT(platform_ != nullptr);

	render_pass_stack_.emplace_back(Gl2Rhi::DefaultRenderPassState { clear });
	apply_default_framebuffer(clear);
}

void Gl2Rhi::push_render_pass(const RenderPassBeginInfo& info)
{
	render_pass_stack_.push_back(info);
	apply_framebuffer(info, false);
}

void Gl2Rhi::pop_render_pass()
{
	SRB2_ASSERT(render_pass_stack_.empty() == false);

	current_program_ = std::nullopt;

	render_pass_stack_.pop_back();

	if (!render_pass_stack_.empty())
	{
		RenderPassState& state = *render_pass_stack_.rbegin();
		// We must not clear the framebuffer when restoring a previous framebuffer,
		// even if the clear was previously requested.
		auto visitor = srb2::Overload {
			[this](const DefaultRenderPassState& s) {
				apply_default_framebuffer(false);
			},
			[this](const RenderPassBeginInfo& info) {
				apply_framebuffer(info, false);
			}
		};
		std::visit(visitor, state);
	}
}

void Gl2Rhi::bind_program(Handle<Program> program)
{
	SRB2_ASSERT(render_pass_stack_.empty() == false);
	Gl2Program& prog = program_slab_[program];

	gl_->UseProgram(prog.program);
	current_program_ = program;
	GLint max_attribs;
	gl_->GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
	GL_ASSERT;
	for (GLint i = 0; i < max_attribs; i++)
	{
		gl_->DisableVertexAttribArray(i);
		GL_ASSERT;
	}
}

void Gl2Rhi::bind_vertex_attrib(const char* name, Handle<Buffer> buffer, VertexAttributeFormat format, uint32_t offset, uint32_t stride)
{
	SRB2_ASSERT(current_program_.has_value());
	SRB2_ASSERT(buffer_slab_.is_valid(buffer));

	GLenum vertex_attr_type = map_vertex_attribute_type(format);
	SRB2_ASSERT(vertex_attr_type != GL_ZERO);
	GLint vertex_attr_size = map_vertex_attribute_format_size(format);
	SRB2_ASSERT(vertex_attr_size != 0);

	Gl2Program& prog = program_slab_[*current_program_];
	Gl2Buffer& cast_buffer = buffer_slab_[buffer];

	SRB2_ASSERT(cast_buffer.desc.type == rhi::BufferType::kVertexBuffer);

	GLint location = gl_->GetAttribLocation(prog.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->BindBuffer(GL_ARRAY_BUFFER, cast_buffer.buffer);
	GL_ASSERT;
	gl_->EnableVertexAttribArray(location);
	GL_ASSERT;
	gl_->VertexAttribPointer(
		location,
		vertex_attr_size,
		vertex_attr_type,
		GL_FALSE,
		stride,
		reinterpret_cast<const void*>(offset)
	);
	GL_ASSERT;
}

void Gl2Rhi::bind_index_buffer(Handle<Buffer> buffer)
{
	SRB2_ASSERT(current_program_.has_value());

	SRB2_ASSERT(buffer_slab_.is_valid(buffer));
	auto& ib = buffer_slab_[buffer];

	SRB2_ASSERT(ib.desc.type == rhi::BufferType::kIndexBuffer);

	current_index_buffer_ = buffer;

	gl_->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.buffer);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, float value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform1f(location, value);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, int value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform1i(location, value);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::vec2 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform2f(location, value.x, value.y);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::vec3 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform3f(location, value.x, value.y, value.z);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::vec4 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform4f(location, value.x, value.y, value.z, value.w);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::ivec2 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform2i(location, value.x, value.y);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::ivec3 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform3i(location, value.x, value.y, value.z);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::ivec4 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->Uniform4i(location, value.x, value.y, value.z, value.w);
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::mat2 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->UniformMatrix2fv(location, 1, false, glm::value_ptr(value));
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::mat3 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->UniformMatrix3fv(location, 1, false, glm::value_ptr(value));
	GL_ASSERT;
}

void Gl2Rhi::set_uniform(const char* name, glm::mat4 value)
{
	SRB2_ASSERT(current_program_.has_value());
	Gl2Program& cast = program_slab_[*current_program_];
	GLint location = gl_->GetUniformLocation(cast.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	gl_->UniformMatrix4fv(location, 1, false, glm::value_ptr(value));
	GL_ASSERT;
}

void Gl2Rhi::set_sampler(const char* name, uint32_t slot, Handle<Texture> texture)
{
	SRB2_ASSERT(slot >= 0 && slot < kMaxSamplers);
	SRB2_ASSERT(current_program_.has_value() && render_pass_stack_.empty() == false);

	Gl2Program& prog = program_slab_[*current_program_];
	SRB2_ASSERT(texture_slab_.is_valid(texture));
	Gl2Texture& tex = texture_slab_[texture];
	GLint location = gl_->GetUniformLocation(prog.program, name);
	GL_ASSERT;
	SRB2_ASSERT(location >= 0);
	GLenum active_texture = GL_TEXTURE0 + slot;
	GLuint uniform_value = slot;
	gl_->ActiveTexture(active_texture);
	GL_ASSERT;
	gl_->BindTexture(GL_TEXTURE_2D, tex.texture);
	GL_ASSERT;
	gl_->Uniform1i(location, uniform_value);
	GL_ASSERT;
	gl_->ActiveTexture(GL_TEXTURE0);
	GL_ASSERT;
}

void Gl2Rhi::set_rasterizer_state(const RasterizerStateDesc& desc)
{
	current_primitive_type_ = desc.primitive;
	if (desc.cull == CullMode::kNone)
	{
		gl_->Disable(GL_CULL_FACE);
	}
	else
	{
		gl_->Enable(GL_CULL_FACE);
		gl_->CullFace(map_cull_mode(desc.cull));
	}
	gl_->ColorMask(desc.color_mask.r, desc.color_mask.g, desc.color_mask.b, desc.color_mask.a);

	GL_ASSERT;
	gl_->FrontFace(map_winding(desc.winding));
	GL_ASSERT;
	if (desc.blend_enabled)
	{
		gl_->Enable(GL_BLEND);
	}
	else
	{
		gl_->Disable(GL_BLEND);
	}
	GL_ASSERT;
	gl_->BlendFuncSeparate(
		map_blend_factor(desc.blend_source_factor_color),
		map_blend_factor(desc.blend_dest_factor_color),
		map_blend_factor(desc.blend_source_factor_alpha),
		map_blend_factor(desc.blend_dest_factor_alpha)
	);
	GL_ASSERT;
	gl_->BlendEquationSeparate(
		map_blend_function(desc.blend_color_function),
		map_blend_function(desc.blend_alpha_function)
	);
	GL_ASSERT;
	gl_->BlendColor(desc.blend_color.r, desc.blend_color.g, desc.blend_color.b, desc.blend_color.a);
	GL_ASSERT;
	if (desc.depth_test)
	{
		gl_->Enable(GL_DEPTH_TEST);
	}
	else
	{
		gl_->Disable(GL_DEPTH_TEST);
	}
	GL_ASSERT;
	gl_->DepthMask(desc.depth_write);
	GL_ASSERT;
	gl_->DepthFunc(map_compare_func(desc.depth_func));
	GL_ASSERT;
	if (desc.stencil_test)
	{
		gl_->Enable(GL_STENCIL_TEST);
	}
	else
	{
		gl_->Disable(GL_STENCIL_TEST);
	}
	GL_ASSERT;
	stencil_front_reference_ = 0;
	stencil_back_reference_ = 0;
	stencil_front_compare_mask_ = 0xFF;
	stencil_back_compare_mask_ = 0xFF;
	stencil_front_write_mask_ = 0xFF;
	stencil_back_write_mask_ = 0xFF;
	stencil_front_func_ = desc.front_stencil_compare;
	stencil_back_func_ = desc.back_stencil_compare;
	gl_->StencilFuncSeparate(
		GL_FRONT,
		map_compare_func(stencil_front_func_),
		stencil_front_reference_,
		stencil_front_compare_mask_
	);
	GL_ASSERT;
	gl_->StencilFuncSeparate(
		GL_BACK,
		map_compare_func(stencil_back_func_),
		stencil_back_reference_,
		stencil_back_compare_mask_
	);
	GL_ASSERT;
	gl_->StencilMaskSeparate(GL_FRONT, stencil_front_write_mask_);
	GL_ASSERT;
	gl_->StencilMaskSeparate(GL_BACK, stencil_back_write_mask_);
	GL_ASSERT;

	gl_->StencilOpSeparate(
		GL_FRONT,
		map_stencil_op(desc.front_fail),
		map_stencil_op(desc.front_depth_fail),
		map_stencil_op(desc.front_pass)
	);
	GL_ASSERT;
	gl_->StencilOpSeparate(
		GL_BACK,
		map_stencil_op(desc.back_fail),
		map_stencil_op(desc.back_depth_fail),
		map_stencil_op(desc.back_pass)
	);
	GL_ASSERT;
	if (desc.scissor_test)
	{
		gl_->Enable(GL_SCISSOR_TEST);
		GL_ASSERT;
	}
	else
	{
		gl_->Disable(GL_SCISSOR_TEST);
	}
	gl_->Scissor(desc.scissor.x, desc.scissor.y, desc.scissor.w, desc.scissor.h);
	GL_ASSERT;
}

void Gl2Rhi::set_viewport(const Rect& rect)
{
	SRB2_ASSERT(render_pass_stack_.empty() == false);

	gl_->Viewport(rect.x, rect.y, rect.w, rect.h);
	GL_ASSERT;
}

void Gl2Rhi::draw(uint32_t vertex_count, uint32_t first_vertex)
{
	SRB2_ASSERT(render_pass_stack_.empty() == false);

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
	SRB2_ASSERT(render_pass_stack_.empty() == false);

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
	std::visit(render_pass_visitor, *render_pass_stack_.rbegin());

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
	SRB2_ASSERT(render_pass_stack_.empty() == false);

	if (face == CullMode::kFront)
	{
		stencil_front_reference_ = reference;
		gl_->StencilFuncSeparate(
			GL_FRONT,
			map_compare_func(stencil_front_func_),
			stencil_front_reference_,
			stencil_front_compare_mask_
		);
	}
	else if (face == CullMode::kBack)
	{
		stencil_back_reference_ = reference;
		gl_->StencilFuncSeparate(
			GL_BACK,
			map_compare_func(stencil_back_func_),
			stencil_back_reference_,
			stencil_back_compare_mask_
		);
	}
}

void Gl2Rhi::set_stencil_compare_mask(CullMode face, uint8_t compare_mask)
{
	SRB2_ASSERT(face != CullMode::kNone);
	SRB2_ASSERT(render_pass_stack_.empty() == false);

	if (face == CullMode::kFront)
	{
		stencil_front_compare_mask_ = compare_mask;
		gl_->StencilFuncSeparate(
			GL_FRONT,
			map_compare_func(stencil_front_func_),
			stencil_front_reference_,
			stencil_front_compare_mask_
		);
	}
	else if (face == CullMode::kBack)
	{
		stencil_back_compare_mask_ = compare_mask;
		gl_->StencilFuncSeparate(
			GL_BACK,
			map_compare_func(stencil_back_func_),
			stencil_back_reference_,
			stencil_back_compare_mask_
		);
	}
}

void Gl2Rhi::set_stencil_write_mask(CullMode face, uint8_t write_mask)
{
	SRB2_ASSERT(face != CullMode::kNone);
	SRB2_ASSERT(render_pass_stack_.empty() == false);

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
	SRB2_ASSERT(render_pass_stack_.empty() == false);
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
	std::visit(render_pass_visitor, *render_pass_stack_.rbegin());

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
