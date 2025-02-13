// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "gles2_rhi.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <glad/gles2.h>

using namespace srb2;

using srb2::rhi::Gles2Platform;
using srb2::rhi::Gles2Rhi;

namespace
{

template <typename D, typename B>
std::unique_ptr<D, std::default_delete<D>> static_unique_ptr_cast(std::unique_ptr<B, std::default_delete<B>> ptr)
{
	D* derived = static_cast<D*>(ptr.release());
	return std::unique_ptr<D, std::default_delete<D>>(derived, std::default_delete<D>());
}

constexpr GLenum map_pixel_format(rhi::PixelFormat format)
{
	switch (format)
	{
	case rhi::PixelFormat::kRGBA8:
		// requires extension GL_OES_rgb8_rgba8, which is always requested
		return GL_RGBA8_OES;
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

/*
constexpr const char* map_vertex_attribute_enable_define(rhi::VertexAttributeName name) {
	switch (name) {
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
*/

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
	default:
		return nullptr;
	}
}

/*
constexpr const char* map_uniform_attribute_enable_define(rhi::UniformName name) {
	switch (name) {
	case rhi::UniformName::kTime:
		return "ENABLE_U_TIME";
	case rhi::UniformName::kModelView:
		return "ENABLE_U_MODELVIEW";
	case rhi::UniformName::kProjection:
		return "ENABLE_U_PROJECTION";
	default:
		return nullptr;
	}
}
*/

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

struct Gles2Texture : public rhi::Texture
{
	GLuint texture;
	rhi::TextureDesc desc;
	Gles2Texture(GLuint texture, const rhi::TextureDesc& desc) noexcept : texture(texture), desc(desc) {}
};

struct Gles2Buffer : public rhi::Buffer
{
	GLuint buffer;
	rhi::BufferDesc desc;
	Gles2Buffer(GLuint buffer, const rhi::BufferDesc& desc) noexcept : buffer(buffer), desc(desc) {}
};

struct Gles2RenderPass : public rhi::RenderPass
{
	rhi::RenderPassDesc desc;
	explicit Gles2RenderPass(const rhi::RenderPassDesc& desc) noexcept : desc(desc) {}
};

struct Gles2Renderbuffer : public rhi::Renderbuffer
{
	GLuint renderbuffer;

	explicit Gles2Renderbuffer(GLuint renderbuffer) noexcept : renderbuffer(renderbuffer) {}
};

struct Gles2Pipeline : public rhi::Pipeline
{
	GLuint vertex_shader = 0;
	GLuint fragment_shader = 0;
	GLuint program = 0;
	std::unordered_map<rhi::VertexAttributeName, GLuint> attrib_locations {2};
	std::unordered_map<rhi::UniformName, GLuint> uniform_locations {2};
	std::unordered_map<rhi::SamplerName, GLuint> sampler_locations {2};
	rhi::PipelineDesc desc;

	Gles2Pipeline() = default;
	explicit Gles2Pipeline(
		GLuint vertex_shader,
		GLuint fragment_shader,
		GLuint program,
		const rhi::PipelineDesc& desc
	) noexcept
		: vertex_shader(vertex_shader), fragment_shader(fragment_shader), program(program), desc(desc)
	{
	}
};

struct Gles2GraphicsContext : public rhi::GraphicsContext
{
};

struct Gles2ActiveUniform
{
	GLenum type;
	GLuint location;
};

} // namespace

Gles2Platform::~Gles2Platform() = default;

Gles2Rhi::Gles2Rhi(std::unique_ptr<Gles2Platform>&& platform) : platform_(std::move(platform))
{
}

Gles2Rhi::~Gles2Rhi() = default;

rhi::Handle<rhi::RenderPass> Gles2Rhi::create_render_pass(const rhi::RenderPassDesc& desc)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	// GL has no formal render pass object
	return render_pass_slab_.insert(std::make_unique<Gles2RenderPass>(desc));
}

void Gles2Rhi::destroy_render_pass(rhi::Handle<rhi::RenderPass>&& handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	std::unique_ptr<rhi::RenderPass> buffer = render_pass_slab_.remove(handle);
	std::unique_ptr<Gles2RenderPass> casted(static_cast<Gles2RenderPass*>(buffer.release()));
}

rhi::Handle<rhi::Texture> Gles2Rhi::create_texture(
	const rhi::TextureDesc& desc,
	srb2::rhi::PixelFormat data_format,
	tcb::span<const std::byte> data
)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	GLint internal_format = map_texture_format(desc.format);
	SRB2_ASSERT(internal_format != GL_ZERO);

	GLuint name = 0;
	glGenTextures(1, &name);

	glBindTexture(GL_TEXTURE_2D, name);

	// if no data is provided, the initial texture is undefined
	GLenum format = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;
	GLuint size = 0;

	const void* raw_data = nullptr;
	std::tie(format, type, size) = map_pixel_data_format(data_format);
	SRB2_ASSERT(format != GL_ZERO && type != GL_ZERO);
	SRB2_ASSERT(internal_format == format);
	if (!data.empty())
	{
		SRB2_ASSERT(size * desc.width * desc.height == data.size_bytes());
		raw_data = static_cast<const void*>(data.data());
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, desc.width, desc.height, 0, format, type, raw_data);

	return texture_slab_.insert(std::make_unique<Gles2Texture>(name, desc));
}

void Gles2Rhi::destroy_texture(rhi::Handle<rhi::Texture>&& handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(texture_slab_.is_valid(handle) == true);
	std::unique_ptr<Gles2Texture> casted = static_unique_ptr_cast<Gles2Texture>(texture_slab_.remove(handle));
	GLuint name = casted->texture;
	disposal_.push_back([name] { glDeleteTextures(1, &name); });
}

void Gles2Rhi::update_texture(
	Handle<Texture> texture,
	Rect region,
	srb2::rhi::PixelFormat data_format,
	tcb::span<const std::byte> data
)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(texture_slab_.is_valid(texture) == true);
	auto& t = *static_cast<Gles2Texture*>(&texture_slab_[texture]);

	GLenum format = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;
	GLuint size = 0;
	std::tie(format, type, size) = map_pixel_data_format(data_format);
	SRB2_ASSERT(format != GL_ZERO && type != GL_ZERO);
	SRB2_ASSERT(map_texture_format(t.desc.format) == format);
	SRB2_ASSERT(region.w * region.h * size == data.size_bytes());
	SRB2_ASSERT(region.x + region.w < t.desc.width && region.y + region.h < t.desc.height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t.texture);
	glTexSubImage2D(
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
}

rhi::Handle<rhi::Buffer> Gles2Rhi::create_buffer(const rhi::BufferDesc& desc, tcb::span<const std::byte> data)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	// If data is provided, it must match the buffer description size exactly
	SRB2_ASSERT(data.size() != 0 ? data.size() == desc.size : true);

	GLenum target = map_buffer_type(desc.type);
	SRB2_ASSERT(target != GL_ZERO);

	GLenum usage = map_buffer_usage(desc.usage);
	SRB2_ASSERT(usage != GL_ZERO);

	GLuint name = 0;
	glGenBuffers(1, &name);

	glBindBuffer(target, name);

	// if no data is provided, the initial buffer data is undefined
	const void* raw_data = nullptr;
	if (!data.empty())
	{
		raw_data = static_cast<const void*>(data.data());
	}
	glBufferData(target, desc.size, raw_data, usage);

	return buffer_slab_.insert(std::make_unique<Gles2Buffer>(name, desc));
}

void Gles2Rhi::destroy_buffer(rhi::Handle<rhi::Buffer>&& handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(buffer_slab_.is_valid(handle) == true);
	SRB2_ASSERT(graphics_context_active_ == false);
	std::unique_ptr<Gles2Buffer> casted = static_unique_ptr_cast<Gles2Buffer>(buffer_slab_.remove(handle));
	GLuint name = casted->buffer;

	disposal_.push_back([name] { glDeleteBuffers(1, &name); });
}

void Gles2Rhi::update_buffer_contents(rhi::Handle<rhi::Buffer> handle, uint32_t offset, tcb::span<const std::byte> data)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(buffer_slab_.is_valid(handle) == true);
	auto& b = *static_cast<Gles2Buffer*>(&buffer_slab_[handle]);

	if (data.size() == 0)
		return;

	SRB2_ASSERT(offset < b.desc.size && offset + data.size() < b.desc.size);

	switch (b.desc.type)
	{
	case rhi::BufferType::kVertexBuffer:
		glBindBuffer(GL_ARRAY_BUFFER, b.buffer);
		glBufferSubData(GL_ARRAY_BUFFER, offset, data.size(), data.data());
		break;
	case rhi::BufferType::kIndexBuffer:
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.buffer);
		glBufferSubData(GL_ARRAY_BUFFER, offset, data.size(), data.data());
		break;
	}
}

rhi::Handle<rhi::Renderbuffer> Gles2Rhi::create_renderbuffer(const rhi::RenderbufferDesc& desc)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	GLuint name = 0;
	glGenRenderbuffers(1, &name);

	// Obtain storage up-front.
	glBindRenderbuffer(GL_RENDERBUFFER, name);
	glRenderbufferStorage(GL_RENDERBUFFER, map_pixel_format(desc.format), desc.width, desc.height);

	return renderbuffer_slab_.insert(std::make_unique<Gles2Renderbuffer>(Gles2Renderbuffer {name}));
}

void Gles2Rhi::destroy_renderbuffer(rhi::Handle<rhi::Renderbuffer>&& handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(renderbuffer_slab_.is_valid(handle) == true);
	std::unique_ptr<Gles2Renderbuffer> casted =
		static_unique_ptr_cast<Gles2Renderbuffer>(renderbuffer_slab_.remove(handle));
	GLuint name = casted->renderbuffer;
	disposal_.push_back([name] { glDeleteRenderbuffers(1, &name); });
}

rhi::Handle<rhi::Pipeline> Gles2Rhi::create_pipeline(const PipelineDesc& desc)
{
	SRB2_ASSERT(platform_ != nullptr);
	// TODO assert compatibility of pipeline description with program using ProgramRequirements

	GLuint vertex = 0;
	GLuint fragment = 0;
	GLuint program = 0;
	Gles2Pipeline pipeline;

	auto [vert_src, frag_src] = platform_->find_shader_sources(desc.program);

	// TODO preprocess shader code with specialization defines based on pipeline configuration

	const char* vert_src_arr[1] = {vert_src.c_str()};
	const GLint vert_src_arr_lens[1] = {static_cast<GLint>(vert_src.size())};
	const char* frag_src_arr[1] = {frag_src.c_str()};
	const GLint frag_src_arr_lens[1] = {static_cast<GLint>(frag_src.size())};

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, vert_src_arr, vert_src_arr_lens);
	glCompileShader(vertex);
	GLint is_compiled = 0;
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		glGetShaderiv(vertex, GL_INFO_LOG_LENGTH, &max_length);
		std::vector<GLchar> compile_error(max_length);
		glGetShaderInfoLog(vertex, max_length, &max_length, compile_error.data());

		glDeleteShader(vertex);
		throw std::runtime_error(std::string("Vertex shader compilation failed: ") + std::string(compile_error.data()));
	}
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, frag_src_arr, frag_src_arr_lens);
	glCompileShader(fragment);
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		glGetShaderiv(fragment, GL_INFO_LOG_LENGTH, &max_length);
		std::vector<GLchar> compile_error(max_length);
		glGetShaderInfoLog(fragment, max_length, &max_length, compile_error.data());

		glDeleteShader(fragment);
		glDeleteShader(vertex);
		throw std::runtime_error(
			std::string("Fragment shader compilation failed: ") + std::string(compile_error.data())
		);
	}
	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);
		std::vector<GLchar> link_error(max_length);
		glGetProgramInfoLog(program, max_length, &max_length, link_error.data());

		glDeleteProgram(program);
		glDeleteShader(fragment);
		glDeleteShader(vertex);
		throw std::runtime_error(std::string("Pipeline program link failed: ") + std::string(link_error.data()));
	}

	std::unordered_map<std::string, Gles2ActiveUniform> active_attributes;
	GLint active_attribute_total = -1;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active_attribute_total);
	if (active_attribute_total < 0)
	{
		glDeleteProgram(program);
		glDeleteShader(fragment);
		glDeleteShader(vertex);
		throw std::runtime_error("Unable to retrieve program active attributes");
	}
	if (desc.vertex_input.attr_layouts.size() != static_cast<GLuint>(active_attribute_total))
	{
		glDeleteProgram(program);
		glDeleteShader(fragment);
		glDeleteShader(vertex);
		std::string ex_msg("Pipeline's enabled attribute count does not match the linked program's total: ");
		ex_msg.append(std::to_string(desc.vertex_input.attr_layouts.size()));
		ex_msg.append(" vs ");
		ex_msg.append(std::to_string(static_cast<GLuint>(active_attribute_total)));
		throw std::runtime_error(std::move(ex_msg));
	}
	for (GLint i = 0; i < active_attribute_total; i++)
	{
		GLsizei name_len = 0;
		GLint size = 0;
		GLenum type = GL_ZERO;
		char name[256];
		glGetActiveAttrib(program, i, 255, &name_len, &size, &type, name);
		active_attributes.insert({std::string(name), Gles2ActiveUniform {type, static_cast<GLuint>(i)}});
	}

	std::unordered_map<std::string, Gles2ActiveUniform> active_uniforms;
	GLint active_uniform_total = -1;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &active_uniform_total);
	if (active_uniform_total < 0)
	{
		glDeleteProgram(program);
		glDeleteShader(fragment);
		glDeleteShader(vertex);
		throw std::runtime_error("Unable to retrieve program active uniforms");
	}
	if (desc.uniform_input.enabled_uniforms.size() + desc.sampler_input.enabled_samplers.size() !=
		static_cast<GLuint>(active_uniform_total))
	{
		glDeleteProgram(program);
		glDeleteShader(fragment);
		glDeleteShader(vertex);
		std::string ex_msg(
			"Pipeline's enabled uniform count (uniforms + samplers) does not match the linked program's total: "
		);
		ex_msg.append(std::to_string(desc.uniform_input.enabled_uniforms.size()));
		ex_msg.append(" vs ");
		ex_msg.append(std::to_string(static_cast<GLuint>(active_uniform_total)));
		throw std::runtime_error(std::move(ex_msg));
	}
	for (GLint i = 0; i < active_uniform_total; i++)
	{
		GLsizei name_len = 0;
		GLint size = 0;
		GLenum type = GL_ZERO;
		char name[256];
		glGetActiveUniform(program, i, 255, &name_len, &size, &type, name);
		active_uniforms.insert({std::string(name), Gles2ActiveUniform {type, static_cast<GLuint>(i)}});
	}

	for (auto& attr : desc.vertex_input.attr_layouts)
	{
		const char* symbol_name = map_vertex_attribute_symbol_name(attr.name);
		SRB2_ASSERT(symbol_name != nullptr);
		if (active_attributes.find(symbol_name) == active_attributes.end())
		{
			glDeleteProgram(program);
			glDeleteShader(fragment);
			glDeleteShader(vertex);
			throw std::runtime_error("Enabled attribute not found in linked program");
		}
		auto& active_attr = active_attributes[symbol_name];
		auto expected_format = rhi::vertex_attribute_format(attr.name);
		auto expected_gl_type = map_vertex_attribute_format(expected_format);
		SRB2_ASSERT(expected_gl_type != GL_ZERO);
		if (expected_gl_type != active_attr.type)
		{
			glDeleteProgram(program);
			glDeleteShader(fragment);
			glDeleteShader(vertex);
			throw std::runtime_error("Active attribute type does not match expected type");
		}

		pipeline.attrib_locations.insert({attr.name, active_attr.location});
	}
	for (auto& uniform : desc.uniform_input.enabled_uniforms)
	{
		const char* symbol_name = map_uniform_attribute_symbol_name(uniform);
		SRB2_ASSERT(symbol_name != nullptr);
		if (active_uniforms.find(symbol_name) == active_uniforms.end())
		{
			glDeleteProgram(program);
			glDeleteShader(fragment);
			glDeleteShader(vertex);
			throw std::runtime_error("Enabled uniform not found in linked program");
		}
		auto& active_uniform = active_uniforms[symbol_name];
		auto expected_format = rhi::uniform_format(uniform);
		auto expected_gl_type = map_uniform_format(expected_format);
		SRB2_ASSERT(expected_gl_type != GL_ZERO);
		if (expected_gl_type != active_uniform.type)
		{
			glDeleteProgram(program);
			glDeleteShader(fragment);
			glDeleteShader(vertex);
			throw std::runtime_error("Active uniform type does not match expected type");
		}

		pipeline.uniform_locations.insert({uniform, active_uniform.location});
	}
	for (auto& sampler : desc.sampler_input.enabled_samplers)
	{
		const char* symbol_name = map_sampler_symbol_name(sampler);
		SRB2_ASSERT(symbol_name != nullptr);
		if (active_uniforms.find(symbol_name) == active_uniforms.end())
		{
			glDeleteProgram(program);
			glDeleteShader(fragment);
			glDeleteShader(vertex);
			throw std::runtime_error("Enabled sampler not found in linked program");
		}
		auto& active_sampler = active_uniforms[symbol_name];
		if (active_sampler.type != GL_SAMPLER_2D)
		{
			glDeleteProgram(program);
			glDeleteShader(fragment);
			glDeleteShader(vertex);
			throw std::runtime_error("Active sampler type does not match expected type");
		}

		pipeline.sampler_locations.insert({sampler, active_sampler.location});
	}

	pipeline.desc = desc;
	pipeline.vertex_shader = vertex;
	pipeline.fragment_shader = fragment;
	pipeline.program = program;

	return pipeline_slab_.insert(std::make_unique<Gles2Pipeline>(std::move(pipeline)));
}

void Gles2Rhi::destroy_pipeline(rhi::Handle<rhi::Pipeline>&& handle)
{
	SRB2_ASSERT(graphics_context_active_ == false);

	SRB2_ASSERT(pipeline_slab_.is_valid(handle) == true);
	std::unique_ptr<Gles2Pipeline> casted = static_unique_ptr_cast<Gles2Pipeline>(pipeline_slab_.remove(handle));
	GLuint vertex_shader = casted->vertex_shader;
	GLuint fragment_shader = casted->fragment_shader;
	GLuint program = casted->program;

	disposal_.push_back([=] { glDeleteShader(fragment_shader); });
	disposal_.push_back([=] { glDeleteShader(vertex_shader); });
	disposal_.push_back([=] { glDeleteProgram(program); });
}

rhi::Handle<rhi::GraphicsContext> Gles2Rhi::begin_graphics()
{
	SRB2_ASSERT(graphics_context_active_ == false);
	graphics_context_active_ = true;
	return rhi::Handle<rhi::GraphicsContext>(0, graphics_context_generation_);
}

void Gles2Rhi::end_graphics(rhi::Handle<rhi::GraphicsContext>&& handle)
{
	SRB2_ASSERT(graphics_context_active_ == true);
	SRB2_ASSERT(current_pipeline_.has_value() == false && current_render_pass_.has_value() == false);
	graphics_context_generation_ += 1;
	graphics_context_active_ = false;
	glFlush();
}

void Gles2Rhi::present()
{
	SRB2_ASSERT(platform_ != nullptr);
	SRB2_ASSERT(graphics_context_active_ == false);

	platform_->present();
}

void Gles2Rhi::push_default_render_pass(Handle<GraphicsContext> ctx)
{
	SRB2_ASSERT(platform_ != nullptr);
	SRB2_ASSERT(graphics_context_active_ == true);
	SRB2_ASSERT(current_render_pass_.has_value() == false);

	const Rect fb_rect = platform_->get_default_framebuffer_dimensions();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_rect.w, fb_rect.h);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepthf(1.0f);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	current_render_pass_ = Gles2Rhi::DefaultRenderPassState {};
}

void Gles2Rhi::push_render_pass(Handle<GraphicsContext> ctx, const RenderPassBeginInfo& info)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == false);

	SRB2_ASSERT(render_pass_slab_.is_valid(info.render_pass) == true);
	auto& rp = *static_cast<Gles2RenderPass*>(&render_pass_slab_[info.render_pass]);
	SRB2_ASSERT(rp.desc.depth_format.has_value() == info.depth_attachment.has_value());

	auto fb_itr = framebuffers_.find(Gles2FramebufferKey {info.color_attachment, info.depth_attachment});
	if (fb_itr == framebuffers_.end())
	{
		// Create a new framebuffer for this color-depth pair
		GLuint fb_name;
		glGenFramebuffers(1, &fb_name);
		glBindFramebuffer(GL_FRAMEBUFFER, fb_name);
		fb_itr =
			framebuffers_
				.insert(
					{Gles2FramebufferKey {info.color_attachment, info.depth_attachment}, static_cast<uint32_t>(fb_name)}
				)
				.first;

		// TODO bind buffers correctly
	}
	auto& fb = *fb_itr;
	glBindFramebuffer(GL_FRAMEBUFFER, fb.second);

	if (rp.desc.load_op == rhi::AttachmentLoadOp::kClear)
	{
		glClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);
		glClearDepthf(1.f);
		glClearStencil(0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	current_render_pass_ = info;
}

void Gles2Rhi::pop_render_pass(Handle<GraphicsContext> ctx)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true);

	current_pipeline_ = std::nullopt;
	current_render_pass_ = std::nullopt;
}

void Gles2Rhi::bind_pipeline(Handle<GraphicsContext> ctx, Handle<Pipeline> pipeline)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(pipeline) == true);
	auto& pl = *static_cast<Gles2Pipeline*>(&pipeline_slab_[pipeline]);
	auto& desc = pl.desc;

	glUseProgram(pl.program);

	glDisable(GL_SCISSOR_TEST);

	if (desc.depth_attachment)
	{
		glEnable(GL_DEPTH_TEST);
		GLenum depth_func = map_compare_func(desc.depth_attachment->func);
		SRB2_ASSERT(depth_func != GL_ZERO);
		glDepthFunc(depth_func);
		glDepthMask(desc.depth_attachment->write ? GL_TRUE : GL_FALSE);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}

	if (desc.color_attachment.blend)
	{
		rhi::BlendDesc& bl = *desc.color_attachment.blend;
		glEnable(GL_BLEND);
		glBlendFuncSeparate(
			map_blend_factor(bl.source_factor_color),
			map_blend_factor(bl.dest_factor_color),
			map_blend_factor(bl.source_factor_alpha),
			map_blend_factor(bl.dest_factor_alpha)
		);
		glBlendEquationSeparate(map_blend_function(bl.color_function), map_blend_function(bl.alpha_function));
		glBlendColor(desc.blend_color.r, desc.blend_color.g, desc.blend_color.b, desc.blend_color.a);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	glColorMask(
		desc.color_attachment.color_mask.r ? GL_TRUE : GL_FALSE,
		desc.color_attachment.color_mask.g ? GL_TRUE : GL_FALSE,
		desc.color_attachment.color_mask.b ? GL_TRUE : GL_FALSE,
		desc.color_attachment.color_mask.a ? GL_TRUE : GL_FALSE
	);

	GLenum cull_face = map_cull_mode(desc.cull);
	if (cull_face == GL_NONE)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);
		glCullFace(cull_face);
	}
	glFrontFace(map_winding(desc.winding));

	current_pipeline_ = pipeline;
	current_primitive_type_ = desc.primitive;
}

void Gles2Rhi::update_bindings(Handle<GraphicsContext> ctx, const UpdateBindingsInfo& info)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(*current_pipeline_));
	auto& pl = *static_cast<Gles2Pipeline*>(&pipeline_slab_[*current_pipeline_]);

	// TODO assert compatibility of binding data with pipeline
	SRB2_ASSERT(info.vertex_buffers.size() == pl.desc.vertex_input.buffer_layouts.size());
	SRB2_ASSERT(info.sampler_textures.size() == pl.desc.sampler_input.enabled_samplers.size());

	// TODO only disable the vertex attributes of the previously bound pipeline (performance)
	for (GLuint i = 0; i < kMaxVertexAttributes; i++)
	{
		glDisableVertexAttribArray(i);
	}

	// Update the vertex attributes with the new vertex buffer bindings.

	// OpenGL ES does not require binding buffers to the pipeline the same way Vulkan does.
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

		rhi::Handle<rhi::Buffer> vertex_buffer_handle;
		uint32_t vertex_buffer_offset;
		std::tie(vertex_buffer_handle, vertex_buffer_offset) = info.vertex_buffers[attr_layout.buffer_index];
		SRB2_ASSERT(buffer_slab_.is_valid(vertex_buffer_handle) == true);
		auto& buffer = *static_cast<Gles2Buffer*>(&buffer_slab_[vertex_buffer_handle]);
		SRB2_ASSERT(buffer.desc.type == rhi::BufferType::kVertexBuffer);

		glBindBuffer(GL_ARRAY_BUFFER, buffer.buffer);
		glEnableVertexAttribArray(gl_attr_location);
		glVertexAttribPointer(
			gl_attr_location,
			vertex_attr_size,
			vertex_attr_type,
			GL_FALSE,
			buffer_layout.stride,
			reinterpret_cast<const void*>(vertex_buffer_offset + attr_layout.offset)
		);
	}

	rhi::Handle<rhi::Buffer> index_buffer_handle;
	std::tie(index_buffer_handle, index_buffer_offset_) = info.index_buffer;
	if (index_buffer_handle == rhi::kNullHandle)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		SRB2_ASSERT(buffer_slab_.is_valid(index_buffer_handle));
		auto& ib = *static_cast<Gles2Buffer*>(&buffer_slab_[index_buffer_handle]);
		SRB2_ASSERT(ib.desc.type == rhi::BufferType::kIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.buffer);
	}

	for (size_t i = 0; i < info.sampler_textures.size(); i++)
	{
		auto& sampler_name = pl.desc.sampler_input.enabled_samplers[i];
		rhi::Handle<rhi::Texture> texture_handle = info.sampler_textures[i];
		SRB2_ASSERT(texture_slab_.is_valid(texture_handle));
		auto& t = *static_cast<Gles2Texture*>(&texture_slab_[texture_handle]);
		SRB2_ASSERT(pl.sampler_locations.find(sampler_name) != pl.sampler_locations.end());
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
		glActiveTexture(active_texture);
		glBindTexture(GL_TEXTURE_2D, t.texture);
		glUniform1i(sampler_uniform_loc, uniform_value);
	}
}

void Gles2Rhi::update_uniforms(Handle<GraphicsContext> ctx, tcb::span<UniformUpdateData> uniforms)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	SRB2_ASSERT(pipeline_slab_.is_valid(*current_pipeline_));
	auto& pl = *static_cast<Gles2Pipeline*>(&pipeline_slab_[*current_pipeline_]);

	// TODO assert compatibility of uniform data with pipeline
	// The uniforms need to be the same size, names and value types as the pipeline.
	// RHI doesn't support updating selectively; the whole set must be updated at once altogether.
	SRB2_ASSERT(uniforms.size() == pl.desc.uniform_input.enabled_uniforms.size());

	for (size_t i = 0; i < uniforms.size(); i++)
	{
		auto& update_data = uniforms[i];

		SRB2_ASSERT(pl.uniform_locations.find(update_data.name) != pl.uniform_locations.end());
		SRB2_ASSERT(pl.desc.uniform_input.enabled_uniforms[i] == update_data.name);
		GLuint pipeline_uniform = pl.uniform_locations[update_data.name];

		struct UniformVariantVisitor
		{
			rhi::UniformName name;
			GLuint uniform;

			void operator()(const float& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kFloat);
				glUniform1f(uniform, value);
			}
			void operator()(const std::array<float, 2>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kFloat2);
				glUniform2f(uniform, value[0], value[1]);
			}
			void operator()(const std::array<float, 3>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kFloat3);
				glUniform3f(uniform, value[0], value[1], value[2]);
			}
			void operator()(const std::array<float, 4>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kFloat4);
				glUniform4f(uniform, value[0], value[1], value[2], value[3]);
			}
			void operator()(const int32_t& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kInt);
				glUniform1i(uniform, value);
			}
			void operator()(const std::array<int32_t, 2>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kInt2);
				glUniform2i(uniform, value[0], value[1]);
			}
			void operator()(const std::array<int32_t, 3>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kInt3);
				glUniform3i(uniform, value[0], value[1], value[2]);
			}
			void operator()(const std::array<int32_t, 4>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kInt4);
				glUniform4i(uniform, value[0], value[1], value[2], value[3]);
			}
			void operator()(const std::array<std::array<float, 2>, 2>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kMat2);
				glUniformMatrix2fv(uniform, 1, false, reinterpret_cast<const GLfloat*>(&value));
			}
			void operator()(const std::array<std::array<float, 3>, 3>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kMat3);
				glUniformMatrix3fv(uniform, 1, false, reinterpret_cast<const GLfloat*>(&value));
			}
			void operator()(const std::array<std::array<float, 4>, 4>& value) const noexcept
			{
				SRB2_ASSERT(rhi::uniform_format(name) == rhi::UniformFormat::kMat4);
				glUniformMatrix4fv(uniform, 1, false, reinterpret_cast<const GLfloat*>(&value));
			}
		};
		std::visit(UniformVariantVisitor {update_data.name, pipeline_uniform}, update_data.value);
	}
}

void Gles2Rhi::set_scissor(Handle<GraphicsContext> ctx, const Rect& rect)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);
	// TODO handle scissor pipeline state
}

void Gles2Rhi::set_viewport(Handle<GraphicsContext> ctx, const Rect& rect)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);
	// TODO handle viewport pipeline state
}

void Gles2Rhi::draw(Handle<GraphicsContext> ctx, uint32_t vertex_count, uint32_t first_vertex)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	SRB2_ASSERT(current_render_pass_.has_value() == true && current_pipeline_.has_value() == true);

	glDrawArrays(map_primitive_mode(current_primitive_type_), first_vertex, vertex_count);
}

void Gles2Rhi::draw_indexed(
	Handle<GraphicsContext> ctx,
	uint32_t index_count,
	uint32_t first_index,
	uint32_t vertex_offset
)
{
	SRB2_ASSERT(graphics_context_active_ == true && graphics_context_generation_ == ctx.generation());
	glDrawElements(
		map_primitive_mode(current_primitive_type_),
		first_index,
		GL_UNSIGNED_SHORT,
		reinterpret_cast<const void*>(index_buffer_offset_)
	);
}

void Gles2Rhi::finish()
{
	SRB2_ASSERT(graphics_context_active_ == false);

	for (auto it = disposal_.begin(); it != disposal_.end(); it++)
	{
		(*it)();
	}
	disposal_.clear();
}

void rhi::load_gles2(Gles2LoadFunc func)
{
	gladLoadGLES2(static_cast<GLADloadfunc>(func));
}
