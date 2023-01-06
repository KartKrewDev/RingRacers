#ifndef __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__
#define __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__

#include "../rhi/rhi.hpp"
#include "../rhi/gl3_core/gl3_core_rhi.hpp"

#include <SDL.h>

namespace srb2::rhi {

struct SdlGlCorePlatform final : public GlCorePlatform {
	SDL_Window* window = nullptr;

	virtual ~SdlGlCorePlatform();

	virtual void present() override;
	virtual std::tuple<std::string, std::string> find_shader_sources(PipelineProgram program) override;
	virtual Rect get_default_framebuffer_dimensions() override;
};

} // namespace srb2::rhi

#endif // __SRB2_SDL_RHI_GLES2_PLATFORM_HPP__
