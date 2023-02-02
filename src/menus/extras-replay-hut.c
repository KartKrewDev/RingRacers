/// \file  menus/extras-replay-hut.c
/// \brief Extras Menu: Replay Hut

#include "../k_menu.h"
#include "../filesrch.h" // Addfile
#include "../d_main.h"
#include "../s_sound.h"
#include "../v_video.h"
#include "../z_zone.h"

// extras menu: replay hut
menuitem_t EXTRAS_ReplayHut[] =
{
	{IT_KEYHANDLER|IT_NOTHING, "", "",			// Dummy menuitem for the replay list
		NULL, {.routine = M_HandleReplayHutList}, 0, 0},

	{IT_NOTHING, "", "",						// Dummy for handling wrapping to the top of the menu..
		NULL, {NULL}, 0, 0},
};

menu_t EXTRAS_ReplayHutDef =
{
	sizeof (EXTRAS_ReplayHut)/sizeof (menuitem_t),
	&EXTRAS_MainDef,
	0,
	EXTRAS_ReplayHut,
	30, 80,
	0, 0,
	"REPLAY",
	41, 1,
	M_DrawReplayHut,
	NULL,
	NULL,
	NULL,
	NULL
};

menuitem_t EXTRAS_ReplayStart[] =
{
	{IT_CALL |IT_STRING,  "Load Addons and Watch", NULL,
		NULL, {.routine = M_HutStartReplay}, 0, 0},

	{IT_CALL |IT_STRING,  "Load Without Addons", NULL,
		NULL, {.routine = M_HutStartReplay}, 10, 0},

	{IT_CALL |IT_STRING,  "Watch Replay", NULL,
		NULL, {.routine = M_HutStartReplay}, 10, 0},

	{IT_SUBMENU |IT_STRING,  "Go Back", NULL,
		NULL, {.submenu = &EXTRAS_ReplayHutDef}, 30, 0},
};


menu_t EXTRAS_ReplayStartDef =
{
	sizeof (EXTRAS_ReplayStart)/sizeof (menuitem_t),
	&EXTRAS_ReplayHutDef,
	0,
	EXTRAS_ReplayStart,
	27, 80,
	0, 0,
	"REPLAY",
	41, 1,
	M_DrawReplayStartMenu,
	NULL,
	NULL,
	NULL,
	NULL
};

void M_PrepReplayList(void)
{
	size_t i;

	if (extrasmenu.demolist)
		Z_Free(extrasmenu.demolist);

	extrasmenu.demolist = Z_Calloc(sizeof(menudemo_t) * sizedirmenu, PU_STATIC, NULL);

	for (i = 0; i < sizedirmenu; i++)
	{
		if (dirmenu[i][DIR_TYPE] == EXT_UP)
		{
			extrasmenu.demolist[i].type = MD_SUBDIR;
			sprintf(extrasmenu.demolist[i].title, "UP");
		}
		else if (dirmenu[i][DIR_TYPE] == EXT_FOLDER)
		{
			extrasmenu.demolist[i].type = MD_SUBDIR;
			strncpy(extrasmenu.demolist[i].title, dirmenu[i] + DIR_STRING, 64);
		}
		else
		{
			extrasmenu.demolist[i].type = MD_NOTLOADED;
			snprintf(extrasmenu.demolist[i].filepath, sizeof extrasmenu.demolist[i].filepath,
					// 255 = UINT8 limit. dirmenu entries are restricted to this length (see DIR_LEN).
					"%s%.255s", menupath, dirmenu[i] + DIR_STRING);
			sprintf(extrasmenu.demolist[i].title, ".....");
		}
	}
}

void M_ReplayHut(INT32 choice)
{
	(void)choice;

	if (demo.inreplayhut)
	{
		demo.rewinding = false;
		CL_ClearRewinds();
	}
	else
	{
		snprintf(menupath, 1024, "%s"PATHSEP"media"PATHSEP"replay"PATHSEP"online"PATHSEP, srb2home);
		menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath);
	}

	if (!preparefilemenu(false, true))
	{
		M_StartMessage("No replays found.\n\nPress (B)\n", NULL, MM_NOTHING);
		demo.inreplayhut = false;
		return;
	}
	else if (!demo.inreplayhut)
	{
		dir_on[menudepthleft] = 0;
	}

	extrasmenu.replayScrollTitle = 0;
	extrasmenu.replayScrollDelay = TICRATE;
	extrasmenu.replayScrollDir = 1;

	M_PrepReplayList();

	if (!demo.inreplayhut)
		M_SetupNextMenu(&EXTRAS_ReplayHutDef, false);

	demo.inreplayhut = true;
}

// key handler
void M_HandleReplayHutList(INT32 choice)
{

	const UINT8 pid = 0;
	(void) choice;

	if (menucmd[pid].dpad_ud < 0)
	{
		if (dir_on[menudepthleft])
			dir_on[menudepthleft]--;
		else
			return;
			//M_PrevOpt();

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;
	}

	else if (menucmd[pid].dpad_ud > 0)
	{
		if (dir_on[menudepthleft] < sizedirmenu-1)
			dir_on[menudepthleft]++;
		else
			return;
			//itemOn = 0; // Not M_NextOpt because that would take us to the extra dummy item

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;
	}

	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);
		M_QuitReplayHut();
	}

	else if (M_MenuConfirmPressed(pid))
	{
		M_SetMenuDelay(pid);
		switch (dirmenu[dir_on[menudepthleft]][DIR_TYPE])
		{
			case EXT_FOLDER:
				strcpy(&menupath[menupathindex[menudepthleft]],dirmenu[dir_on[menudepthleft]]+DIR_STRING);
				if (menudepthleft)
				{
					menupathindex[--menudepthleft] = strlen(menupath);
					menupath[menupathindex[menudepthleft]] = 0;

					if (!preparefilemenu(false, true))
					{
						S_StartSound(NULL, sfx_s224);
						M_StartMessage(va("%c%s\x80\nThis folder is empty.\n\nPress (B)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
						menupath[menupathindex[++menudepthleft]] = 0;

						if (!preparefilemenu(true, true))
						{
							M_QuitReplayHut();
							return;
						}
					}
					else
					{
						S_StartSound(NULL, sfx_s3k5b);
						dir_on[menudepthleft] = 1;
						M_PrepReplayList();
					}
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
				if (!preparefilemenu(false, true))
				{
					M_QuitReplayHut();
					return;
				}
				M_PrepReplayList();
				break;
			default:
				M_SetupNextMenu(&EXTRAS_ReplayStartDef, true);

				extrasmenu.replayScrollTitle = 0;
				extrasmenu.replayScrollDelay = TICRATE;
				extrasmenu.replayScrollDir = 1;

				switch (extrasmenu.demolist[dir_on[menudepthleft]].addonstatus)
				{
				case DFILE_ERROR_CANNOTLOAD:
					// Only show "Watch Replay Without Addons"
					EXTRAS_ReplayStart[0].status = IT_DISABLED;
					EXTRAS_ReplayStart[1].status = IT_CALL|IT_STRING;
					//EXTRAS_ReplayStart[1].alphaKey = 0;
					EXTRAS_ReplayStart[2].status = IT_DISABLED;
					itemOn = 1;
					break;

				case DFILE_ERROR_NOTLOADED:
				case DFILE_ERROR_INCOMPLETEOUTOFORDER:
					// Show "Load Addons and Watch Replay" and "Watch Replay Without Addons"
					EXTRAS_ReplayStart[0].status = IT_CALL|IT_STRING;
					EXTRAS_ReplayStart[1].status = IT_CALL|IT_STRING;
					//EXTRAS_ReplayStart[1].alphaKey = 10;
					EXTRAS_ReplayStart[2].status = IT_DISABLED;
					itemOn = 0;
					break;

				case DFILE_ERROR_EXTRAFILES:
				case DFILE_ERROR_OUTOFORDER:
				default:
					// Show "Watch Replay"
					EXTRAS_ReplayStart[0].status = IT_DISABLED;
					EXTRAS_ReplayStart[1].status = IT_DISABLED;
					EXTRAS_ReplayStart[2].status = IT_CALL|IT_STRING;
					//EXTRAS_ReplayStart[2].alphaKey = 0;
					itemOn = 2;
					break;
				}
		}
	}
}

boolean M_QuitReplayHut(void)
{
	if (extrasmenu.demolist)
		Z_Free(extrasmenu.demolist);
	extrasmenu.demolist = NULL;

	demo.inreplayhut = false;

	M_GoBack(0);
	return true;
}

void M_HutStartReplay(INT32 choice)
{
	(void)choice;

	restoreMenu = &EXTRAS_ReplayHutDef;

	M_ClearMenus(false);
	demo.loadfiles = (itemOn == 0);
	demo.ignorefiles = (itemOn != 0);

	G_DoPlayDemo(extrasmenu.demolist[dir_on[menudepthleft]].filepath);
}
