// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/extras-wrong.c
/// \brief Wrongwarp

#include "../k_menu.h"
#include "../s_sound.h"
#include "../m_random.h"
#include "../music.h"
#include "../r_skins.h"
#include "../v_video.h"
#include "../z_zone.h"

struct wrongwarp_s wrongwarp;

static menuitem_t MISC_WrongWarpMenu[] =
{
	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

void M_WrongWarp(INT32 choice)
{
	(void)choice;

	wrongwarp.ticker = 0;

	M_SetupNextMenu(&MISC_WrongWarpDef, false);

	// Done here to avoid immediate music credit
	Music_Remap("menu_nocred", "YEAWAY");
	Music_Play("menu_nocred");
}

static void M_WrongWarpTick(void)
{
	static boolean firsteggman = true;
	static boolean antitailgate = false;

	UINT8 i, j;

	wrongwarp.ticker++;
	if (wrongwarp.ticker < 2*TICRATE)
		return;

	if (wrongwarp.ticker == 2*TICRATE)
	{
		S_ShowMusicCredit();

		for (i = 0; i < MAXWRONGPLAYER; i++)
			wrongwarp.wrongplayers[i].skin = MAXSKINS;

		firsteggman = true;
	}

	// SMK title screen recreation!?

	for (i = 0; i < MAXWRONGPLAYER; i++)
	{
		if (wrongwarp.wrongplayers[i].skin == MAXSKINS)
			continue;

		wrongwarp.wrongplayers[i].across += 5;
		if (wrongwarp.wrongplayers[i].across < BASEVIDWIDTH + WRONGPLAYEROFFSCREEN)
			continue;

		wrongwarp.wrongplayers[i].skin = MAXSKINS;
	}

	if (wrongwarp.delaytowrongplayer)
	{
		wrongwarp.delaytowrongplayer--;
		return;
	}

	wrongwarp.delaytowrongplayer = M_RandomRange(TICRATE/3, 2*TICRATE/3);

	if (wrongwarp.ticker == 2*TICRATE)
		return;

	UINT32 rskin = 0;

	if (firsteggman == true)
	{
		// Eggman always leads the pack. It's not Sonic's game anymore...
		firsteggman = false;

		i = 0;
		wrongwarp.wrongplayers[i].spinout = false;
	}
	else
	{
		rskin = R_GetLocalRandomSkin();

		for (i = 0; i < MAXWRONGPLAYER; i++)
		{
			// Already in use.
			if (wrongwarp.wrongplayers[i].skin == rskin)
				return;

			// Prevent tailgating.
			if (antitailgate == !!(i & 1))
				continue;

			// Slot isn't free.
			if (wrongwarp.wrongplayers[i].skin != MAXSKINS)
				continue;

			break;
		}

		// No free slots.
		if (i == MAXWRONGPLAYER)
			return;

		// Check to see if any later entry uses the skin too
		for (j = i+1; j < MAXWRONGPLAYER; j++)
		{
			if (wrongwarp.wrongplayers[j].skin != rskin)
				continue;

			return;
		}

		wrongwarp.wrongplayers[i].spinout = M_RandomChance(FRACUNIT/11);
	}

	// Add the new character!
	wrongwarp.wrongplayers[i].skin = rskin;
	wrongwarp.wrongplayers[i].across = -WRONGPLAYEROFFSCREEN;

	antitailgate = !!(i & 1);
}

static boolean M_WrongWarpInputs(INT32 ch)
{
	(void)ch;

	if (wrongwarp.ticker < TICRATE/2)
		return true;

	return false;
}

static INT32 M_WrongWarpFallingHelper(INT32 y, INT32 falltime)
{
	if (wrongwarp.ticker < falltime)
	{
		return y;
	}

	if (wrongwarp.ticker > falltime + 2*TICRATE)
	{
		return INT32_MAX;
	}

	if (wrongwarp.ticker < falltime + TICRATE)
	{
		y += + ((wrongwarp.ticker - falltime) & 1 ? 1 : -1);
		return y;
	}

	y += floor(pow(1.5, (double)(wrongwarp.ticker - (falltime + TICRATE))));
	return y;
}

static void M_DrawWrongWarpBack(void)
{
	INT32 x, y;

	if (gamestate == GS_MENU)
	{
		patch_t *pat, *pat2;
		INT32 animtimer, anim2 = 0;

		pat = W_CachePatchName("TITLEBG1", PU_CACHE);
		pat2 = W_CachePatchName("TITLEBG2", PU_CACHE);

		animtimer = ((wrongwarp.ticker*5)/16) % SHORT(pat->width);
		anim2 = SHORT(pat2->width) - (((wrongwarp.ticker*5)/16) % SHORT(pat2->width));

		// SRB2Kart: F_DrawPatchCol is over-engineered; recoded to be less shitty and error-prone
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);

		x = -((INT32)animtimer);
		y = 0;
		while (x < BASEVIDWIDTH)
		{
			V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, pat, NULL);
			x += SHORT(pat->width);
		}

		x = -anim2;
		y = BASEVIDHEIGHT - SHORT(pat2->height);
		while (x < BASEVIDWIDTH)
		{
			V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, pat2, NULL);
			x += SHORT(pat2->width);
		}
	}

	{
		patch_t *ttcheckers = W_CachePatchName("TTCHECK", PU_CACHE);

		y = FixedMul(40<<FRACBITS, FixedDiv(wrongwarp.ticker%70, 70));

		V_DrawSciencePatch(0, -y, 0, ttcheckers, FRACUNIT);
		V_DrawSciencePatch(280<<FRACBITS, -(40<<FRACBITS) + y, 0, ttcheckers, FRACUNIT);

		y = M_WrongWarpFallingHelper(36, 7*TICRATE/4);
		if (y != INT32_MAX)
		{
			patch_t *ttbanner = W_CachePatchName("TTKBANNR", PU_CACHE);
			V_DrawSmallScaledPatch(84, y, 0, ttbanner);
		}

		y = M_WrongWarpFallingHelper(87, 4*TICRATE/3);
		if (y != INT32_MAX)
		{
			patch_t *ttkart = W_CachePatchName("TTKART", PU_CACHE);
			V_DrawSmallScaledPatch(84, y, 0, ttkart);
		}
	}
}

menu_t MISC_WrongWarpDef = {
	sizeof (MISC_WrongWarpMenu)/sizeof (menuitem_t),
	NULL,
	0,
	MISC_WrongWarpMenu,
	0, 0,
	0, 0,
	MBF_SOUNDLESS|MBF_DRAWBGWHILEPLAYING,
	".",
	98, 0,
	M_DrawWrongWarp,
	M_DrawWrongWarpBack,
	M_WrongWarpTick,
	NULL,
	NULL,
	M_WrongWarpInputs,
};
