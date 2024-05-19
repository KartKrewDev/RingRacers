// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Vivian "toastergrl" Grannell.
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/extras-addons.c
/// \brief Addons menu!

#include "../k_menu.h"
#include "../filesrch.h" // Addfile
#include "../d_main.h"
#include "../z_zone.h"
#include "../s_sound.h"
#include "../v_video.h"

menuitem_t MISC_AddonsMenu[] =
{
	{IT_STRING | IT_CVAR | IT_CV_STRING, "Search for add-ons", "Provide a full or partial name to filter available files by.",
		NULL, {.cvar = &cv_dummyaddonsearch}, 0, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL,
		NULL, {.routine = M_HandleAddons}, 0, 0},     // dummy menuitem for the control func
};

menu_t MISC_AddonsDef = {
	sizeof (MISC_AddonsMenu)/sizeof (menuitem_t),
	NULL,
	0,
	MISC_AddonsMenu,
	50, 28,
	0, 0,
	MBF_NOLOOPENTRIES,
	"EXTRAS",
	0, 0,
	M_DrawAddons,
	M_DrawExtrasBack,
	M_AddonsRefresh,
	NULL,
	NULL,
	NULL
};

// Addons menu: (Merely copypasted, original code by toaster)

void M_Addons(INT32 choice)
{
	const char *pathname = ".";

	(void)choice;

	pathname = addonsdir;

	strlcpy(menupath, pathname, 1024);
	menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath) + 1;

	if (menupath[menupathindex[menudepthleft]-2] != PATHSEP[0])
	{
		menupath[menupathindex[menudepthleft]-1] = PATHSEP[0];
		menupath[menupathindex[menudepthleft]] = 0;
	}
	else
		--menupathindex[menudepthleft];

	if (!preparefilemenu(false, false))
	{
		M_StartMessage("Add-ons Menu", va("No files/folders found.\n\n%s\n", LOCATIONSTRING1),NULL,MM_NOTHING, NULL, NULL);
		return;
	}
	else
		dir_on[menudepthleft] = 0;

	MISC_AddonsDef.lastOn = 0; // Always start on search

	MISC_AddonsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_AddonsDef, false);
}


char *M_AddonsHeaderPath(void)
{
	UINT32 len;
	static char header[1024];

	strlcpy(header, va("addons%s", menupath+menupathindex[menudepth-1]-1), 1024);
	len = strlen(header);
	if (len > 34)
	{
		len = len-34;
		header[len] = header[len+1] = header[len+2] = '.';
	}
	else
		len = 0;

	return header+len;
}

#define UNEXIST S_StartSound(NULL, sfx_s26d);\
		M_SetupNextMenu(MISC_AddonsDef.prevMenu, false);\
		M_StartMessage("Add-ons Menu", va("\x82%s\x80\nThis folder no longer exists!\nAborting to main menu.\n", M_AddonsHeaderPath()),NULL,MM_NOTHING, NULL, NULL)

#define CLEARNAME Z_Free(refreshdirname);\
					refreshdirname = NULL

static boolean prevmajormods = false;

static void M_AddonsClearName(INT32 choice)
{
	(void)choice;

	if (!majormods || prevmajormods)
	{
		CLEARNAME;
	}
}

// Handles messages for addon errors.
void M_AddonsRefresh(void)
{
	if ((refreshdirmenu & REFRESHDIR_NORMAL) && !preparefilemenu(true, false))
	{
		UNEXIST;
		if (refreshdirname)
		{
			CLEARNAME;
		}
		return;// true;
	}

#ifdef DEVELOP
	prevmajormods = majormods;
#else
 	if (!majormods && prevmajormods)
 		prevmajormods = false;
#endif

	if ((refreshdirmenu & REFRESHDIR_ADDFILE) || (majormods && !prevmajormods))
	{
		char *message = NULL;

		if (refreshdirmenu & REFRESHDIR_NOTLOADED)
		{
			S_StartSound(NULL, sfx_s26d);
			if (refreshdirmenu & REFRESHDIR_MAX)
				message = va("%c%s\x80\nMaximum number of addons reached.\nA file could not be loaded.\nIf you wish to play with this addon, restart the game to clear existing ones.\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			else
				message = va("%c%s\x80\nA file was not loaded.\nCheck the console log for more info.\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
		}
		else if (refreshdirmenu & (REFRESHDIR_WARNING|REFRESHDIR_ERROR))
		{
			S_StartSound(NULL, sfx_s224);
			message = va("%c%s\x80\nA file was loaded with %s.\nCheck the console log for more info.\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname, ((refreshdirmenu & REFRESHDIR_ERROR) ? "errors" : "warnings"));
		}
		else if (majormods && !prevmajormods)
		{
			S_StartSound(NULL, sfx_s221);
			message = va("%c%s\x80\nYou've loaded a gameplay-modifying addon.\nCheck the console log for more info.\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			prevmajormods = majormods;
		}

		if (message)
		{
			M_StartMessage("Add-ons Menu", message, &M_AddonsClearName,MM_NOTHING, NULL, NULL);
			return;// true;
		}

		S_StartSound(NULL, sfx_s221);
		CLEARNAME;
	}

	return;// false;
}

static void M_AddonExec(INT32 ch)
{
	if (ch == MA_YES)
	{
		S_StartSound(NULL, sfx_zoom);
		COM_BufAddText(va("exec \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
	}
}

void M_UpdateAddonsSearch(void);
void M_UpdateAddonsSearch(void)
{
	menusearch[0] = strlen(cv_dummyaddonsearch.string);
	strlcpy(menusearch+1, cv_dummyaddonsearch.string, MAXSTRINGLENGTH);
	if (!cv_addons_search_case.value)
		strupr(menusearch+1);

#if 0 // much slower
	if (!preparefilemenu(true, false))
	{
		UNEXIST;
		return;
	}
#else // streamlined
	searchfilemenu(NULL);
#endif
}

void M_HandleAddons(INT32 choice)
{
	const UINT8 pid = 0;
	boolean exitmenu = false; // exit to previous menu

	(void) choice;

	if (menucmd[pid].dpad_ud > 0)
	{
		if (dir_on[menudepthleft] < sizedirmenu-1)
		{
			dir_on[menudepthleft]++;
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_NextOpt())
		{
			S_StartSound(NULL, sfx_s3k5b);
		}
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		if (dir_on[menudepthleft])
		{
			dir_on[menudepthleft]--;
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_PrevOpt())
		{
			S_StartSound(NULL, sfx_s3k5b);
		}
		M_SetMenuDelay(pid);
	}

	else if (M_MenuButtonPressed(pid, MBT_L))
	{
		UINT8 i;
		for (i = numaddonsshown; i && (dir_on[menudepthleft] < sizedirmenu-1); i--)
			dir_on[menudepthleft]++;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	else if (M_MenuButtonPressed(pid, MBT_R))
	{
		UINT8 i;
		for (i = numaddonsshown; i && (dir_on[menudepthleft]); i--)
			dir_on[menudepthleft]--;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	else if (M_MenuConfirmPressed(pid))
	{
		boolean refresh = true;
		M_SetMenuDelay(pid);

		if (!dirmenu[dir_on[menudepthleft]])
			S_StartSound(NULL, sfx_s26d);
		else
		{
			switch (dirmenu[dir_on[menudepthleft]][DIR_TYPE])
			{
				case EXT_FOLDER:
					strcpy(&menupath[menupathindex[menudepthleft]],dirmenu[dir_on[menudepthleft]]+DIR_STRING);
					if (menudepthleft)
					{
						menupathindex[--menudepthleft] = strlen(menupath);
						menupath[menupathindex[menudepthleft]] = 0;

						if (!preparefilemenu(false, false))
						{
							S_StartSound(NULL, sfx_s224);
							M_StartMessage("Add-ons Menu", va("%c%s\x80\nThis folder is empty.\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING, NULL, NULL);
							menupath[menupathindex[++menudepthleft]] = 0;

							if (!preparefilemenu(true, false))
							{
								UNEXIST;
								return;
							}
						}
						else
						{
							S_StartSound(NULL, sfx_s3k5b);
							dir_on[menudepthleft] = 1;
						}
						refresh = false;
					}
					else
					{
						S_StartSound(NULL, sfx_s26d);
						M_StartMessage("Add-ons Menu", va("%c%s\x80\nThis folder is too deep to navigate to!\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING, NULL, NULL);
						menupath[menupathindex[menudepthleft]] = 0;
					}
					break;

				case EXT_UP:
					S_StartSound(NULL, sfx_s3k5b);
					menupath[menupathindex[++menudepthleft]] = 0;
					if (!preparefilemenu(false, false))
					{
						UNEXIST;
						return;
					}
					break;

				case EXT_TXT:
					M_StartMessage("Add-ons Menu", va("%c%s\x80\nThis file may not be a console script.\nAttempt to run anyways?\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),&M_AddonExec,MM_YESNO, NULL, NULL);
					break;

				case EXT_CFG:
					M_StartMessage("Add-ons Menu", va("%c%s\x80\nThis file may modify your settings.\nAttempt to run anyways?\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),&M_AddonExec,MM_YESNO, NULL, NULL);
					break;

				case EXT_LUA:
				case EXT_SOC:
				case EXT_WAD:
#ifdef USE_KART
				case EXT_KART:
#endif
				case EXT_PK3:
					COM_BufAddText(va("addfile \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
					break;

				default:
					S_StartSound(NULL, sfx_s26d);
			}

			if (refresh)
				refreshdirmenu |= REFRESHDIR_NORMAL;
		}
	}
	else if (M_MenuBackPressed(pid))
	{
		exitmenu = true;
		M_SetMenuDelay(pid);
	}


	if (exitmenu)
	{
		closefilemenu(true);

		// Secret menu!
		//MainMenu[secrets].status = (M_AnySecretUnlocked()) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		// I could guard it, but let's just always do this.
		M_InitExtras(-1);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(M_InterruptMenuWithChallenges(currentMenu->prevMenu), false);
		else
			M_ClearMenus(true);

		M_SetMenuDelay(pid);
	}
}
