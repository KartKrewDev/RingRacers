// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Extras Menu: Egg TV

#include "class-egg-tv/EggTV.hpp"

#include "../k_menu.h"
#include "../s_sound.h"

#ifdef HAVE_DISCORDRPC
#include "../discord.h"
#endif

using namespace srb2::menus::egg_tv;

namespace
{

std::unique_ptr<EggTV> g_egg_tv;

void M_DrawEggTV()
{
	g_egg_tv->draw();
}

boolean M_QuitEggTV()
{
	g_egg_tv = {};

	return true;
}

boolean M_HandleEggTV(INT32 choice)
{
	(void)choice;

	const UINT8 pid = 0;
	const EggTV::InputReaction reaction = g_egg_tv->input(pid);

	if (reaction.bypass)
	{
		return false;
	}

	if (reaction.effect)
	{
		S_StartSound(nullptr, reaction.sound);
		M_SetMenuDelay(pid);
	}

	return true;
}

void M_DeleteReplayChoice(INT32 choice)
{
	if (choice == MA_YES)
	{
		g_egg_tv->erase();

		//S_StartSound(nullptr, sfx_s3k4e); // BOOM
		S_StartSound(nullptr, sfx_monch); // :)
	}
}

void M_DeleteReplay(INT32 c)
{
	(void)c;
	M_StartMessage("Egg TV",
		"Are you sure you want to\n"
		"delete this replay?\n"
		"\n"
		"\x85" "This cannot be undone.\n",
		&M_DeleteReplayChoice,
		MM_YESNO,
		nullptr, nullptr
	);
	S_StartSound(nullptr, sfx_s3k36); // lel skid
}

void M_FavoriteReplay(INT32 c)
{
	(void)c;

	g_egg_tv->toggle_favorite();

	S_StartSound(nullptr, sfx_s1c9);
}

}; // namespace

// extras menu: replay hut
menuitem_t EXTRAS_EggTV[] =
{
	{IT_STRING | IT_CALL, "WATCH REPLAY", NULL, NULL, srb2::itemaction([](auto) -> void { g_egg_tv->watch(); }), 0, 0 },
	{IT_STRING | IT_CALL, "STANDINGS", NULL, NULL, srb2::itemaction([](auto) { g_egg_tv->standings(); }), 0, 0},
	{IT_STRING | IT_CALL, "FAVORITE", NULL, NULL, srb2::itemaction(M_FavoriteReplay), 0, 0},

	{IT_SPACE},

	{IT_STRING | IT_CALL, "DELETE REPLAY", NULL, NULL, srb2::itemaction(M_DeleteReplay), 0, 0},

	{IT_SPACE},

	{IT_STRING | IT_CALL, "GO BACK", NULL, NULL, srb2::itemaction([](auto) { g_egg_tv->back(); }), 0, 0},
};

menu_t EXTRAS_EggTVDef =
{
	sizeof (EXTRAS_EggTV)/sizeof (menuitem_t),
	&EXTRAS_MainDef,
	0,
	EXTRAS_EggTV,
	30, 80,
	0, 0,
	0,
	"REPLAY", // music
	41, 1,
	M_DrawEggTV,
	NULL,
	NULL,
	NULL,
	M_QuitEggTV,
	M_HandleEggTV
};

// Call this to construct Egg TV menu
void M_EggTV(INT32 choice)
{
	g_egg_tv = std::make_unique<EggTV>();

	M_SetupNextMenu(&EXTRAS_EggTVDef, false);

#ifdef HAVE_DISCORDRPC
	DRPC_UpdatePresence();
#endif
}

void M_EggTV_RefreshButtonLabels()
{
	EXTRAS_EggTV[2].text = g_egg_tv->favorited() ? "UNFAVORITE" : "FAVORITE";
}
