// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_HWR2_BLENDMODE_HPP
#define SRB2_HWR2_BLENDMODE_HPP

namespace srb2::hwr2
{

enum class BlendMode
{
	kAlphaTransparent,
	kModulate,
	kAdditive,
	kSubtractive,
	kReverseSubtractive,
	kInvertDest
};

} // namespace srb2::hwr2

#endif // SRB2_HWR2_BLENDMODE_HPP
