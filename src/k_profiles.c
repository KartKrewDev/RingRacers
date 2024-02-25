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
#include "monocypher/monocypher.h"
#include "stun.h"
#include "k_color.h"
#include "command.h"

CV_PossibleValue_t lastprofile_cons_t[] = {{-1, "MIN"}, {MAXPROFILES, "MAX"}, {0, NULL}};

// List of all the profiles.
static profile_t *profilesList[MAXPROFILES+1]; // +1 because we're gonna add a default "GUEST' profile.
static UINT8 numprofiles = 0; // # of loaded profiles

INT32 PR_GetNumProfiles(void)
{
	return numprofiles;
}

static void PR_GenerateProfileKeys(profile_t *new)
{
	static uint8_t seed[32];
	csprng(seed, 32);
	crypto_eddsa_key_pair(new->secret_key, new->public_key, seed);
}

profile_t* PR_MakeProfile(
	const char *prname,
	const char *pname,
	const char *sname, const UINT16 col,
	const char *fname, const UINT16 fcol,
	INT32 controlarray[num_gamecontrols][MAXINPUTMAPPING],
	boolean guest)
{
	profile_t *new = Z_Calloc(sizeof(profile_t), PU_STATIC, NULL);

	new->version = PROFILEVER;

	memset(new->secret_key, 0, sizeof(new->secret_key));
	memset(new->public_key, 0, sizeof(new->public_key));

	if (!guest)
	{
		PR_GenerateProfileKeys(new);
	}

	strcpy(new->profilename, prname);
	new->profilename[sizeof new->profilename - 1] = '\0';

	strcpy(new->skinname, sname);
	strcpy(new->playername, pname);
	new->color = col;

	strcpy(new->follower, fname);
	new->followercolor = fcol;
	new->kickstartaccel = false;
	new->autoroulette = false;
	new->litesteer = true;
	new->rumble = true;

	// Copy from gamecontrol directly as we'll be setting controls up directly in the profile.
	memcpy(new->controls, controlarray, sizeof(new->controls));

	new->wins = 0;

	return new;
}

profile_t* PR_MakeProfileFromPlayer(const char *prname, const char *pname, const char *sname, const UINT16 col, const char *fname, UINT16 fcol, UINT8 pnum)
{
	// Generate profile using the player's gamecontrol, as we set them directly when making profiles from menus.
	profile_t *new = PR_MakeProfile(prname, pname, sname, col, fname, fcol, gamecontrol[pnum], false);

	// Player bound cvars:
	new->kickstartaccel = cv_kickstartaccel[pnum].value;
	new->autoroulette = cv_autoroulette[pnum].value;
	new->litesteer = cv_litesteer[pnum].value;
	new->rumble = cv_rumble[pnum].value;

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
	savebuffer_t save = {0};

	if (profilesList[PROFILE_GUEST] == NULL)
	{
		// Profiles have not been loaded yet, don't overwrite with garbage.
		return;
	}

	if (P_SaveBufferAlloc(&save, sizeof(UINT32) + (numprofiles * sizeof(profile_t))) == false)
	{
		I_Error("No more free memory for saving profiles\n");
		return;
	}

	// Add header.
	WRITESTRINGN(save.p, PROFILEHEADER, headerlen);
	WRITEUINT8(save.p, PROFILEVER);
	WRITEUINT8(save.p, numprofiles);

	for (i = 1; i < numprofiles; i++)
	{
		// Names and keys, all the string data up front
		WRITESTRINGN(save.p, profilesList[i]->profilename, PROFILENAMELEN);
		WRITEMEM(save.p, profilesList[i]->public_key, sizeof(((profile_t *)0)->public_key));
		WRITEMEM(save.p, profilesList[i]->secret_key, sizeof(((profile_t *)0)->secret_key));
		WRITESTRINGN(save.p, profilesList[i]->playername, MAXPLAYERNAME);

		// Character and colour.
		WRITESTRINGN(save.p, profilesList[i]->skinname, SKINNAMESIZE);
		WRITEUINT16(save.p, profilesList[i]->color);

		// Follower and colour.
		WRITESTRINGN(save.p, profilesList[i]->follower, SKINNAMESIZE);
		WRITEUINT16(save.p, profilesList[i]->followercolor);

		WRITEUINT32(save.p, profilesList[i]->wins);

		// Consvars.
		WRITEUINT8(save.p, profilesList[i]->kickstartaccel);
		WRITEUINT8(save.p, profilesList[i]->autoroulette);
		WRITEUINT8(save.p, profilesList[i]->litesteer);
		WRITEUINT8(save.p, profilesList[i]->rumble);

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
		P_SaveBufferFree(&save);
		I_Error("Couldn't save profiles. Are you out of Disk space / playing in a protected folder?");
	}
	P_SaveBufferFree(&save);
}

void PR_LoadProfiles(void)
{
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
	savebuffer_t save = {0};

	if (P_SaveBufferFromFile(&save, va(pandf, srb2home, PROFILESFILE)) == false)
	{
		// No profiles. Add the default one.
		PR_AddProfile(dprofile);
		return;
	}

	if (strncmp(PROFILEHEADER, (const char *)save.buffer, headerlen))
	{
		const char *gdfolder = "the Ring Racers folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		P_SaveBufferFree(&save);
		I_Error("Not a valid Profile file.\nDelete %s (maybe in %s) and try again.", PROFILESFILE, gdfolder);
	}
	save.p += headerlen;

	version = READUINT8(save.p);
	if (version > PROFILEVER)
	{
		P_SaveBufferFree(&save);
		I_Error("Existing %s is from the future! (expected %d, got %d)", PROFILESFILE, PROFILEVER, version);
	}
	else if (version < PROFILEVER)
	{
		// We're converting - let'd create a backup.
		FIL_WriteFile(va("%s" PATHSEP "%s.bak", srb2home, PROFILESFILE), save.buffer, save.size);
	}

	numprofiles = READUINT8(save.p);
	if (numprofiles > MAXPROFILES)
		numprofiles = MAXPROFILES;

	for (i = 1; i < numprofiles; i++)
	{
		profilesList[i] = Z_Calloc(sizeof(profile_t), PU_STATIC, NULL);

		// Version. (We always update this on successful forward step)
		profilesList[i]->version = PROFILEVER;

		// Names and keys, all the identity stuff up front
		READSTRINGN(save.p, profilesList[i]->profilename, PROFILENAMELEN);

		// Profile update 2-->3: Add profile keys.
		if (version < 3)
		{
			// Generate missing keys.
			PR_GenerateProfileKeys(profilesList[i]);
		}
		else
		{
			READMEM(save.p, profilesList[i]->public_key, sizeof(((profile_t *)0)->public_key));
			READMEM(save.p, profilesList[i]->secret_key, sizeof(((profile_t *)0)->secret_key));
		}

		READSTRINGN(save.p, profilesList[i]->playername, MAXPLAYERNAME);

		// Character and colour.
		READSTRINGN(save.p, profilesList[i]->skinname, SKINNAMESIZE);
		profilesList[i]->color = READUINT16(save.p);

		if (profilesList[i]->color == SKINCOLOR_NONE)
		{
			; // Valid, even outside the bounds
		}
		else if (profilesList[i]->color >= numskincolors
			|| K_ColorUsable(profilesList[i]->color, false, false) == false)
		{
			profilesList[i]->color = PROFILEDEFAULTCOLOR;
		}

		// Follower and colour.
		READSTRINGN(save.p, profilesList[i]->follower, SKINNAMESIZE);
		profilesList[i]->followercolor = READUINT16(save.p);

		if (profilesList[i]->followercolor == FOLLOWERCOLOR_MATCH
			|| profilesList[i]->followercolor == FOLLOWERCOLOR_OPPOSITE
			|| profilesList[i]->followercolor == SKINCOLOR_NONE)
		{
			; // Valid, even outside the bounds
		}
		else if (profilesList[i]->followercolor >= numskincolors
			|| K_ColorUsable(profilesList[i]->followercolor, true, false) == false)
		{
			profilesList[i]->followercolor = PROFILEDEFAULTFOLLOWERCOLOR;
		}

		// Profile update 5-->6: PWR isn't in profile data anymore.
		if (version < 6)
		{
			save.p += PWRLV_NUMTYPES*2;
			profilesList[i]->wins = 0;
		}
		else
		{
			profilesList[i]->wins = READUINT32(save.p);
		}

		// Consvars.
		profilesList[i]->kickstartaccel = (boolean)READUINT8(save.p);

		// 6->7, add autoroulette
		if (version < 7)
		{
			profilesList[i]->autoroulette = false;
			
		}
		else
		{
			profilesList[i]->autoroulette = (boolean)READUINT8(save.p);
		}

		// 7->8, add litesteer
		if (version < 8)
		{
			profilesList[i]->litesteer = true;
			
		}
		else
		{
			profilesList[i]->litesteer = (boolean)READUINT8(save.p);
		}

		if (version < 4)
		{
			profilesList[i]->rumble = true;
		}
		else
		{
			profilesList[i]->rumble = (boolean)READUINT8(save.p);
		}

		// Controls.
		for (j = 0; j < num_gamecontrols; j++)
		{
#ifdef DEVELOP
			// Profile update 1-->2: Add gc_rankings.
			// Profile update 4-->5: Add gc_startlossless.
			if ((j == gc_rankings && version < 2) ||
				(j == gc_startlossless && version < 5))
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

static void PR_ApplyProfile_Appearance(profile_t *p, UINT8 playernum)
{
	CV_StealthSet(&cv_skin[playernum], p->skinname);
	CV_StealthSetValue(&cv_playercolor[playernum], p->color);
	CV_StealthSet(&cv_playername[playernum], p->playername);

	// Followers
	CV_StealthSet(&cv_follower[playernum], p->follower);
	CV_StealthSetValue(&cv_followercolor[playernum], p->followercolor);
}

static void PR_ApplyProfile_Settings(profile_t *p, UINT8 playernum)
{
	// toggles
	CV_StealthSetValue(&cv_kickstartaccel[playernum], p->kickstartaccel);
	CV_StealthSetValue(&cv_autoroulette[playernum], p->autoroulette);
	CV_StealthSetValue(&cv_litesteer[playernum], p->litesteer);
	CV_StealthSetValue(&cv_rumble[playernum], p->rumble);

	// set controls...
	G_ApplyControlScheme(playernum, p->controls);
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

profile_t *PR_GetLocalPlayerProfile(INT32 player)
{
	if (player >= MAXSPLITSCREENPLAYERS)
		return NULL;
	return PR_GetProfile(cv_lastprofile[player].value);
}

boolean PR_IsLocalPlayerGuest(INT32 player)
{
	return !(cv_lastprofile[player].value);
}

static char rrid_buf[256];

char *GetPrettyRRID(const unsigned char *bin, boolean brief)
{
	size_t i;
	size_t len = PUBKEYLENGTH;

	if (brief)
		len = 8;

	if (bin == NULL || len == 0)
		return NULL;

	for (i=0; i<len; i++)
	{
		rrid_buf[i*2]   = "0123456789ABCDEF"[bin[i] >> 4];
		rrid_buf[i*2+1] = "0123456789ABCDEF"[bin[i] & 0x0F];
	}
	
	rrid_buf[len*2] = '\0';

	return rrid_buf;
}

unsigned char *FromPrettyRRID(unsigned char *bin, const char *text)
{
	size_t i;
	size_t len = PUBKEYLENGTH * 2;

	if (strlen(text) != len)
		return NULL;

	for (i = 0; i < len; i += 2)
	{
		char byt[3] = { text[i], text[i+1], '\0' };
		char *p;

		bin[i/2] = strtol(byt, &p, 16);

		if (*p) // input is not hexadecimal
			return NULL;
	}

	return bin;
}


static char allZero[PUBKEYLENGTH];

boolean PR_IsKeyGuest(uint8_t *key)
{
	//memset(allZero, 0, PUBKEYLENGTH); -- not required, allZero is 0's
	return (memcmp(key, allZero, PUBKEYLENGTH) == 0);
}
