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
/// \file  menus/extras-challenges.c
/// \brief Statistics menu

#include <algorithm>

#include "../k_menu.h"
#include "../z_zone.h"
#include "../m_cond.h" // Condition Sets
#include "../s_sound.h"
#include "../r_skins.h"

struct statisticsmenu_s statisticsmenu;

static boolean M_StatisticsAddMap(UINT16 map, cupheader_t *cup, boolean *headerexists, boolean tutorial)
{
	if (!mapheaderinfo[map])
		return false;

	if (mapheaderinfo[map]->typeoflevel & TOL_SPECIAL)
	{
		if (gamedata->sealedswaps[GDMAX_SEALEDSWAPS-1] != NULL // all found
		|| (mapheaderinfo[map]->cup
			&& mapheaderinfo[map]->cup->id >= basenumkartcupheaders) // custom content
		|| M_SecretUnlocked(SECRET_SPECIALATTACK, true)) // true order
		{
			if (mapheaderinfo[map]->cup != cup)
				return false;
		}
		else if (cup) // Appear under Lost & Found until you've ordered them.
			return false;
	}
	else if (mapheaderinfo[map]->cup != cup)
		return false;

	if (((mapheaderinfo[map]->typeoflevel & TOL_TUTORIAL) == TOL_TUTORIAL) != tutorial)
		return false;

	// Check for no visibility
	if (mapheaderinfo[map]->menuflags & LF2_HIDEINMENU)
		return false;

	// Check for visitation
	// (M_CanShowLevelInList also checks being the first map in a cup,
	// but we don't need to do this here because it'd just muddy stats!)
	if (!(mapheaderinfo[map]->menuflags & LF2_NOVISITNEEDED)
	&& !(mapheaderinfo[map]->records.mapvisited & MV_VISITED))
		return false;

#if 0
	// Check for completion
	if ((mapheaderinfo[map]->menuflags & LF2_FINISHNEEDED)
	&& !(mapheaderinfo[map]->records.mapvisited & MV_BEATEN))
		return false;
#endif

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

static void M_StatisticsMaps(void)
{
	cupheader_t *cup;
	UINT16 i;
	boolean headerexists;

	statisticsmenu.maplist = static_cast<UINT16*>(Z_Malloc(sizeof(UINT16) * (nummapheaders+1 + numkartcupheaders), PU_STATIC, NULL));
	statisticsmenu.nummaps = 0;

	// Cups
	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		headerexists = false;

		if (M_CupLocked(cup))
			continue;

		for (i = 0; i < CUPCACHE_PODIUM; i++)
		{
			if (cup->cachedlevels[i] >= nummapheaders)
				continue;

			M_StatisticsAddMap(cup->cachedlevels[i], cup, &headerexists, false);
		}
	}

	// Lost and Found
	headerexists = false;
	for (i = 0; i < nummapheaders; i++)
	{
		M_StatisticsAddMap(i, NULL, &headerexists, false);
	}

	// Tutorial
	headerexists = false;
	for (i = 0; i < nummapheaders; i++)
	{
		M_StatisticsAddMap(i, NULL, &headerexists, true);
	}

	if ((i = statisticsmenu.numextramedals) != 0)
		i += 2;

	statisticsmenu.maplist[statisticsmenu.nummaps] = NEXTMAP_INVALID;
	statisticsmenu.maxscroll = (statisticsmenu.nummaps + i) - 12;
	statisticsmenu.location = 0;

	if (statisticsmenu.maxscroll < 0)
	{
		statisticsmenu.maxscroll = 0;
	}
}

static void M_StatisticsChars(void)
{
	UINT16 i;

	statisticsmenu.maplist = static_cast<UINT16*>(Z_Malloc(sizeof(UINT16) * (1 + numskins), PU_STATIC, NULL));
	statisticsmenu.nummaps = 0;

	UINT32 beststat = 0;

	for (i = 0; i < numskins; i++)
	{
		if (!R_SkinUsable(-1, i, false))
			continue;

		statisticsmenu.maplist[statisticsmenu.nummaps++] = i;

		if (skins[i]->records.wins == 0)
			continue;

		// The following is a partial duplication of R_GetEngineClass
		{
			if (skins[i]->flags & SF_IRONMAN)
				continue; // does not add to any engine class

			INT32 s = (skins[i]->kartspeed - 1);
			INT32 w = (skins[i]->kartweight - 1);

			#define LOCKSTAT(stat) \
				if (stat < 0) { continue; } \
				if (stat > 8) { continue; }
				LOCKSTAT(s);
				LOCKSTAT(w);
			#undef LOCKSTAT

			if (
				statisticsmenu.statgridplayed[s][w] > skins[i]->records.wins
				&& (UINT32_MAX - statisticsmenu.statgridplayed[s][w]) < skins[i]->records.wins
			)
				continue; // overflow protection

			statisticsmenu.statgridplayed[s][w] += skins[i]->records.wins;

			if (beststat >= statisticsmenu.statgridplayed[s][w])
				continue;

			beststat = statisticsmenu.statgridplayed[s][w];
		}
	}

	statisticsmenu.maplist[statisticsmenu.nummaps] = MAXSKINS;

	std::sort(
		statisticsmenu.maplist,
		statisticsmenu.maplist + statisticsmenu.nummaps,
		[](UINT16 a, UINT16 b) {
			if (skins[a]->records.rounds > skins[b]->records.rounds)
				return true;
			if (skins[a]->records.rounds != skins[b]->records.rounds)
				return false;
			// Stable for skin ID
			return (a < b);
		}
	);

	statisticsmenu.location = 0;
	statisticsmenu.maxscroll = statisticsmenu.nummaps - 6;

	if (statisticsmenu.maxscroll < 0)
	{
		statisticsmenu.maxscroll = 0;
	}

	if (beststat != 0)
	{
		UINT16 j;
		UINT8 shif = 0;

		// Done this way to ensure ample precision but also prevent overflow
		while (beststat < FRACUNIT)
		{
			beststat <<= 1;
			shif++;
		}

		for (i = 0; i < 9; i++)
		{
			for (j = 0; j < 9; j++)
			{
				if (statisticsmenu.statgridplayed[i][j] == 0)
					continue;

				statisticsmenu.statgridplayed[i][j] =
					FixedDiv(
						statisticsmenu.statgridplayed[i][j] << shif,
						beststat
					);

				if (statisticsmenu.statgridplayed[i][j] == 0)
					statisticsmenu.statgridplayed[i][j] = 1;
			}
		}
	}
}

static void M_StatisticsGP(void)
{
	statisticsmenu.maplist = static_cast<UINT16*>(Z_Malloc(sizeof(UINT16) * (1 + numkartcupheaders), PU_STATIC, NULL));
	statisticsmenu.nummaps = 0;

	cupheader_t *cup;

	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		if (M_CupLocked(cup))
			continue;

		statisticsmenu.maplist[statisticsmenu.nummaps++] = cup->id;
	}

	statisticsmenu.maplist[statisticsmenu.nummaps] = UINT16_MAX;

	statisticsmenu.location = 0;
	statisticsmenu.maxscroll = statisticsmenu.nummaps - 5;

	if (statisticsmenu.maxscroll < 0)
	{
		statisticsmenu.maxscroll = 0;
	}
}

static void M_StatisticsPageInit(void)
{
	switch (statisticsmenu.page)
	{
		case statisticspage_maps:
		{
			M_StatisticsMaps();
			break;
		}

		case statisticspage_chars:
		{
			M_StatisticsChars();
			break;
		}

		case statisticspage_gp:
		{
			M_StatisticsGP();
			break;
		}

		default:
		{
			statisticsmenu.maplist = NULL;
			break;
		}
	}
}

static void M_StatisticsPageClear(void)
{
	if (statisticsmenu.maplist != NULL)
	{
		Z_Free(statisticsmenu.maplist);
		statisticsmenu.maplist = NULL;
	}
}

void M_Statistics(INT32 choice)
{
	(void)choice;

	UINT16 i;

	statisticsmenu.gotmedals = M_CountMedals(false, false);
	statisticsmenu.nummedals = M_CountMedals(true, false);
	statisticsmenu.numextramedals = M_CountMedals(true, true);

	statisticsmenu.numcanbonus = 0;
	for (i = 0; i < basenummapheaders; i++)
	{
		if (!mapheaderinfo[i])
			continue;
		if (mapheaderinfo[i]->records.spraycan != MCAN_BONUS)
			continue;
		statisticsmenu.numcanbonus++;
	}

	M_StatisticsPageInit();

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

		M_StatisticsPageClear();

		return true;
	}

	if (menucmd[pid].dpad_lr != 0)
	{
		M_StatisticsPageClear();

		int newpage = static_cast<int>(statisticsmenu.page)
			+ static_cast<int>(statisticspage_max)
			+ (
				(menucmd[pid].dpad_lr > 0)
					? 1
					: -1
			);

		newpage %= static_cast<int>(statisticspage_max);

		statisticsmenu.page = static_cast<statisticspage_t>(newpage);

		M_StatisticsPageInit();

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);

		return true;
	}

	if (statisticsmenu.maplist == NULL)
		return true;

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
