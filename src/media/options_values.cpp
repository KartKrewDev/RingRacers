// RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cstdint>

#include <vpx/vpx_encoder.h>

#include "options.hpp"
#include "vorbis.hpp"
#include "vp8.hpp"

using namespace srb2::media;

// NOTE: Options::cvars_ MUST be initialized before any
// Options instances construct. For static objects, they have
// to be defined in the same translation unit as
// Options::cvars_ to guarantee initialization order.

std::vector<consvar_t*> Options::cvars_;

// clang-format off
const Options VorbisEncoder::options_("vorbis", {
	{"quality",			Options::range<float>("0", -0.1f, 1.f)},
	{"max_bitrate",		Options::range_min<int>("-1", -1)},
	{"nominal_bitrate",	Options::range_min<int>("-1", -1)},
	{"min_bitrate",		Options::range_min<int>("-1", -1)},
});

const Options VP8Encoder::options_("vp8", {
	{"quality_mode", Options::value_map<int>("q", {
		{"vbr", VPX_VBR},
		{"cbr", VPX_CBR},
		{"cq", VPX_CQ},
		{"q", VPX_Q},
	})},
	{"target_bitrate", Options::range_min<int>("800", 1)},
	{"min_q", Options::range<int>("4", 4, 63)},
	{"max_q", Options::range<int>("55", 4, 63)},
	{"kf_min", Options::range_min<int>("0", 0)},
	{"kf_max", Options::value_map<int>("auto", {
		{"auto", static_cast<int>(KeyFrameOption::kAuto)},
		{"MIN", 0},
		{"MAX", INT32_MAX},
	})},
	{"cpu_used", Options::range<int>("0", -16, 16)},
	{"cq_level", Options::range<int>("10", 0, 63)},
	{"deadline", Options::value_map<int>("10", {
		{"infinite", static_cast<int>(DeadlineOption::kInfinite)},
		{"MIN", 1},
		{"MAX", INT32_MAX},
	})},
	{"sharpness", Options::range<int>("7", 0, 7)},
	{"token_parts", Options::range<int>("0", 0, 3)},
	{"threads", Options::range_min<int>("1", 1)},
});
// clang-format on
