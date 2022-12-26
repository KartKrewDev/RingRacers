// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_profiles.c
/// \brief implements methods for profiles etc.

#include "doomtype.h"
#include "d_main.h" // pandf
#include "byteptr.h" // READ/WRITE macros
#include "p_saveg.h" // savebuffer_t
#include "m_misc.h" //FIL_WriteFile()
#include "k_profiles.h"
#include "z_zone.h"
#include "r_skins.h"

// List of all the profiles.
static profile_t *profilesList[MAXPROFILES+1]; // +1 because we're gonna add a default "GUEST' profile.
static UINT8 numprofiles = 0; // # of loaded profiles

INT32 PR_GetNumProfiles(void)
{
	return numprofiles;
}

profile_t* PR_MakeProfile(
	const char *prname,
	const char *pname,
	const char *sname, const UINT16 col,
	const char *fname, const UINT16 fcol,
	INT32 controlarray[num_gamecontrols][MAXINPUTMAPPING],
	boolean guest)
{
	profile_t *new = Z_Malloc(sizeof(profile_t), PU_STATIC, NULL);
	UINT8 i;

	new->version = PROFILEVER;

	strcpy(new->profilename, prname);
	new->profilename[sizeof new->profilename - 1] = '\0';

	strcpy(new->skinname, sname);
	strcpy(new->playername, pname);
	new->color = col;

	strcpy(new->follower, fname);
	new->followercolor = fcol;
	new->kickstartaccel = false;

	// Copy from gamecontrol directly as we'll be setting controls up directly in the profile.
	memcpy(new->controls, controlarray, sizeof(new->controls));

	// Init both power levels
	for (i = 0; i < PWRLV_NUMTYPES; i++)
	{
		new->powerlevels[i] = (guest ? 0 : PWRLVRECORD_START);
	}

	return new;
}

profile_t* PR_MakeProfileFromPlayer(const char *prname, const char *pname, const char *sname, const UINT16 col, const char *fname, UINT16 fcol, UINT8 pnum)
{
	// Generate profile using the player's gamecontrol, as we set them directly when making profiles from menus.
	profile_t *new = PR_MakeProfile(prname, pname, sname, col, fname, fcol, gamecontrol[pnum], false);

	// Player bound cvars:
	new->kickstartaccel = cv_kickstartaccel[pnum].value;

	return new;
}

boolean PR_AddProfile(profile_t *p)
{
	if (numprofiles < MAXPROFILES+1)
	{
		profilesList[numprofiles] = p;
		numprofiles++;

		CONS_Printf("Profile '%s' added\n", p->profilename);

		return true;
	}
	else
		return false;
}

profile_t* PR_GetProfile(INT32 num)
{
	if (num < numprofiles)
		return profilesList[num];
	else
		return NULL;
}

boolean PR_DeleteProfile(INT32 num)
{
	UINT8 i;
	profile_t* sacrifice;

	if (num <= 0 || num > numprofiles)
	{
		return false;
	}

	sacrifice = profilesList[num];

	// If we're deleting inbetween profiles, move everything.
	if (num < numprofiles)
	{
		for (i = num; i < numprofiles-1; i++)
		{
			profilesList[i] = profilesList[i+1];
		}

		// Make sure to move cv_lastprofile (and title/current profile) values as well!
		for (i = 0; i < MAXSPLITSCREENPLAYERS+2; i++)
		{
			consvar_t *cv;

			if (i < MAXSPLITSCREENPLAYERS)
				cv = &cv_lastprofile[i];
			else if (i == MAXSPLITSCREENPLAYERS)
				cv = &cv_ttlprofilen;
			else
				cv = &cv_currprofile;

			if (cv->value < num)
			{
				// Not affected.
				continue;
			}

			if (cv->value > num)
			{
				// Shift our lastprofile number down to match the new order.
				CV_StealthSetValue(cv, cv->value-1);
				continue;
			}

			if (cv != &cv_currprofile)
			{
				// There's no hope for it. If we were on the deleted profile, default back to guest.
				CV_StealthSetValue(cv, PROFILE_GUEST);
				continue;
			}

			// Oh boy, now we're really in for it.
			CV_StealthSetValue(cv, -1);
		}
	}

	// In any case, delete the last profile as well.
	profilesList[numprofiles] = NULL;
	numprofiles--;

	PR_SaveProfiles();

	// Finally, clear up our memory!
	Z_Free(sacrifice);

	return true;
}

void PR_InitNewProfile(void)
{
	char pname[PROFILENAMELEN+1] = "PRF";
	profile_t *dprofile;
	UINT8 usenum = numprofiles-1;
	UINT8 i;
	boolean nameok = false;

	pname[4] = '\0';

	// When deleting profile, it's possible to do some pretty wacko stuff that would lead a new fresh profile to share the same name as another profile we have never changed the name of.
	// This could become an infinite loop if MAXPROFILES >= 26.
	while (!nameok)
	{
		pname[3] = 'A'+usenum;

		for (i = 0; i < numprofiles; i++)
		{
			profile_t *pr = PR_GetProfile(i);
			if (!strcmp(pr->profilename, pname))
			{
				usenum++;
				if (pname[3] == 'Z')
					usenum = 0;

				break;
			}

			// if we got here, then it means the name is okay!
			if (i == numprofiles-1)
				nameok = true;
		}
	}

	dprofile = PR_MakeProfile(
		pname,
		PROFILEDEFAULTPNAME,
		PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR,
		PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR,
		gamecontroldefault,
		false
	);
	PR_AddProfile(dprofile);
}

void PR_SaveProfiles(void)
{
	size_t length = 0;
	const size_t headerlen = strlen(PROFILEHEADER);
	UINT8 i, j, k;
	savebuffer_t save;

	save.size = sizeof(UINT32) + (numprofiles * sizeof(profile_t));
	save.p = save.buffer = (UINT8 *)malloc(save.size);
	if (!save.p)
	{
		I_Error("No more free memory for saving profiles\n");
		return;
	}
	save.end = save.buffer + save.size;

	// Add header.
	WRITESTRINGN(save.p, PROFILEHEADER, headerlen);
	WRITEUINT8(save.p, PROFILEVER);
	WRITEUINT8(save.p, numprofiles);

	for (i = 1; i < numprofiles; i++)
	{
		// Names.
		WRITESTRINGN(save.p, profilesList[i]->profilename, PROFILENAMELEN);
		WRITESTRINGN(save.p, profilesList[i]->playername, MAXPLAYERNAME);

		// Character and colour.
		WRITESTRINGN(save.p, profilesList[i]->skinname, SKINNAMESIZE);
		WRITEUINT16(save.p, profilesList[i]->color);

		// Follower and colour.
		WRITESTRINGN(save.p, profilesList[i]->follower, SKINNAMESIZE);
		WRITEUINT16(save.p, profilesList[i]->followercolor);

		// PWR.
		for (j = 0; j < PWRLV_NUMTYPES; j++)
		{
			WRITEUINT16(save.p, profilesList[i]->powerlevels[j]);
		}

		// Consvars.
		WRITEUINT8(save.p, profilesList[i]->kickstartaccel);

		// Controls.
		for (j = 0; j < num_gamecontrols; j++)
		{
			for (k = 0; k < MAXINPUTMAPPING; k++)
			{
				WRITEINT32(save.p, profilesList[i]->controls[j][k]);
			}
		}
	}

	length = save.p - save.buffer;

	if (!FIL_WriteFile(va(pandf, srb2home, PROFILESFILE), save.buffer, length))
	{
		free(save.buffer);
		I_Error("Couldn't save profiles. Are you out of Disk space / playing in a protected folder?");
	}
	free(save.buffer);
}

void PR_LoadProfiles(void)
{
	size_t length = 0;
	const size_t headerlen = strlen(PROFILEHEADER);
	UINT8 i, j, k, version;
	profile_t *dprofile = PR_MakeProfile(
		PROFILEDEFAULTNAME,
		PROFILEDEFAULTPNAME,
		PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR,
		PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR,
		gamecontroldefault,
		true
	);
	savebuffer_t save;

	length = FIL_ReadFile(va(pandf, srb2home, PROFILESFILE), &save.buffer);
	if (!length)
	{
		// No profiles. Add the default one.
		PR_AddProfile(dprofile);
		return;
	}

	save.p = save.buffer;

	if (strncmp(PROFILEHEADER, (const char *)save.buffer, headerlen))
	{
		const char *gdfolder = "the Ring Racers folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		Z_Free(save.buffer);
		save.p = NULL;
		I_Error("Not a valid Profile file.\nDelete %s (maybe in %s) and try again.", PROFILESFILE, gdfolder);
	}
	save.p += headerlen;

	version = READUINT8(save.p);
	if (version > PROFILEVER)
	{
		Z_Free(save.buffer);
		save.p = NULL;
		I_Error("Existing %s is from the future! (expected %d, got %d)", PROFILESFILE, PROFILEVER, version);
	}

	numprofiles = READUINT8(save.p);
	if (numprofiles > MAXPROFILES)
		numprofiles = MAXPROFILES;

	for (i = 1; i < numprofiles; i++)
	{
		profilesList[i] = Z_Malloc(sizeof(profile_t), PU_STATIC, NULL);

		// Version. (We always update this on successful forward step)
		profilesList[i]->version = PROFILEVER;

		// Names.
		READSTRINGN(save.p, profilesList[i]->profilename, PROFILENAMELEN);
		READSTRINGN(save.p, profilesList[i]->playername, MAXPLAYERNAME);

		// Character and colour.
		READSTRINGN(save.p, profilesList[i]->skinname, SKINNAMESIZE);
		profilesList[i]->color = READUINT16(save.p);

		if (profilesList[i]->color == SKINCOLOR_NONE)
		{
			; // Valid, even outside the bounds
		}
		else if (profilesList[i]->color >= numskincolors
			|| skincolors[profilesList[i]->color].accessible == false)
		{
			profilesList[i]->color = PROFILEDEFAULTCOLOR;
		}

		// Follower and colour.
		READSTRINGN(save.p, profilesList[i]->follower, SKINNAMESIZE);
		profilesList[i]->followercolor = READUINT16(save.p);

		if (profilesList[i]->followercolor == FOLLOWERCOLOR_MATCH
			|| profilesList[i]->followercolor == FOLLOWERCOLOR_OPPOSITE)
		{
			; // Valid, even outside the bounds
		}
		else if (profilesList[i]->followercolor >= numskincolors
			|| profilesList[i]->followercolor == SKINCOLOR_NONE
			|| skincolors[profilesList[i]->followercolor].accessible == false)
		{
			profilesList[i]->followercolor = PROFILEDEFAULTFOLLOWERCOLOR;
		}

		// PWR.
		for (j = 0; j < PWRLV_NUMTYPES; j++)
		{
			profilesList[i]->powerlevels[j] = READUINT16(save.p);
			if (profilesList[i]->powerlevels[j] < PWRLVRECORD_MIN
				|| profilesList[i]->powerlevels[j] > PWRLVRECORD_MAX)
			{
				// invalid, reset
				profilesList[i]->powerlevels[j] = PWRLVRECORD_START;
			}
		}

		// Consvars.
		profilesList[i]->kickstartaccel = (boolean)READUINT8(save.p);

		// Controls.
		for (j = 0; j < num_gamecontrols; j++)
		{
#ifdef DEVELOP
			// Profile update 1-->2: Add gc_rankings.
			if (j == gc_rankings && version < 2)
			{
				for (k = 0; k < MAXINPUTMAPPING; k++)
				{
					profilesList[i]->controls[j][k] = gamecontroldefault[j][k];
				}
				continue;
			}
#endif

			for (k = 0; k < MAXINPUTMAPPING; k++)
			{
				profilesList[i]->controls[j][k] = READINT32(save.p);
			}
		}
	}

	// Add the the default profile directly to avoid letting anyone tamper with it.
	profilesList[PROFILE_GUEST] = dprofile;
}

skincolornum_t PR_GetProfileColor(profile_t *p)
{
	if (p->color == SKINCOLOR_NONE)
	{
		// Get skin's prefcolor.
		INT32 foundskin = R_SkinAvailable(p->skinname);
		if (foundskin == -1)
		{
			// Return random default value
			return SKINCOLOR_RED;
		}

		return skins[foundskin].prefcolor;
	}

	// Get exact color.
	return p->color;
}

static void PR_ApplyProfile_Appearance(profile_t *p, UINT8 playernum)
{
	CV_StealthSet(&cv_skin[playernum], p->skinname);
	CV_StealthSetValue(&cv_playercolor[playernum], PR_GetProfileColor(p));
	CV_StealthSet(&cv_playername[playernum], p->playername);

	// Followers
	CV_StealthSet(&cv_follower[playernum], p->follower);
	CV_StealthSetValue(&cv_followercolor[playernum], p->followercolor);
}

static void PR_ApplyProfile_Settings(profile_t *p, UINT8 playernum)
{
	// toggles
	CV_StealthSetValue(&cv_kickstartaccel[playernum], p->kickstartaccel);

	// set controls...
	memcpy(&gamecontrol[playernum], p->controls, sizeof(gamecontroldefault));
}

static void PR_ApplyProfile_Memory(UINT8 profilenum, UINT8 playernum)
{
	// set memory cvar
	CV_StealthSetValue(&cv_lastprofile[playernum], profilenum);

	// If we're doing this on P1, also change current profile.
	if (playernum == 0)
	{
		CV_StealthSetValue(&cv_currprofile, profilenum);
	}
}

void PR_ApplyProfile(UINT8 profilenum, UINT8 playernum)
{
	profile_t *p = PR_GetProfile(profilenum);

	// this CAN happen!!
	if (dedicated || p == NULL)
	{
		if (!dedicated)
			CONS_Printf("Profile '%d' could not be loaded as it does not exist. Guest Profile will be loaded instead.\n", profilenum);
		profilenum = 0; // make sure to set this so that the cvar is set properly.
		p = PR_GetProfile(profilenum);
	}

	PR_ApplyProfile_Appearance(p, playernum);
	PR_ApplyProfile_Settings(p, playernum);
	PR_ApplyProfile_Memory(profilenum, playernum);
}

void PR_ApplyProfileLight(UINT8 profilenum, UINT8 playernum)
{
	profile_t *p = PR_GetProfile(profilenum);

	// this CAN happen!!
	if (p == NULL)
	{
		// no need to be as loud...
		profilenum = 0; // make sure to set this so that the cvar is set properly.
		p = PR_GetProfile(profilenum);
	}

	PR_ApplyProfile_Appearance(p, playernum);
}

void PR_ApplyProfilePretend(UINT8 profilenum, UINT8 playernum)
{
	profile_t *p = PR_GetProfile(profilenum);

	// this CAN happen!!
	if (dedicated || p == NULL)
	{
		if (!dedicated)
			CONS_Printf("Profile '%d' could not be loaded as it does not exist. Guest Profile will be loaded instead.\n", profilenum);
		profilenum = 0; // make sure to set this so that the cvar is set properly.
		p = PR_GetProfile(profilenum);
	}

	PR_ApplyProfile_Memory(profilenum, playernum);
}

UINT8 PR_GetProfileNum(profile_t *p)
{
	UINT8 i;
	for (i = 0; i < MAXPROFILES+1; i++)
	{
		profile_t *comp = PR_GetProfile(i);
		if (comp == p)
			return i;
	}
	return 0;
}

SINT8 PR_ProfileUsedBy(profile_t *p)
{
	UINT8 i;
	UINT8 prn = PR_GetProfileNum(p);

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (prn == cv_lastprofile[i].value)
			return i;
	}

	return -1;
}

profile_t *PR_GetPlayerProfile(player_t *player)
{
	const UINT8 playerNum = (player - players);
	UINT8 i;

	if (demo.playback)
	{
		return NULL;
	}

	for (i = 0; i <= splitscreen; i++)
	{
		if (playerNum == g_localplayers[i])
		{
			return PR_GetProfile(cv_lastprofile[i].value);
		}
	}

	return NULL;
}
