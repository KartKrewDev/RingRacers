// RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_debug.cpp
/// \brief Software renderer debugging

#include <algorithm> // std::clamp

#include "cxxutil.hpp"

#include "command.h"
#include "m_fixed.h"
#include "r_main.h"

namespace
{

CV_PossibleValue_t contrast_cons_t[] = {{-FRACUNIT, "MIN"}, {FRACUNIT, "MAX"}, {}};

}; // namespace

consvar_t cv_debugrender_contrast =
	CVAR_INIT("debugrender_contrast", "0.0", CV_CHEAT | CV_FLOAT, contrast_cons_t, nullptr);

INT32 R_AdjustLightLevel(INT32 light)
{
	constexpr fixed_t kRange = (LIGHTLEVELS - 1) * FRACUNIT;
	const fixed_t adjust = FixedMul(cv_debugrender_contrast.value, kRange);

	light = std::clamp((light * FRACUNIT) - adjust, 0, kRange);

	return light / FRACUNIT;
}
