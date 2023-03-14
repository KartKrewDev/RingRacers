/// \file  menus/extras-challenges.c
/// \brief Statistics menu

#include "../k_menu.h"
#include "../z_zone.h"
#include "../m_cond.h" // Condition Sets
#include "../s_sound.h"

struct statisticsmenu_s statisticsmenu;

static boolean M_StatisticsAddMap(UINT16 map, cupheader_t *cup, boolean *headerexists)
{
	if (!mapheaderinfo[map])
		return false;

	if (mapheaderinfo[map]->cup != cup)
		return false;

	// Check for no visibility
	if (mapheaderinfo[map]->menuflags & (LF2_NOTIMEATTACK|LF2_HIDEINSTATS|LF2_HIDEINMENU))
		return false;

	// No TEST RUN, as that's another exception to Time Attack too
	if (!mapheaderinfo[map]->typeoflevel)
		return false;

	// Check for completion
	if ((mapheaderinfo[map]->menuflags & LF2_FINISHNEEDED)
	&& !(mapheaderinfo[map]->mapvisited & MV_BEATEN))
		return false;

	// Check for unlock
	if (M_MapLocked(map+1))
		return false;

	if (*headerexists == false)
	{
		statisticsmenu.maplist[statisticsmenu.nummaps++] = NEXTMAP_TITLE; // cheeky hack
		*headerexists = true;
	}

	statisticsmenu.maplist[statisticsmenu.nummaps++] = map;
	return true;
}

void M_Statistics(INT32 choice)
{
	cupheader_t *cup;
	UINT16 i;
	boolean headerexists;

	(void)choice;

	statisticsmenu.maplist = Z_Malloc(sizeof(UINT16) * (nummapheaders+1 + numkartcupheaders), PU_STATIC, NULL);
	statisticsmenu.nummaps = 0;

	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		headerexists = false;

		if (M_CupLocked(cup))
			continue;

		for (i = 0; i < CUPCACHE_MAX; i++)
		{
			if (cup->cachedlevels[i] >= nummapheaders)
				continue;

			M_StatisticsAddMap(cup->cachedlevels[i], cup, &headerexists);
		}
	}

	headerexists = false;

	for (i = 0; i < nummapheaders; i++)
	{
		M_StatisticsAddMap(i, NULL, &headerexists);
	}

	if ((i = statisticsmenu.numextramedals = M_CountMedals(true, true)) != 0)
		i += 2;

	statisticsmenu.maplist[statisticsmenu.nummaps] = NEXTMAP_INVALID;
	statisticsmenu.maxscroll = (statisticsmenu.nummaps + i) - 11;
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
