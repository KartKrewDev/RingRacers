#include "i_video.h"

#include <algorithm>
#include <array>
#include <vector>

#include <imgui.h>

#include "cxxutil.hpp"
#include "hwr2/pass_imgui.hpp"
#include "hwr2/pass_software.hpp"
#include "v_video.h"

// KILL THIS WHEN WE KILL OLD OGL SUPPORT PLEASE
#include "sdl/ogl_sdl.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

static SoftwareBlitPass g_sw_pass;
static ImguiPass g_imgui_pass;

Handle<Rhi> srb2::sys::g_current_rhi = kNullHandle;

static bool rhi_changed()
{
	return false;
}

void I_FinishUpdate(void)
{
	if (rendermode == render_none)
	{
		return;
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		OglSdlFinishUpdate(cv_vidwait.value);
		return;
	}
#endif

	// TODO move this to srb2loop
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = vid.realwidth;
	io.DisplaySize.y = vid.realheight;
	ImGui::NewFrame();

	if (rhi_changed())
	{
		// reinitialize passes
		g_sw_pass = SoftwareBlitPass();
		g_imgui_pass = ImguiPass();
	}

	rhi::Rhi* rhi = sys::get_rhi(sys::g_current_rhi);

	if (rhi == nullptr)
	{
		// ???
		return;
	}

	// Prepare phase
	if (rendermode == render_soft)
	{
		g_sw_pass.prepass(*rhi);
	}
	g_imgui_pass.prepass(*rhi);

	// Transfer phase
	Handle<TransferContext> tc;
	tc = rhi->begin_transfer();

	if (rendermode == render_soft)
	{
		g_sw_pass.transfer(*rhi, tc);
	}
	g_imgui_pass.transfer(*rhi, tc);

	rhi->end_transfer(tc);

	// Graphics phase
	Handle<GraphicsContext> gc;
	gc = rhi->begin_graphics();

	// Standard drawing passes...
	if (rendermode == render_soft)
	{
		g_sw_pass.graphics(*rhi, gc);
	}
	g_imgui_pass.graphics(*rhi, gc);

	rhi->end_graphics(gc);

	// Postpass phase
	if (rendermode == render_soft)
	{
		g_sw_pass.postpass(*rhi);
	}
	g_imgui_pass.postpass(*rhi);

	// Present

	rhi->present();

	rhi->finish();
}
