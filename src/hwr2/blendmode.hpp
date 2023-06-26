// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_BLENDMODE_HPP__
#define __SRB2_HWR2_BLENDMODE_HPP__

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

#endif // __SRB2_HWR2_BLENDMODE_HPP__
