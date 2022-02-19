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

#include "k_profiles.h"

// List of all the profiles.
static profile_t profilesList[MAXPROFILES+1];		// +1 because we're gonna add a default "GUEST' profile.
static UINT8 numprofiles = 0;	// # of loaded profiles

INT32 PR_GetNumProfiles(void)
{
	return numprofiles;
}

profile_t PR_MakeProfile(const char *prname, const char *pname, const char *sname, const UINT16 col, const char *fname, UINT16 fcol, INT32 controlarray[num_gamecontrols][MAXINPUTMAPPING])
{
	profile_t new;

	new.version = PROFILEVER;

	strcpy(new.profilename, prname);

	strcpy(new.skinname, sname);
	strcpy(new.playername, pname);
	new.color = col;

	strcpy(new.follower, fname);
	new.followercolor = fcol;

	// Copy from gamecontrol directly as we'll be setting controls up directly in the profile.
	memcpy(new.controls, controlarray, sizeof(new.controls));

	return new;
}

profile_t PR_MakeProfileFromPlayer(const char *prname, const char *pname, const char *sname, const UINT16 col, const char *fname, UINT16 fcol, UINT8 pnum)
{
	// Generate profile using the player's gamecontrol, as we set them directly when making profiles from menus.
	profile_t new = PR_MakeProfile(prname, pname, sname, col, fname, fcol, gamecontrol[pnum]);

	// Player bound cvars:
	new.kickstartaccel = cv_kickstartaccel[pnum].value;

	return new;
}

boolean PR_AddProfile(profile_t p)
{
	if (numprofiles < MAXPROFILES)
	{
		memcpy(&profilesList[numprofiles], &p, sizeof(profile_t));
		numprofiles++;

		CONS_Printf("Profile '%s' added\n", p.profilename);

		return true;
	}
	else
		return false;
}

profile_t* PR_GetProfile(INT32 num)
{
	if (num < MAXPROFILES+1)
		return &profilesList[num];
	else
		return NULL;
}

void PR_InitNewProfile(void)
{
	char pname[PROFILENAMELEN+1] = "PRF";
	profile_t dprofile;

	strcpy(pname, va("PRF%c", 'A'+numprofiles-1));

	dprofile = PR_MakeProfile(pname, PROFILEDEFAULTPNAME, PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR, PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR, gamecontroldefault);
	PR_AddProfile(dprofile);
}

void PR_SaveProfiles(void)
{
	FILE *f = NULL;

	f = fopen(PROFILESFILE, "w");
	if (f != NULL)
	{
		fwrite(profilesList, sizeof(profile_t), MAXPROFILES+1, f);
		fclose(f);
	}
	else
		I_Error("Couldn't save profiles. Are you out of Disk space / playing in a protected folder?");
}

void PR_LoadProfiles(void)
{
	FILE *f = NULL;
	profile_t dprofile = PR_MakeProfile(PROFILEDEFAULTNAME, PROFILEDEFAULTPNAME, PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR, PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR, gamecontroldefault);
	f = fopen(PROFILESFILE, "r");

	if (f != NULL)
	{
		INT32 i;
		fread(profilesList, sizeof(profile_t)*(MAXPROFILES+1), MAXPROFILES+1, f);
		fclose(f);

		// Overwrite the first profile for the default profile to avoid letting anyone tamper with it.
		memcpy(&profilesList[0], &dprofile, sizeof(profile_t));

		// Omega, count how many profiles there are in the list.
		// WHY DID YOU ASK HIM TO DO THAT IT'S GOING TO TAKE FOR-EVER
		for (i=0; i < MAXPROFILES; i++)
		{
			if (!profilesList[i].version)
			{
				numprofiles = i;
				return;
			}
		}

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
	// @TODO followers

	// set controls...
	memcpy(&gamecontrol[playernum], p->controls, sizeof(gamecontroldefault));
}