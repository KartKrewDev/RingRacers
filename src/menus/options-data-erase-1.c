/// \file  menus/options-data-erase-1.c
/// \brief Erase Data Menu

#include "../k_menu.h"
#include "../s_sound.h"
#include "../m_cond.h" // Condition Sets
#include "../f_finale.h"

menuitem_t OPTIONS_DataErase[] =
{

	{IT_STRING | IT_CALL, "Erase Time Attack Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

	{IT_STRING | IT_CALL, "Erase Unlockable Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "Erase Profile Data...", "Select a Profile to erase.",
		NULL, {.routine = M_CheckProfileData}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "\x85\x45rase all Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

};

menu_t OPTIONS_DataEraseDef = {
	sizeof (OPTIONS_DataErase) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataErase,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
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
	if (optionsmenu.erasecontext == 2)
	{
		// SRB2Kart: This actually needs to be done FIRST, so that you don't immediately regain playtime/matches secrets
		gamedata->totalplaytime = 0;
		gamedata->matchesplayed = 0;
	}
	if (optionsmenu.erasecontext != 1)
		G_ClearRecords();
	if (optionsmenu.erasecontext != 0)
		M_ClearSecrets();

	F_StartIntro();
	M_ClearMenus(true);
}

void M_EraseData(INT32 choice)
{
	const char *eschoice, *esstr = M_GetText("Are you sure you want to erase\n%s?\n\nPress (A) to confirm or (B) to cancel\n");

	optionsmenu.erasecontext = (UINT8)choice;

	if (choice == 0)
		eschoice = M_GetText("Time Attack data");
	else if (choice == 1)
		eschoice = M_GetText("Secrets data");
	else
		eschoice = M_GetText("ALL game data");

	M_StartMessage(va(esstr, eschoice), FUNCPTRCAST(M_EraseDataResponse), MM_YESNO);
}
