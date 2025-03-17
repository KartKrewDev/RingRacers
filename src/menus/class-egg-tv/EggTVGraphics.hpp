// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __EGGTVGRAPHICS_HPP__
#define __EGGTVGRAPHICS_HPP__

#include <array>
#include <string_view>

#include "../../core/hash_map.hpp"
#include "../../doomdef.h" // skincolornum_t
#include "../../v_draw.hpp"

namespace srb2::menus::egg_tv
{

class EggTVGraphics
{
public:
	struct PatchManager
	{
		using patch = const char*;

		template <int Size>
		using array = std::array<patch, Size>;

		template <int Size>
		struct Animation
		{
			array<Size> data;

			patch animate(int interval) const;
		};

		Animation<2> bg = {
			"RHETSCB1",
			"RHETSCB2",
		};

		patch mod			= "RHTVMOD";
		patch overlay		= "RHETTV";

		patch bar			= "RH_BAR";

		array<2> folder = {
			"RHFLDR02",
			"RHFLDR03",
		};

		patch dash			= "RHMQBR";
		patch grid			= "RHGRID";

		patch empty			= "RHTVSQN0";

		Animation<4> tv = {
			"RHTVSTC1",
			"RHTVSTC2",
			"RHTVSTC3",
			"RHTVSTC4",
		};

		std::array<Animation<2>, 2> nodata = {
			Animation<2> {
				"RHTVSQN1",
				"RHTVSQN2",
			},
			Animation<2> {
				"RHTVSQN3",
				"RHTVSQN4",
			},
		};

		std::array<Animation<2>, 2> corrupt = {
			Animation<2> {
				"RHTVSQN5",
				"RHTVSQN6",
			},
			Animation<2> {
				"RHTVSQN7",
				"RHTVSQN8",
			},
		};

		patch select		= "RHTVSQSL";

		srb2::HashMap<std::string_view, patch> gametype = {
			{"Race",			"RHGT1"},
			{"Battle",			"RHGT2"},
			{"Prison Break",	"RHGT3"},
			{"Sealed Star",		"RHGT4"},
			{"Versus",			"RHGT5"},
		};

		patch fav			= "RHFAV";

		patch button		= "RHMNBR";

		struct
		{
			struct
			{
				patch up			= "RHSBRU";
				patch down			= "RHSBRD";
			}
			arrow;

			struct
			{
				patch top			= "RHSBR01";
				patch mid			= "RHSBR02";
				patch bottom		= "RHSBR03";
			}
			bead;
		}
		scroll;
	};

	struct ColorManager
	{
		using color = skincolornum_t;

		template <int Size>
		using array = std::array<color, Size>;

		array<2> bar = {
			SKINCOLOR_JET,
			SKINCOLOR_BANANA,
		};

		array<2> folder = {
			SKINCOLOR_NONE,
			SKINCOLOR_THUNDER,
		};

		array<2> button = {
			SKINCOLOR_BLACK,
			SKINCOLOR_THUNDER,
		};

		color scroll			= SKINCOLOR_THUNDER;

		array<2> select = {
			SKINCOLOR_NONE,
			SKINCOLOR_MAROON,
		};
	};

	static const PatchManager patches_;
	static const ColorManager colors_;
};

}; // namespace srb2::menus::egg_tv

#endif // __EGGTVGRAPHICS_HPP__
