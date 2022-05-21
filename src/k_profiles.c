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
		new->powerlevels[i] = PWRLVRECORD_START;

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
	UINT8 i, j;
	if (num <= 0 || num > numprofiles)
		return false;

	// If we're deleting inbetween profiles, move everything.
	if (num < numprofiles)
	{
		for (i = num; i < numprofiles-1; i++)
		{
			profilesList[i] = profilesList[i+1];

			// Make sure to move cv_lastprofile values as well
			for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
			{
				if (cv_lastprofile[j].value == num)
					CV_StealthSetValue(&cv_lastprofile[j], 0);	// If we were on the deleted profile, default back to guest.

				else if (cv_lastprofile[j].value == i+1)				// Otherwise, shift our lastprofile number down to match the new order.
					CV_StealthSetValue(&cv_lastprofile[j], cv_lastprofile[j].value-1);
			}
		}
	}

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
	UINT8 usenum = numprofiles-1;
	UINT8 i;
	boolean nameok = false;

	// When deleting profile, it's possible to do some pretty wacko stuff that would lead a new fresh profile to share the same name as another profile we have never changed the name of.
	while (!nameok)
	{
		strcpy(pname, va("PRF%c", 'A'+usenum-1));

		for (i = 0; i < numprofiles; i++)
		{
			profile_t *pr = PR_GetProfile(i);
			if (!strcmp(pr->profilename, pname))
			{
				usenum++;
				if (usenum > 'Z' -1)
					usenum = 'A';

				break;
			}

			// if we got here, then it means the name is okay!
			if (i == numprofiles-1)
				nameok = true;
		}
	}

	dprofile = PR_MakeProfile(pname, PROFILEDEFAULTPNAME, PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR, PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR, gamecontroldefault);
	PR_AddProfile(dprofile);
}

void PR_SaveProfiles(void)
{
	FILE *f = NULL;

	// save powerlevel in the current profile.
	// granted we're using a profile that isn't guest, that is.
	if (cv_currprofile.value > 0)
	{
		profile_t *pr = PR_GetProfile(cv_currprofile.value);
		memcpy(&pr->powerlevels, vspowerlevel, sizeof(vspowerlevel));
	}

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

	// this CAN happen!!
	if (p == NULL)
	{
		CONS_Printf("Profile '%d' could not be loaded as it does not exist. Guest Profile will be loaded instead.\n", profilenum);
		profilenum = 0;			// make sure to set this so that the cvar is set properly.
		p = PR_GetProfile(0);	// Use guest profile instead if things went south somehow.
	}

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

	// If we're doing this on P1, also change current profile.
	// and update the powerlevel local array.

	if (!playernum)
	{
		CV_StealthSetValue(&cv_currprofile, profilenum);
		memcpy(&vspowerlevel, p->powerlevels, sizeof(p->powerlevels));
	}
}

void PR_ApplyProfileLight(UINT8 profilenum, UINT8 playernum)
{
	profile_t *p = PR_GetProfile(profilenum);

	CV_StealthSet(&cv_skin[playernum], p->skinname);
	CV_StealthSetValue(&cv_playercolor[playernum], p->color);
	CV_StealthSet(&cv_playername[playernum], p->playername);

	// Followers
	CV_StealthSet(&cv_follower[playernum], p->follower);
	CV_StealthSetValue(&cv_followercolor[playernum], p->followercolor);
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

	for (i=0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (prn == cv_lastprofile[i].value)
			return i;
	}

	return -1;
}