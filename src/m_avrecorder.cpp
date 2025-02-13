// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>

#include <fmt/format.h>
#include <tcb/span.hpp>

#include "cxxutil.hpp"
#include "m_avrecorder.hpp"
#include "media/options.hpp"

#include "command.h"
#include "i_sound.h"
#include "m_avrecorder.h"
#include "m_fixed.h"
#include "screen.h"	  // vid global
#include "st_stuff.h" // st_palette
#include "v_video.h"  // pLocalPalette

using namespace srb2::media;

namespace
{

namespace Res
{

// Using an unscoped enum here so it can implicitly cast to
// int (in CV_PossibleValue_t). Wrap this in a namespace so
// access is still scoped. E.g. Res::kGame

enum : int32_t
{
	kGame, // user chosen resolution, vid.width
	kBase, // smallest version maintaining aspect ratio, vid.width / vid.dupx
	kBase2x,
	kBase4x,
	kWindow, // window size (monitor in fullscreen), vid.realwidth
	kCustom, // movie_custom_resolution
};

}; // namespace Res

}; // namespace

extern "C" {

CV_PossibleValue_t movie_resolution_cons_t[] = {
	{Res::kGame, "Native"},
	{Res::kBase, "Small"},
	{Res::kBase2x, "Medium"},
	{Res::kBase4x, "Large"},
	{Res::kWindow, "Window"},
	{Res::kCustom, "Custom"},
	{0, NULL}};

}; // extern "C"

std::shared_ptr<AVRecorder> g_av_recorder;

void M_AVRecorder_AddCommands(void)
{
	srb2::media::Options::register_all();
}

static AVRecorder::Config configure()
{
	AVRecorder::Config cfg {};

	if (cv_movie_duration.value > 0)
	{
		cfg.max_duration = std::chrono::duration<float>(FixedToFloat(cv_movie_duration.value));
	}

	if (cv_movie_size.value > 0)
	{
		cfg.max_size = FixedToFloat(cv_movie_size.value) * 1024 * 1024;
	}

	if (sound_started && cv_movie_sound.value)
	{
		cfg.audio = AVRecorder::Config::Audio { 44100 };
	}

	cfg.video = AVRecorder::Config::Video { };
	AVRecorder::Config::Video& v = *cfg.video;
	v.frame_rate = cv_movie_fps.value;

	auto basex = [&v](int scale)
	{
		v.width = vid.width / vid.dupx * scale;
		v.height = vid.height / vid.dupy * scale;
	};

	switch (cv_movie_resolution.value)
	{
	case Res::kGame:
		v.width = vid.width;
		v.height = vid.height;
		break;

	case Res::kBase:
		basex(1);
		break;

	case Res::kBase2x:
		basex(2);
		break;

	case Res::kBase4x:
		basex(4);
		break;

	case Res::kWindow:
		v.width = vid.realwidth;
		v.height = vid.realheight;
		break;

	case Res::kCustom:
		if (sscanf(cv_movie_custom_resolution.string, "%dx%d", &v.width, &v.height) != 2)
		{
			throw std::invalid_argument(fmt::format(
				"Bad movie_custom_resolution '{}', should be <width>x<height> (e.g. 640x400)",
				cv_movie_custom_resolution.string
			));
		}
		break;

	default:
		SRB2_ASSERT(false);
	}

	return cfg;
}

boolean M_AVRecorder_Open(const char* filename)
{
	try
	{
		AVRecorder::Config cfg = configure();

		cfg.file_name = filename;

		g_av_recorder = std::make_shared<AVRecorder>(cfg);

		I_UpdateAudioRecorder();

		return true;
	}
	catch (const std::exception& ex)
	{
		CONS_Alert(CONS_ERROR, "Exception starting video recorder: %s\n", ex.what());
		return false;
	}
}

void M_AVRecorder_Close(void)
{
	g_av_recorder.reset();

	I_UpdateAudioRecorder();
}

const char* M_AVRecorder_GetFileExtension(void)
{
	return AVRecorder::file_extension();
}

const char* M_AVRecorder_GetCurrentFormat(void)
{
	SRB2_ASSERT(g_av_recorder != nullptr);

	return g_av_recorder->format_name();
}

void M_AVRecorder_PrintCurrentConfiguration(void)
{
	SRB2_ASSERT(g_av_recorder != nullptr);

	g_av_recorder->print_configuration();
}

boolean M_AVRecorder_IsExpired(void)
{
	SRB2_ASSERT(g_av_recorder != nullptr);

	return g_av_recorder->invalid();
}

void M_AVRecorder_DrawFrameRate(void)
{
	if (!cv_movie_showfps.value || !g_av_recorder)
	{
		return;
	}

	g_av_recorder->draw_statistics();
}
