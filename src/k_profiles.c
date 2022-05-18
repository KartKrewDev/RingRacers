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

#include "d_main.h" // pandf
#include "k_profiles.h"
#include "z_zone.h"

// List of all the profiles.
static profile_t *profilesList[MAXPROFILES+1];		// +1 because we're gonna add a default "GUEST' profile.
static UINT8 numprofiles = 0;	// # of loaded profiles

INT32 PR_GetNumProfiles(void)
{
	return numprofiles;
}

profile_t* PR_MakeProfile(const char *prname, const char *pname, const char *sname, const UINT16 col, const char *fname, UINT16 fcol, INT32 controlarray[num_gamecontrols][MAXINPUTMAPPING])
{
	profile_t *new = Z_Malloc(sizeof(profile_t), PU_STATIC, NULL);

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

	return new;
}

profile_t* PR_MakeProfileFromPlayer(const char *prname, const char *pname, const char *sname, const UINT16 col, const char *fname, UINT16 fcol, UINT8 pnum)
{
	// Generate profile using the player's gamecontrol, as we set them directly when making profiles from menus.
	profile_t *new = PR_MakeProfile(prname, pname, sname, col, fname, fcol, gamecontrol[pnum]);

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
	if (num <= 0 || num > numprofiles)
		return false;

	// If we're deleting inbetween profiles, move everything.
	if (num < numprofiles)
		for (i = num; i < numprofiles-1; i++)
			profilesList[i] = profilesList[i+1];

	// In any case, delete the last profile as well.
	profilesList[numprofiles] = NULL;
	numprofiles--;

	PR_SaveProfiles();
	return true;
}

void PR_InitNewProfile(void)
{
	char pname[PROFILENAMELEN+1] = "PRF";
	profile_t *dprofile;

	strcpy(pname, va("PRF%c", 'A'+numprofiles-1));

	dprofile = PR_MakeProfile(pname, PROFILEDEFAULTPNAME, PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR, PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR, gamecontroldefault);
	PR_AddProfile(dprofile);
}

void PR_SaveProfiles(void)
{
	FILE *f = NULL;

	f = fopen(va(pandf, srb2home, PROFILESFILE), "w");
	if (f != NULL)
	{
		UINT8 i;

		fwrite(&numprofiles, sizeof numprofiles, 1, f);

		for (i = 1; i < numprofiles; ++i)
		{
			fwrite(profilesList[i], sizeof(profile_t), 1, f);
		}

		fclose(f);
	}
	else
		I_Error("Couldn't save profiles. Are you out of Disk space / playing in a protected folder?");
}

void PR_LoadProfiles(void)
{
	FILE *f = NULL;
	profile_t *dprofile = PR_MakeProfile(PROFILEDEFAULTNAME, PROFILEDEFAULTPNAME, PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR, PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR, gamecontroldefault);
	f = fopen(va(pandf, srb2home, PROFILESFILE), "r");

	if (f != NULL)
	{
		INT32 i;

		fread(&numprofiles, sizeof numprofiles, 1, f);

		for (i = 1; i < numprofiles; ++i)
		{
			profilesList[i] = Z_Malloc(sizeof(profile_t), PU_STATIC, NULL);
			fread(profilesList[i], sizeof(profile_t), 1, f);
		}

		fclose(f);

		// Overwrite the first profile for the default profile to avoid letting anyone tamper with it.
		profilesList[0] = dprofile;
	}
	else
	{
		// No profiles. Add the default one.
		PR_AddProfile(dprofile);
	}
}

void PR_ApplyProfile(UINT8 profilenum, UINT8 playernum)
{
	profile_t *p = PR_GetProfile(profilenum);

	CV_StealthSet(&cv_skin[playernum], p->skinname);
	CV_StealthSetValue(&cv_playercolor[playernum], p->color);
	CV_StealthSet(&cv_playername[playernum], p->playername);

	// Followers
	CV_StealthSet(&cv_follower[playernum], p->follower);
	CV_StealthSetValue(&cv_followercolor[playernum], p->followercolor);

	// toggles
	CV_StealthSetValue(&cv_kickstartaccel[playernum], p->kickstartaccel);

	// set controls...
	memcpy(&gamecontrol[playernum], p->controls, sizeof(gamecontroldefault));

	// set memory cvar
	CV_StealthSetValue(&cv_lastprofile[playernum], profilenum);
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