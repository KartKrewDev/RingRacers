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
	{IT_STRING | IT_CVAR | IT_CV_STRING, NULL, NULL,
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
	"EXTRAS",
	0, 0,
	M_DrawAddons,
	M_AddonsRefresh,
	NULL,
	NULL,
	NULL
};

// Addons menu: (Merely copypasted, original code by toaster)

static void M_UpdateAddonsSearch(void);
consvar_t cv_dummyaddonsearch = CVAR_INIT ("dummyaddonsearch", "", CV_HIDDEN|CV_CALL|CV_NOINIT, NULL, M_UpdateAddonsSearch);

void M_Addons(INT32 choice)
{
	const char *pathname = ".";

	(void)choice;

#if 1
	if (cv_addons_option.value == 0)
		pathname = addonsdir;
	else if (cv_addons_option.value == 1)
		pathname = srb2home;
	else if (cv_addons_option.value == 2)
		pathname = srb2path;
	else
#endif
	if (cv_addons_option.value == 3 && *cv_addons_folder.string != '\0')
		pathname = cv_addons_folder.string;

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
		M_StartMessage(va("No files/folders found.\n\n%s\n\nPress (B)\n", LOCATIONSTRING1),NULL,MM_NOTHING);
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

	strlcpy(header, va("%s folder%s", cv_addons_option.string, menupath+menupathindex[menudepth-1]-1), 1024);
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
		M_StartMessage(va("\x82%s\x80\nThis folder no longer exists!\nAborting to main menu.\n\nPress (B)\n", M_AddonsHeaderPath()),NULL,MM_NOTHING)

#define CLEARNAME Z_Free(refreshdirname);\
					refreshdirname = NULL

static boolean prevmajormods = false;

static void M_AddonsClearName(INT32 choice)
{
	if (!majormods || prevmajormods)
	{
		CLEARNAME;
	}
	M_StopMessage(choice);
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
				message = va("%c%s\x80\nMaximum number of addons reached.\nA file could not be loaded.\nIf you wish to play with this addon, restart the game to clear existing ones.\n\nPress (B)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			else
				message = va("%c%s\x80\nA file was not loaded.\nCheck the console log for more info.\n\nPress (B)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
		}
		else if (refreshdirmenu & (REFRESHDIR_WARNING|REFRESHDIR_ERROR))
		{
			S_StartSound(NULL, sfx_s224);
			message = va("%c%s\x80\nA file was loaded with %s.\nCheck the console log for more info.\n\nPress (B)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname, ((refreshdirmenu & REFRESHDIR_ERROR) ? "errors" : "warnings"));
		}
		else if (majormods && !prevmajormods)
		{
			S_StartSound(NULL, sfx_s221);
			message = va("%c%s\x80\nYou've loaded a gameplay-modifying addon.\n\nRecord Attack has been disabled, but you\ncan still play alone in local Multiplayer.\n\nIf you wish to play Record Attack mode, restart the game to disable loaded addons.\n\nPress (B)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			prevmajormods = majormods;
		}

		if (message)
		{
			M_StartMessage(message,FUNCPTRCAST(M_AddonsClearName),MM_YESNO);
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

static void M_UpdateAddonsSearch(void)
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
							M_StartMessage(va("%c%s\x80\nThis folder is empty.\n\nPress (B)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
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
						M_StartMessage(va("%c%s\x80\nThis folder is too deep to navigate to!\n\nPress (B)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
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
					M_StartMessage(va("%c%s\x80\nThis file may not be a console script.\nAttempt to run anyways? \n\nPress (A) to confirm or (B) to cancel\n\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),FUNCPTRCAST(M_AddonExec),MM_YESNO);
					break;

				case EXT_CFG:
					M_StartMessage(va("%c%s\x80\nThis file may modify your settings.\nAttempt to run anyways? \n\nPress (A) to confirm or (B) to cancel\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),FUNCPTRCAST(M_AddonExec),MM_YESNO);
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

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);

		M_SetMenuDelay(pid);
	}
}
