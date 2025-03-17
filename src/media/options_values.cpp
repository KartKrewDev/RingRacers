// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cstdint>

#include <vpx/vpx_encoder.h>

#include "../core/vector.hpp"
#include "options.hpp"
#include "vorbis.hpp"
#include "vp8.hpp"

using namespace srb2::media;

// NOTE: Options::cvars_ MUST be initialized before any
// Options instances construct. For static objects, they have
// to be defined in the same translation unit as
// Options::cvars_ to guarantee initialization order.

srb2::Vector<consvar_t*> Options::cvars_;

// clang-format off
const Options VorbisEncoder::options_("vorbis", {
	{"quality",			Options::values<float>("0", {-0.1f, 1.f})},
	{"max_bitrate",		Options::values<int>("-1", {-1})},
	{"nominal_bitrate",	Options::values<int>("-1", {-1})},
	{"min_bitrate",		Options::values<int>("-1", {-1})},
});

const Options VP8Encoder::options_("vp8", {
	{"quality_mode", Options::values<int>("q", {}, {
		{"vbr", VPX_VBR},
		{"cbr", VPX_CBR},
		{"cq", VPX_CQ},
		{"q", VPX_Q},
	})},
	{"target_bitrate", Options::values<int>("800", {1})},
	{"min_q", Options::values<int>("4", {4, 63})},
	{"max_q", Options::values<int>("55", {4, 63})},
	{"kf_min", Options::values<int>("0", {0})},
	{"kf_max", Options::values<int>("auto", {0}, {
		{"auto", static_cast<int>(KeyFrameOption::kAuto)},
	})},
	{"cpu_used", Options::values<int>("0", {-16, 16})},
	{"cq_level", Options::values<int>("10", {0, 63})},
	{"deadline", Options::values<int>("10", {1}, {
		{"infinite", static_cast<int>(DeadlineOption::kInfinite)},
	})},
	{"sharpness", Options::values<int>("7", {0, 7})},
	{"token_parts", Options::values<int>("0", {0, 3})},
	{"threads", Options::values<int>("1", {1})},
});
// clang-format on
