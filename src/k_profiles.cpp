// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by "Lat'".
// Copyright (C) 2024 by AJ "Tyron" Martinez.
// Copyright (C) 2024 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_profiles.c
/// \brief implements methods for profiles etc.

#include <algorithm>
#include <exception>

#include <fmt/format.h>

#include "io/streams.hpp"
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

extern "C" consvar_t cv_dummyprofilefov, cv_fov[MAXSPLITSCREENPLAYERS];

CV_PossibleValue_t lastprofile_cons_t[] = {{-1, "MIN"}, {MAXPROFILES, "MAX"}, {0, NULL}};

// List of all the profiles.
static profile_t *profilesList[MAXPROFILES+1]; // +1 because we're gonna add a default "GUEST' profile.
static UINT8 numprofiles = 0; // # of loaded profiles

INT32 PR_GetNumProfiles(void)
{
	return numprofiles;
}

static void PR_GenerateProfileKeys(profile_t *newprofile)
{
	static uint8_t seed[32];
	csprng(seed, 32);
	crypto_eddsa_key_pair(newprofile->secret_key, newprofile->public_key, seed);
}

profile_t* PR_MakeProfile(
	const char *prname,
	const char *pname,
	const char *sname, const UINT16 col,
	const char *fname, const UINT16 fcol,
	INT32 controlarray[num_gamecontrols][MAXINPUTMAPPING],
	boolean guest)
{
	profile_t *newprofile = static_cast<profile_t*>(Z_Calloc(sizeof(profile_t), PU_STATIC, NULL));

	newprofile->version = PROFILEVER;

	memset(newprofile->secret_key, 0, sizeof(newprofile->secret_key));
	memset(newprofile->public_key, 0, sizeof(newprofile->public_key));

	if (!guest)
	{
		PR_GenerateProfileKeys(newprofile);
	}

	strcpy(newprofile->profilename, prname);
	newprofile->profilename[sizeof newprofile->profilename - 1] = '\0';

	strcpy(newprofile->skinname, sname);
	strcpy(newprofile->playername, pname);
	newprofile->color = col;

	strcpy(newprofile->follower, fname);
	newprofile->followercolor = fcol;
	newprofile->kickstartaccel = false;
	newprofile->autoroulette = false;
	newprofile->litesteer = false;
	newprofile->autoring = false;
	newprofile->rumble = true;
	newprofile->fov = atoi(cv_dummyprofilefov.defaultvalue);

	// Copy from gamecontrol directly as we'll be setting controls up directly in the profile.
	memcpy(newprofile->controls, controlarray, sizeof(newprofile->controls));

	newprofile->wins = 0;
	newprofile->rounds = 0;

	return newprofile;
}

profile_t* PR_MakeProfileFromPlayer(const char *prname, const char *pname, const char *sname, const UINT16 col, const char *fname, UINT16 fcol, UINT8 pnum)
{
	// Generate profile using the player's gamecontrol, as we set them directly when making profiles from menus.
	profile_t *newprofile = PR_MakeProfile(prname, pname, sname, col, fname, fcol, gamecontrol[pnum], false);

	// Player bound cvars:
	newprofile->kickstartaccel = cv_kickstartaccel[pnum].value;
	newprofile->autoroulette = cv_autoroulette[pnum].value;
	newprofile->litesteer = cv_litesteer[pnum].value;
	newprofile->autoring = cv_autoring[pnum].value;
	newprofile->rumble = cv_rumble[pnum].value;
	newprofile->fov = cv_fov[pnum].value / FRACUNIT;

	return newprofile;
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
	namespace fs = std::filesystem;
	using json = nlohmann::json;
	using namespace srb2;
	namespace io = srb2::io;

	if (profilesList[PROFILE_GUEST] == NULL)
	{
		// Profiles have not been loaded yet, don't overwrite with garbage.
		return;
	}

	ProfilesJson ng{};

	for (size_t i = 1; i < numprofiles; i++)
	{
		ProfileJson jsonprof;
		profile_t* cprof = profilesList[i];

		jsonprof.version = PROFILEVER;
		jsonprof.profilename = std::string(cprof->profilename);
		std::copy(std::begin(cprof->public_key), std::end(cprof->public_key), std::begin(jsonprof.publickey));
		std::copy(std::begin(cprof->secret_key), std::end(cprof->secret_key), std::begin(jsonprof.secretkey));
		jsonprof.playername = std::string(cprof->playername);
		jsonprof.skinname = std::string(cprof->skinname);
		jsonprof.colorname = std::string(skincolors[cprof->color].name);
		jsonprof.followername = std::string(cprof->follower);
		if (cprof->followercolor == FOLLOWERCOLOR_MATCH)
		{
			jsonprof.followercolorname = "Match";
		}
		else if (cprof->followercolor == FOLLOWERCOLOR_OPPOSITE)
		{
			jsonprof.followercolorname = "Opposite";
		}
		else if (cprof->followercolor == SKINCOLOR_NONE)
		{
			jsonprof.followercolorname = "Default";
		}
		else if (cprof->followercolor >= numskincolors)
		{
			jsonprof.followercolorname = std::string();
		}
		else
		{
			jsonprof.followercolorname = std::string(skincolors[cprof->followercolor].name);
		}
		jsonprof.records.wins = cprof->wins;
		jsonprof.records.rounds = cprof->rounds;
		jsonprof.preferences.kickstartaccel = cprof->kickstartaccel;
		jsonprof.preferences.autoroulette = cprof->autoroulette;
		jsonprof.preferences.litesteer = cprof->litesteer;
		jsonprof.preferences.autoring = cprof->autoring;
		jsonprof.preferences.rumble = cprof->rumble;
		jsonprof.preferences.fov = cprof->fov;

		for (size_t j = 0; j < num_gamecontrols; j++)
		{
			for (size_t k = 0; k < MAXINPUTMAPPING; k++)
			{
				jsonprof.controls[j][k] = cprof->controls[j][k];
			}
		}

		ng.profiles.emplace_back(std::move(jsonprof));
	}

	std::vector<uint8_t> ubjson = json::to_ubjson(ng);

	std::string realpath = fmt::format("{}/{}", srb2home, PROFILESFILE);
	std::string bakpath = fmt::format("{}.bak", realpath);

	if (fs::exists(realpath))
	{
		try
		{
			fs::rename(realpath, bakpath);
		}
		catch (const fs::filesystem_error& ex)
		{
			CONS_Alert(CONS_ERROR, "Failed to record profiles backup. Not attempting to save profiles. %s\n", ex.what());
			return;
		}
	}

	try
	{
		io::FileStream file {realpath, io::FileStreamMode::kWrite};

		io::write(static_cast<uint32_t>(0x52494E47), file, io::Endian::kBE); // "RING"
		io::write(static_cast<uint32_t>(0x5052464C), file, io::Endian::kBE); // "PRFL"
		io::write(static_cast<uint8_t>(0), file); // reserved1
		io::write(static_cast<uint8_t>(0), file); // reserved2
		io::write(static_cast<uint8_t>(0), file); // reserved3
		io::write(static_cast<uint8_t>(0), file); // reserved4
		io::write_exact(file, tcb::as_bytes(tcb::make_span(ubjson)));
		file.close();
	}
	catch (const std::exception& ex)
	{
		I_Error("Couldn't save profiles. Are you out of Disk space / playing in a protected folder? Check directory for a ringprofiles.prf.bak if the profiles file is corrupt.\n\nException: %s", ex.what());
	}
	catch (...)
	{
		I_Error("Couldn't save profiles. Are you out of Disk space / playing in a protected folder? Check directory for a ringprofiles.prf.bak if the profiles file is corrupt.");
	}
}

void PR_LoadProfiles(void)
{
	namespace fs = std::filesystem;
	using namespace srb2;
	namespace io = srb2::io;
	using json = nlohmann::json;

	profile_t *dprofile = PR_MakeProfile(
		PROFILEDEFAULTNAME,
		PROFILEDEFAULTPNAME,
		PROFILEDEFAULTSKIN, PROFILEDEFAULTCOLOR,
		PROFILEDEFAULTFOLLOWER, PROFILEDEFAULTFOLLOWERCOLOR,
		gamecontroldefault,
		true
	);

	std::string datapath {fmt::format("{}/{}", srb2home, PROFILESFILE)};

	io::BufferedInputStream<io::FileStream> bis;
	try
	{
		io::FileStream file {datapath, io::FileStreamMode::kRead};
		bis = io::BufferedInputStream(std::move(file));
	}
	catch (const io::FileStreamException& ex)
	{
		PR_AddProfile(dprofile);
		return;
	}

	ProfilesJson js;
	try
	{
		uint32_t magic1;
		uint32_t magic2;
		uint8_t reserved1;
		uint8_t reserved2;
		uint8_t reserved3;
		uint8_t reserved4;
		magic1 = io::read_uint32(bis, io::Endian::kBE);
		magic2 = io::read_uint32(bis, io::Endian::kBE);
		reserved1 = io::read_uint8(bis);
		reserved2 = io::read_uint8(bis);
		reserved3 = io::read_uint8(bis);
		reserved4 = io::read_uint8(bis);

		if (magic1 != 0x52494E47 || magic2 != 0x5052464C || reserved1 != 0 || reserved2 != 0 || reserved3 != 0 || reserved4 != 0)
		{
			throw std::domain_error("Header is incompatible");
		}

		std::vector<std::byte> remainder = io::read_to_vec(bis);
		// safety: std::byte repr is always uint8_t 1-byte aligned
		tcb::span<uint8_t> remainder_as_u8 = tcb::span((uint8_t*)remainder.data(), remainder.size());
		json parsed = json::from_ubjson(remainder_as_u8);
		js = parsed.template get<ProfilesJson>();
	}
	catch (const std::exception& ex)
	{
		I_Error("Profiles file is corrupt.\n\nException: %s", ex.what());
		return;
	}
	catch (...)
	{
		I_Error("Profiles file is corrupt.");
		return;
	}

	numprofiles = js.profiles.size() + 1; // 1 for guest
	if (numprofiles > MAXPROFILES+1)
	{
		numprofiles = MAXPROFILES+1;
	}

	for (size_t i = 1; i < numprofiles; i++)
	{
		bool converted = false;
		auto& jsprof = js.profiles[i - 1];
		profile_t* newprof = static_cast<profile_t*>(Z_Calloc(sizeof(profile_t), PU_STATIC, NULL));
		profilesList[i] = newprof;

		newprof->version = jsprof.version;
		strlcpy(newprof->profilename, jsprof.profilename.c_str(), sizeof(newprof->profilename));
		memcpy(newprof->public_key, jsprof.publickey.data(), sizeof(newprof->public_key));
		memcpy(newprof->secret_key, jsprof.secretkey.data(), sizeof(newprof->secret_key));

		strlcpy(newprof->playername, jsprof.playername.c_str(), sizeof(newprof->playername));
		strlcpy(newprof->skinname, jsprof.skinname.c_str(), sizeof(newprof->skinname));
		newprof->color = PROFILEDEFAULTCOLOR;
		for (size_t c = 0; c < numskincolors; c++)
		{
			if (jsprof.colorname == skincolors[c].name && K_ColorUsable(static_cast<skincolornum_t>(c), false, false))
			{
				newprof->color = c;
				break;
			}
		}

		strlcpy(newprof->follower, jsprof.followername.c_str(), sizeof(newprof->follower));
		newprof->followercolor = PROFILEDEFAULTFOLLOWERCOLOR;
		if (jsprof.followercolorname == "Match")
		{
			newprof->followercolor = FOLLOWERCOLOR_MATCH;
		}
		else if (jsprof.followercolorname == "Opposite")
		{
			newprof->followercolor = FOLLOWERCOLOR_OPPOSITE;
		}
		else if (jsprof.followercolorname == "Default")
		{
			newprof->followercolor = SKINCOLOR_NONE;
		}
		else if (!jsprof.followercolorname.empty())
		{
			for (size_t c = 0; c < numskincolors; c++)
			{
				if (jsprof.followercolorname == skincolors[c].name && K_ColorUsable(static_cast<skincolornum_t>(c), false, false))
				{
					newprof->followercolor = c;
					break;
				}
			}
		}

		newprof->wins = jsprof.records.wins;
		newprof->rounds = jsprof.records.rounds;
		newprof->kickstartaccel = jsprof.preferences.kickstartaccel;
		newprof->autoroulette = jsprof.preferences.autoroulette;
		newprof->litesteer = jsprof.preferences.litesteer;
		newprof->autoring = jsprof.preferences.autoring;
		newprof->rumble = jsprof.preferences.rumble;
		newprof->fov = jsprof.preferences.fov;

		try
		{
			for (size_t j = 0; j < num_gamecontrols; j++)
			{
				for (size_t k = 0; k < MAXINPUTMAPPING; k++)
				{
					newprof->controls[j][k] = jsprof.controls.at(j).at(k);
				}
			}
		}
		catch (const std::out_of_range& ex)
		{
			I_Error("Profile '%s' controls are corrupt", jsprof.playername.c_str());
			return;
		}

		if (jsprof.version == 1)
		{
			// Version 1 -> 2:
			// - litesteer is now off by default, reset old profiles
			newprof->litesteer = false;

			// - fov is now 100 by default, reset if it was left at 90 (old default)
			if (newprof->fov == 90)
			{
				newprof->fov = 100;
			}

			auto unbound = [](const INT32* map)
			{
				INT32 zero[MAXINPUTMAPPING] = {};
				return !memcmp(map, zero, sizeof zero);
			};
			if (unbound(newprof->controls[gc_talk]))
			{
				// - unbound talk control gets reset to default
				memcpy(newprof->controls[gc_talk], gamecontroldefault[gc_talk], sizeof newprof->controls[gc_talk]);
			}

			converted = true;
		}

		if (jsprof.version < 3)
		{
			// Version 2 -> 3:
			// - Auto Roulette is turned off again so people have to see the warning message
			newprof->autoroulette = false;

			converted = true;
		}

		if (converted)
		{
			CONS_Printf("Profile '%s' was converted from version %d to version %d\n",
				newprof->profilename, jsprof.version, PROFILEVER);
		}
	}

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
	CV_StealthSetValue(&cv_autoring[playernum], p->autoring);
	CV_StealthSetValue(&cv_rumble[playernum], p->rumble);
	CV_StealthSetValue(&cv_fov[playernum], p->fov);

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
