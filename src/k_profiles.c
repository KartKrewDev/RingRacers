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

profile_t PR_MakeProfile(const char *prname, const char *pname, const UINT16 col, const char *fname, UINT16 fcol, INT32 controlarray[num_gamecontrols][MAXINPUTMAPPING])
{
	profile_t new;
	
	new.version = PROFILEVER;
	
	strcpy(new.profilename, prname);
	
	strcpy(new.playername, pname);
	new.color = col;
	
	strcpy(new.follower, fname);
	new.followercolor = fcol;
	
	// Copy from gamecontrol directly as we'll be setting controls up directly in the profile.
	memcpy(new.controls, controlarray, sizeof(new.controls));	
	
	return new;
}

profile_t PR_MakeProfileFromPlayer(const char *prname, const char *pname, const UINT16 col, const char *fname, UINT16 fcol, UINT8 pnum)
{
	// Generate profile using the player's gamecontrol, as we set them directly when making profiles from menus.
	profile_t new = PR_MakeProfile(prname, pname, col, fname, fcol, gamecontrol[pnum]);
	
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

void PR_SaveProfiles(void)
{
	FILE *f = NULL;
	
	f = fopen(PROFILESFILE, "w");
	if (f != NULL)
	{
		fwrite(profilesList, sizeof(profile_t), MAXPROFILES, f);
		fclose(f);
	}
}	

void PR_LoadProfiles(void)
{
	//FILE *f = NULL;
	profile_t dprofile = PR_MakeProfile(PROFILEDEFAULTNAME, PROFILEDEFAULTPNAME, PROFILEDEFAULTCOLOR, PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR, gamecontroldefault);
	PR_AddProfile(dprofile);
	
	/*f = fopen(PROFILESFILE, "r");
	
	if (f != NULL)
	{
		fread(&profilesList[1], sizeof(profile_t)*(MAXPROFILES), MAXPROFILES, f);
		fclose(f);
	}*/
}