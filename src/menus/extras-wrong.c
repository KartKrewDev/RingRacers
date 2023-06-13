/// \file  menus/extras-wrong.c
/// \brief Wrongwarp

#include "../k_menu.h"
#include "../s_sound.h"
#include "../m_random.h"
#include "../r_skins.h"

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
	S_ChangeMusicInternal("YEAWAY", true);
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

menu_t MISC_WrongWarpDef = {
	sizeof (MISC_WrongWarpMenu)/sizeof (menuitem_t),
	NULL,
	0,
	MISC_WrongWarpMenu,
	0, 0,
	0, 0,
	MBF_SOUNDLESS,
	".",
	98, 0,
	M_DrawWrongWarp,
	M_WrongWarpTick,
	NULL,
	NULL,
	M_WrongWarpInputs,
};
