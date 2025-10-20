// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by "Lat'".
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.

// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-char-select.c
/// \brief Character Select

#include "../i_time.h"
#include "../k_menu.h"
#include "../r_skins.h"
#include "../s_sound.h"
#include "../k_grandprix.h" // K_CanChangeRules
#include "../m_cond.h" // Condition Sets
#include "../k_color.h"
#include "../z_zone.h"

//#define CHARSELECT_DEVICEDEBUG

menuitem_t PLAY_CharSelect[] =
{
	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

static void M_DrawCharacterBack(void)
{
	if (!optionsmenu.profile)
	{
		if (gamestate == GS_MENU)
			M_DrawMenuBackground();
		return;
	}

	M_DrawOptionsCogs();
}

menu_t PLAY_CharSelectDef = {
	sizeof (PLAY_CharSelect) / sizeof (menuitem_t),
	&MainDef,
	0,
	PLAY_CharSelect,
	0, 0,
	SKINCOLOR_ULTRAMARINE, 0,
	0,
	NULL,
	2, 5, // matches OPTIONS_EditProfileDef
	M_DrawCharacterSelect,
	M_DrawCharacterBack,
	M_CharacterSelectTick,
	M_CharacterSelectInit,
	M_CharacterSelectQuit,
	M_CharacterSelectHandler
};

CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};

// Character Select!
// @TODO: Splitscreen handling when profiles are added into the game. ...I probably won't be the one to handle this however. -Lat'

struct setup_chargrid_s setup_chargrid[9][9];
setup_player_t setup_player[MAXSPLITSCREENPLAYERS];

UINT8 setup_followercategories[MAXFOLLOWERCATEGORIES][2];
UINT8 setup_numfollowercategories;

UINT8 setup_numplayers = 0; // This variable is very important, it was extended to determine how many players exist in ALL menus.
tic_t setup_animcounter = 0;

UINT8 setup_page = 0;
UINT8 setup_maxpage = 0;	// For charsel page to identify alts easier...

static void M_PushMenuColor(setup_player_colors_t *colors, UINT16 newColor)
{
	if (colors->listLen >= colors->listCap)
	{
		if (colors->listCap == 0)
		{
			colors->listCap = 64;
		}
		else
		{
			colors->listCap *= 2;
		}

		colors->list = Z_ReallocAlign(
			colors->list,
			sizeof(UINT16) * colors->listCap,
			PU_STATIC,
			NULL,
			sizeof(UINT16) * 8
		);
	}

	colors->list[colors->listLen] = newColor;
	colors->listLen++;
}

static void M_ClearMenuColors(setup_player_colors_t *colors)
{
	if (colors->list != NULL)
	{
		Z_Free(colors->list);
		colors->list = NULL;
	}

	colors->listLen = colors->listCap = 0;
}

UINT16 M_GetColorAfter(setup_player_colors_t *colors, UINT16 value, INT32 amount)
{
	const INT32 sign = (amount < 0) ? -1 : 1;
	INT32 index = -1;
	size_t i = SIZE_MAX;

	if (amount == 0 || colors->listLen == 0)
	{
		return value;
	}

	for (i = 0; i < colors->listLen; i++)
	{
		if (colors->list[i] == value)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		// Not in list, so no real correct behavior here.
		// Just try to get back to the list.
		return colors->list[0];
	}

	amount = abs(amount);

	while (amount > 0)
	{
		index += sign;

		if (index < 0)
		{
			index = colors->listLen - 1;
		}
		else if (index >= (INT32)colors->listLen)
		{
			index = 0;
		}

		amount--;
	}

	return colors->list[index];
}

static void M_NewPlayerColors(setup_player_t *p)
{
	const boolean follower = (p->mdepth >= CSSTEP_FOLLOWER);
	INT32 i = INT32_MAX;

	M_ClearMenuColors(&p->colors);

	// Add all unlocked colors
	for (i = SKINCOLOR_NONE+1; i < numskincolors; i++)
	{
		if (K_ColorUsable(i, follower, true) == true)
		{
			M_PushMenuColor(&p->colors, i);
		}
	}

	// Add special colors
	M_PushMenuColor(&p->colors, SKINCOLOR_NONE);

	if (follower == true)
	{
		// Add special follower colors
		M_PushMenuColor(&p->colors, FOLLOWERCOLOR_MATCH);
		M_PushMenuColor(&p->colors, FOLLOWERCOLOR_OPPOSITE);
	}
}

static INT16 M_GetMenuCategoryFromFollower(setup_player_t *p)
{
	if (p->followern < 0
	|| p->followern >= numfollowers
	|| !K_FollowerUsable(p->followern))
		return -1;

	INT16 i;

	for (i = 0; i < setup_numfollowercategories; i++)
	{
		if (followers[p->followern].category != setup_followercategories[i][1])
			continue;

		break;
	}

	if (i >= setup_numfollowercategories)
		return -1;

	return i;
}

// sets up the grid pos for the skin used by the profile.
static void M_SetupProfileGridPos(setup_player_t *p)
{
	profile_t *pr = PR_GetProfile(p->profilen);
	INT32 i = R_SkinAvailableEx(pr->skinname, false);
	INT32 alt = 0;	// Hey it's my character's name!

	if (i == -1)
		i = 0;

	// While we're here, read follower values.
	p->followern = K_FollowerAvailable(pr->follower);

	p->followercategory = M_GetMenuCategoryFromFollower(p);
	if (p->followercategory == -1) // unlock gate failed?
		p->followern = -1;

	p->followercolor = pr->followercolor;
	if (K_ColorUsable(p->followercolor, true, true) == false)
	{
		p->followercolor = SKINCOLOR_NONE;
	}

	if (!R_SkinUsable(g_localplayers[0], i, false))
	{
		i = GetSkinNumClosestToStats(skins[i]->kartspeed, skins[i]->kartweight, skins[i]->flags, false);
	}

	// Now position the grid for skin
	p->gridx = skins[i]->kartspeed-1;
	p->gridy = skins[i]->kartweight-1;

	// Now this put our cursor on the good alt
	while (alt < setup_chargrid[p->gridx][p->gridy].numskins && setup_chargrid[p->gridx][p->gridy].skinlist[alt] != i)
		alt++;

	p->clonenum = alt;

	p->color = pr->color;
	if (K_ColorUsable(p->color, false, true) == false)
	{
		p->color = SKINCOLOR_NONE;
	}

}

static void M_SetupMidGameGridPos(setup_player_t *p, UINT8 num)
{
	INT32 i = R_SkinAvailableEx(cv_skin[num].zstring, false);
	INT32 alt = 0;	// Hey it's my character's name!

	if (i == -1)
		i = 0;

	// While we're here, read follower values.
	p->followern = cv_follower[num].value;
	p->followercolor = cv_followercolor[num].value;

	p->followercategory = M_GetMenuCategoryFromFollower(p);
	if (p->followercategory == -1) // unlock gate failed?
		p->followern = -1;

	// Now position the grid for skin
	p->gridx = skins[i]->kartspeed-1;
	p->gridy = skins[i]->kartweight-1;

	// Now this put our cursor on the good alt
	while (alt < setup_chargrid[p->gridx][p->gridy].numskins && setup_chargrid[p->gridx][p->gridy].skinlist[alt] != i)
		alt++;

	p->clonenum = alt;
	p->color = cv_playercolor[num].value;
	return;	// we're done here
}


void M_CharacterSelectInit(void)
{
	UINT16 i, j;
	setup_maxpage = 0;

	memset(setup_chargrid, -1, sizeof(setup_chargrid));
	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
			setup_chargrid[i][j].numskins = 0;
	}

	memset(setup_player, 0, sizeof(setup_player));
	setup_numplayers = 0;

	memset(setup_explosions, 0, sizeof(setup_explosions));
	setup_animcounter = 0;

	for (i = 0; i < numskins; i++)
	{
		UINT8 x = skins[i]->kartspeed-1;
		UINT8 y = skins[i]->kartweight-1;

		if (!R_SkinUsable(g_localplayers[0], i, false))
			continue;

		if (setup_chargrid[x][y].numskins >= MAXCLONES)
			CONS_Alert(CONS_ERROR, "Max character alts reached for %d,%d\n", x+1, y+1);
		else
		{
			setup_chargrid[x][y].skinlist[setup_chargrid[x][y].numskins] = i;
			setup_chargrid[x][y].numskins++;

			setup_maxpage = max(setup_maxpage, setup_chargrid[x][y].numskins-1);
		}
	}

	setup_numfollowercategories = 0;
	for (i = 0; i < numfollowercategories; i++)
	{
		if (followercategories[i].numincategory == 0)
			continue;

		setup_followercategories[setup_numfollowercategories][0] = 0;

		for (j = 0; j < numfollowers; j++)
		{
			if (followers[j].category != i)
				continue;

			if (!K_FollowerUsable(j))
				continue;

			setup_followercategories[setup_numfollowercategories][0]++;
			setup_followercategories[setup_numfollowercategories][1] = i;
		}

		if (!setup_followercategories[setup_numfollowercategories][0])
			continue;

		setup_numfollowercategories++;
	}

	setup_page = 0;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		// Default to no follower / match colour.
		setup_player[i].followern = -1;
		setup_player[i].followercategory = -1;
		setup_player[i].followercolor = SKINCOLOR_NONE;

		setup_player[i].profilen_slide.start = 0;
		setup_player[i].profilen_slide.dist = 0;

		// If we're on prpfile select, skip straight to CSSTEP_CHARS
		// do the same if we're midgame, but make sure to consider splitscreen properly.
		if (optionsmenu.profile && i == 0)
		{
			setup_player[i].profilen = optionsmenu.profilen;
			//PR_ApplyProfileLight(setup_player[i].profilen, 0);
			M_SetupProfileGridPos(&setup_player[i]);
			setup_player[i].mdepth = CSSTEP_CHARS;
		}
		else
		{
			// Set default selected profile to the last used profile for each player:
			// (Make sure we don't overshoot it somehow if we deleted profiles or whatnot)
			setup_player[i].profilen = min(cv_lastprofile[i].value, PR_GetNumProfiles());

			if (gamestate != GS_MENU && i <= splitscreen)
			{
				M_SetupMidGameGridPos(&setup_player[i], i);
				setup_player[i].mdepth = CSSTEP_CHARS;
			}
			else
			{
				// Un-set devices
				G_SetDeviceForPlayer(i, -1);
#ifdef CHARSELECT_DEVICEDEBUG
				CONS_Printf("M_CharacterSelectInit: Device for %d set to %d\n", i, -1);
#endif
			}
		}
	}
}


void M_CharacterSelect(INT32 choice)
{
	(void)choice;
	PLAY_CharSelectDef.music = currentMenu->music;
	PLAY_CharSelectDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PLAY_CharSelectDef, false);
}


// Gets the selected follower's state for a given setup player.
static void M_GetFollowerState(setup_player_t *p)
{
	if (p->followern < 0 || p->followern >= numfollowers)
		return;

	p->follower_state = &states[followers[p->followern].followstate];

	if (p->follower_state->frame & FF_ANIMATE)
		p->follower_tics = p->follower_state->var2;	// support for FF_ANIMATE
	else
		p->follower_tics = p->follower_state->tics;

	p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
}

static boolean M_DeviceAvailable(INT32 deviceID, UINT8 numPlayers)
{
	INT32 i;

	if (numPlayers == 0)
	{
		// All of them are available!
		return true;
	}

	for (i = 0; i < numPlayers; i++)
	{
		int player_device = G_GetDeviceForPlayer(i);
		if (player_device == -1)
		{
			continue;
		}
		if (player_device == deviceID)
		{
			// This one's already being used.
			return false;
		}
	}

	// This device is good to go.
	return true;
}

static boolean M_HandlePressStart(setup_player_t *p, UINT8 num)
{
	INT32 i, j;
	INT32 num_gamepads_available;

	if (optionsmenu.profile)
		return false;	// Don't allow for the possibility of SOMEHOW another player joining in.

	// Detect B press first ... this means P1 can actually exit out of the menu.
	if (M_MenuBackPressed(num))
	{
		M_SetMenuDelay(num);

		if (num == 0)
		{
			// We're done here.
			memset(setup_player, 0, sizeof(setup_player));	// Reset this to avoid funky things with profile display.
			M_GoBack(0);
			return true;
		}

		// Don't allow this press to ever count as "start".
		return false;
	}

	if (num != setup_numplayers)
	{
		// Only detect devices for the last player.
		return false;
	}

	// Now detect new devices trying to join.
	num_gamepads_available = G_GetNumAvailableGamepads();
	for (i = 0; i < num_gamepads_available + 1; i++)
	{
		INT32 device = 0;

		if (i > 0)
		{
			device = G_GetAvailableGamepadDevice(i - 1);
		}

		if (device == KEYBOARD_MOUSE_DEVICE && num != 0)
		{
			// Only player 1 can be assigned to the KBM device.
			continue;
		}

		if (G_IsDeviceResponding(device) != true)
		{
			// No buttons are being pushed.
			continue;
		}

		if (M_DeviceAvailable(device, setup_numplayers) == true)
		{
			// Available!! Let's use this one!!

			// if P1 is setting up using keyboard (device 0), save their last used device.
			// this is to allow them to retain controller usage when they play alone.
			// Because let's face it, when you test mods, you're often lazy to grab your controller for menuing :)
			if (i == 0 && num == 0)
			{
				setup_player[num].ponedevice = G_GetDeviceForPlayer(num);
			}
			else if (num)
			{
				// For any player past player 1, set controls to default profile controls, otherwise it's generally awful to do any menuing...
				G_ApplyControlScheme(num, gamecontroldefault);
			}

			G_SetDeviceForPlayer(num, device);
#ifdef CHARSELECT_DEVICEDEBUG
			CONS_Printf("M_HandlePressStart: Device for %d set to %d\n", num, device);
#endif

			for (j = num + 1; j < MAXSPLITSCREENPLAYERS; j++)
			{
				// Un-set devices for other players.
				G_SetDeviceForPlayer(j, -1);
#ifdef CHARSELECT_DEVICEDEBUG
				CONS_Printf("M_HandlePressStart: Device for %d set to %d\n", j, -1);
#endif
			}

			//setup_numplayers++;
			p->mdepth = CSSTEP_PROFILE;
			S_StartSound(NULL, sfx_s3k65);

			// Prevent quick presses for multiple players
			for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
			{
				setup_player[j].delay = MENUDELAYTIME;
				M_SetMenuDelay(j);
				menucmd[j].buttonsHeld |= MBT_X;
			}

			G_ResetAllDeviceResponding();
			return true;
		}
	}

	return false;
}

static boolean M_HandleCSelectProfile(setup_player_t *p, UINT8 num)
{
	const UINT8 maxp = PR_GetNumProfiles() -1;
	UINT8 realnum = num;	// Used for profile when using splitdevice.
	UINT8 i;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_ud > 0)
	{
		UINT8 oldn = p->profilen;
		p->profilen++;
		if (p->profilen > maxp)
			p->profilen = 0;
		p->profilen_slide.dist = p->profilen - oldn;
		p->profilen_slide.start = I_GetTime();

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_ud < 0)
	{
		UINT8 oldn = p->profilen;
		if (p->profilen == 0)
			p->profilen = maxp;
		else
			p->profilen--;
		p->profilen_slide.dist = p->profilen - oldn;
		p->profilen_slide.start = I_GetTime();

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		if (num == setup_numplayers-1)
		{

			p->mdepth = CSSTEP_NONE;
			S_StartSound(NULL, sfx_s3k5b);

			// Prevent quick presses for multiple players
			for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
			{
				setup_player[i].delay = MENUDELAYTIME;
				M_SetMenuDelay(i);
				menucmd[i].buttonsHeld |= MBT_X;
			}

			G_SetDeviceForPlayer(num, -1);
#ifdef CHARSELECT_DEVICEDEBUG
			CONS_Printf("M_HandleCSelectProfile: Device for %d set to %d\n", num, -1);
#endif

			return true;
		}
		else
		{
			S_StartSound(NULL, sfx_s3kb2);
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuConfirmPressed(num))
	{
		SINT8 belongsTo = -1;

		if (p->profilen != PROFILE_GUEST)
		{
			for (i = 0; i < setup_numplayers; i++)
			{
				if (setup_player[i].mdepth > CSSTEP_PROFILE
					&& setup_player[i].profilen == p->profilen)
				{
					belongsTo = i;
					break;
				}
			}
		}

		if (belongsTo != -1 && belongsTo != num)
		{
			S_StartSound(NULL, sfx_s3k7b);
			M_SetMenuDelay(num);
			return false;
		}

		// Apply the profile.
		PR_ApplyProfile(p->profilen, realnum);	// Otherwise P1 would inherit the last player's profile in splitdevice and that's not what we want...
		M_SetupProfileGridPos(p);

		p->changeselect = 0;

		if (p->profilen == PROFILE_GUEST)
		{
			// Guest profile, always ask for options.
			p->mdepth = CSSTEP_CHARS;
		}
		else
		{
			p->mdepth = CSSTEP_ASKCHANGES;
			M_GetFollowerState(p);
		}

		S_StartSound(NULL, sfx_s3k63);
	}
	else if (M_MenuExtraPressed(num))
	{
		UINT8 oldn = p->profilen;
		UINT8 yourprofile = min(cv_lastprofile[realnum].value, PR_GetNumProfiles());
		if (p->profilen == yourprofile)
			p->profilen = PROFILE_GUEST;
		else
			p->profilen = yourprofile;
		p->profilen_slide.dist = p->profilen - oldn;
		p->profilen_slide.start = I_GetTime();
		S_StartSound(NULL, sfx_s3k7b); //sfx_s3kc3s
		M_SetMenuDelay(num);
	}

	return false;

}


static void M_HandlePlayerFinalise(setup_player_t *p)
{
	p->mdepth = CSSTEP_READY;
	p->delay = TICRATE;
	M_SetupReadyExplosions(true, p->gridx, p->gridy, p->color);
	S_StartSound(NULL, sfx_s3k4e);
}


static void M_HandleCharAskChange(setup_player_t *p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	// there's only 2 options so lol
	if (menucmd[num].dpad_ud != 0)
	{
		p->changeselect = (p->changeselect == 0) ? 1 : 0;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->changeselect = 0;
		p->mdepth = CSSTEP_PROFILE;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuConfirmPressed(num))
	{
		if (!p->changeselect)
		{
			// no changes
			M_HandlePlayerFinalise(p);
		}
		else
		{
			// changes
			p->mdepth = CSSTEP_CHARS;
			S_StartSound(NULL, sfx_s3k63);
		}

		M_SetMenuDelay(num);
	}
}

boolean M_CharacterSelectForceInAction(void)
{
	if (!Playing())
		return false;

	if (K_CanChangeRules(true) == false)
		return false;

	return (cv_forceskin.value != -1);
}

static void M_HandleBackToChars(setup_player_t *p)
{
	boolean forceskin = M_CharacterSelectForceInAction();

	if (forceskin
	|| setup_chargrid[p->gridx][p->gridy].numskins == 1)
	{
		p->mdepth = CSSTEP_CHARS; // Skip clones menu
	}
	else
	{
		p->mdepth = CSSTEP_ALTS;
	}
}

static boolean M_HandleBeginningColors(setup_player_t *p)
{
	p->mdepth = CSSTEP_COLORS;

	if (Playing() && G_GametypeHasTeams())
	{
		size_t pnum = (p - setup_player);
		if (pnum <= splitscreen)
		{
			if (players[g_localplayers[pnum]].team != TEAM_UNASSIGNED)
			{
				p->color = g_teaminfo[players[g_localplayers[pnum]].team].color;
				return false;
			}
		}
	}

	M_NewPlayerColors(p);
	if (p->colors.listLen != 1)
		return true;

	p->color = p->colors.list[0];
	return false;
}

static void M_HandleBeginningFollowers(setup_player_t *p)
{
	if (horngoner || setup_numfollowercategories == 0)
	{
		p->followern = -1;
		M_HandlePlayerFinalise(p);
	}
	else
	{
		p->mdepth = CSSTEP_FOLLOWERCATEGORY;
		S_StartSound(NULL, sfx_s3k63);
	}
}

static void M_HandleBeginningColorsOrFollowers(setup_player_t *p)
{
	if (p->skin != -1)
		S_StartSound(NULL, skins[p->skin]->soundsid[S_sfx[sfx_kattk1].skinsound]);
	if (M_HandleBeginningColors(p))
		S_StartSound(NULL, sfx_s3k63);
	else
		M_HandleBeginningFollowers(p);
}

static boolean M_HandleCharacterGrid(setup_player_t *p, UINT8 num)
{
	UINT8 numclones;
	INT32 skin;
	boolean forceskin = M_CharacterSelectForceInAction();

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_ud > 0)
	{
		p->gridy++;
		if (p->gridy > 8)
			p->gridy = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_ud < 0)
	{
		p->gridy--;
		if (p->gridy < 0)
			p->gridy = 8;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}

	if (menucmd[num].dpad_lr > 0)
	{
		p->gridx++;
		if (p->gridx > 8)
			p->gridx = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->gridx--;
		if (p->gridx < 0)
			p->gridx = 8;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->gridx /= 3;
		p->gridx = (3*p->gridx) + 1;
		p->gridy /= 3;
		p->gridy = (3*p->gridy) + 1;
		S_StartSound(NULL, sfx_s3k7b); //sfx_s3kc3s
		M_SetMenuDelay(num);
	}

	// try to set the clone num to the page # if possible.
	p->clonenum = setup_page;

	// Process this after possible pad movement,
	// this makes sure we don't have a weird ghost hover on a character with no clones.
	numclones = setup_chargrid[p->gridx][p->gridy].numskins;

	if (p->clonenum >= numclones)
		p->clonenum = 0;

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		if (forceskin)
		{
			if ((p->gridx != skins[cv_forceskin.value]->kartspeed-1)
				|| (p->gridy != skins[cv_forceskin.value]->kartweight-1))
			{
				S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2
			}
			else
			{
				M_HandleBeginningColorsOrFollowers(p);
			}
		}
		else
		{
			skin = setup_chargrid[p->gridx][p->gridy].skinlist[setup_page];
			if (setup_page >= setup_chargrid[p->gridx][p->gridy].numskins || skin == -1)
			{
				S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2
			}
			else
			{
				if (setup_page+1 == setup_chargrid[p->gridx][p->gridy].numskins)
				{
					M_HandleBeginningColorsOrFollowers(p);
				}
				else
				{
					p->mdepth = CSSTEP_ALTS;
					S_StartSound(NULL, sfx_s3k63);
				}
			}
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		// for profiles / gameplay, exit out of the menu instantly,
		// we don't want to go to the input detection menu.
		if (optionsmenu.profile || gamestate != GS_MENU)
		{
			memset(setup_player, 0, sizeof(setup_player));	// Reset setup_player otherwise it does some VERY funky things.
			M_SetMenuDelay(0);
			M_GoBack(0);
			return true;
		}
		else	// in main menu
		{
			p->mdepth = CSSTEP_PROFILE;
			S_StartSound(NULL, sfx_s3k5b);
		}
		M_SetMenuDelay(num);
	}

	if (num == 0 && setup_numplayers == 1 && setup_maxpage && !forceskin)	// ONLY one player.
	{
		if (M_MenuButtonPressed(num, MBT_L))
		{
			if (setup_page == setup_maxpage)
				setup_page = 0;
			else
				setup_page++;

			S_StartSound(NULL, sfx_s3k63);
			M_SetMenuDelay(num);
		}
	}

	return false;
}

static void M_HandleCharRotate(setup_player_t *p, UINT8 num)
{
	UINT8 numclones = setup_chargrid[p->gridx][p->gridy].numskins;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->clonenum++;
		if (p->clonenum >= numclones)
			p->clonenum = 0;
		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->clonenum--;
		if (p->clonenum < 0)
			p->clonenum = numclones-1;
		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		M_HandleBeginningColorsOrFollowers(p);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->mdepth = CSSTEP_CHARS;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->clonenum = 0;
		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); //sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

static void M_HandleColorRotate(setup_player_t *p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->color = M_GetColorAfter(&p->colors, p->color, 1);
		p->rotate = CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->color = M_GetColorBefore(&p->colors, p->color, 1);
		p->rotate = -CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}

	 if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		M_HandleBeginningFollowers(p);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		M_HandleBackToChars(p);

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		if (p->skin >= 0)
		{
			if (p->color == SKINCOLOR_NONE)
				p->color = PR_GetProfile(p->profilen)->color;
			else
				p->color = SKINCOLOR_NONE;
			p->rotate = CSROTATETICS;
			p->hitlag = true;
			S_StartSound(NULL, sfx_s3k7b); //sfx_s3kc3s
			M_SetMenuDelay(num);
		}
	}
}

static void M_AnimateFollower(setup_player_t *p)
{
	if (--p->follower_tics <= 0)
	{

		// FF_ANIMATE; cycle through FRAMES and get back afterwards. This will be prominent amongst followers hence why it's being supported here.
		if (p->follower_state->frame & FF_ANIMATE)
		{
			p->follower_frame++;
			p->follower_tics = p->follower_state->var2;
			if (p->follower_frame > (p->follower_state->frame & FF_FRAMEMASK) + p->follower_state->var1)	// that's how it works, right?
				p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
		}
		else
		{
			if (p->follower_state->nextstate != S_NULL)
				p->follower_state = &states[p->follower_state->nextstate];
			p->follower_tics = p->follower_state->tics;
			/*if (p->follower_tics == -1)
				p->follower_tics = 15;	// er, what?*/
			// get spritedef:
			p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
		}
	}

	p->follower_timer++;
}

static void M_HandleFollowerCategoryRotate(setup_player_t *p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->followercategory++;
		if (p->followercategory >= setup_numfollowercategories)
			p->followercategory = -1;

		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->followercategory--;
		if (p->followercategory < -1)
			p->followercategory = setup_numfollowercategories-1;

		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		if (p->followercategory < 0)
		{
			p->followern = -1;
			M_HandlePlayerFinalise(p);
		}
		else
		{
			if (p->followern < 0
			|| followers[p->followern].category != setup_followercategories[p->followercategory][1])
			{
				p->followern = 0;
				while (p->followern < numfollowers)
				{
					if (followers[p->followern].category == setup_followercategories[p->followercategory][1]
						&& K_FollowerUsable(p->followern))
						break;
					p->followern++;
				}
			}

			if (p->followern < numfollowers)
			{
				M_GetFollowerState(p);
				p->mdepth = CSSTEP_FOLLOWER;
				S_StartSound(NULL, sfx_s3k63);
			}
			else
			{
				p->followern = -1;
				S_StartSound(NULL, sfx_s3kb2);
			}
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		if (!M_HandleBeginningColors(p))
			M_HandleBackToChars(p);

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->followercategory = (p->followercategory == -1)
			? M_GetMenuCategoryFromFollower(p)
			: -1;

		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); //sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

static void M_HandleFollowerRotate(setup_player_t *p, UINT8 num)
{
	INT16 startfollowern = p->followern;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		do
		{
			p->followern++;
			if (p->followern >= numfollowers)
				p->followern = 0;
			if (p->followern == startfollowern)
				break;
		}
		while (followers[p->followern].category != setup_followercategories[p->followercategory][1] || !K_FollowerUsable(p->followern));

		M_GetFollowerState(p);

		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		do
		{
			p->followern--;
			if (p->followern < 0)
				p->followern = numfollowers-1;
			if (p->followern == startfollowern)
				break;
		}
		while (followers[p->followern].category != setup_followercategories[p->followercategory][1] || !K_FollowerUsable(p->followern));

		M_GetFollowerState(p);

		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		if (p->followern > -1)
		{
			p->mdepth = CSSTEP_FOLLOWERCOLORS;
			M_NewPlayerColors(p);
			S_StartSound(NULL, sfx_s3k63);
			if (p->followern != -1)
				S_StartSound(NULL, followers[p->followern].hornsound);
		}
		else
		{
			M_HandlePlayerFinalise(p);
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->mdepth = CSSTEP_FOLLOWERCATEGORY;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		p->mdepth = CSSTEP_FOLLOWERCATEGORY;
		p->followercategory = -1;
		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); //sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

static void M_HandleFollowerColorRotate(setup_player_t *p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->followercolor = M_GetColorAfter(&p->colors, p->followercolor, 1);
		p->rotate = CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->followercolor = M_GetColorBefore(&p->colors, p->followercolor, 1);
		p->rotate = -CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		M_HandlePlayerFinalise(p);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		M_GetFollowerState(p);
		p->mdepth = CSSTEP_FOLLOWER;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuExtraPressed(num))
	{
		UINT16 profile_followercolor = PR_GetProfile(p->profilen)->followercolor;
		if (p->followercolor == FOLLOWERCOLOR_MATCH)
			p->followercolor = FOLLOWERCOLOR_OPPOSITE;
		else if (p->followercolor == FOLLOWERCOLOR_OPPOSITE && profile_followercolor != FOLLOWERCOLOR_OPPOSITE && profile_followercolor != FOLLOWERCOLOR_MATCH)
			p->followercolor = profile_followercolor;
		else if (p->followercolor == SKINCOLOR_NONE)
			p->followercolor = FOLLOWERCOLOR_MATCH;
		else
			p->followercolor = SKINCOLOR_NONE;
		p->rotate = CSROTATETICS;
		p->hitlag = true;
		S_StartSound(NULL, sfx_s3k7b); //sfx_s3kc3s
		M_SetMenuDelay(num);
	}
}

boolean M_CharacterSelectHandler(INT32 choice)
{
	INT32 i;
	boolean forceskin = M_CharacterSelectForceInAction();

	(void)choice;

	for (i = MAXSPLITSCREENPLAYERS-1; i >= 0; i--)
	{
		setup_player_t *p = &setup_player[i];
		boolean playersChanged = false;

		if (p->mdepth > CSSTEP_FOLLOWER)
		{
			M_AnimateFollower(p);
		}

		if (p->delay == 0 && menucmd[i].delay == 0)
		{
			if (!optionsmenu.profile)
			{
				// If splitdevice is true, only do the last non-ready setups.
				if (cv_splitdevice.value)
				{
					// Previous setup isn't ready, go there.
					// In any case, do setup 0 first.
					if (i > 0 && setup_player[i-1].mdepth < CSSTEP_READY)
						continue;
				}

				if (M_MenuButtonPressed(i, MBT_R))
				{
					p->showextra ^= true;
				}
			}

			switch (p->mdepth)
			{
				case CSSTEP_NONE: // Enter Game
					if (gamestate == GS_MENU)	// do NOT handle that outside of GS_MENU.
						playersChanged = M_HandlePressStart(p, i);
					break;
				case CSSTEP_PROFILE:
					playersChanged = M_HandleCSelectProfile(p, i);
					break;
				case CSSTEP_ASKCHANGES:
					M_HandleCharAskChange(p, i);
					break;
				case CSSTEP_CHARS: // Character Select grid
					M_HandleCharacterGrid(p, i);
					break;
				case CSSTEP_ALTS: // Select clone
					M_HandleCharRotate(p, i);
					break;
				case CSSTEP_COLORS: // Select color
					M_HandleColorRotate(p, i);
					break;
				case CSSTEP_FOLLOWERCATEGORY:
					M_HandleFollowerCategoryRotate(p, i);
					break;
				case CSSTEP_FOLLOWER:
					M_HandleFollowerRotate(p, i);
					break;
				case CSSTEP_FOLLOWERCOLORS:
					M_HandleFollowerColorRotate(p, i);
					break;
				case CSSTEP_READY:
				default: // Unready
					if (M_MenuBackPressed(i))
					{
						if (!M_HandleBeginningColors(p))
							M_HandleBackToChars(p);

						S_StartSound(NULL, sfx_s3k5b);
						M_SetMenuDelay(i);
					}
					break;
			}
		}

		// Just makes it easier to access later
		if (forceskin)
		{
			if (p->gridx != skins[cv_forceskin.value]->kartspeed-1
				|| p->gridy != skins[cv_forceskin.value]->kartweight-1)
				p->skin = -1;
			else
				p->skin = cv_forceskin.value;
		}
		else
		{
			p->skin = setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum];
		}

		if (playersChanged == true)
		{
			setup_page = 0;	// reset that.
			break;
		}
	}

	// Setup new numplayers
	setup_numplayers = 0;
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].mdepth == CSSTEP_NONE)
			break;

		setup_numplayers = i+1;
	}

	return true;
}

// Apply character skin and colour changes while ingame (we just call the skin / color commands.)
// ...Will this cause command buffer issues? -Lat'
static void M_MPConfirmCharacterSelection(void)
{
	UINT8 i;
	INT16 col;

	for (i = 0; i < splitscreen +1; i++)
	{
		// colour
		// (convert the number that's saved to a string we can use)
		col = setup_player[i].color;
		CV_StealthSetValue(&cv_playercolor[i], col);

		// follower
		if (horngoner)
			;
		else if (setup_player[i].followern < 0)
			CV_StealthSet(&cv_follower[i], "None");
		else
			CV_StealthSet(&cv_follower[i], followers[setup_player[i].followern].name);

		// finally, call the skin[x] console command.
		// This will call SendNameAndColor which will synch everything we sent here and apply the changes!

		CV_StealthSet(&cv_skin[i], skins[setup_player[i].skin]->name);

		// ...actually, let's do this last - Skin_OnChange has some return-early occasions
		// follower color
		CV_SetValue(&cv_followercolor[i], setup_player[i].followercolor);

	}
	M_ClearMenus(true);
}

void M_CharacterSelectTick(void)
{
	UINT8 i;
	boolean setupnext = true;

	setup_animcounter++;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].delay)
			setup_player[i].delay--;

		if (setup_player[i].rotate > 0)
			setup_player[i].rotate--;
		else if (setup_player[i].rotate < 0)
			setup_player[i].rotate++;
		else
			setup_player[i].hitlag = false;

		if (i >= setup_numplayers)
			continue;

		if (setup_player[i].mdepth < CSSTEP_READY || setup_player[i].delay > 0)
		{
			// Someone's not ready yet.
			setupnext = false;
		}
	}

	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		if (setup_explosions[i].tics > 0)
			setup_explosions[i].tics--;
	}

	if (setupnext && setup_numplayers > 0)
	{
		// Selecting from the menu
		if (gamestate == GS_MENU)
		{
			// in a profile; update the selected profile and then go back to the profile menu.
			if (optionsmenu.profile)
			{
				// save player
				strcpy(optionsmenu.profile->skinname, skins[setup_player[0].skin]->name);
				optionsmenu.profile->color = setup_player[0].color;

				if (!horngoner) // so you don't lose your choice after annoying the game
				{
					// save follower
					strcpy(optionsmenu.profile->follower, followers[setup_player[0].followern].name);
					optionsmenu.profile->followercolor = setup_player[0].followercolor;
				}

				// reset setup_player
				memset(setup_player, 0, sizeof(setup_player));
				setup_numplayers = 0;

				M_GoBack(0);
				return;
			}
			else	// in a normal menu, stealthset the cvars and then go to the play menu.
			{
				for (i = 0; i < setup_numplayers; i++)
				{
					CV_StealthSet(&cv_skin[i], skins[setup_player[i].skin]->name);
					CV_StealthSetValue(&cv_playercolor[i], setup_player[i].color);

					if (horngoner)
						;
					else if (setup_player[i].followern < 0)
						CV_StealthSet(&cv_follower[i], "None");
					else
						CV_StealthSet(&cv_follower[i], followers[setup_player[i].followern].name);
					CV_StealthSetValue(&cv_followercolor[i], setup_player[i].followercolor);

					G_SetPlayerGamepadIndicatorToPlayerColor(i);
				}

				CV_StealthSetValue(&cv_splitplayers, setup_numplayers);

				#if defined (TESTERS)
					M_MPOptSelectInit(0);
				#else
					M_SetupPlayMenu(0);
				#endif

			}
		}
		else	// In a game
		{
			// 23/05/2022: Since there's already restrictskinchange, just allow this to happen regardless.
			M_MPConfirmCharacterSelection();
		}
	}

	if (optionsmenu.profile)
		M_OptionsTick();
}

boolean M_CharacterSelectQuit(void)
{
	return true;
}

void Splitplayers_OnChange(void);
void Splitplayers_OnChange(void)
{
#if 0
	if (cv_splitplayers.value < setupm_pselect)
		setupm_pselect = 1;
#endif
}
