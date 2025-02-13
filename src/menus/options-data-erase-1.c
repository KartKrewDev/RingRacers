// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-data-erase-1.c
/// \brief Erase Data Menu

#include "../k_menu.h"
#include "../s_sound.h"
#include "../m_cond.h" // Condition Sets
#include "../f_finale.h"

#define EC_CHALLENGES	0x01
#define EC_STATISTICS	0x02
#define EC_TIMEATTACK	0x04
#define EC_ALLGAME		(EC_CHALLENGES|EC_STATISTICS|EC_TIMEATTACK)

menuitem_t OPTIONS_DataErase[] =
{
	{IT_STRING | IT_CALL, "Erase Challenges Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, EC_CHALLENGES, 0},

	{IT_STRING | IT_CALL, "Erase Statistics Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, EC_STATISTICS, 0},

	{IT_STRING | IT_CALL, "Erase GP & Record Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, EC_TIMEATTACK, 0},

	{IT_STRING | IT_CALL, "\x85\x45rase all Game Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, EC_ALLGAME, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_LINKTEXT | IT_CALL, "Erase a Profile...", "Select a Profile to erase.",
		NULL, {.routine = M_CheckProfileData}, 0, 0},

};

menu_t OPTIONS_DataEraseDef = {
	sizeof (OPTIONS_DataErase) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataErase,
	48, 80,
	SKINCOLOR_BLACK, 0,
	MBF_DRAWBGWHILEPLAYING,
	"SHWDN2", // Danger.
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

static void M_EraseDataResponse(INT32 ch)
{
	if (ch == MA_NO)
		return;

	S_StartSound(NULL, sfx_itrole); // bweh heh heh

	// Delete the data
	// see also G_LoadGameData
	// We do these in backwards order to prevent things from being immediately re-unlocked.

	gamedata->loaded = false;

	if (optionsmenu.erasecontext & EC_TIMEATTACK)
		G_ClearRecords();
	if (optionsmenu.erasecontext & EC_STATISTICS)
		M_ClearStats();
	if (optionsmenu.erasecontext & EC_CHALLENGES)
		M_ClearSecrets();

	M_FinaliseGameData();

	// Don't softlock the Stereo on if you won't be able to access it anymore!?
	if (soundtest.playing && M_SecretUnlocked(SECRET_SOUNDTEST, true) == false)
		S_SoundTestStop();

	F_StartIntro();
	M_ClearMenus(true);
}

void M_EraseData(INT32 choice)
{
	const char *eschoice, *esstr = M_GetText("Are you sure you want\nto erase %s?\n");
	(void)choice;

	optionsmenu.erasecontext = (UINT8)currentMenu->menuitems[itemOn].mvar1;

	if (optionsmenu.erasecontext == EC_CHALLENGES)
		eschoice = M_GetText("Challenges data");
	else if (optionsmenu.erasecontext == EC_STATISTICS)
		eschoice = M_GetText("Statistics data");
	else if (optionsmenu.erasecontext == EC_TIMEATTACK)
		eschoice = M_GetText("GP & Record data");
	else if (optionsmenu.erasecontext == EC_ALLGAME)
		eschoice = M_GetText("ALL game data");
	else
		eschoice = "[misconfigured erasecontext]";
	

	M_StartMessage("Data Erase", va(esstr, eschoice), &M_EraseDataResponse, MM_YESNO, NULL, NULL);
}
