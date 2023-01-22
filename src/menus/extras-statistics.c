/// \file  menus/extras-challenges.c
/// \brief Statistics menu

#include "../k_menu.h"
#include "../z_zone.h"
#include "../m_cond.h" // Condition Sets
#include "../s_sound.h"

struct statisticsmenu_s statisticsmenu;

void M_Statistics(INT32 choice)
{
	UINT16 i = 0;

	(void)choice;

	statisticsmenu.maplist = Z_Malloc(sizeof(UINT16) * nummapheaders, PU_STATIC, NULL);
	statisticsmenu.nummaps = 0;

	for (i = 0; i < nummapheaders; i++)
	{
		if (!mapheaderinfo[i])
			continue;

		// Check for no visibility + legacy box
		if (mapheaderinfo[i]->menuflags & (LF2_NOTIMEATTACK|LF2_HIDEINSTATS|LF2_HIDEINMENU))
			continue;

		// Check for completion
		if ((mapheaderinfo[i]->menuflags & LF2_FINISHNEEDED)
		&& !(mapheaderinfo[i]->mapvisited & MV_BEATEN))
			continue;

		// Check for unlock
		if (M_MapLocked(i+1))
			continue;

		statisticsmenu.maplist[statisticsmenu.nummaps++] = i;
	}
	statisticsmenu.maplist[statisticsmenu.nummaps] = NEXTMAP_INVALID;
	statisticsmenu.maxscroll = (statisticsmenu.nummaps + M_CountMedals(true, true) + 2) - 10;
	statisticsmenu.location = 0;

	if (statisticsmenu.maxscroll < 0)
	{
		statisticsmenu.maxscroll = 0;
	}

	MISC_StatisticsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_StatisticsDef, false);
}

boolean M_StatisticsInputs(INT32 ch)
{
	const UINT8 pid = 0;

	(void)ch;

	if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);

		Z_Free(statisticsmenu.maplist);
		statisticsmenu.maplist = NULL;

		return true;
	}

	if (M_MenuExtraPressed(pid))
	{
		if (statisticsmenu.location > 0)
		{
			statisticsmenu.location = 0;
			S_StartSound(NULL, sfx_s3k5b);
			M_SetMenuDelay(pid);
		}
	}
	else if (menucmd[pid].dpad_ud > 0)
	{
		if (statisticsmenu.location < statisticsmenu.maxscroll)
		{
			statisticsmenu.location++;
			S_StartSound(NULL, sfx_s3k5b);
			M_SetMenuDelay(pid);
		}
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		if (statisticsmenu.location > 0)
		{
			statisticsmenu.location--;
			S_StartSound(NULL, sfx_s3k5b);
			M_SetMenuDelay(pid);
		}
	}

	return true;
}
