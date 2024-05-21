// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_saveg.c
/// \brief Archiving: SaveGame I/O

#include "doomdef.h"
#include "byteptr.h"
#include "d_main.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_random.h"
#include "m_misc.h"
#include "p_local.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "r_data.h"
#include "r_fps.h"
#include "r_textures.h"
#include "r_things.h"
#include "r_skins.h"
#include "r_state.h"
#include "w_wad.h"
#include "y_inter.h"
#include "z_zone.h"
#include "r_main.h"
#include "r_sky.h"
#include "p_polyobj.h"
#include "lua_script.h"
#include "p_slopes.h"
#include "m_cond.h" // netUnlocked
#include "p_link.h"

// SRB2Kart
#include "k_grandprix.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "k_terrain.h"
#include "acs/interface.h"
#include "g_party.h"
#include "k_vote.h"
#include "k_zvote.h"
#include "k_endcam.h"

#include <tracy/tracy/TracyC.h>

savedata_t savedata;
savedata_cup_t cupsavedata;

static savebuffer_t *current_savebuffer;

// Block UINT32s to attempt to ensure that the correct data is
// being sent and received
#define ARCHIVEBLOCK_MISC			0x7FEEDEED
#define ARCHIVEBLOCK_PLAYERS		0x7F448008
#define ARCHIVEBLOCK_PARTIES		0x7F87AF0C
#define ARCHIVEBLOCK_ROUNDQUEUE		0x7F721331
#define ARCHIVEBLOCK_ZVOTE			0x7F54FF0D
#define ARCHIVEBLOCK_WORLD			0x7F8C08C0
#define ARCHIVEBLOCK_POBJS			0x7F928546
#define ARCHIVEBLOCK_THINKERS		0x7F37037C
#define ARCHIVEBLOCK_SPECIALS		0x7F228378
#define ARCHIVEBLOCK_WAYPOINTS		0x7F46498F
#define ARCHIVEBLOCK_RNG			0x7FAAB5BD

// Note: This cannot be bigger
// than an UINT16 (for now)
typedef enum
{
	AWAYVIEW   = 0x0001,
	FOLLOWITEM = 0x0002,
	FOLLOWER   = 0x0004,
	SKYBOXVIEW = 0x0008,
	SKYBOXCENTER = 0x0010,
	HOVERHYUDORO = 0x0020,
	STUMBLE = 0x0040,
	WAVEDASH = 0x0080,
	RINGSHOOTER = 0x0100,
	WHIP = 0x0200,
	HAND = 0x0400,
	FLICKYATTACKER = 0x0800,
	FLICKYCONTROLLER = 0x1000,
	TRICKINDICATOR = 0x2000,
	BARRIER = 0x4000,
} player_saveflags;

static inline void P_ArchivePlayer(savebuffer_t *save)
{
	const player_t *player = &players[consoleplayer];

	// Prevent an exploit from occuring.
	WRITESINT8(save->p, (player->lives - 1));
	WRITEUINT32(save->p, player->score);
	WRITEUINT16(save->p, player->totalring);

	INT32 skin = player->skin;
	if (skin > numskins)
		skin = 0;

	WRITESTRINGN(save->p, skins[skin].name, SKINNAMESIZE);

	if (player->followerskin < 0 || player->followerskin >= numfollowers)
		WRITESTRINGN(save->p, "None", SKINNAMESIZE);
	else
		WRITESTRINGN(save->p, followers[player->followerskin].name, SKINNAMESIZE);

	WRITEUINT16(save->p, player->skincolor);
	WRITEUINT16(save->p, player->followercolor);

	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false)
			continue;
		if (players[i].bot == false)
			continue;

		WRITEUINT8(save->p, i);

		skin = players[i].skin;
		if (skin > numskins)
			skin = 0;

		WRITESTRINGN(save->p, skins[skin].name, SKINNAMESIZE);

		WRITEUINT8(save->p, players[i].botvars.difficulty);
		WRITEUINT8(save->p, (UINT8)players[i].botvars.rival);

		WRITEUINT32(save->p, players[i].score);
	}

	WRITEUINT8(save->p, 0xFE);
}

static boolean P_UnArchivePlayer(savebuffer_t *save)
{
	savedata.lives = READSINT8(save->p);
	savedata.score = READUINT32(save->p);
	savedata.totalring = READUINT16(save->p);

	char skinname[SKINNAMESIZE+1];
	INT32 skin;

	READSTRINGN(save->p, skinname, SKINNAMESIZE);
	skin = R_SkinAvailableEx(skinname, false);

	if (skin == -1)
	{
		CONS_Alert(CONS_ERROR, "P_UnArchivePlayer: Character \"%s\" is not currently loaded.\n", skinname);
		return false;
	}

	savedata.skin = skin;

	READSTRINGN(save->p, skinname, SKINNAMESIZE);
	savedata.followerskin = K_FollowerAvailable(skinname);

	savedata.skincolor = READUINT16(save->p);
	savedata.followercolor = READUINT16(save->p);

	memset(&savedata.bots, 0, sizeof(savedata.bots));

	UINT8 pid;
	const UINT8 defaultbotskin = R_BotDefaultSkin();

	while ((pid = READUINT8(save->p)) < MAXPLAYERS)
	{
		savedata.bots[pid].valid = true;

		READSTRINGN(save->p, skinname, SKINNAMESIZE);
		skin = R_SkinAvailableEx(skinname, false);

		if (skin == -1)
		{
			// It is not worth destroying an otherwise good savedata over extra added skins.
			// Let's just say they didn't show up to the rematch, so some Eggrobos subbed in.
			CONS_Alert(CONS_WARNING, "P_UnArchivePlayer: Bot's character \"%s\" was not loaded, replacing with default \"%s\".\n", skinname, skins[defaultbotskin].name);
			skin = defaultbotskin;
		}

		savedata.bots[pid].skin = skin;

		savedata.bots[pid].difficulty = READUINT8(save->p);
		savedata.bots[pid].rival = (boolean)READUINT8(save->p);
		savedata.bots[pid].score = READUINT32(save->p);
	}

	return (pid == 0xFE);
}

static void P_NetArchivePlayers(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i, j;
	UINT16 flags;
	size_t q;

	WRITEUINT32(save->p, ARCHIVEBLOCK_PLAYERS);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		WRITESINT8(save->p, (SINT8)adminplayers[i]);

		for (j = 0; j < PWRLV_NUMTYPES; j++)
		{
			WRITEINT16(save->p, clientpowerlevels[i][j]);
		}
		WRITEINT16(save->p, clientPowerAdd[i]);

		if (!playeringame[i])
			continue;

		flags = 0;

		// no longer send ticcmds

		WRITESTRINGN(save->p, player_names[i], MAXPLAYERNAME);

		WRITEUINT8(save->p, playerconsole[i]);
		WRITEINT32(save->p, splitscreen_invitations[i]);

		WRITEINT16(save->p, players[i].steering);
		WRITEANGLE(save->p, players[i].angleturn);
		WRITEANGLE(save->p, players[i].aiming);
		WRITEANGLE(save->p, players[i].drawangle);
		WRITEANGLE(save->p, players[i].viewrollangle);
		WRITEANGLE(save->p, players[i].tilt);
		WRITEINT32(save->p, players[i].awayview.tics);

		WRITEUINT8(save->p, players[i].playerstate);
		WRITEUINT32(save->p, players[i].pflags);
		WRITEUINT8(save->p, players[i].panim);
		WRITEUINT8(save->p, players[i].spectator);
		WRITEUINT32(save->p, players[i].spectatewait);

		WRITEUINT16(save->p, players[i].flashpal);
		WRITEUINT16(save->p, players[i].flashcount);

		WRITEUINT8(save->p, players[i].skincolor);
		WRITEINT32(save->p, players[i].skin);

		for (j = 0; j < MAXAVAILABILITY; j++)
		{
			WRITEUINT8(save->p, players[i].availabilities[j]);
		}

		WRITEUINT8(save->p, players[i].fakeskin);
		WRITEUINT8(save->p, players[i].lastfakeskin);
		WRITEUINT32(save->p, players[i].score);
		WRITESINT8(save->p, players[i].lives);
		WRITESINT8(save->p, players[i].xtralife);
		WRITEFIXED(save->p, players[i].speed);
		WRITEFIXED(save->p, players[i].lastspeed);
		WRITEINT32(save->p, players[i].deadtimer);
		WRITEUINT32(save->p, players[i].exiting);

		////////////////////////////
		// Conveyor Belt Movement //
		////////////////////////////
		WRITEFIXED(save->p, players[i].cmomx); // Conveyor momx
		WRITEFIXED(save->p, players[i].cmomy); // Conveyor momy
		WRITEFIXED(save->p, players[i].rmomx); // "Real" momx (momx - cmomx)
		WRITEFIXED(save->p, players[i].rmomy); // "Real" momy (momy - cmomy)

		WRITEINT16(save->p, players[i].totalring);
		WRITEUINT32(save->p, players[i].realtime);
		for (j = 0; j < LAP__MAX; j++)
		{
			WRITEUINT32(save->p, players[i].laptime[j]);
		}
		WRITEUINT8(save->p, players[i].laps);
		WRITEUINT8(save->p, players[i].latestlap);
		WRITEUINT32(save->p, players[i].lapPoints);
		WRITEINT32(save->p, players[i].cheatchecknum);
		WRITEINT32(save->p, players[i].checkpointId);

		WRITEUINT8(save->p, players[i].ctfteam);

		WRITEUINT8(save->p, players[i].checkskip);

		WRITEINT16(save->p, players[i].lastsidehit);
		WRITEINT16(save->p, players[i].lastlinehit);

		WRITEINT32(save->p, players[i].onconveyor);

		WRITEUINT8(save->p, players[i].timeshit);
		WRITEUINT8(save->p, players[i].timeshitprev);

		WRITEUINT32(save->p, players[i].jointime);

		WRITEUINT32(save->p, players[i].spectatorReentry);
		WRITEUINT32(save->p, players[i].griefValue);
		WRITEUINT8(save->p, players[i].griefStrikes);
		WRITEUINT8(save->p, players[i].griefWarned);

		WRITEUINT8(save->p, players[i].splitscreenindex);

		if (players[i].awayview.mobj)
			flags |= AWAYVIEW;

		if (players[i].followmobj)
			flags |= FOLLOWITEM;

		if (players[i].follower)
			flags |= FOLLOWER;

		if (players[i].skybox.viewpoint)
			flags |= SKYBOXVIEW;

		if (players[i].skybox.centerpoint)
			flags |= SKYBOXCENTER;

		if (players[i].hoverhyudoro)
			flags |= HOVERHYUDORO;

		if (players[i].stumbleIndicator)
			flags |= STUMBLE;

		if (players[i].wavedashIndicator)
			flags |= WAVEDASH;

		if (players[i].trickIndicator)
			flags |= TRICKINDICATOR;

		if (players[i].whip)
			flags |= WHIP;

		if (players[i].hand)
			flags |= HAND;

		if (players[i].ringShooter)
			flags |= RINGSHOOTER;

		if (players[i].flickyAttacker)
			flags |= FLICKYATTACKER;

		if (players[i].powerup.flickyController)
			flags |= FLICKYCONTROLLER;

		if (players[i].powerup.barrier)
			flags |= BARRIER;

		WRITEUINT16(save->p, flags);

		if (flags & SKYBOXVIEW)
			WRITEUINT32(save->p, players[i].skybox.viewpoint->mobjnum);

		if (flags & SKYBOXCENTER)
			WRITEUINT32(save->p, players[i].skybox.centerpoint->mobjnum);

		if (flags & AWAYVIEW)
			WRITEUINT32(save->p, players[i].awayview.mobj->mobjnum);

		if (flags & FOLLOWITEM)
			WRITEUINT32(save->p, players[i].followmobj->mobjnum);

		if (flags & HOVERHYUDORO)
			WRITEUINT32(save->p, players[i].hoverhyudoro->mobjnum);

		if (flags & STUMBLE)
			WRITEUINT32(save->p, players[i].stumbleIndicator->mobjnum);

		if (flags & WAVEDASH)
			WRITEUINT32(save->p, players[i].wavedashIndicator->mobjnum);

		if (flags & TRICKINDICATOR)
			WRITEUINT32(save->p, players[i].trickIndicator->mobjnum);

		if (flags & WHIP)
			WRITEUINT32(save->p, players[i].whip->mobjnum);

		if (flags & HAND)
			WRITEUINT32(save->p, players[i].hand->mobjnum);

		if (flags & RINGSHOOTER)
			WRITEUINT32(save->p, players[i].ringShooter->mobjnum);

		if (flags & FLICKYATTACKER)
			WRITEUINT32(save->p, players[i].flickyAttacker->mobjnum);

		if (flags & FLICKYCONTROLLER)
			WRITEUINT32(save->p, players[i].powerup.flickyController->mobjnum);

		if (flags & BARRIER)
			WRITEUINT32(save->p, players[i].powerup.barrier->mobjnum);

		WRITEUINT32(save->p, (UINT32)players[i].followitem);

		WRITEUINT32(save->p, players[i].charflags);

		// SRB2kart
		WRITEUINT8(save->p, players[i].kartspeed);
		WRITEUINT8(save->p, players[i].kartweight);

		WRITEUINT8(save->p, players[i].followerskin);
		WRITEUINT8(save->p, players[i].followerready);	// booleans are really just numbers eh??
		WRITEUINT16(save->p, players[i].followercolor);
		if (flags & FOLLOWER)
			WRITEUINT32(save->p, players[i].follower->mobjnum);

		WRITEUINT16(save->p, players[i].nocontrol);
		WRITEUINT8(save->p, players[i].carry);
		WRITEUINT16(save->p, players[i].dye);

		WRITEUINT8(save->p, players[i].position);
		WRITEUINT8(save->p, players[i].oldposition);
		WRITEUINT8(save->p, players[i].positiondelay);
		WRITEUINT32(save->p, players[i].distancetofinish);
		WRITEUINT32(save->p, players[i].distancetofinishprev);
		WRITEUINT32(save->p, players[i].lastpickupdistance);
		WRITEUINT8(save->p, players[i].lastpickuptype);
		WRITEUINT32(save->p, K_GetWaypointHeapIndex(players[i].currentwaypoint));
		WRITEUINT32(save->p, K_GetWaypointHeapIndex(players[i].nextwaypoint));
		WRITEUINT32(save->p, players[i].airtime);
		WRITEUINT32(save->p, players[i].lastairtime);
		WRITEUINT16(save->p, players[i].bigwaypointgap);
		WRITEUINT8(save->p, players[i].startboost);
		WRITEUINT8(save->p, players[i].dropdashboost);

		WRITEUINT16(save->p, players[i].flashing);
		WRITEUINT16(save->p, players[i].spinouttimer);
		WRITEUINT8(save->p, players[i].spinouttype);
		WRITEUINT8(save->p, players[i].instashield);
		WRITEINT32(save->p, players[i].nullHitlag);
		WRITEUINT8(save->p, players[i].wipeoutslow);
		WRITEUINT8(save->p, players[i].justbumped);
		WRITEUINT8(save->p, players[i].noEbrakeMagnet);
		WRITEUINT8(save->p, players[i].tumbleBounces);
		WRITEUINT16(save->p, players[i].tumbleHeight);

		WRITEUINT8(save->p, players[i].justDI);
		WRITEUINT8(save->p, players[i].flipDI);

		WRITESINT8(save->p, players[i].drift);
		WRITEFIXED(save->p, players[i].driftcharge);
		WRITEUINT16(save->p, players[i].driftboost);
		WRITEUINT16(save->p, players[i].strongdriftboost);

		WRITEUINT16(save->p, players[i].gateBoost);
		WRITEUINT8(save->p, players[i].gateSound);

		WRITESINT8(save->p, players[i].aizdriftstrat);
		WRITESINT8(save->p, players[i].aizdriftextend);
		WRITEINT32(save->p, players[i].aizdrifttilt);
		WRITEINT32(save->p, players[i].aizdriftturn);

		WRITEINT32(save->p, players[i].underwatertilt);

		WRITEFIXED(save->p, players[i].offroad);

		WRITEUINT16(save->p, players[i].tiregrease);
		WRITEUINT16(save->p, players[i].springstars);
		WRITEUINT16(save->p, players[i].springcolor);
		WRITEUINT8(save->p, players[i].dashpadcooldown);

		WRITEUINT16(save->p, players[i].spindash);
		WRITEFIXED(save->p, players[i].spindashspeed);
		WRITEUINT8(save->p, players[i].spindashboost);

		WRITEFIXED(save->p, players[i].fastfall);
		WRITEFIXED(save->p, players[i].fastfallBase);

		WRITEUINT8(save->p, players[i].numboosts);
		WRITEFIXED(save->p, players[i].boostpower);
		WRITEFIXED(save->p, players[i].speedboost);
		WRITEFIXED(save->p, players[i].accelboost);
		WRITEFIXED(save->p, players[i].handleboost);
		WRITEANGLE(save->p, players[i].boostangle);

		WRITEFIXED(save->p, players[i].draftpower);
		WRITEUINT16(save->p, players[i].draftleeway);
		WRITESINT8(save->p, players[i].lastdraft);

		WRITEUINT8(save->p, players[i].tripwireState);
		WRITEUINT8(save->p, players[i].tripwirePass);
		WRITEUINT16(save->p, players[i].tripwireLeniency);
		WRITEUINT8(save->p, players[i].fakeBoost);

		WRITESINT8(save->p, players[i].itemtype);
		WRITEUINT8(save->p, players[i].itemamount);
		WRITESINT8(save->p, players[i].throwdir);

		WRITEUINT8(save->p, players[i].sadtimer);

		WRITESINT8(save->p, players[i].rings);
		WRITESINT8(save->p, players[i].hudrings);
		WRITEUINT8(save->p, players[i].pickuprings);
		WRITEUINT8(save->p, players[i].ringdelay);
		WRITEUINT16(save->p, players[i].ringboost);
		WRITEUINT8(save->p, players[i].sparkleanim);
		WRITEUINT16(save->p, players[i].superring);
		WRITEUINT8(save->p, players[i].nextringaward);
		WRITEUINT8(save->p, players[i].ringvolume);
		WRITEUINT8(save->p, players[i].ringtransparency);
		WRITEUINT16(save->p, players[i].ringburst);

		WRITEUINT8(save->p, players[i].curshield);
		WRITEUINT8(save->p, players[i].bubblecool);
		WRITEUINT8(save->p, players[i].bubbleblowup);
		WRITEUINT16(save->p, players[i].flamedash);
		WRITEUINT16(save->p, players[i].counterdash);
		WRITEUINT16(save->p, players[i].flamemeter);
		WRITEUINT8(save->p, players[i].flamelength);

		WRITEUINT16(save->p, players[i].ballhogcharge);
		WRITEUINT8(save->p, players[i].ballhogtap);

		WRITEUINT16(save->p, players[i].hyudorotimer);
		WRITESINT8(save->p, players[i].stealingtimer);

		WRITEUINT16(save->p, players[i].sneakertimer);
		WRITEUINT8(save->p, players[i].numsneakers);
		WRITEUINT8(save->p, players[i].floorboost);

		WRITEINT16(save->p, players[i].growshrinktimer);
		WRITEUINT16(save->p, players[i].rocketsneakertimer);
		WRITEUINT16(save->p, players[i].invincibilitytimer);
		WRITEUINT16(save->p, players[i].invincibilityextensions);

		WRITEUINT8(save->p, players[i].eggmanexplode);
		WRITESINT8(save->p, players[i].eggmanblame);

		WRITEUINT8(save->p, players[i].bananadrag);

		WRITESINT8(save->p, players[i].lastjawztarget);
		WRITEUINT8(save->p, players[i].jawztargetdelay);

		WRITEUINT8(save->p, players[i].confirmVictim);
		WRITEUINT8(save->p, players[i].confirmVictimDelay);

		WRITEUINT8(save->p, players[i].trickpanel);
		WRITEUINT8(save->p, players[i].tricktime);
		WRITEUINT32(save->p, players[i].trickboostpower);
		WRITEUINT8(save->p, players[i].trickboostdecay);
		WRITEUINT8(save->p, players[i].trickboost);
		WRITEUINT8(save->p, players[i].tricklock);

		WRITEUINT8(save->p, players[i].dashRingPullTics);
		WRITEUINT8(save->p, players[i].dashRingPushTics);

		WRITEUINT8(save->p, players[i].pullup);

		WRITEUINT32(save->p, players[i].ebrakefor);

		WRITEUINT32(save->p, players[i].roundscore);
		WRITEUINT8(save->p, players[i].emeralds);
		WRITEINT16(save->p, players[i].karmadelay);
		WRITEINT16(save->p, players[i].spheres);
		WRITEUINT32(save->p, players[i].spheredigestion);

		WRITESINT8(save->p, players[i].glanceDir);

		WRITEUINT16(save->p, players[i].breathTimer);

		WRITEUINT8(save->p, players[i].typing_timer);
		WRITEUINT8(save->p, players[i].typing_duration);

		WRITEUINT8(save->p, players[i].kickstartaccel);
		WRITEUINT8(save->p, players[i].autoring);

		WRITEUINT8(save->p, players[i].stairjank);
		WRITEUINT8(save->p, players[i].topdriftheld);
		WRITEUINT8(save->p, players[i].topinfirst);

		WRITEUINT8(save->p, players[i].shrinkLaserDelay);

		WRITEUINT8(save->p, players[i].eggmanTransferDelay);

		WRITEUINT8(save->p, players[i].tripwireReboundDelay);

		WRITEUINT16(save->p, players[i].wavedash);
		WRITEUINT8(save->p, players[i].wavedashdelay);
		WRITEUINT16(save->p, players[i].wavedashboost);
		WRITEFIXED(save->p, players[i].wavedashpower);
		WRITEUINT16(save->p, players[i].speedpunt);
		WRITEUINT16(save->p, players[i].trickcharge);

		WRITEUINT16(save->p, players[i].infinitether);

		WRITEUINT8(save->p, players[i].finalfailsafe);

		WRITEUINT8(save->p, players[i].lastsafelap);
		WRITEUINT8(save->p, players[i].lastsafecheatcheck);

		WRITEUINT8(save->p, players[i].ignoreAirtimeLeniency);

		WRITEFIXED(save->p, players[i].topAccel);

		WRITEMEM(save->p, players[i].public_key, PUBKEYLENGTH);

		WRITESINT8(save->p, players[i].pitblame);

		WRITEUINT8(save->p, players[i].instaWhipCharge);
		WRITEUINT8(save->p, players[i].defenseLockout);
		WRITEUINT8(save->p, players[i].oldGuard);
		WRITEUINT8(save->p, players[i].powerupVFXTimer);

		WRITEUINT8(save->p, players[i].preventfailsafe);

		WRITEUINT8(save->p, players[i].tripwireUnstuck);
		WRITEUINT8(save->p, players[i].bumpUnstuck);

		WRITEUINT8(save->p, players[i].handtimer);
		WRITEANGLE(save->p, players[i].besthanddirection);

		WRITEINT16(save->p, players[i].incontrol);
		WRITEUINT16(save->p, players[i].progressivethrust);
		WRITEUINT8(save->p, players[i].ringvisualwarning);

		WRITEUINT8(save->p, players[i].analoginput);

		WRITEUINT8(save->p, players[i].markedfordeath);
		WRITEUINT8(save->p, players[i].dotrickfx);
		WRITEUINT8(save->p, players[i].stingfx);
		WRITEUINT8(save->p, players[i].bumperinflate);

		WRITEUINT8(save->p, players[i].ringboxdelay);
		WRITEUINT8(save->p, players[i].ringboxaward);

		WRITEUINT8(save->p, players[i].itemflags);

		WRITEFIXED(save->p, players[i].outrun);

		WRITEUINT8(save->p, players[i].rideroid);
		WRITEUINT8(save->p, players[i].rdnodepull);
		WRITEINT32(save->p, players[i].rideroidangle);
		WRITEFIXED(save->p, players[i].rideroidspeed);
		WRITEINT32(save->p, players[i].rideroidrollangle);
		WRITEFIXED(save->p, players[i].rdaddmomx);
		WRITEFIXED(save->p, players[i].rdaddmomy);
		WRITEFIXED(save->p, players[i].rdaddmomz);

		WRITEUINT8(save->p, players[i].bungee);

		WRITEUINT32(save->p, players[i].lasthover);

		WRITEUINT32(save->p, players[i].dlzrocket);
		WRITEANGLE(save->p, players[i].dlzrocketangle);
		WRITEINT32(save->p, players[i].dlzrocketanglev);
		WRITEFIXED(save->p, players[i].dlzrocketspd);

		WRITEUINT8(save->p, players[i].seasaw);
		WRITEUINT32(save->p, players[i].seasawcooldown);
		WRITEFIXED(save->p, players[i].seasawdist);
		WRITEINT32(save->p, players[i].seasawangle);
		WRITEINT32(save->p, players[i].seasawangleadd);
		WRITEINT32(save->p, players[i].seasawmoreangle);
		WRITEUINT8(save->p, players[i].seasawdir);

		WRITEUINT32(save->p, players[i].turbine);
		WRITEINT32(save->p, players[i].turbineangle);
		WRITEFIXED(save->p, players[i].turbineheight);
		WRITEUINT8(save->p, players[i].turbinespd);

		WRITEUINT32(save->p, players[i].cloud);
		WRITEUINT32(save->p, players[i].cloudlaunch);
		WRITEUINT32(save->p, players[i].cloudbuf);

		WRITEUINT32(save->p, players[i].tulip);
		WRITEUINT32(save->p, players[i].tuliplaunch);
		WRITEUINT32(save->p, players[i].tulipbuf);

		// respawnvars_t
		WRITEUINT8(save->p, players[i].respawn.state);
		WRITEUINT32(save->p, K_GetWaypointHeapIndex(players[i].respawn.wp));
		WRITEFIXED(save->p, players[i].respawn.pointx);
		WRITEFIXED(save->p, players[i].respawn.pointy);
		WRITEFIXED(save->p, players[i].respawn.pointz);
		WRITEUINT8(save->p, players[i].respawn.flip);
		WRITEUINT32(save->p, players[i].respawn.timer);
		WRITEUINT32(save->p, players[i].respawn.airtimer);
		WRITEUINT32(save->p, players[i].respawn.distanceleft);
		WRITEUINT32(save->p, players[i].respawn.dropdash);
		WRITEUINT8(save->p, players[i].respawn.truedeath);
		WRITEUINT8(save->p, players[i].respawn.manual);
		WRITEUINT8(save->p, players[i].respawn.fast);
		WRITEUINT32(save->p, players[i].respawn.returnspeed);

		// botvars_t
		WRITEUINT8(save->p, players[i].bot);
		WRITEUINT8(save->p, players[i].botvars.difficulty);
		WRITEUINT8(save->p, players[i].botvars.diffincrease);
		WRITEUINT8(save->p, players[i].botvars.rival);
		WRITEFIXED(save->p, players[i].botvars.rubberband);
		WRITEUINT32(save->p, players[i].botvars.itemdelay);
		WRITEUINT32(save->p, players[i].botvars.itemconfirm);
		WRITESINT8(save->p, players[i].botvars.turnconfirm);
		WRITEUINT32(save->p, players[i].botvars.spindashconfirm);
		WRITEUINT32(save->p, players[i].botvars.respawnconfirm);
		WRITEUINT8(save->p, players[i].botvars.roulettePriority);
		WRITEUINT32(save->p, players[i].botvars.rouletteTimeout);

		// itemroulette_t
		WRITEUINT8(save->p, players[i].itemRoulette.active);

#ifdef ITEM_LIST_SIZE
		WRITEUINT32(save->p, players[i].itemRoulette.itemListLen);

		for (q = 0; q < ITEM_LIST_SIZE; q++)
		{
			if (q >= players[i].itemRoulette.itemListLen)
			{
				WRITESINT8(save->p, KITEM_NONE);
			}
			else
			{
				WRITESINT8(save->p, players[i].itemRoulette.itemList[q]);
			}
		}
#else
		if (players[i].itemRoulette.itemList == NULL)
		{
			WRITEUINT32(save->p, 0);
			WRITEUINT32(save->p, 0);
		}
		else
		{
			WRITEUINT32(save->p, players[i].itemRoulette.itemListCap);
			WRITEUINT32(save->p, players[i].itemRoulette.itemListLen);

			for (q = 0; q < players[i].itemRoulette.itemListLen; q++)
			{
				WRITESINT8(save->p, players[i].itemRoulette.itemList[q]);
			}
		}
#endif

		WRITEUINT8(save->p, players[i].itemRoulette.useOdds);
		WRITEUINT32(save->p, players[i].itemRoulette.dist);
		WRITEUINT32(save->p, players[i].itemRoulette.index);
		WRITEUINT8(save->p, players[i].itemRoulette.sound);
		WRITEUINT32(save->p, players[i].itemRoulette.speed);
		WRITEUINT32(save->p, players[i].itemRoulette.tics);
		WRITEUINT32(save->p, players[i].itemRoulette.elapsed);
		WRITEUINT8(save->p, players[i].itemRoulette.eggman);
		WRITEUINT8(save->p, players[i].itemRoulette.ringbox);
		WRITEUINT8(save->p, players[i].itemRoulette.autoroulette);
		WRITEUINT8(save->p, players[i].itemRoulette.reserved);

		// sonicloopsvars_t
		WRITEFIXED(save->p, players[i].loop.radius);
		WRITEFIXED(save->p, players[i].loop.revolution);
		WRITEFIXED(save->p, players[i].loop.min_revolution);
		WRITEFIXED(save->p, players[i].loop.max_revolution);
		WRITEANGLE(save->p, players[i].loop.yaw);
		WRITEFIXED(save->p, players[i].loop.origin.x);
		WRITEFIXED(save->p, players[i].loop.origin.y);
		WRITEFIXED(save->p, players[i].loop.origin.z);
		WRITEFIXED(save->p, players[i].loop.origin_shift.x);
		WRITEFIXED(save->p, players[i].loop.origin_shift.y);
		WRITEFIXED(save->p, players[i].loop.shift.x);
		WRITEFIXED(save->p, players[i].loop.shift.y);
		WRITEUINT8(save->p, players[i].loop.flip);

		// sonicloopcamvars_t
		WRITEUINT32(save->p, players[i].loop.camera.enter_tic);
		WRITEUINT32(save->p, players[i].loop.camera.exit_tic);
		WRITEUINT32(save->p, players[i].loop.camera.zoom_in_speed);
		WRITEUINT32(save->p, players[i].loop.camera.zoom_out_speed);
		WRITEFIXED(save->p, players[i].loop.camera.dist);
		WRITEANGLE(save->p, players[i].loop.camera.pan);
		WRITEFIXED(save->p, players[i].loop.camera.pan_speed);
		WRITEUINT32(save->p, players[i].loop.camera.pan_accel);
		WRITEUINT32(save->p, players[i].loop.camera.pan_back);

		// ACS has read access to this, so it has to be net-communicated.
		// It is the ONLY roundcondition that is sent over the wire and I'd like it to stay that way.
		WRITEUINT32(save->p, players[i].roundconditions.unlocktriggers);

		// powerupvars_t
		WRITEUINT16(save->p, players[i].powerup.superTimer);
		WRITEUINT16(save->p, players[i].powerup.barrierTimer);
		WRITEUINT16(save->p, players[i].powerup.rhythmBadgeTimer);

		// level_tally_t
		WRITEUINT8(save->p, players[i].tally.active);
		if (players[i].tally.active)
		{
			WRITEUINT16(save->p, players[i].tally.gt);
			WRITEUINT8(save->p, players[i].tally.gotThru);
			WRITESTRINGN(save->p, players[i].tally.header, 63);
			WRITEUINT8(save->p, players[i].tally.showRoundNum);
			WRITEINT32(save->p, players[i].tally.gradeVoice);

			WRITEINT32(save->p, players[i].tally.time);
			WRITEUINT16(save->p, players[i].tally.ringPool);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				WRITEINT32(save->p, players[i].tally.stats[q]);

			WRITEUINT8(save->p, players[i].tally.position);
			WRITEUINT8(save->p, players[i].tally.numPlayers);
			WRITEUINT8(save->p, players[i].tally.rings);
			WRITEUINT16(save->p, players[i].tally.laps);
			WRITEUINT16(save->p, players[i].tally.totalLaps);
			WRITEUINT16(save->p, players[i].tally.prisons);
			WRITEUINT16(save->p, players[i].tally.totalPrisons);
			WRITEINT32(save->p, players[i].tally.points);
			WRITEINT32(save->p, players[i].tally.pointLimit);
			WRITEUINT8(save->p, players[i].tally.powerStones);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				WRITEINT32(save->p, players[i].tally.bonuses[q]);
			WRITEINT32(save->p, players[i].tally.rank);

			WRITEINT32(save->p, players[i].tally.state);
			WRITEINT32(save->p, players[i].tally.hudSlide);
			WRITEINT32(save->p, players[i].tally.delay);
			WRITEINT32(save->p, players[i].tally.transition);
			WRITEINT32(save->p, players[i].tally.transitionTime);
			WRITEUINT8(save->p, players[i].tally.lines);
			WRITEUINT8(save->p, players[i].tally.lineCount);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				WRITEINT32(save->p, players[i].tally.displayStat[q]);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				WRITEINT32(save->p, players[i].tally.displayBonus[q]);
			WRITEUINT8(save->p, players[i].tally.tickSound);
			WRITEUINT8(save->p, players[i].tally.xtraBlink);
			WRITEUINT8(save->p, players[i].tally.showGrade);
			WRITEUINT8(save->p, players[i].tally.done);
		}

		// icecubevars_t
		WRITEUINT32(save->p, players[i].icecube.hitat);
		WRITEUINT8(save->p, players[i].icecube.frozen);
		WRITEUINT8(save->p, players[i].icecube.wiggle);
		WRITEUINT32(save->p, players[i].icecube.frozenat);
		WRITEUINT8(save->p, players[i].icecube.shaketimer);

		// darkness
		WRITEUINT32(save->p, players[i].darkness_start);
		WRITEUINT32(save->p, players[i].darkness_end);
	}

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchivePlayers(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i, j;
	UINT16 flags;
	size_t q;

	if (READUINT32(save->p) != ARCHIVEBLOCK_PLAYERS)
		I_Error("Bad $$$.sav at archive block Players");

	for (i = 0; i < MAXPLAYERS; i++)
	{
		adminplayers[i] = (INT32)READSINT8(save->p);

		for (j = 0; j < PWRLV_NUMTYPES; j++)
		{
			clientpowerlevels[i][j] = READINT16(save->p);
		}
		clientPowerAdd[i] = READINT16(save->p);

		// Do NOT memset player struct to 0
		// other areas may initialize data elsewhere
		//memset(&players[i], 0, sizeof (player_t));
		if (!playeringame[i])
			continue;

		// NOTE: sending tics should (hopefully) no longer be necessary

		READSTRINGN(save->p, player_names[i], MAXPLAYERNAME);

		playerconsole[i] = READUINT8(save->p);
		splitscreen_invitations[i] = READINT32(save->p);

		players[i].steering = READINT16(save->p);
		players[i].angleturn = READANGLE(save->p);
		players[i].aiming = READANGLE(save->p);
		players[i].drawangle = players[i].old_drawangle = READANGLE(save->p);
		players[i].viewrollangle = READANGLE(save->p);
		players[i].tilt = READANGLE(save->p);
		players[i].awayview.tics = READINT32(save->p);

		players[i].playerstate = READUINT8(save->p);
		players[i].pflags = READUINT32(save->p);
		players[i].panim = READUINT8(save->p);
		players[i].spectator = READUINT8(save->p);
		players[i].spectatewait = READUINT32(save->p);

		players[i].flashpal = READUINT16(save->p);
		players[i].flashcount = READUINT16(save->p);

		players[i].skincolor = READUINT8(save->p);
		players[i].skin = READINT32(save->p);

		for (j = 0; j < MAXAVAILABILITY; j++)
		{
			players[i].availabilities[j] = READUINT8(save->p);
		}

		players[i].fakeskin = READUINT8(save->p);
		players[i].lastfakeskin = READUINT8(save->p);
		players[i].score = READUINT32(save->p);
		players[i].lives = READSINT8(save->p);
		players[i].xtralife = READSINT8(save->p); // Ring Extra Life counter
		players[i].speed = READFIXED(save->p); // Player's speed (distance formula of MOMX and MOMY values)
		players[i].lastspeed = READFIXED(save->p);
		players[i].deadtimer = READINT32(save->p); // End game if game over lasts too long
		players[i].exiting = READUINT32(save->p); // Exitlevel timer

		////////////////////////////
		// Conveyor Belt Movement //
		////////////////////////////
		players[i].cmomx = READFIXED(save->p); // Conveyor momx
		players[i].cmomy = READFIXED(save->p); // Conveyor momy
		players[i].rmomx = READFIXED(save->p); // "Real" momx (momx - cmomx)
		players[i].rmomy = READFIXED(save->p); // "Real" momy (momy - cmomy)

		players[i].totalring = READINT16(save->p); // Total number of rings obtained for GP
		players[i].realtime = READUINT32(save->p); // integer replacement for leveltime
		for (j = 0; j < LAP__MAX; j++)
		{
			players[i].laptime[j] = READUINT32(save->p);
		}
		players[i].laps = READUINT8(save->p); // Number of laps (optional)
		players[i].latestlap = READUINT8(save->p);
		players[i].lapPoints = READUINT32(save->p);
		players[i].cheatchecknum = READINT32(save->p);
		players[i].checkpointId = READINT32(save->p);

		players[i].ctfteam = READUINT8(save->p); // 1 == Red, 2 == Blue

		players[i].checkskip = READUINT8(save->p);

		players[i].lastsidehit = READINT16(save->p);
		players[i].lastlinehit = READINT16(save->p);

		players[i].timeshit = READUINT8(save->p);
		players[i].timeshitprev = READUINT8(save->p);

		players[i].onconveyor = READINT32(save->p);

		players[i].jointime = READUINT32(save->p);

		players[i].spectatorReentry = READUINT32(save->p);
		players[i].griefValue = READUINT32(save->p);
		players[i].griefStrikes = READUINT8(save->p);
		players[i].griefWarned = READUINT8(save->p);

		players[i].splitscreenindex = READUINT8(save->p);

		flags = READUINT16(save->p);

		if (flags & SKYBOXVIEW)
			players[i].skybox.viewpoint = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & SKYBOXCENTER)
			players[i].skybox.centerpoint = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & AWAYVIEW)
			players[i].awayview.mobj = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & FOLLOWITEM)
			players[i].followmobj = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & HOVERHYUDORO)
			players[i].hoverhyudoro = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & STUMBLE)
			players[i].stumbleIndicator = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & WAVEDASH)
			players[i].wavedashIndicator = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & TRICKINDICATOR)
			players[i].trickIndicator = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & WHIP)
			players[i].whip = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & HAND)
			players[i].hand = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & RINGSHOOTER)
			players[i].ringShooter = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & FLICKYATTACKER)
			players[i].flickyAttacker = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & FLICKYCONTROLLER)
			players[i].powerup.flickyController = (mobj_t *)(size_t)READUINT32(save->p);

		if (flags & BARRIER)
			players[i].powerup.barrier = (mobj_t *)(size_t)READUINT32(save->p);

		players[i].followitem = (mobjtype_t)READUINT32(save->p);

		//SetPlayerSkinByNum(i, players[i].skin);
		players[i].charflags = READUINT32(save->p);

		// SRB2kart
		players[i].kartspeed = READUINT8(save->p);
		players[i].kartweight = READUINT8(save->p);

		players[i].followerskin = READUINT8(save->p);
		players[i].followerready = READUINT8(save->p);
		players[i].followercolor = READUINT16(save->p);
		if (flags & FOLLOWER)
			players[i].follower = (mobj_t *)(size_t)READUINT32(save->p);

		players[i].nocontrol = READUINT16(save->p);
		players[i].carry = READUINT8(save->p);
		players[i].dye = READUINT16(save->p);

		players[i].position = READUINT8(save->p);
		players[i].oldposition = READUINT8(save->p);
		players[i].positiondelay = READUINT8(save->p);
		players[i].distancetofinish = READUINT32(save->p);
		players[i].distancetofinishprev = READUINT32(save->p);
		players[i].lastpickupdistance = READUINT32(save->p);
		players[i].lastpickuptype = READUINT8(save->p);
		players[i].currentwaypoint = (waypoint_t *)(size_t)READUINT32(save->p);
		players[i].nextwaypoint = (waypoint_t *)(size_t)READUINT32(save->p);
		players[i].airtime = READUINT32(save->p);
		players[i].lastairtime = READUINT32(save->p);
		players[i].bigwaypointgap = READUINT16(save->p);
		players[i].startboost = READUINT8(save->p);
		players[i].dropdashboost = READUINT8(save->p);

		players[i].flashing = READUINT16(save->p);
		players[i].spinouttimer = READUINT16(save->p);
		players[i].spinouttype = READUINT8(save->p);
		players[i].instashield = READUINT8(save->p);
		players[i].nullHitlag = READINT32(save->p);
		players[i].wipeoutslow = READUINT8(save->p);
		players[i].justbumped = READUINT8(save->p);
		players[i].noEbrakeMagnet = READUINT8(save->p);
		players[i].tumbleBounces = READUINT8(save->p);
		players[i].tumbleHeight = READUINT16(save->p);

		players[i].justDI = READUINT8(save->p);
		players[i].flipDI = (boolean)READUINT8(save->p);

		players[i].drift = READSINT8(save->p);
		players[i].driftcharge = READFIXED(save->p);
		players[i].driftboost = READUINT16(save->p);
		players[i].strongdriftboost = READUINT16(save->p);

		players[i].gateBoost = READUINT16(save->p);
		players[i].gateSound = READUINT8(save->p);

		players[i].aizdriftstrat = READSINT8(save->p);
		players[i].aizdriftextend = READSINT8(save->p);
		players[i].aizdrifttilt = READINT32(save->p);
		players[i].aizdriftturn = READINT32(save->p);

		players[i].underwatertilt = READINT32(save->p);

		players[i].offroad = READFIXED(save->p);

		players[i].tiregrease = READUINT16(save->p);
		players[i].springstars = READUINT16(save->p);
		players[i].springcolor = READUINT16(save->p);
		players[i].dashpadcooldown = READUINT8(save->p);

		players[i].spindash = READUINT16(save->p);
		players[i].spindashspeed = READFIXED(save->p);
		players[i].spindashboost = READUINT8(save->p);

		players[i].fastfall = READFIXED(save->p);
		players[i].fastfallBase = READFIXED(save->p);

		players[i].numboosts = READUINT8(save->p);
		players[i].boostpower = READFIXED(save->p);
		players[i].speedboost = READFIXED(save->p);
		players[i].accelboost = READFIXED(save->p);
		players[i].handleboost = READFIXED(save->p);
		players[i].boostangle = READANGLE(save->p);

		players[i].draftpower = READFIXED(save->p);
		players[i].draftleeway = READUINT16(save->p);
		players[i].lastdraft = READSINT8(save->p);

		players[i].tripwireState = READUINT8(save->p);
		players[i].tripwirePass = READUINT8(save->p);
		players[i].tripwireLeniency = READUINT16(save->p);
		players[i].fakeBoost = READUINT8(save->p);

		players[i].itemtype = READSINT8(save->p);
		players[i].itemamount = READUINT8(save->p);
		players[i].throwdir = READSINT8(save->p);

		players[i].sadtimer = READUINT8(save->p);

		players[i].rings = READSINT8(save->p);
		players[i].hudrings = READSINT8(save->p);
		players[i].pickuprings = READUINT8(save->p);
		players[i].ringdelay = READUINT8(save->p);
		players[i].ringboost = READUINT16(save->p);
		players[i].sparkleanim = READUINT8(save->p);
		players[i].superring = READUINT16(save->p);
		players[i].nextringaward = READUINT8(save->p);
		players[i].ringvolume = READUINT8(save->p);
		players[i].ringtransparency = READUINT8(save->p);
		players[i].ringburst = READUINT16(save->p);

		players[i].curshield = READUINT8(save->p);
		players[i].bubblecool = READUINT8(save->p);
		players[i].bubbleblowup = READUINT8(save->p);
		players[i].flamedash = READUINT16(save->p);
		players[i].counterdash = READUINT16(save->p);
		players[i].flamemeter = READUINT16(save->p);
		players[i].flamelength = READUINT8(save->p);

		players[i].ballhogcharge = READUINT16(save->p);
		players[i].ballhogtap = READUINT8(save->p);

		players[i].hyudorotimer = READUINT16(save->p);
		players[i].stealingtimer = READSINT8(save->p);

		players[i].sneakertimer = READUINT16(save->p);
		players[i].numsneakers = READUINT8(save->p);
		players[i].floorboost = READUINT8(save->p);

		players[i].growshrinktimer = READINT16(save->p);
		players[i].rocketsneakertimer = READUINT16(save->p);
		players[i].invincibilitytimer = READUINT16(save->p);
		players[i].invincibilityextensions = READUINT16(save->p);

		players[i].eggmanexplode = READUINT8(save->p);
		players[i].eggmanblame = READSINT8(save->p);

		players[i].bananadrag = READUINT8(save->p);

		players[i].lastjawztarget = READSINT8(save->p);
		players[i].jawztargetdelay = READUINT8(save->p);

		players[i].confirmVictim = READUINT8(save->p);
		players[i].confirmVictimDelay = READUINT8(save->p);

		players[i].trickpanel = READUINT8(save->p);
		players[i].tricktime = READUINT8(save->p);
		players[i].trickboostpower = READUINT32(save->p);
		players[i].trickboostdecay = READUINT8(save->p);
		players[i].trickboost = READUINT8(save->p);
		players[i].tricklock = READUINT8(save->p);

		players[i].dashRingPullTics = READUINT8(save->p);
		players[i].dashRingPushTics = READUINT8(save->p);

		players[i].pullup = READUINT8(save->p);

		players[i].ebrakefor = READUINT32(save->p);

		players[i].roundscore = READUINT32(save->p);
		players[i].emeralds = READUINT8(save->p);
		players[i].karmadelay = READINT16(save->p);
		players[i].spheres = READINT16(save->p);
		players[i].spheredigestion = READUINT32(save->p);

		players[i].glanceDir = READSINT8(save->p);

		players[i].breathTimer = READUINT16(save->p);

		players[i].typing_timer = READUINT8(save->p);
		players[i].typing_duration = READUINT8(save->p);

		players[i].kickstartaccel = READUINT8(save->p);
		players[i].autoring = READUINT8(save->p);

		players[i].stairjank = READUINT8(save->p);
		players[i].topdriftheld = READUINT8(save->p);
		players[i].topinfirst = READUINT8(save->p);

		players[i].shrinkLaserDelay = READUINT8(save->p);

		players[i].eggmanTransferDelay = READUINT8(save->p);

		players[i].tripwireReboundDelay = READUINT8(save->p);

		players[i].wavedash = READUINT16(save->p);
		players[i].wavedashdelay = READUINT8(save->p);
		players[i].wavedashboost = READUINT16(save->p);
		players[i].wavedashpower = READFIXED(save->p);
		players[i].speedpunt = READUINT16(save->p);
		players[i].trickcharge = READUINT16(save->p);

		players[i].infinitether = READUINT16(save->p);

		players[i].finalfailsafe = READUINT8(save->p);

		players[i].lastsafelap = READUINT8(save->p);
		players[i].lastsafecheatcheck = READUINT8(save->p);

		players[i].ignoreAirtimeLeniency = READUINT8(save->p);

		players[i].topAccel = READFIXED(save->p);

		READMEM(save->p, players[i].public_key, PUBKEYLENGTH);

		players[i].pitblame = READSINT8(save->p);

		players[i].instaWhipCharge = READUINT8(save->p);
		players[i].defenseLockout = READUINT8(save->p);
		players[i].oldGuard = READUINT8(save->p);
		players[i].powerupVFXTimer = READUINT8(save->p);

		players[i].preventfailsafe = READUINT8(save->p);

		players[i].tripwireUnstuck = READUINT8(save->p);
		players[i].bumpUnstuck = READUINT8(save->p);

		players[i].handtimer = READUINT8(save->p);
		players[i].besthanddirection = READANGLE(save->p);

		players[i].incontrol = READINT16(save->p);
		players[i].progressivethrust = READUINT16(save->p);
		players[i].ringvisualwarning = READUINT8(save->p);

		players[i].analoginput = READUINT8(save->p);

		players[i].markedfordeath = READUINT8(save->p);
		players[i].dotrickfx = READUINT8(save->p);
		players[i].stingfx = READUINT8(save->p);
		players[i].bumperinflate = READUINT8(save->p);

		players[i].ringboxdelay = READUINT8(save->p);
		players[i].ringboxaward = READUINT8(save->p);

		players[i].itemflags = READUINT8(save->p);

		players[i].outrun = READFIXED(save->p);

		players[i].rideroid = (boolean)READUINT8(save->p);
		players[i].rdnodepull = (boolean)READUINT8(save->p);
		players[i].rideroidangle = READINT32(save->p);
		players[i].rideroidspeed = READFIXED(save->p);
		players[i].rideroidrollangle = READINT32(save->p);
		players[i].rdaddmomx = READFIXED(save->p);
		players[i].rdaddmomy = READFIXED(save->p);
		players[i].rdaddmomz = READFIXED(save->p);

		players[i].bungee = READUINT8(save->p);

		players[i].lasthover = (tic_t)READUINT32(save->p);

		players[i].dlzrocket = (tic_t)READUINT32(save->p);
		players[i].dlzrocketangle = READANGLE(save->p);
		players[i].dlzrocketanglev = READINT32(save->p);
		players[i].dlzrocketspd = READFIXED(save->p);

		players[i].seasaw = (boolean)READUINT8(save->p);
		players[i].seasawcooldown = READUINT32(save->p);
		players[i].seasawdist = READFIXED(save->p);
		players[i].seasawangle = READINT32(save->p);
		players[i].seasawangleadd = READINT32(save->p);
		players[i].seasawmoreangle = READINT32(save->p);
		players[i].seasawdir = (boolean)READUINT8(save->p);

		players[i].turbine = (tic_t)READUINT32(save->p);
		players[i].turbineangle = READINT32(save->p);
		players[i].turbineheight = READFIXED(save->p);
		players[i].turbinespd = (boolean)READUINT8(save->p);

		players[i].cloud = (tic_t)READUINT32(save->p);
		players[i].cloudlaunch = (tic_t)READUINT32(save->p);
		players[i].cloudbuf = (tic_t)READUINT32(save->p);

		players[i].tulip = (tic_t)READUINT32(save->p);
		players[i].tuliplaunch = (tic_t)READUINT32(save->p);
		players[i].tulipbuf = (tic_t)READUINT32(save->p);

		// respawnvars_t
		players[i].respawn.state = READUINT8(save->p);
		players[i].respawn.wp = (waypoint_t *)(size_t)READUINT32(save->p);
		players[i].respawn.pointx = READFIXED(save->p);
		players[i].respawn.pointy = READFIXED(save->p);
		players[i].respawn.pointz = READFIXED(save->p);
		players[i].respawn.flip = (boolean)READUINT8(save->p);
		players[i].respawn.timer = READUINT32(save->p);
		players[i].respawn.airtimer = READUINT32(save->p);
		players[i].respawn.distanceleft = READUINT32(save->p);
		players[i].respawn.dropdash = READUINT32(save->p);
		players[i].respawn.truedeath = READUINT8(save->p);
		players[i].respawn.manual = READUINT8(save->p);
		players[i].respawn.fast = READUINT8(save->p);
		players[i].respawn.returnspeed = READUINT32(save->p);

		// botvars_t
		players[i].bot = READUINT8(save->p);
		players[i].botvars.difficulty = READUINT8(save->p);
		players[i].botvars.diffincrease = READUINT8(save->p);
		players[i].botvars.rival = (boolean)READUINT8(save->p);
		players[i].botvars.rubberband = READFIXED(save->p);
		players[i].botvars.itemdelay = READUINT32(save->p);
		players[i].botvars.itemconfirm = READUINT32(save->p);
		players[i].botvars.turnconfirm = READSINT8(save->p);
		players[i].botvars.spindashconfirm = READUINT32(save->p);
		players[i].botvars.respawnconfirm = READUINT32(save->p);
		players[i].botvars.roulettePriority = READUINT8(save->p);
		players[i].botvars.rouletteTimeout = READUINT32(save->p);

		// itemroulette_t
		players[i].itemRoulette.active = (boolean)READUINT8(save->p);

#ifdef ITEM_LIST_SIZE
		players[i].itemRoulette.itemListLen = (size_t)READUINT32(save->p);

		for (q = 0; q < ITEM_LIST_SIZE; q++)
		{
			players[i].itemRoulette.itemList[q] = READSINT8(save->p);
		}
#else
		players[i].itemRoulette.itemListCap = (size_t)READUINT32(save->p);
		players[i].itemRoulette.itemListLen = (size_t)READUINT32(save->p);

		if (players[i].itemRoulette.itemListCap > 0)
		{
			if (players[i].itemRoulette.itemList == NULL)
			{
				players[i].itemRoulette.itemList = Z_Calloc(
					sizeof(SINT8) * players[i].itemRoulette.itemListCap,
					PU_STATIC,
					&players[i].itemRoulette.itemList
				);
			}
			else
			{
				players[i].itemRoulette.itemList = Z_Realloc(
					players[i].itemRoulette.itemList,
					sizeof(SINT8) * players[i].itemRoulette.itemListCap,
					PU_STATIC,
					&players[i].itemRoulette.itemList
				);
			}

			if (players[i].itemRoulette.itemList == NULL)
			{
				I_Error("Not enough memory for item roulette list\n");
			}

			for (q = 0; q < players[i].itemRoulette.itemListLen; q++)
			{
				players[i].itemRoulette.itemList[q] = READSINT8(save->p);
			}
		}
#endif

		players[i].itemRoulette.useOdds = READUINT8(save->p);
		players[i].itemRoulette.dist = READUINT32(save->p);
		players[i].itemRoulette.index = (size_t)READUINT32(save->p);
		players[i].itemRoulette.sound = READUINT8(save->p);
		players[i].itemRoulette.speed = (tic_t)READUINT32(save->p);
		players[i].itemRoulette.tics = (tic_t)READUINT32(save->p);
		players[i].itemRoulette.elapsed = (tic_t)READUINT32(save->p);
		players[i].itemRoulette.eggman = (boolean)READUINT8(save->p);
		players[i].itemRoulette.ringbox = (boolean)READUINT8(save->p);
		players[i].itemRoulette.autoroulette = (boolean)READUINT8(save->p);
		players[i].itemRoulette.reserved = READUINT8(save->p);

		// sonicloopsvars_t
		players[i].loop.radius = READFIXED(save->p);
		players[i].loop.revolution = READFIXED(save->p);
		players[i].loop.min_revolution = READFIXED(save->p);
		players[i].loop.max_revolution = READFIXED(save->p);
		players[i].loop.yaw = READANGLE(save->p);
		players[i].loop.origin.x = READFIXED(save->p);
		players[i].loop.origin.y = READFIXED(save->p);
		players[i].loop.origin.z = READFIXED(save->p);
		players[i].loop.origin_shift.x = READFIXED(save->p);
		players[i].loop.origin_shift.y = READFIXED(save->p);
		players[i].loop.shift.x = READFIXED(save->p);
		players[i].loop.shift.y = READFIXED(save->p);
		players[i].loop.flip = READUINT8(save->p);

		// sonicloopcamvars_t
		players[i].loop.camera.enter_tic = READUINT32(save->p);
		players[i].loop.camera.exit_tic = READUINT32(save->p);
		players[i].loop.camera.zoom_in_speed = READUINT32(save->p);
		players[i].loop.camera.zoom_out_speed = READUINT32(save->p);
		players[i].loop.camera.dist = READFIXED(save->p);
		players[i].loop.camera.pan = READANGLE(save->p);
		players[i].loop.camera.pan_speed = READFIXED(save->p);
		players[i].loop.camera.pan_accel = READUINT32(save->p);
		players[i].loop.camera.pan_back = READUINT32(save->p);

		// ACS has read access to this, so it has to be net-communicated.
		// It is the ONLY roundcondition that is sent over the wire and I'd like it to stay that way.
		players[i].roundconditions.unlocktriggers = READUINT32(save->p);

		// powerupvars_t
		players[i].powerup.superTimer = READUINT16(save->p);
		players[i].powerup.barrierTimer = READUINT16(save->p);
		players[i].powerup.rhythmBadgeTimer = READUINT16(save->p);

		// level_tally_t
		players[i].tally.active = READUINT8(save->p);
		if (players[i].tally.active)
		{
			players[i].tally.owner = &players[i];
			players[i].tally.gt = READUINT16(save->p);
			players[i].tally.gotThru = (boolean)READUINT8(save->p);

			READSTRINGN(save->p, players[i].tally.header, 63);
			players[i].tally.header[63] = '\0';

			players[i].tally.showRoundNum = (boolean)READUINT8(save->p);
			players[i].tally.gradeVoice = READINT32(save->p);

			players[i].tally.time = READINT32(save->p);
			players[i].tally.ringPool = READUINT16(save->p);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				players[i].tally.stats[q] = READINT32(save->p);

			players[i].tally.position = READUINT8(save->p);
			players[i].tally.numPlayers = READUINT8(save->p);
			players[i].tally.rings = READUINT8(save->p);
			players[i].tally.laps = READUINT16(save->p);
			players[i].tally.totalLaps = READUINT16(save->p);
			players[i].tally.prisons = READUINT16(save->p);
			players[i].tally.totalPrisons = READUINT16(save->p);
			players[i].tally.points = READINT32(save->p);
			players[i].tally.pointLimit = READINT32(save->p);
			players[i].tally.powerStones = READUINT8(save->p);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				players[i].tally.bonuses[q] = READINT32(save->p);
			players[i].tally.rank = READINT32(save->p);

			players[i].tally.state = READINT32(save->p);
			players[i].tally.hudSlide = READINT32(save->p);
			players[i].tally.delay = READINT32(save->p);
			players[i].tally.transition = READINT32(save->p);
			players[i].tally.transitionTime = READINT32(save->p);
			players[i].tally.lines = READUINT8(save->p);
			players[i].tally.lineCount = READUINT8(save->p);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				players[i].tally.displayStat[q] = READINT32(save->p);
			for (q = 0; q < TALLY_WINDOW_SIZE; q++)
				players[i].tally.displayBonus[q] = READINT32(save->p);
			players[i].tally.tickSound = READUINT8(save->p);
			players[i].tally.xtraBlink = READUINT8(save->p);
			players[i].tally.showGrade = (boolean)READUINT8(save->p);
			players[i].tally.done = (boolean)READUINT8(save->p);
		}

		// icecubevars_t
		players[i].icecube.hitat = READUINT32(save->p);
		players[i].icecube.frozen = READUINT8(save->p);
		players[i].icecube.wiggle = READUINT8(save->p);
		players[i].icecube.frozenat = READUINT32(save->p);
		players[i].icecube.shaketimer = READUINT8(save->p);

		// darkness
		players[i].darkness_start = READUINT32(save->p);
		players[i].darkness_end = READUINT32(save->p);

		//players[i].viewheight = P_GetPlayerViewHeight(players[i]); // scale cannot be factored in at this point
	}

	TracyCZoneEnd(__zone);
}

static void P_NetArchiveParties(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i, k;
	UINT8 partySize;

	WRITEUINT32(save->p, ARCHIVEBLOCK_PARTIES);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		partySize = G_PartySize(i);

		WRITEUINT8(save->p, partySize);

		for (k = 0; k < partySize; ++k)
		{
			WRITEUINT8(save->p, G_PartyMember(i, k));
		}
	}

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveParties(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i, k;
	UINT8 partySize;

	if (READUINT32(save->p) != ARCHIVEBLOCK_PARTIES)
		I_Error("Bad $$$.sav at archive block Parties");

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		G_DestroyParty(i);
		G_BuildLocalSplitscreenParty(i);
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		partySize = READUINT8(save->p);

		for (k = 0; k < partySize; ++k)
		{
			G_JoinParty(i, READUINT8(save->p));
		}
	}

	TracyCZoneEnd(__zone);
}

static void P_NetArchiveRoundQueue(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	UINT8 i;
	WRITEUINT32(save->p, ARCHIVEBLOCK_ROUNDQUEUE);

	WRITEUINT8(save->p, roundqueue.position);
	WRITEUINT8(save->p, roundqueue.size);
	WRITEUINT8(save->p, roundqueue.roundnum);

	for (i = 0; i < roundqueue.size; i++)
	{
		//WRITEUINT16(save->p, roundqueue.entries[i].mapnum);
		/* NOPE! Clients do not need to know what is in the roundqueue.
		This disincentivises cheaty clients in future tournament environments.
		~toast 080423 */
		WRITEUINT8(save->p, roundqueue.entries[i].gametype);
		WRITEUINT8(save->p, (UINT8)roundqueue.entries[i].encore);
		WRITEUINT8(save->p, (UINT8)roundqueue.entries[i].rankrestricted);
		WRITEUINT8(save->p, (UINT8)roundqueue.entries[i].overridden);
	}

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveRoundQueue(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	UINT8 i;
	if (READUINT32(save->p) != ARCHIVEBLOCK_ROUNDQUEUE)
		I_Error("Bad $$$.sav at archive block Round-queue");

	memset(&roundqueue, 0, sizeof(struct roundqueue));

	roundqueue.position = READUINT8(save->p);
	roundqueue.size = READUINT8(save->p);
	if (roundqueue.size > ROUNDQUEUE_MAX)
		I_Error("Bad $$$.sav at illegitimate roundqueue size");

	roundqueue.roundnum = READUINT8(save->p);

	for (i = 0; i < roundqueue.size; i++)
	{
		roundqueue.entries[i].mapnum = 0; // TEST RUN -- dummy, has to be < nummapheaders
		roundqueue.entries[i].gametype = READUINT8(save->p);
		roundqueue.entries[i].encore = (boolean)READUINT8(save->p);
		roundqueue.entries[i].rankrestricted = (boolean)READUINT8(save->p);
		roundqueue.entries[i].overridden = (boolean)READUINT8(save->p);
	}

	TracyCZoneEnd(__zone);
}

static void P_NetArchiveZVote(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i;

	WRITEUINT32(save->p, ARCHIVEBLOCK_ZVOTE);
	WRITEUINT8(save->p, g_midVote.active);

	if (g_midVote.active == true)
	{
		WRITEUINT8(
			save->p,
			(g_midVote.caller != NULL) ? (g_midVote.caller - players) : UINT8_MAX
		);

		WRITEUINT8(
			save->p,
			(g_midVote.victim != NULL) ? (g_midVote.victim - players) : UINT8_MAX
		);

		for (i = 0; i < MAXPLAYERS; i++)
		{
			WRITEUINT8(save->p, g_midVote.votes[i]);
		}

		WRITEUINT8(save->p, g_midVote.type);
		WRITEINT32(save->p, g_midVote.variable);

		WRITEUINT32(save->p, g_midVote.time);

		WRITEUINT32(save->p, g_midVote.end);
		WRITEUINT8(save->p, g_midVote.endVotes);
		WRITEUINT8(save->p, g_midVote.endRequired);
	}

	WRITEUINT32(save->p, g_midVote.delay);

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveZVote(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i;

	if (READUINT32(save->p) != ARCHIVEBLOCK_ZVOTE)
	{
		I_Error("Bad $$$.sav at archive block Z-Vote");
	}

	K_ResetMidVote();
	g_midVote.active = (boolean)READUINT8(save->p);

	if (g_midVote.active == true)
	{
		UINT8 callerID = READUINT8(save->p);
		UINT8 victimID = READUINT8(save->p);

		if (callerID < MAXPLAYERS)
		{
			g_midVote.caller = &players[callerID];
		}

		if (victimID < MAXPLAYERS)
		{
			g_midVote.victim = &players[victimID];
		}

		for (i = 0; i < MAXPLAYERS; i++)
		{
			g_midVote.votes[i] = (boolean)READUINT8(save->p);
		}

		g_midVote.type = READUINT8(save->p);
		g_midVote.variable = READINT32(save->p);

		g_midVote.time = (tic_t)READUINT32(save->p);
		g_midVote.end = (tic_t)READUINT32(save->p);
		g_midVote.endVotes = READUINT8(save->p);
		g_midVote.endRequired = READUINT8(save->p);
	}

	g_midVote.delay = (tic_t)READUINT32(save->p);

	TracyCZoneEnd(__zone);
}

///
/// Colormaps
///

static extracolormap_t *net_colormaps = NULL;
static UINT32 num_net_colormaps = 0;
static UINT32 num_ffloors = 0; // for loading

// Copypasta from r_data.c AddColormapToList
// But also check for equality and return the matching index
static UINT32 CheckAddNetColormapToList(extracolormap_t *extra_colormap)
{
	extracolormap_t *exc, *exc_prev = NULL;
	UINT32 i = 0;

	if (!net_colormaps)
	{
		net_colormaps = R_CopyColormap(extra_colormap, false);
		net_colormaps->next = 0;
		net_colormaps->prev = 0;
		num_net_colormaps = i+1;
		return i;
	}

	for (exc = net_colormaps; exc; exc_prev = exc, exc = exc->next)
	{
		if (R_CheckEqualColormaps(exc, extra_colormap, true, true, true))
			return i;
		i++;
	}

	exc_prev->next = R_CopyColormap(extra_colormap, false);
	extra_colormap->prev = exc_prev;
	extra_colormap->next = 0;

	num_net_colormaps = i+1;
	return i;
}

static extracolormap_t *GetNetColormapFromList(UINT32 index)
{
	// For loading, we have to be tricky:
	// We load the sectors BEFORE knowing the colormap values
	// So if an index doesn't exist, fill our list with dummy colormaps
	// until we get the index we want
	// Then when we load the color data, we set up the dummy colormaps

	extracolormap_t *exc, *last_exc = NULL;
	UINT32 i = 0;

	if (!net_colormaps) // initialize our list
		net_colormaps = R_CreateDefaultColormap(false);

	for (exc = net_colormaps; exc; last_exc = exc, exc = exc->next)
	{
		if (i++ == index)
			return exc;
	}


	// LET'S HOPE that index is a sane value, because we create up to [index]
	// entries in net_colormaps. At this point, we don't know
	// what the total colormap count is
	if (index >= numsectors*3 + num_ffloors)
		// if every sector had a unique colormap change AND a fade color thinker which has two colormap entries
		// AND every ffloor had a fade FOF thinker with one colormap entry
		I_Error("Colormap %d from server is too high for sectors %d", index, (UINT32)numsectors);

	// our index doesn't exist, so just make the entry
	for (; i <= index; i++)
	{
		exc = R_CreateDefaultColormap(false);
		if (last_exc)
			last_exc->next = exc;
		exc->prev = last_exc;
		exc->next = NULL;
		last_exc = exc;
	}
	return exc;
}

static void ClearNetColormaps(void)
{
	// We're actually Z_Freeing each entry here,
	// so don't call this in P_NetUnArchiveColormaps (where entries will be used in-game)
	extracolormap_t *exc, *exc_next;

	for (exc = net_colormaps; exc; exc = exc_next)
	{
		exc_next = exc->next;
		Z_Free(exc);
	}
	num_net_colormaps = 0;
	num_ffloors = 0;
	net_colormaps = NULL;
}

static void P_NetArchiveColormaps(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	// We save and then we clean up our colormap mess
	extracolormap_t *exc, *exc_next;
	UINT32 i = 0;
	WRITEUINT32(save->p, num_net_colormaps); // save for safety

	for (exc = net_colormaps; i < num_net_colormaps; i++, exc = exc_next)
	{
		// We must save num_net_colormaps worth of data
		// So fill non-existent entries with default.
		if (!exc)
			exc = R_CreateDefaultColormap(false);

		WRITEUINT8(save->p, exc->fadestart);
		WRITEUINT8(save->p, exc->fadeend);
		WRITEUINT8(save->p, exc->flags);

		WRITEINT32(save->p, exc->rgba);
		WRITEINT32(save->p, exc->fadergba);

#ifdef EXTRACOLORMAPLUMPS
		WRITESTRINGN(save->p, exc->lumpname, 9);
#endif

		exc_next = exc->next;
		Z_Free(exc); // don't need anymore
	}

	num_net_colormaps = 0;
	num_ffloors = 0;
	net_colormaps = NULL;

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveColormaps(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	// When we reach this point, we already populated our list with
	// dummy colormaps. Now that we are loading the color data,
	// set up the dummies.
	extracolormap_t *exc, *existing_exc, *exc_next = NULL;
	UINT32 i = 0;

	num_net_colormaps = READUINT32(save->p);

	for (exc = net_colormaps; i < num_net_colormaps; i++, exc = exc_next)
	{
		UINT8 fadestart, fadeend, flags;
		INT32 rgba, fadergba;
#ifdef EXTRACOLORMAPLUMPS
		char lumpname[9];
#endif

		fadestart = READUINT8(save->p);
		fadeend = READUINT8(save->p);
		flags = READUINT8(save->p);

		rgba = READINT32(save->p);
		fadergba = READINT32(save->p);

#ifdef EXTRACOLORMAPLUMPS
		READSTRINGN(save->p, lumpname, 9);

		if (lumpname[0])
		{
			if (!exc)
				// no point making a new entry since nothing points to it,
				// but we needed to read the data so now continue
				continue;

			exc_next = exc->next; // this gets overwritten during our operations here, so get it now
			existing_exc = R_ColormapForName(lumpname);
			*exc = *existing_exc;
			R_AddColormapToList(exc); // see HACK note below on why we're adding duplicates
			continue;
		}
#endif

		if (!exc)
			// no point making a new entry since nothing points to it,
			// but we needed to read the data so now continue
			continue;

		exc_next = exc->next; // this gets overwritten during our operations here, so get it now

		exc->fadestart = fadestart;
		exc->fadeend = fadeend;
		exc->flags = flags;

		exc->rgba = rgba;
		exc->fadergba = fadergba;

#ifdef EXTRACOLORMAPLUMPS
		exc->lump = LUMPERROR;
		exc->lumpname[0] = 0;
#endif

		existing_exc = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags);

		if (existing_exc)
			exc->colormap = existing_exc->colormap;
		else
			// CONS_Debug(DBG_RENDER, "Creating Colormap: rgba(%d,%d,%d,%d) fadergba(%d,%d,%d,%d)\n",
			// 	R_GetRgbaR(rgba), R_GetRgbaG(rgba), R_GetRgbaB(rgba), R_GetRgbaA(rgba),
			//	R_GetRgbaR(fadergba), R_GetRgbaG(fadergba), R_GetRgbaB(fadergba), R_GetRgbaA(fadergba));
			exc->colormap = R_CreateLightTable(exc);

		// HACK: If this dummy is a duplicate, we're going to add it
		// to the extra_colormaps list anyway. I think this is faster
		// than going through every loaded sector and correcting their
		// colormap address to the pre-existing one, PER net_colormap entry
		R_AddColormapToList(exc);

		if (i < num_net_colormaps-1 && !exc_next)
			exc_next = R_CreateDefaultColormap(false);
	}

	// if we still have a valid net_colormap after iterating up to num_net_colormaps,
	// some sector had a colormap index higher than num_net_colormaps. We done goofed or $$$ was corrupted.
	// In any case, add them to the colormap list too so that at least the sectors' colormap
	// addresses are valid and accounted properly
	if (exc_next)
	{
		existing_exc = R_GetDefaultColormap();
		for (exc = exc_next; exc; exc = exc->next)
		{
			exc->colormap = existing_exc->colormap; // all our dummies are default values
			R_AddColormapToList(exc);
		}
	}

	// Don't need these anymore
	num_net_colormaps = 0;
	num_ffloors = 0;
	net_colormaps = NULL;

	TracyCZoneEnd(__zone);
}

///
/// World Archiving
///

#define SD_FLOORHT  0x01
#define SD_CEILHT   0x02
#define SD_FLOORPIC 0x04
#define SD_CEILPIC  0x08
#define SD_LIGHT    0x10
#define SD_SPECIAL  0x20
#define SD_DIFF2    0x40
#define SD_FFLOORS  0x80

// diff2 flags
#define SD_FXOFFS    0x01
#define SD_FYOFFS    0x02
#define SD_CXOFFS    0x04
#define SD_CYOFFS    0x08
#define SD_FLOORANG  0x10
#define SD_CEILANG   0x20
#define SD_TAG       0x40
#define SD_DIFF3     0x80

// diff3 flags
#define SD_TAGLIST   0x01
#define SD_COLORMAP  0x02
#define SD_CRUMBLESTATE 0x04
#define SD_FLOORLIGHT 0x08
#define SD_CEILLIGHT 0x10
#define SD_FLAG      0x20
#define SD_SPECIALFLAG 0x40
#define SD_DIFF4     0x80

//diff4 flags
#define SD_DAMAGETYPE 0x01
#define SD_TRIGGERTAG 0x02
#define SD_TRIGGERER  0x04
#define SD_GRAVITY    0x08
#define SD_ACTION     0x10
#define SD_ARGS       0x20
#define SD_STRINGARGS 0x40
#define SD_DIFF5      0x80

//diff5 flags
#define SD_ACTIVATION 0x01
#define SD_BOTCONTROLLER 0x02

static boolean P_SectorArgsEqual(const sector_t *sc, const sector_t *spawnsc)
{
	UINT8 i;
	for (i = 0; i < NUM_SCRIPT_ARGS; i++)
		if (sc->args[i] != spawnsc->args[i])
			return false;

	return true;
}

static boolean P_SectorStringArgsEqual(const sector_t *sc, const sector_t *spawnsc)
{
	UINT8 i;
	for (i = 0; i < NUM_SCRIPT_STRINGARGS; i++)
	{
		if (!sc->stringargs[i])
			return !spawnsc->stringargs[i];

		if (strcmp(sc->stringargs[i], spawnsc->stringargs[i]))
			return false;
	}

	return true;
}

#define LD_FLAG     0x01
#define LD_SPECIAL  0x02
#define LD_CLLCOUNT 0x04
#define LD_S1TEXOFF 0x08
#define LD_S1TOPTEX 0x10
#define LD_S1BOTTEX 0x20
#define LD_S1MIDTEX 0x40
#define LD_DIFF2    0x80

// diff2 flags
#define LD_S2TEXOFF      0x01
#define LD_S2TOPTEX      0x02
#define LD_S2BOTTEX      0x04
#define LD_S2MIDTEX      0x08
#define LD_ARGS          0x10
#define LD_STRINGARGS    0x20
#define LD_EXECUTORDELAY 0x40
#define LD_DIFF3         0x80

// diff3 flags
#define LD_ACTIVATION    0x01

static boolean P_LineArgsEqual(const line_t *li, const line_t *spawnli)
{
	UINT8 i;
	for (i = 0; i < NUM_SCRIPT_ARGS; i++)
		if (li->args[i] != spawnli->args[i])
			return false;

	return true;
}

static boolean P_LineStringArgsEqual(const line_t *li, const line_t *spawnli)
{
	UINT8 i;
	for (i = 0; i < NUM_SCRIPT_STRINGARGS; i++)
	{
		if (!li->stringargs[i])
			return !spawnli->stringargs[i];

		if (strcmp(li->stringargs[i], spawnli->stringargs[i]))
			return false;
	}

	return true;
}

#define FD_FLAGS 0x01
#define FD_ALPHA 0x02

// Check if any of the sector's FOFs differ from how they spawned
static boolean CheckFFloorDiff(const sector_t *ss)
{
	ffloor_t *rover;

	for (rover = ss->ffloors; rover; rover = rover->next)
	{
		if (rover->fofflags != rover->spawnflags
		|| rover->alpha != rover->spawnalpha)
			{
				return true; // we found an FOF that changed!
				// don't bother checking for more, we do that later
			}
	}
	return false;
}

// Special case: save the stats of all modified ffloors along with their ffloor "number"s
// we don't bother with ffloors that haven't changed, that would just add to savegame even more than is really needed
static void ArchiveFFloors(savebuffer_t *save, const sector_t *ss)
{
	size_t j = 0; // ss->ffloors is saved as ffloor #0, ss->ffloors->next is #1, etc
	ffloor_t *rover;
	UINT8 fflr_diff;
	for (rover = ss->ffloors; rover; rover = rover->next)
	{
		fflr_diff = 0; // reset diff flags
		if (rover->fofflags != rover->spawnflags)
			fflr_diff |= FD_FLAGS;
		if (rover->alpha != rover->spawnalpha)
			fflr_diff |= FD_ALPHA;

		if (fflr_diff)
		{
			WRITEUINT16(save->p, j); // save ffloor "number"
			WRITEUINT8(save->p, fflr_diff);
			if (fflr_diff & FD_FLAGS)
				WRITEUINT32(save->p, rover->fofflags);
			if (fflr_diff & FD_ALPHA)
				WRITEINT16(save->p, rover->alpha);
		}
		j++;
	}
	WRITEUINT16(save->p, 0xffff);
}

static void UnArchiveFFloors(savebuffer_t *save, const sector_t *ss)
{
	UINT16 j = 0; // number of current ffloor in loop
	UINT16 fflr_i; // saved ffloor "number" of next modified ffloor
	UINT16 fflr_diff; // saved ffloor diff
	ffloor_t *rover;

	rover = ss->ffloors;
	if (!rover) // it is assumed sectors[i].ffloors actually exists, but just in case...
		I_Error("Sector does not have any ffloors!");

	fflr_i = READUINT16(save->p); // get first modified ffloor's number ready
	for (;;) // for some reason the usual for (rover = x; ...) thing doesn't work here?
	{
		if (fflr_i == 0xffff) // end of modified ffloors list, let's stop already
			break;
		// should NEVER need to be checked
		//if (rover == NULL)
			//break;
		if (j != fflr_i) // this ffloor was not modified
		{
			j++;
			rover = rover->next;
			continue;
		}

		fflr_diff = READUINT8(save->p);

		if (fflr_diff & FD_FLAGS)
			rover->fofflags = READUINT32(save->p);
		if (fflr_diff & FD_ALPHA)
			rover->alpha = READINT16(save->p);

		fflr_i = READUINT16(save->p); // get next ffloor "number" ready

		j++;
		rover = rover->next;
	}
}

static void ArchiveSectors(savebuffer_t *save)
{
	size_t i, j;
	const sector_t *ss = sectors;
	const sector_t *spawnss = spawnsectors;
	UINT8 diff, diff2, diff3, diff4, diff5;

	for (i = 0; i < numsectors; i++, ss++, spawnss++)
	{
		diff = diff2 = diff3 = diff4 = diff5 = 0;
		if (ss->floorheight != spawnss->floorheight)
			diff |= SD_FLOORHT;
		if (ss->ceilingheight != spawnss->ceilingheight)
			diff |= SD_CEILHT;
		//
		// flats
		//
		if (ss->floorpic != spawnss->floorpic)
			diff |= SD_FLOORPIC;
		if (ss->ceilingpic != spawnss->ceilingpic)
			diff |= SD_CEILPIC;

		if (ss->lightlevel != spawnss->lightlevel)
			diff |= SD_LIGHT;
		if (ss->special != spawnss->special)
			diff |= SD_SPECIAL;

		if (ss->floor_xoffs != spawnss->floor_xoffs)
			diff2 |= SD_FXOFFS;
		if (ss->floor_yoffs != spawnss->floor_yoffs)
			diff2 |= SD_FYOFFS;
		if (ss->ceiling_xoffs != spawnss->ceiling_xoffs)
			diff2 |= SD_CXOFFS;
		if (ss->ceiling_yoffs != spawnss->ceiling_yoffs)
			diff2 |= SD_CYOFFS;
		if (ss->floorpic_angle != spawnss->floorpic_angle)
			diff2 |= SD_FLOORANG;
		if (ss->ceilingpic_angle != spawnss->ceilingpic_angle)
			diff2 |= SD_CEILANG;

		if (!Tag_Compare(&ss->tags, &spawnss->tags))
			diff2 |= SD_TAG;

		if (ss->extra_colormap != spawnss->extra_colormap)
			diff3 |= SD_COLORMAP;
		if (ss->crumblestate)
			diff3 |= SD_CRUMBLESTATE;

		if (ss->floorlightlevel != spawnss->floorlightlevel || ss->floorlightabsolute != spawnss->floorlightabsolute)
			diff3 |= SD_FLOORLIGHT;
		if (ss->ceilinglightlevel != spawnss->ceilinglightlevel || ss->ceilinglightabsolute != spawnss->ceilinglightabsolute)
			diff3 |= SD_CEILLIGHT;
		if (ss->flags != spawnss->flags)
			diff3 |= SD_FLAG;
		if (ss->specialflags != spawnss->specialflags)
			diff3 |= SD_SPECIALFLAG;
		if (ss->damagetype != spawnss->damagetype)
			diff4 |= SD_DAMAGETYPE;
		if (ss->triggertag != spawnss->triggertag)
			diff4 |= SD_TRIGGERTAG;
		if (ss->triggerer != spawnss->triggerer)
			diff4 |= SD_TRIGGERER;
		if (ss->gravity != spawnss->gravity)
			diff4 |= SD_GRAVITY;

		if (ss->action != spawnss->action)
			diff4 |= SD_ACTION;
		if (!P_SectorArgsEqual(ss, spawnss))
			diff4 |= SD_ARGS;
		if (!P_SectorStringArgsEqual(ss, spawnss))
			diff4 |= SD_STRINGARGS;
		if (ss->activation != spawnss->activation)
			diff5 |= SD_ACTIVATION;
		if (ss->botController.trick != spawnss->botController.trick
			|| ss->botController.flags != spawnss->botController.flags
			|| ss->botController.forceAngle != spawnss->botController.forceAngle)
		{
			diff5 |= SD_BOTCONTROLLER;
		}

		if (ss->ffloors && CheckFFloorDiff(ss))
			diff |= SD_FFLOORS;

		if (diff5)
			diff4 |= SD_DIFF5;

		if (diff4)
			diff3 |= SD_DIFF4;

		if (diff3)
			diff2 |= SD_DIFF3;

		if (diff2)
			diff |= SD_DIFF2;

		if (diff)
		{
			WRITEUINT16(save->p, i);
			WRITEUINT8(save->p, diff);
			if (diff & SD_DIFF2)
				WRITEUINT8(save->p, diff2);
			if (diff2 & SD_DIFF3)
				WRITEUINT8(save->p, diff3);
			if (diff3 & SD_DIFF4)
				WRITEUINT8(save->p, diff4);
			if (diff4 & SD_DIFF5)
				WRITEUINT8(save->p, diff5);
			if (diff & SD_FLOORHT)
				WRITEFIXED(save->p, ss->floorheight);
			if (diff & SD_CEILHT)
				WRITEFIXED(save->p, ss->ceilingheight);
			if (diff & SD_FLOORPIC)
				WRITEMEM(save->p, levelflats[ss->floorpic].name, 8);
			if (diff & SD_CEILPIC)
				WRITEMEM(save->p, levelflats[ss->ceilingpic].name, 8);
			if (diff & SD_LIGHT)
				WRITEINT16(save->p, ss->lightlevel);
			if (diff & SD_SPECIAL)
				WRITEINT16(save->p, ss->special);
			if (diff2 & SD_FXOFFS)
				WRITEFIXED(save->p, ss->floor_xoffs);
			if (diff2 & SD_FYOFFS)
				WRITEFIXED(save->p, ss->floor_yoffs);
			if (diff2 & SD_CXOFFS)
				WRITEFIXED(save->p, ss->ceiling_xoffs);
			if (diff2 & SD_CYOFFS)
				WRITEFIXED(save->p, ss->ceiling_yoffs);
			if (diff2 & SD_FLOORANG)
				WRITEANGLE(save->p, ss->floorpic_angle);
			if (diff2 & SD_CEILANG)
				WRITEANGLE(save->p, ss->ceilingpic_angle);
			if (diff2 & SD_TAG)
			{
				WRITEUINT32(save->p, ss->tags.count);
				for (j = 0; j < ss->tags.count; j++)
					WRITEINT16(save->p, ss->tags.tags[j]);
			}

			if (diff3 & SD_COLORMAP)
				WRITEUINT32(save->p, CheckAddNetColormapToList(ss->extra_colormap));
					// returns existing index if already added, or appends to net_colormaps and returns new index
			if (diff3 & SD_CRUMBLESTATE)
				WRITEINT32(save->p, ss->crumblestate);
			if (diff3 & SD_FLOORLIGHT)
			{
				WRITEINT16(save->p, ss->floorlightlevel);
				WRITEUINT8(save->p, ss->floorlightabsolute);
			}
			if (diff3 & SD_CEILLIGHT)
			{
				WRITEINT16(save->p, ss->ceilinglightlevel);
				WRITEUINT8(save->p, ss->ceilinglightabsolute);
			}
			if (diff3 & SD_FLAG)
				WRITEUINT32(save->p, ss->flags);
			if (diff3 & SD_SPECIALFLAG)
				WRITEUINT32(save->p, ss->specialflags);
			if (diff4 & SD_DAMAGETYPE)
				WRITEUINT8(save->p, ss->damagetype);
			if (diff4 & SD_TRIGGERTAG)
				WRITEINT16(save->p, ss->triggertag);
			if (diff4 & SD_TRIGGERER)
				WRITEUINT8(save->p, ss->triggerer);
			if (diff4 & SD_GRAVITY)
				WRITEFIXED(save->p, ss->gravity);

			if (diff4 & SD_ACTION)
				WRITEINT16(save->p, ss->action);
			if (diff4 & SD_ARGS)
			{
				for (j = 0; j < NUM_SCRIPT_ARGS; j++)
					WRITEINT32(save->p, ss->args[j]);
			}
			if (diff4 & SD_STRINGARGS)
			{
				for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
				{
					size_t len, k;

					if (!ss->stringargs[j])
					{
						WRITEINT32(save->p, 0);
						continue;
					}

					len = strlen(ss->stringargs[j]);
					WRITEINT32(save->p, len);
					for (k = 0; k < len; k++)
						WRITECHAR(save->p, ss->stringargs[j][k]);
				}
			}
			if (diff5 & SD_ACTIVATION)
				WRITEUINT32(save->p, ss->activation);
			if (diff5 & SD_BOTCONTROLLER)
			{
				WRITEUINT8(save->p, ss->botController.trick);
				WRITEUINT32(save->p, ss->botController.flags);
				WRITEANGLE(save->p, ss->botController.forceAngle);
			}

			if (diff & SD_FFLOORS)
				ArchiveFFloors(save, ss);
		}
	}

	WRITEUINT16(save->p, 0xffff);
}

static void UnArchiveSectors(savebuffer_t *save)
{
	UINT16 i, j;
	UINT8 diff, diff2, diff3, diff4, diff5;
	for (;;)
	{
		i = READUINT16(save->p);

		if (i == 0xffff)
			break;

		if (i > numsectors)
			I_Error("Invalid sector number %u from server (expected end at %s)", i, sizeu1(numsectors));

		diff = READUINT8(save->p);
		if (diff & SD_DIFF2)
			diff2 = READUINT8(save->p);
		else
			diff2 = 0;
		if (diff2 & SD_DIFF3)
			diff3 = READUINT8(save->p);
		else
			diff3 = 0;
		if (diff3 & SD_DIFF4)
			diff4 = READUINT8(save->p);
		else
			diff4 = 0;

		if (diff4 & SD_DIFF5)
			diff5 = READUINT8(save->p);
		else
			diff5 = 0;

		if (diff & SD_FLOORHT)
			sectors[i].floorheight = READFIXED(save->p);
		if (diff & SD_CEILHT)
			sectors[i].ceilingheight = READFIXED(save->p);
		if (diff & SD_FLOORPIC)
		{
			sectors[i].floorpic = P_AddLevelFlatRuntime((char *)save->p);
			save->p += 8;
		}
		if (diff & SD_CEILPIC)
		{
			sectors[i].ceilingpic = P_AddLevelFlatRuntime((char *)save->p);
			save->p += 8;
		}
		if (diff & SD_LIGHT)
			sectors[i].lightlevel = READINT16(save->p);
		if (diff & SD_SPECIAL)
			sectors[i].special = READINT16(save->p);

		if (diff2 & SD_FXOFFS)
			sectors[i].floor_xoffs = READFIXED(save->p);
		if (diff2 & SD_FYOFFS)
			sectors[i].floor_yoffs = READFIXED(save->p);
		if (diff2 & SD_CXOFFS)
			sectors[i].ceiling_xoffs = READFIXED(save->p);
		if (diff2 & SD_CYOFFS)
			sectors[i].ceiling_yoffs = READFIXED(save->p);
		if (diff2 & SD_FLOORANG)
			sectors[i].floorpic_angle  = READANGLE(save->p);
		if (diff2 & SD_CEILANG)
			sectors[i].ceilingpic_angle = READANGLE(save->p);
		if (diff2 & SD_TAG)
		{
			size_t ncount = READUINT32(save->p);

			// Remove entries from global lists.
			for (j = 0; j < sectors[i].tags.count; j++)
				Taggroup_Remove(tags_sectors, sectors[i].tags.tags[j], i);

			// Reallocate if size differs.
			if (ncount != sectors[i].tags.count)
			{
				sectors[i].tags.count = ncount;
				sectors[i].tags.tags = Z_Realloc(sectors[i].tags.tags, ncount*sizeof(mtag_t), PU_LEVEL, NULL);
			}

			for (j = 0; j < ncount; j++)
				sectors[i].tags.tags[j] = READINT16(save->p);

			// Add new entries.
			for (j = 0; j < sectors[i].tags.count; j++)
				Taggroup_Remove(tags_sectors, sectors[i].tags.tags[j], i);
		}


		if (diff3 & SD_COLORMAP)
			sectors[i].extra_colormap = GetNetColormapFromList(READUINT32(save->p));
		if (diff3 & SD_CRUMBLESTATE)
			sectors[i].crumblestate = READINT32(save->p);
		if (diff3 & SD_FLOORLIGHT)
		{
			sectors[i].floorlightlevel = READINT16(save->p);
			sectors[i].floorlightabsolute = READUINT8(save->p);
		}
		if (diff3 & SD_CEILLIGHT)
		{
			sectors[i].ceilinglightlevel = READINT16(save->p);
			sectors[i].ceilinglightabsolute = READUINT8(save->p);
		}
		if (diff3 & SD_FLAG)
		{
			sectors[i].flags = READUINT32(save->p);
			CheckForReverseGravity |= (sectors[i].flags & MSF_GRAVITYFLIP);
		}
		if (diff3 & SD_SPECIALFLAG)
			sectors[i].specialflags = READUINT32(save->p);
		if (diff4 & SD_DAMAGETYPE)
			sectors[i].damagetype = READUINT8(save->p);
		if (diff4 & SD_TRIGGERTAG)
			sectors[i].triggertag = READINT16(save->p);
		if (diff4 & SD_TRIGGERER)
			sectors[i].triggerer = READUINT8(save->p);
		if (diff4 & SD_GRAVITY)
			sectors[i].gravity = READFIXED(save->p);

		if (diff4 & SD_ACTION)
			sectors[i].action = READINT16(save->p);
		if (diff4 & SD_ARGS)
		{
			for (j = 0; j < NUM_SCRIPT_ARGS; j++)
				sectors[i].args[j] = READINT32(save->p);
		}
		if (diff4 & SD_STRINGARGS)
		{
			for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
			{
				size_t len = READINT32(save->p);
				size_t k;

				if (!len)
				{
					Z_Free(sectors[i].stringargs[j]);
					sectors[i].stringargs[j] = NULL;
					continue;
				}

				sectors[i].stringargs[j] = Z_Realloc(sectors[i].stringargs[j], len + 1, PU_LEVEL, NULL);
				for (k = 0; k < len; k++)
					sectors[i].stringargs[j][k] = READCHAR(save->p);
				sectors[i].stringargs[j][len] = '\0';
			}
		}
		if (diff5 & SD_ACTIVATION)
			sectors[i].activation = READUINT32(save->p);
		if (diff5 & SD_BOTCONTROLLER)
		{
			sectors[i].botController.trick = READUINT8(save->p);
			sectors[i].botController.flags = READUINT32(save->p);
			sectors[i].botController.forceAngle = READANGLE(save->p);
		}

		if (diff & SD_FFLOORS)
			UnArchiveFFloors(save, &sectors[i]);
	}
}

static void ArchiveLines(savebuffer_t *save)
{
	size_t i;
	const line_t *li = lines;
	const line_t *spawnli = spawnlines;
	const side_t *si;
	const side_t *spawnsi;
	UINT8 diff, diff2, diff3;

	for (i = 0; i < numlines; i++, spawnli++, li++)
	{
		diff = diff2 = diff3 = 0;

		if (li->flags != spawnli->flags)
			diff |= LD_FLAG;

		if (li->special != spawnli->special)
			diff |= LD_SPECIAL;

		if (spawnli->special == 321 || spawnli->special == 322) // only reason li->callcount would be non-zero is if either of these are involved
			diff |= LD_CLLCOUNT;

		if (!P_LineArgsEqual(li, spawnli))
			diff2 |= LD_ARGS;

		if (!P_LineStringArgsEqual(li, spawnli))
			diff2 |= LD_STRINGARGS;

		if (li->executordelay != spawnli->executordelay)
			diff2 |= LD_EXECUTORDELAY;

		if (li->activation != spawnli->activation)
			diff3 |= LD_ACTIVATION;

		if (li->sidenum[0] != 0xffff)
		{
			si = &sides[li->sidenum[0]];
			spawnsi = &spawnsides[li->sidenum[0]];
			if (si->textureoffset != spawnsi->textureoffset)
				diff |= LD_S1TEXOFF;
			//SoM: 4/1/2000: Some textures are colormaps. Don't worry about invalid textures.
			if (si->toptexture != spawnsi->toptexture)
				diff |= LD_S1TOPTEX;
			if (si->bottomtexture != spawnsi->bottomtexture)
				diff |= LD_S1BOTTEX;
			if (si->midtexture != spawnsi->midtexture)
				diff |= LD_S1MIDTEX;
		}
		if (li->sidenum[1] != 0xffff)
		{
			si = &sides[li->sidenum[1]];
			spawnsi = &spawnsides[li->sidenum[1]];
			if (si->textureoffset != spawnsi->textureoffset)
				diff2 |= LD_S2TEXOFF;
			if (si->toptexture != spawnsi->toptexture)
				diff2 |= LD_S2TOPTEX;
			if (si->bottomtexture != spawnsi->bottomtexture)
				diff2 |= LD_S2BOTTEX;
			if (si->midtexture != spawnsi->midtexture)
				diff2 |= LD_S2MIDTEX;
		}

		if (diff3)
			diff2 |= LD_DIFF3;

		if (diff2)
			diff |= LD_DIFF2;

		if (diff)
		{
			WRITEINT16(save->p, i);
			WRITEUINT8(save->p, diff);
			if (diff & LD_DIFF2)
				WRITEUINT8(save->p, diff2);
			if (diff2 & LD_DIFF3)
				WRITEUINT8(save->p, diff3);
			if (diff & LD_FLAG)
				WRITEUINT32(save->p, li->flags);
			if (diff & LD_SPECIAL)
				WRITEINT16(save->p, li->special);
			if (diff & LD_CLLCOUNT)
				WRITEINT16(save->p, li->callcount);

			si = &sides[li->sidenum[0]];
			if (diff & LD_S1TEXOFF)
				WRITEFIXED(save->p, si->textureoffset);
			if (diff & LD_S1TOPTEX)
				WRITEINT32(save->p, si->toptexture);
			if (diff & LD_S1BOTTEX)
				WRITEINT32(save->p, si->bottomtexture);
			if (diff & LD_S1MIDTEX)
				WRITEINT32(save->p, si->midtexture);

			si = &sides[li->sidenum[1]];
			if (diff2 & LD_S2TEXOFF)
				WRITEFIXED(save->p, si->textureoffset);
			if (diff2 & LD_S2TOPTEX)
				WRITEINT32(save->p, si->toptexture);
			if (diff2 & LD_S2BOTTEX)
				WRITEINT32(save->p, si->bottomtexture);
			if (diff2 & LD_S2MIDTEX)
				WRITEINT32(save->p, si->midtexture);
			if (diff2 & LD_ARGS)
			{
				UINT8 j;
				for (j = 0; j < NUM_SCRIPT_ARGS; j++)
					WRITEINT32(save->p, li->args[j]);
			}
			if (diff2 & LD_STRINGARGS)
			{
				UINT8 j;
				for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
				{
					size_t len, k;

					if (!li->stringargs[j])
					{
						WRITEINT32(save->p, 0);
						continue;
					}

					len = strlen(li->stringargs[j]);
					WRITEINT32(save->p, len);
					for (k = 0; k < len; k++)
						WRITECHAR(save->p, li->stringargs[j][k]);
				}
			}
			if (diff2 & LD_EXECUTORDELAY)
				WRITEINT32(save->p, li->executordelay);
			if (diff3 & LD_ACTIVATION)
				WRITEUINT32(save->p, li->activation);
		}
	}
	WRITEUINT16(save->p, 0xffff);
}

static void UnArchiveLines(savebuffer_t *save)
{
	UINT16 i;
	line_t *li;
	side_t *si;
	UINT8 diff, diff2, diff3;

	for (;;)
	{
		i = READUINT16(save->p);

		if (i == 0xffff)
			break;
		if (i > numlines)
			I_Error("Invalid line number %u from server", i);

		diff = READUINT8(save->p);
		li = &lines[i];

		if (diff & LD_DIFF2)
			diff2 = READUINT8(save->p);
		else
			diff2 = 0;

		if (diff2 & LD_DIFF3)
			diff3 = READUINT8(save->p);
		else
			diff3 = 0;

		if (diff & LD_FLAG)
			li->flags = READUINT32(save->p);
		if (diff & LD_SPECIAL)
			li->special = READINT16(save->p);
		if (diff & LD_CLLCOUNT)
			li->callcount = READINT16(save->p);

		si = &sides[li->sidenum[0]];
		if (diff & LD_S1TEXOFF)
			si->textureoffset = READFIXED(save->p);
		if (diff & LD_S1TOPTEX)
			si->toptexture = READINT32(save->p);
		if (diff & LD_S1BOTTEX)
			si->bottomtexture = READINT32(save->p);
		if (diff & LD_S1MIDTEX)
			si->midtexture = READINT32(save->p);

		si = &sides[li->sidenum[1]];
		if (diff2 & LD_S2TEXOFF)
			si->textureoffset = READFIXED(save->p);
		if (diff2 & LD_S2TOPTEX)
			si->toptexture = READINT32(save->p);
		if (diff2 & LD_S2BOTTEX)
			si->bottomtexture = READINT32(save->p);
		if (diff2 & LD_S2MIDTEX)
			si->midtexture = READINT32(save->p);
		if (diff2 & LD_ARGS)
		{
			UINT8 j;
			for (j = 0; j < NUM_SCRIPT_ARGS; j++)
				li->args[j] = READINT32(save->p);
		}
		if (diff2 & LD_STRINGARGS)
		{
			UINT8 j;
			for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
			{
				size_t len = READINT32(save->p);
				size_t k;

				if (!len)
				{
					Z_Free(li->stringargs[j]);
					li->stringargs[j] = NULL;
					continue;
				}

				li->stringargs[j] = Z_Realloc(li->stringargs[j], len + 1, PU_LEVEL, NULL);
				for (k = 0; k < len; k++)
					li->stringargs[j][k] = READCHAR(save->p);
				li->stringargs[j][len] = '\0';
			}
		}
		if (diff2 & LD_EXECUTORDELAY)
			li->executordelay = READINT32(save->p);
		if (diff3 & LD_ACTIVATION)
			li->activation = READUINT32(save->p);
	}
}

static void P_NetArchiveWorld(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	// initialize colormap vars because paranoia
	ClearNetColormaps();

	WRITEUINT32(save->p, ARCHIVEBLOCK_WORLD);

	ArchiveSectors(save);
	ArchiveLines(save);
	R_ClearTextureNumCache(false);

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveWorld(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	UINT16 i;

	if (READUINT32(save->p) != ARCHIVEBLOCK_WORLD)
		I_Error("Bad $$$.sav at archive block World");

	// initialize colormap vars because paranoia
	ClearNetColormaps();

	// count the level's ffloors so that colormap loading can have an upper limit
	for (i = 0; i < numsectors; i++)
	{
		ffloor_t *rover;
		for (rover = sectors[i].ffloors; rover; rover = rover->next)
			num_ffloors++;
	}

	UnArchiveSectors(save);
	UnArchiveLines(save);

	TracyCZoneEnd(__zone);
}

//
// Thinkers
//

static boolean P_ThingArgsEqual(const mobj_t *mobj, const mapthing_t *mapthing)
{
	UINT8 i;
	for (i = 0; i < NUM_MAPTHING_ARGS; i++)
		if (mobj->thing_args[i] != mapthing->thing_args[i])
			return false;

	return true;
}

static boolean P_ThingStringArgsEqual(const mobj_t *mobj, const mapthing_t *mapthing)
{
	UINT8 i;
	for (i = 0; i < NUM_MAPTHING_STRINGARGS; i++)
	{
		if (!mobj->thing_stringargs[i])
			return !mapthing->thing_stringargs[i];

		if (strcmp(mobj->thing_stringargs[i], mapthing->thing_stringargs[i]))
			return false;
	}

	return true;
}

static boolean P_ThingScriptEqual(const mobj_t *mobj, const mapthing_t *mapthing)
{
	UINT8 i;
	if (mobj->special != mapthing->special)
		return false;

	for (i = 0; i < NUM_SCRIPT_ARGS; i++)
		if (mobj->script_args[i] != mapthing->script_args[i])
			return false;

	for (i = 0; i < NUM_SCRIPT_STRINGARGS; i++)
	{
		if (!mobj->script_stringargs[i])
			return !mapthing->script_stringargs[i];

		if (strcmp(mobj->script_stringargs[i], mapthing->script_stringargs[i]))
			return false;
	}

	return true;
}

typedef enum
{
	MD_SPAWNPOINT  = 1,
	MD_POS         = 1<<1,
	MD_TYPE        = 1<<2,
	MD_MOM         = 1<<3,
	MD_RADIUS      = 1<<4,
	MD_HEIGHT      = 1<<5,
	MD_FLAGS       = 1<<6,
	MD_HEALTH      = 1<<7,
	MD_RTIME       = 1<<8,
	MD_STATE       = 1<<9,
	MD_TICS        = 1<<10,
	MD_SPRITE      = 1<<11,
	MD_FRAME       = 1<<12,
	MD_EFLAGS      = 1<<13,
	MD_PLAYER      = 1<<14,
	MD_MOVEDIR     = 1<<15,
	MD_MOVECOUNT   = 1<<16,
	MD_THRESHOLD   = 1<<17,
	MD_LASTLOOK    = 1<<18,
	MD_TARGET      = 1<<19,
	MD_TRACER      = 1<<20,
	MD_FRICTION    = 1<<21,
	MD_MOVEFACTOR  = 1<<22,
	MD_FLAGS2      = 1<<23,
	MD_FUSE        = 1<<24,
	MD_WATERTOP    = 1<<25,
	MD_WATERBOTTOM = 1<<26,
	MD_SCALE       = 1<<27,
	MD_DSCALE      = 1<<28,
	MD_ARGS        = 1<<29,
	MD_STRINGARGS  = 1<<30,
	MD_MORE        = (INT32)(1U<<31)
} mobj_diff_t;

typedef enum
{
	MD2_CUSVAL       = 1,
	MD2_CVMEM        = 1<<1,
	MD2_SKIN         = 1<<2,
	MD2_COLOR        = 1<<3,
	MD2_SCALESPEED   = 1<<4,
	MD2_EXTVAL1      = 1<<5,
	MD2_EXTVAL2      = 1<<6,
	MD2_HNEXT        = 1<<7,
	MD2_HPREV        = 1<<8,
	MD2_FLOORROVER   = 1<<9,
	MD2_CEILINGROVER = 1<<10,
	MD2_SLOPE        = 1<<11,
	MD2_COLORIZED    = 1<<12,
	MD2_MIRRORED     = 1<<13,
	MD2_ROLLANGLE    = 1<<14,
	MD2_SHADOWSCALE  = 1<<15,
	MD2_RENDERFLAGS  = 1<<16,
	MD2_TID          = 1<<17,
	MD2_SPRITESCALE  = 1<<18,
	MD2_SPRITEOFFSET = 1<<19,
	MD2_WORLDOFFSET  = 1<<20,
	MD2_SPECIAL      = 1<<21,
	MD2_FLOORSPRITESLOPE = 1<<22,
	MD2_DISPOFFSET   = 1<<23,
	MD2_HITLAG       = 1<<24,
	MD2_WAYPOINTCAP  = 1<<25,
	MD2_KITEMCAP     = 1<<26,
	MD2_ITNEXT       = 1<<27,
	MD2_FROZEN       = 1<<28,
	MD2_TERRAIN      = 1<<29,
	MD2_WATERSKIP    = 1<<30,
	MD2_MORE         = (INT32)(1U<<31),
} mobj_diff2_t;

typedef enum
{
	MD3_LIGHTLEVEL		= 1,
	MD3_REAPPEAR		= 1<<1,
	MD3_PUNT_REF		= 1<<2,
	MD3_OWNER			= 1<<3,
} mobj_diff3_t;

typedef enum
{
	tc_mobj,
	tc_ceiling,
	tc_floor,
	tc_flash,
	tc_strobe,
	tc_glow,
	tc_fireflicker,
	tc_thwomp,
	tc_camerascanner,
	tc_elevator,
	tc_continuousfalling,
	tc_bouncecheese,
	tc_startcrumble,
	tc_marioblock,
	tc_marioblockchecker,
	tc_floatsector,
	tc_crushceiling,
	tc_scroll,
	tc_friction,
	tc_pusher,
	tc_laserflash,
	tc_lightfade,
	tc_executor,
	tc_raisesector,
	tc_noenemies,
	tc_eachtime,
	tc_disappear,
	tc_fade,
	tc_fadecolormap,
	tc_planedisplace,
	tc_dynslopeline,
	tc_dynslopevert,
	tc_polyrotate, // haleyjd 03/26/06: polyobjects
	tc_polymove,
	tc_polywaypoint,
	tc_polyslidedoor,
	tc_polyswingdoor,
	tc_polyflag,
	tc_polydisplace,
	tc_polyrotdisplace,
	tc_polyfade,
	tc_end
} specials_e;

static inline UINT32 SaveMobjnum(const mobj_t *mobj)
{
	if (mobj) return mobj->mobjnum;
	return 0;
}

static UINT32 SaveSector(const sector_t *sector)
{
	if (sector) return (UINT32)(sector - sectors);
	return 0xFFFFFFFF;
}

static UINT32 SaveLine(const line_t *line)
{
	if (line) return (UINT32)(line - lines);
	return 0xFFFFFFFF;
}

static inline UINT32 SavePlayer(const player_t *player)
{
	if (player) return (UINT32)(player - players);
	return 0xFFFFFFFF;
}

static UINT32 SaveSlope(const pslope_t *slope)
{
	if (slope) return (UINT32)(slope->id);
	return 0xFFFFFFFF;
}

boolean TypeIsNetSynced(mobjtype_t type)
{
	// Ignore stationary hoops - these will be respawned from mapthings.
	if (type == MT_HOOP)
		return false;

	// These are NEVER saved.
	if (type == MT_HOOPCOLLIDE)
		return false;

	// This hoop has already been collected.
	if (type == MT_HOOPCENTER)// && mobj->threshold == 4242)
		return false;

	// MT_SPARK: used for debug stuff
	if (type == MT_SPARK)
		return false;

	// MT_HORNCODE: So it turns out hornmod is fundamentally incompatible with netsync
	if (type == MT_HORNCODE)
		return false;

	// MT_PRISONEGGDROP: Yeah these are completely local
	if (type == MT_PRISONEGGDROP)
		return false;

	return true;
}

static void SaveMobjThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const mobj_t *mobj = (const mobj_t *)th;
	UINT32 diff;
	UINT32 diff2;
	UINT32 diff3;
	size_t j;

	if (TypeIsNetSynced(mobj->type) == false)
		return;

	diff2 = 0;
	diff3 = 0;

	if (mobj->spawnpoint)
	{
		// spawnpoint is not modified but we must save it since it is an identifier
		diff = MD_SPAWNPOINT;

		if ((mobj->x != mobj->spawnpoint->x << FRACBITS) ||
			(mobj->y != mobj->spawnpoint->y << FRACBITS) ||
			(mobj->angle != FixedAngle(mobj->spawnpoint->angle*FRACUNIT)) ||
			(mobj->pitch != FixedAngle(mobj->spawnpoint->pitch*FRACUNIT)) ||
			(mobj->roll != FixedAngle(mobj->spawnpoint->roll*FRACUNIT)) )
			diff |= MD_POS;

		if (mobj->info->doomednum != mobj->spawnpoint->type)
			diff |= MD_TYPE;

		if (!P_ThingArgsEqual(mobj, mobj->spawnpoint))
			diff |= MD_ARGS;

		if (!P_ThingStringArgsEqual(mobj, mobj->spawnpoint))
			diff |= MD_STRINGARGS;

		if (!P_ThingScriptEqual(mobj, mobj->spawnpoint))
			diff2 |= MD2_SPECIAL;
	}
	else
	{
		// not a map spawned thing, so make it from scratch
		diff = MD_POS | MD_TYPE;

		for (j = 0; j < NUM_MAPTHING_ARGS; j++)
		{
			if (mobj->thing_args[j] != 0)
			{
				diff |= MD_ARGS;
				break;
			}
		}

		for (j = 0; j < NUM_MAPTHING_STRINGARGS; j++)
		{
			if (mobj->thing_stringargs[j] != NULL)
			{
				diff |= MD_STRINGARGS;
				break;
			}
		}

		if (mobj->special != 0)
		{
			diff2 |= MD2_SPECIAL;
		}

		for (j = 0; j < NUM_SCRIPT_ARGS; j++)
		{
			if (mobj->script_args[j] != 0)
			{
				diff2 |= MD2_SPECIAL;
				break;
			}
		}

		for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
		{
			if (mobj->script_stringargs[j] != NULL)
			{
				diff2 |= MD2_SPECIAL;
				break;
			}
		}
	}

	// not the default but the most probable
	if (mobj->momx != 0 || mobj->momy != 0 || mobj->momz != 0 || mobj->pmomz != 0 || mobj->lastmomz != 0)
		diff |= MD_MOM;
	if (mobj->radius != mobj->info->radius)
		diff |= MD_RADIUS;
	if (mobj->height != mobj->info->height)
		diff |= MD_HEIGHT;
	if (mobj->flags != mobj->info->flags)
		diff |= MD_FLAGS;
	if (mobj->flags2)
		diff |= MD_FLAGS2;
	if (mobj->health != mobj->info->spawnhealth)
		diff |= MD_HEALTH;
	if (mobj->reactiontime != mobj->info->reactiontime)
		diff |= MD_RTIME;
	if ((statenum_t)(mobj->state-states) != mobj->info->spawnstate)
		diff |= MD_STATE;
	if (mobj->tics != mobj->state->tics)
		diff |= MD_TICS;
	if (mobj->sprite != mobj->state->sprite)
		diff |= MD_SPRITE;
	if (mobj->sprite == SPR_PLAY && mobj->sprite2 != (mobj->state->frame&FF_FRAMEMASK))
		diff |= MD_SPRITE;
	if (mobj->frame != mobj->state->frame)
		diff |= MD_FRAME;
	if (mobj->anim_duration != (UINT16)mobj->state->var2)
		diff |= MD_FRAME;
	if (mobj->eflags)
		diff |= MD_EFLAGS;
	if (mobj->player)
		diff |= MD_PLAYER;
	if (mobj->movedir)
		diff |= MD_MOVEDIR;
	if (mobj->movecount)
		diff |= MD_MOVECOUNT;
	if (mobj->threshold)
		diff |= MD_THRESHOLD;
	if (mobj->lastlook != -1)
		diff |= MD_LASTLOOK;
	if (mobj->target)
		diff |= MD_TARGET;
	if (mobj->tracer)
		diff |= MD_TRACER;
	if (mobj->friction != ORIG_FRICTION)
		diff |= MD_FRICTION;
	if (mobj->movefactor != FRACUNIT)
		diff |= MD_MOVEFACTOR;
	if (mobj->fuse)
		diff |= MD_FUSE;
	if (mobj->watertop)
		diff |= MD_WATERTOP;
	if (mobj->waterbottom)
		diff |= MD_WATERBOTTOM;
	if (mobj->scale != FRACUNIT)
		diff |= MD_SCALE;
	if (mobj->destscale != mobj->scale)
		diff |= MD_DSCALE;
	if (mobj->scalespeed != mapobjectscale/12)
		diff2 |= MD2_SCALESPEED;
	if (mobj->cusval)
		diff2 |= MD2_CUSVAL;
	if (mobj->cvmem)
		diff2 |= MD2_CVMEM;
	if (mobj->color)
		diff2 |= MD2_COLOR;
	if (mobj->skin)
		diff2 |= MD2_SKIN;
	if (mobj->extravalue1)
		diff2 |= MD2_EXTVAL1;
	if (mobj->extravalue2)
		diff2 |= MD2_EXTVAL2;
	if (mobj->hnext)
		diff2 |= MD2_HNEXT;
	if (mobj->hprev)
		diff2 |= MD2_HPREV;
	if (mobj->standingslope)
		diff2 |= MD2_SLOPE;
	if (mobj->colorized)
		diff2 |= MD2_COLORIZED;
	if (mobj->floorrover)
		diff2 |= MD2_FLOORROVER;
	if (mobj->ceilingrover)
		diff2 |= MD2_CEILINGROVER;
	if (mobj->mirrored)
		diff2 |= MD2_MIRRORED;
	if (mobj->rollangle)
		diff2 |= MD2_ROLLANGLE;
	if (mobj->shadowscale)
		diff2 |= MD2_SHADOWSCALE;
	if (mobj->renderflags)
		diff2 |= MD2_RENDERFLAGS;
	if (mobj->tid != 0)
		diff2 |= MD2_TID;
	if (mobj->spritexscale != FRACUNIT || mobj->spriteyscale != FRACUNIT)
		diff2 |= MD2_SPRITESCALE;
	if (mobj->spritexoffset || mobj->spriteyoffset)
		diff2 |= MD2_SPRITEOFFSET;
	if (mobj->sprxoff || mobj->spryoff || mobj->sprzoff)
		diff2 |= MD2_WORLDOFFSET;
	if (mobj->floorspriteslope)
	{
		pslope_t *slope = mobj->floorspriteslope;
		if (slope->zangle || slope->zdelta || slope->xydirection
		|| slope->o.x || slope->o.y || slope->o.z
		|| slope->d.x || slope->d.y
		|| slope->normal.x || slope->normal.y
		|| (slope->normal.z != FRACUNIT))
			diff2 |= MD2_FLOORSPRITESLOPE;
	}
	if (mobj->hitlag)
		diff2 |= MD2_HITLAG;
	if (mobj->waterskip)
		diff2 |= MD2_WATERSKIP;
	if (mobj->dispoffset)
		diff2 |= MD2_DISPOFFSET;
	if (mobj == waypointcap)
		diff2 |= MD2_WAYPOINTCAP;
	if (mobj == trackercap)
		diff2 |= MD2_KITEMCAP;
	if (mobj->itnext)
		diff2 |= MD2_ITNEXT;
	if (mobj->frozen)
		diff2 |= MD2_FROZEN;
	if (mobj->terrain != NULL || mobj->terrainOverlay != NULL)
		diff2 |= MD2_TERRAIN;

	if (mobj->lightlevel)
		diff3 |= MD3_LIGHTLEVEL;
	if (mobj->reappear)
		diff3 |= MD3_REAPPEAR;
	if (mobj->punt_ref)
		diff3 |= MD3_PUNT_REF;
	if (mobj->owner)
		diff3 |= MD3_OWNER;

	if (diff3 != 0)
		diff2 |= MD2_MORE;

	if (diff2 != 0)
		diff |= MD_MORE;

	// Scrap all of that. If we're a hoop center, this is ALL we're saving.
	if (mobj->type == MT_HOOPCENTER)
		diff = MD_SPAWNPOINT;

	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, diff);
	if (diff & MD_MORE)
		WRITEUINT32(save->p, diff2);
	if (diff2 & MD2_MORE)
		WRITEUINT32(save->p, diff3);

	WRITEFIXED(save->p, mobj->z); // Force this so 3dfloor problems don't arise.
	WRITEFIXED(save->p, mobj->floorz);
	WRITEFIXED(save->p, mobj->ceilingz);

	if (diff2 & MD2_FLOORROVER)
	{
		WRITEUINT32(save->p, SaveSector(mobj->floorrover->target));
		WRITEUINT16(save->p, P_GetFFloorID(mobj->floorrover));
	}

	if (diff2 & MD2_CEILINGROVER)
	{
		WRITEUINT32(save->p, SaveSector(mobj->ceilingrover->target));
		WRITEUINT16(save->p, P_GetFFloorID(mobj->ceilingrover));
	}

	if (diff & MD_SPAWNPOINT)
	{
		size_t z;

		for (z = 0; z < nummapthings; z++)
		{
			if (&mapthings[z] != mobj->spawnpoint)
				continue;
			WRITEUINT16(save->p, z);
			break;
		}
		if (mobj->type == MT_HOOPCENTER)
			return;
	}

	if (diff & MD_TYPE)
		WRITEUINT32(save->p, mobj->type);
	if (diff & MD_POS)
	{
		WRITEFIXED(save->p, mobj->x);
		WRITEFIXED(save->p, mobj->y);
		WRITEANGLE(save->p, mobj->angle);
		WRITEANGLE(save->p, mobj->pitch);
		WRITEANGLE(save->p, mobj->roll);
	}
	if (diff & MD_MOM)
	{
		WRITEFIXED(save->p, mobj->momx);
		WRITEFIXED(save->p, mobj->momy);
		WRITEFIXED(save->p, mobj->momz);
		WRITEFIXED(save->p, mobj->pmomz);
		WRITEFIXED(save->p, mobj->lastmomz);
	}
	if (diff & MD_RADIUS)
		WRITEFIXED(save->p, mobj->radius);
	if (diff & MD_HEIGHT)
		WRITEFIXED(save->p, mobj->height);
	if (diff & MD_FLAGS)
		WRITEUINT32(save->p, mobj->flags);
	if (diff & MD_FLAGS2)
		WRITEUINT32(save->p, mobj->flags2);
	if (diff & MD_HEALTH)
		WRITEINT32(save->p, mobj->health);
	if (diff & MD_RTIME)
		WRITEINT32(save->p, mobj->reactiontime);
	if (diff & MD_STATE)
		WRITEUINT16(save->p, mobj->state-states);
	if (diff & MD_TICS)
		WRITEINT32(save->p, mobj->tics);
	if (diff & MD_SPRITE) {
		WRITEUINT16(save->p, mobj->sprite);
		if (mobj->sprite == SPR_PLAY)
			WRITEUINT8(save->p, mobj->sprite2);
	}
	if (diff & MD_FRAME)
	{
		WRITEUINT32(save->p, mobj->frame);
		WRITEUINT16(save->p, mobj->anim_duration);
	}
	if (diff & MD_EFLAGS)
		WRITEUINT16(save->p, mobj->eflags);
	if (diff & MD_PLAYER)
		WRITEUINT8(save->p, mobj->player-players);
	if (diff & MD_MOVEDIR)
		WRITEANGLE(save->p, mobj->movedir);
	if (diff & MD_MOVECOUNT)
		WRITEINT32(save->p, mobj->movecount);
	if (diff & MD_THRESHOLD)
		WRITEINT32(save->p, mobj->threshold);
	if (diff & MD_LASTLOOK)
		WRITEINT32(save->p, mobj->lastlook);
	if (diff & MD_TARGET)
		WRITEUINT32(save->p, mobj->target->mobjnum);
	if (diff & MD_TRACER)
		WRITEUINT32(save->p, mobj->tracer->mobjnum);
	if (diff & MD_FRICTION)
		WRITEFIXED(save->p, mobj->friction);
	if (diff & MD_MOVEFACTOR)
		WRITEFIXED(save->p, mobj->movefactor);
	if (diff & MD_FUSE)
		WRITEINT32(save->p, mobj->fuse);
	if (diff & MD_WATERTOP)
		WRITEFIXED(save->p, mobj->watertop);
	if (diff & MD_WATERBOTTOM)
		WRITEFIXED(save->p, mobj->waterbottom);
	if (diff & MD_SCALE)
		WRITEFIXED(save->p, mobj->scale);
	if (diff & MD_DSCALE)
		WRITEFIXED(save->p, mobj->destscale);
	if (diff2 & MD2_SCALESPEED)
		WRITEFIXED(save->p, mobj->scalespeed);
	if (diff & MD_ARGS)
	{
		for (j = 0; j < NUM_MAPTHING_ARGS; j++)
			WRITEINT32(save->p, mobj->thing_args[j]);
	}
	if (diff & MD_STRINGARGS)
	{
		for (j = 0; j < NUM_MAPTHING_STRINGARGS; j++)
		{
			size_t len, k;

			if (!mobj->thing_stringargs[j])
			{
				WRITEINT32(save->p, 0);
				continue;
			}

			len = strlen(mobj->thing_stringargs[j]);
			WRITEINT32(save->p, len);
			for (k = 0; k < len; k++)
				WRITECHAR(save->p, mobj->thing_stringargs[j][k]);
		}
	}
	if (diff2 & MD2_CUSVAL)
		WRITEINT32(save->p, mobj->cusval);
	if (diff2 & MD2_CVMEM)
		WRITEINT32(save->p, mobj->cvmem);
	if (diff2 & MD2_SKIN)
		WRITEUINT8(save->p, (UINT8)((skin_t *)mobj->skin - skins));
	if (diff2 & MD2_COLOR)
		WRITEUINT16(save->p, mobj->color);
	if (diff2 & MD2_EXTVAL1)
		WRITEINT32(save->p, mobj->extravalue1);
	if (diff2 & MD2_EXTVAL2)
		WRITEINT32(save->p, mobj->extravalue2);
	if (diff2 & MD2_HNEXT)
		WRITEUINT32(save->p, mobj->hnext->mobjnum);
	if (diff2 & MD2_HPREV)
		WRITEUINT32(save->p, mobj->hprev->mobjnum);
	if (diff2 & MD2_ITNEXT)
		WRITEUINT32(save->p, mobj->itnext->mobjnum);
	if (diff2 & MD2_SLOPE)
		WRITEUINT16(save->p, mobj->standingslope->id);
	if (diff2 & MD2_COLORIZED)
		WRITEUINT8(save->p, mobj->colorized);
	if (diff2 & MD2_MIRRORED)
		WRITEUINT8(save->p, mobj->mirrored);
	if (diff2 & MD2_ROLLANGLE)
		WRITEANGLE(save->p, mobj->rollangle);
	if (diff2 & MD2_SHADOWSCALE)
	{
		WRITEFIXED(save->p, mobj->shadowscale);
		WRITEUINT8(save->p, mobj->whiteshadow);
		WRITEUINT8(save->p, mobj->shadowcolor);
	}
	if (diff2 & MD2_RENDERFLAGS)
	{
		UINT32 rf = mobj->renderflags;
		UINT32 q = rf & RF_DONTDRAW;

		if (q != RF_DONTDRAW // visible for more than one local player
		&& q != (RF_DONTDRAWP1|RF_DONTDRAWP2|RF_DONTDRAWP3)
		&& q != (RF_DONTDRAWP4|RF_DONTDRAWP1|RF_DONTDRAWP2)
		&& q != (RF_DONTDRAWP4|RF_DONTDRAWP1|RF_DONTDRAWP3)
		&& q != (RF_DONTDRAWP4|RF_DONTDRAWP2|RF_DONTDRAWP3))
			rf &= ~q;

		WRITEUINT32(save->p, rf);
	}
	if (diff2 & MD2_TID)
		WRITEINT16(save->p, mobj->tid);
	if (diff2 & MD2_SPRITESCALE)
	{
		WRITEFIXED(save->p, mobj->spritexscale);
		WRITEFIXED(save->p, mobj->spriteyscale);
	}
	if (diff2 & MD2_SPRITEOFFSET)
	{
		WRITEFIXED(save->p, mobj->spritexoffset);
		WRITEFIXED(save->p, mobj->spriteyoffset);
	}
	if (diff2 & MD2_WORLDOFFSET)
	{
		WRITEFIXED(save->p, mobj->sprxoff);
		WRITEFIXED(save->p, mobj->spryoff);
		WRITEFIXED(save->p, mobj->sprzoff);
	}
	if (diff2 & MD2_SPECIAL)
	{
		WRITEINT16(save->p, mobj->special);

		for (j = 0; j < NUM_SCRIPT_ARGS; j++)
			WRITEINT32(save->p, mobj->script_args[j]);

		for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
		{
			size_t len, k;

			if (!mobj->script_stringargs[j])
			{
				WRITEINT32(save->p, 0);
				continue;
			}

			len = strlen(mobj->script_stringargs[j]);
			WRITEINT32(save->p, len);
			for (k = 0; k < len; k++)
				WRITECHAR(save->p, mobj->script_stringargs[j][k]);
		}
	}
	if (diff2 & MD2_FLOORSPRITESLOPE)
	{
		pslope_t *slope = mobj->floorspriteslope;

		WRITEFIXED(save->p, slope->zdelta);
		WRITEANGLE(save->p, slope->zangle);
		WRITEANGLE(save->p, slope->xydirection);

		WRITEFIXED(save->p, slope->o.x);
		WRITEFIXED(save->p, slope->o.y);
		WRITEFIXED(save->p, slope->o.z);

		WRITEFIXED(save->p, slope->d.x);
		WRITEFIXED(save->p, slope->d.y);

		WRITEFIXED(save->p, slope->normal.x);
		WRITEFIXED(save->p, slope->normal.y);
		WRITEFIXED(save->p, slope->normal.z);
	}
	if (diff2 & MD2_HITLAG)
	{
		WRITEINT32(save->p, mobj->hitlag);
	}
	if (diff2 & MD2_WATERSKIP)
	{
		WRITEUINT8(save->p, mobj->waterskip);
	}
	if (diff2 & MD2_DISPOFFSET)
	{
		WRITEINT32(save->p, mobj->dispoffset);
	}
	if (diff2 & MD2_FROZEN)
	{
		WRITEUINT8(save->p, mobj->frozen);
	}
	if (diff2 & MD2_TERRAIN)
	{
		WRITEUINT32(save->p, K_GetTerrainHeapIndex(mobj->terrain));
		WRITEUINT32(save->p, SaveMobjnum(mobj->terrainOverlay));
	}

	if (diff3 & MD3_LIGHTLEVEL)
	{
		WRITEINT16(save->p, mobj->lightlevel);
	}
	if (diff3 & MD3_REAPPEAR)
	{
		WRITEUINT32(save->p, mobj->reappear);
	}
	if (diff3 & MD3_PUNT_REF)
	{
		WRITEUINT32(save->p, mobj->punt_ref->mobjnum);
	}
	if (diff3 & MD3_OWNER)
	{
		WRITEUINT32(save->p, mobj->owner->mobjnum);
	}

	WRITEUINT32(save->p, mobj->mobjnum);
}

static void SaveNoEnemiesThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const noenemies_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
}

static void SaveBounceCheeseThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const bouncecheese_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEFIXED(save->p, ht->speed);
	WRITEFIXED(save->p, ht->distance);
	WRITEFIXED(save->p, ht->floorwasheight);
	WRITEFIXED(save->p, ht->ceilingwasheight);
	WRITECHAR(save->p, ht->low);
}

static void SaveContinuousFallThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const continuousfall_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEFIXED(save->p, ht->speed);
	WRITEINT32(save->p, ht->direction);
	WRITEFIXED(save->p, ht->floorstartheight);
	WRITEFIXED(save->p, ht->ceilingstartheight);
	WRITEFIXED(save->p, ht->destheight);
}

static void SaveMarioBlockThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const mariothink_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEFIXED(save->p, ht->speed);
	WRITEINT32(save->p, ht->direction);
	WRITEFIXED(save->p, ht->floorstartheight);
	WRITEFIXED(save->p, ht->ceilingstartheight);
	WRITEINT16(save->p, ht->tag);
}

static void SaveMarioCheckThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const mariocheck_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	WRITEUINT32(save->p, SaveSector(ht->sector));
}

static void SaveThwompThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const thwomp_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEFIXED(save->p, ht->crushspeed);
	WRITEFIXED(save->p, ht->retractspeed);
	WRITEINT32(save->p, ht->direction);
	WRITEFIXED(save->p, ht->floorstartheight);
	WRITEFIXED(save->p, ht->ceilingstartheight);
	WRITEINT32(save->p, ht->delay);
	WRITEINT16(save->p, ht->tag);
	WRITEUINT16(save->p, ht->sound);
	WRITEINT32(save->p, ht->initDelay);
}

static void SaveFloatThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const floatthink_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT16(save->p, ht->tag);
}

static void SaveEachTimeThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const eachtime_t *ht  = (const void *)th;
	size_t i;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	for (i = 0; i < MAXPLAYERS; i++)
	{
		WRITECHAR(save->p, ht->playersInArea[i]);
	}
	WRITECHAR(save->p, ht->triggerOnExit);
}

static void SaveRaiseThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const raise_t *ht  = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT16(save->p, ht->tag);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEFIXED(save->p, ht->ceilingbottom);
	WRITEFIXED(save->p, ht->ceilingtop);
	WRITEFIXED(save->p, ht->basespeed);
	WRITEFIXED(save->p, ht->extraspeed);
	WRITEUINT8(save->p, ht->shaketimer);
	WRITEUINT8(save->p, ht->flags);
}

static void SaveCeilingThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const ceiling_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT8(save->p, ht->type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEFIXED(save->p, ht->bottomheight);
	WRITEFIXED(save->p, ht->topheight);
	WRITEFIXED(save->p, ht->speed);
	WRITEFIXED(save->p, ht->delay);
	WRITEFIXED(save->p, ht->delaytimer);
	WRITEUINT8(save->p, ht->crush);
	WRITEINT32(save->p, ht->texture);
	WRITEINT32(save->p, ht->direction);
	WRITEINT16(save->p, ht->tag);
	WRITEFIXED(save->p, ht->origspeed);
	WRITEFIXED(save->p, ht->crushHeight);
	WRITEFIXED(save->p, ht->crushSpeed);
	WRITEFIXED(save->p, ht->returnHeight);
	WRITEFIXED(save->p, ht->returnSpeed);
}

static void SaveFloormoveThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const floormove_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT8(save->p, ht->type);
	WRITEUINT8(save->p, ht->crush);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT32(save->p, ht->direction);
	WRITEINT32(save->p, ht->texture);
	WRITEFIXED(save->p, ht->floordestheight);
	WRITEFIXED(save->p, ht->speed);
	WRITEFIXED(save->p, ht->origspeed);
	WRITEFIXED(save->p, ht->delay);
	WRITEFIXED(save->p, ht->delaytimer);
	WRITEINT16(save->p, ht->tag);
	WRITEFIXED(save->p, ht->crushHeight);
	WRITEFIXED(save->p, ht->crushSpeed);
	WRITEFIXED(save->p, ht->returnHeight);
	WRITEFIXED(save->p, ht->returnSpeed);
}

static void SaveLightflashThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const lightflash_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT32(save->p, ht->maxlight);
	WRITEINT32(save->p, ht->minlight);
}

static void SaveStrobeThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const strobe_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT32(save->p, ht->count);
	WRITEINT16(save->p, ht->minlight);
	WRITEINT16(save->p, ht->maxlight);
	WRITEINT32(save->p, ht->darktime);
	WRITEINT32(save->p, ht->brighttime);
}

static void SaveGlowThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const glow_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT16(save->p, ht->minlight);
	WRITEINT16(save->p, ht->maxlight);
	WRITEINT16(save->p, ht->direction);
	WRITEINT16(save->p, ht->speed);
}

static inline void SaveFireflickerThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const fireflicker_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT32(save->p, ht->count);
	WRITEINT32(save->p, ht->resetcount);
	WRITEINT16(save->p, ht->maxlight);
	WRITEINT16(save->p, ht->minlight);
}

static void SaveElevatorThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const elevator_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT8(save->p, ht->type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEUINT32(save->p, SaveSector(ht->actionsector));
	WRITEINT32(save->p, ht->direction);
	WRITEFIXED(save->p, ht->floordestheight);
	WRITEFIXED(save->p, ht->ceilingdestheight);
	WRITEFIXED(save->p, ht->speed);
	WRITEFIXED(save->p, ht->origspeed);
	WRITEFIXED(save->p, ht->low);
	WRITEFIXED(save->p, ht->high);
	WRITEFIXED(save->p, ht->distance);
	WRITEFIXED(save->p, ht->delay);
	WRITEFIXED(save->p, ht->delaytimer);
	WRITEFIXED(save->p, ht->floorwasheight);
	WRITEFIXED(save->p, ht->ceilingwasheight);
}

static void SaveCrumbleThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const crumble_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEUINT32(save->p, SaveSector(ht->actionsector));
	WRITEUINT32(save->p, SavePlayer(ht->player)); // was dummy
	WRITEINT32(save->p, ht->direction);
	WRITEINT32(save->p, ht->origalpha);
	WRITEINT32(save->p, ht->timer);
	WRITEFIXED(save->p, ht->speed);
	WRITEFIXED(save->p, ht->floorwasheight);
	WRITEFIXED(save->p, ht->ceilingwasheight);
	WRITEUINT8(save->p, ht->flags);
}

static inline void SaveScrollThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const scroll_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEFIXED(save->p, ht->dx);
	WRITEFIXED(save->p, ht->dy);
	WRITEINT32(save->p, ht->affectee);
	WRITEINT32(save->p, ht->control);
	WRITEFIXED(save->p, ht->last_height);
	WRITEFIXED(save->p, ht->vdx);
	WRITEFIXED(save->p, ht->vdy);
	WRITEINT32(save->p, ht->accel);
	WRITEINT32(save->p, ht->exclusive);
	WRITEUINT8(save->p, ht->type);
}

static inline void SaveFrictionThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const friction_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->friction);
	WRITEINT32(save->p, ht->movefactor);
	WRITEINT32(save->p, ht->affectee);
	WRITEINT32(save->p, ht->referrer);
	WRITEUINT8(save->p, ht->roverfriction);
}

static inline void SavePusherThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const pusher_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT8(save->p, ht->type);
	WRITEFIXED(save->p, ht->x_mag);
	WRITEFIXED(save->p, ht->y_mag);
	WRITEFIXED(save->p, ht->z_mag);
	WRITEINT32(save->p, ht->affectee);
	WRITEUINT8(save->p, ht->roverpusher);
	WRITEINT32(save->p, ht->referrer);
	WRITEINT32(save->p, ht->exclusive);
	WRITEINT32(save->p, ht->slider);
}

static void SaveLaserThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const laserthink_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT16(save->p, ht->tag);
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	WRITEUINT8(save->p, ht->nobosses);
}

static void SaveLightlevelThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const lightlevel_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT16(save->p, ht->sourcelevel);
	WRITEINT16(save->p, ht->destlevel);
	WRITEFIXED(save->p, ht->fixedcurlevel);
	WRITEFIXED(save->p, ht->fixedpertic);
	WRITEINT32(save->p, ht->timer);
}

static void SaveExecutorThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const executor_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveLine(ht->line));
	WRITEUINT32(save->p, SaveMobjnum(ht->caller));
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEINT32(save->p, ht->timer);
}

static void SaveDisappearThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const disappear_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, ht->appeartime);
	WRITEUINT32(save->p, ht->disappeartime);
	WRITEUINT32(save->p, ht->offset);
	WRITEUINT32(save->p, ht->timer);
	WRITEINT32(save->p, ht->affectee);
	WRITEINT32(save->p, ht->sourceline);
	WRITEINT32(save->p, ht->exists);
}

static void SaveFadeThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const fade_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, CheckAddNetColormapToList(ht->dest_exc));
	WRITEUINT32(save->p, ht->sectornum);
	WRITEUINT32(save->p, ht->ffloornum);
	WRITEINT32(save->p, ht->alpha);
	WRITEINT16(save->p, ht->sourcevalue);
	WRITEINT16(save->p, ht->destvalue);
	WRITEINT16(save->p, ht->destlightlevel);
	WRITEINT16(save->p, ht->speed);
	WRITEUINT8(save->p, (UINT8)ht->ticbased);
	WRITEINT32(save->p, ht->timer);
	WRITEUINT8(save->p, ht->doexists);
	WRITEUINT8(save->p, ht->dotranslucent);
	WRITEUINT8(save->p, ht->dolighting);
	WRITEUINT8(save->p, ht->docolormap);
	WRITEUINT8(save->p, ht->docollision);
	WRITEUINT8(save->p, ht->doghostfade);
	WRITEUINT8(save->p, ht->exactalpha);
}

static void SaveFadeColormapThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const fadecolormap_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSector(ht->sector));
	WRITEUINT32(save->p, CheckAddNetColormapToList(ht->source_exc));
	WRITEUINT32(save->p, CheckAddNetColormapToList(ht->dest_exc));
	WRITEUINT8(save->p, (UINT8)ht->ticbased);
	WRITEINT32(save->p, ht->duration);
	WRITEINT32(save->p, ht->timer);
}

static void SavePlaneDisplaceThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const planedisplace_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->affectee);
	WRITEINT32(save->p, ht->control);
	WRITEFIXED(save->p, ht->last_height);
	WRITEFIXED(save->p, ht->speed);
	WRITEUINT8(save->p, ht->type);
}

static inline void SaveDynamicLineSlopeThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const dynlineplanethink_t* ht = (const void*)th;

	WRITEUINT8(save->p, type);
	WRITEUINT8(save->p, ht->type);
	WRITEUINT32(save->p, SaveSlope(ht->slope));
	WRITEUINT32(save->p, SaveLine(ht->sourceline));
	WRITEFIXED(save->p, ht->extent);
}

static inline void SaveDynamicVertexSlopeThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	size_t i;
	const dynvertexplanethink_t* ht = (const void*)th;

	WRITEUINT8(save->p, type);
	WRITEUINT32(save->p, SaveSlope(ht->slope));
	for (i = 0; i < 3; i++)
		WRITEUINT32(save->p, SaveSector(ht->secs[i]));
	WRITEMEM(save->p, ht->vex, sizeof(ht->vex));
	WRITEMEM(save->p, ht->origsecheights, sizeof(ht->origsecheights));
	WRITEMEM(save->p, ht->origvecheights, sizeof(ht->origvecheights));
	WRITEUINT8(save->p, ht->relative);
}

static inline void SavePolyrotatetThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const polyrotate_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEINT32(save->p, ht->speed);
	WRITEINT32(save->p, ht->distance);
	WRITEUINT8(save->p, ht->turnobjs);
}

static void SavePolymoveThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const polymove_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEINT32(save->p, ht->speed);
	WRITEFIXED(save->p, ht->momx);
	WRITEFIXED(save->p, ht->momy);
	WRITEINT32(save->p, ht->distance);
	WRITEANGLE(save->p, ht->angle);
}

static void SavePolywaypointThinker(savebuffer_t *save, const thinker_t *th, UINT8 type)
{
	const polywaypoint_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEINT32(save->p, ht->speed);
	WRITEINT32(save->p, ht->sequence);
	WRITEINT32(save->p, ht->pointnum);
	WRITEINT32(save->p, ht->direction);
	WRITEUINT8(save->p, ht->returnbehavior);
	WRITEUINT8(save->p, ht->continuous);
	WRITEUINT8(save->p, ht->stophere);
}

static void SavePolyslidedoorThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const polyslidedoor_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEINT32(save->p, ht->delay);
	WRITEINT32(save->p, ht->delayCount);
	WRITEINT32(save->p, ht->initSpeed);
	WRITEINT32(save->p, ht->speed);
	WRITEINT32(save->p, ht->initDistance);
	WRITEINT32(save->p, ht->distance);
	WRITEUINT32(save->p, ht->initAngle);
	WRITEUINT32(save->p, ht->angle);
	WRITEUINT32(save->p, ht->revAngle);
	WRITEFIXED(save->p, ht->momx);
	WRITEFIXED(save->p, ht->momy);
	WRITEUINT8(save->p, ht->closing);
}

static void SavePolyswingdoorThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const polyswingdoor_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEINT32(save->p, ht->delay);
	WRITEINT32(save->p, ht->delayCount);
	WRITEINT32(save->p, ht->initSpeed);
	WRITEINT32(save->p, ht->speed);
	WRITEINT32(save->p, ht->initDistance);
	WRITEINT32(save->p, ht->distance);
	WRITEUINT8(save->p, ht->closing);
}

static void SavePolydisplaceThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const polydisplace_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEUINT32(save->p, SaveSector(ht->controlSector));
	WRITEFIXED(save->p, ht->dx);
	WRITEFIXED(save->p, ht->dy);
	WRITEFIXED(save->p, ht->oldHeights);
}

static void SavePolyrotdisplaceThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const polyrotdisplace_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEUINT32(save->p, SaveSector(ht->controlSector));
	WRITEFIXED(save->p, ht->rotscale);
	WRITEUINT8(save->p, ht->turnobjs);
	WRITEFIXED(save->p, ht->oldHeights);
}

static void SavePolyfadeThinker(savebuffer_t *save, const thinker_t *th, const UINT8 type)
{
	const polyfade_t *ht = (const void *)th;
	WRITEUINT8(save->p, type);
	WRITEINT32(save->p, ht->polyObjNum);
	WRITEINT32(save->p, ht->sourcevalue);
	WRITEINT32(save->p, ht->destvalue);
	WRITEUINT8(save->p, (UINT8)ht->docollision);
	WRITEUINT8(save->p, (UINT8)ht->doghostfade);
	WRITEUINT8(save->p, (UINT8)ht->ticbased);
	WRITEINT32(save->p, ht->duration);
	WRITEINT32(save->p, ht->timer);
}

static void WriteMobjPointer(mobj_t *mobj)
{
	WRITEUINT32(current_savebuffer->p, SaveMobjnum(mobj));
}

static void P_NetArchiveThinkers(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	const thinker_t *th;
	UINT32 i;

	WRITEUINT32(save->p, ARCHIVEBLOCK_THINKERS);

	P_SaveMobjPointers(WriteMobjPointer);

	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		UINT32 numsaved = 0;
		// save off the current thinkers
		for (th = thlist[i].next; th != &thlist[i]; th = th->next)
		{
			if (!(th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed
			 || th->function.acp1 == (actionf_p1)P_NullPrecipThinker))
				numsaved++;

			if (th->function.acp1 == (actionf_p1)P_MobjThinker)
			{
				SaveMobjThinker(save, th, tc_mobj);
				continue;
			}
	#ifdef PARANOIA
			else if (th->function.acp1 == (actionf_p1)P_NullPrecipThinker);
	#endif
			else if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
			{
				SaveCeilingThinker(save, th, tc_ceiling);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_CrushCeiling)
			{
				SaveCeilingThinker(save, th, tc_crushceiling);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MoveFloor)
			{
				SaveFloormoveThinker(save, th, tc_floor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_LightningFlash)
			{
				SaveLightflashThinker(save, th, tc_flash);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
			{
				SaveStrobeThinker(save, th, tc_strobe);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Glow)
			{
				SaveGlowThinker(save, th, tc_glow);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_FireFlicker)
			{
				SaveFireflickerThinker(save, th, tc_fireflicker);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MoveElevator)
			{
				SaveElevatorThinker(save, th, tc_elevator);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_ContinuousFalling)
			{
				SaveContinuousFallThinker(save, th, tc_continuousfalling);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_ThwompSector)
			{
				SaveThwompThinker(save, th, tc_thwomp);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_NoEnemiesSector)
			{
				SaveNoEnemiesThinker(save, th, tc_noenemies);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_EachTimeThinker)
			{
				SaveEachTimeThinker(save, th, tc_eachtime);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_RaiseSector)
			{
				SaveRaiseThinker(save, th, tc_raisesector);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_CameraScanner)
			{
				SaveElevatorThinker(save, th, tc_camerascanner);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Scroll)
			{
				SaveScrollThinker(save, th, tc_scroll);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Friction)
			{
				SaveFrictionThinker(save, th, tc_friction);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Pusher)
			{
				SavePusherThinker(save, th, tc_pusher);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_BounceCheese)
			{
				SaveBounceCheeseThinker(save, th, tc_bouncecheese);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_StartCrumble)
			{
				SaveCrumbleThinker(save, th, tc_startcrumble);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MarioBlock)
			{
				SaveMarioBlockThinker(save, th, tc_marioblock);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MarioBlockChecker)
			{
				SaveMarioCheckThinker(save, th, tc_marioblockchecker);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_FloatSector)
			{
				SaveFloatThinker(save, th, tc_floatsector);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_LaserFlash)
			{
				SaveLaserThinker(save, th, tc_laserflash);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_LightFade)
			{
				SaveLightlevelThinker(save, th, tc_lightfade);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_ExecutorDelay)
			{
				SaveExecutorThinker(save, th, tc_executor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Disappear)
			{
				SaveDisappearThinker(save, th, tc_disappear);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Fade)
			{
				SaveFadeThinker(save, th, tc_fade);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_FadeColormap)
			{
				SaveFadeColormapThinker(save, th, tc_fadecolormap);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PlaneDisplace)
			{
				SavePlaneDisplaceThinker(save, th, tc_planedisplace);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjRotate)
			{
				SavePolyrotatetThinker(save, th, tc_polyrotate);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjMove)
			{
				SavePolymoveThinker(save, th, tc_polymove);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjWaypoint)
			{
				SavePolywaypointThinker(save, th, tc_polywaypoint);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyDoorSlide)
			{
				SavePolyslidedoorThinker(save, th, tc_polyslidedoor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyDoorSwing)
			{
				SavePolyswingdoorThinker(save, th, tc_polyswingdoor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjFlag)
			{
				SavePolymoveThinker(save, th, tc_polyflag);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjDisplace)
			{
				SavePolydisplaceThinker(save, th, tc_polydisplace);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjRotDisplace)
			{
				SavePolyrotdisplaceThinker(save, th, tc_polyrotdisplace);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjFade)
			{
				SavePolyfadeThinker(save, th, tc_polyfade);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_DynamicSlopeLine)
			{
				SaveDynamicLineSlopeThinker(save, th, tc_dynslopeline);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_DynamicSlopeVert)
			{
				SaveDynamicVertexSlopeThinker(save, th, tc_dynslopevert);
				continue;
			}
#ifdef PARANOIA
			else
				I_Assert(th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed); // wait garbage collection
#endif
		}

		CONS_Debug(DBG_NETPLAY, "%u thinkers saved in list %d\n", numsaved, i);

		WRITEUINT8(save->p, tc_end);
	}

	TracyCZoneEnd(__zone);
}

static void P_NetArchiveWaypoints(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	waypoint_t *waypoint;
	size_t i;
	size_t numWaypoints = K_GetNumWaypoints();

	WRITEUINT32(save->p, ARCHIVEBLOCK_WAYPOINTS);
	WRITEUINT32(save->p, numWaypoints);

	for (i = 0U; i < numWaypoints; i++) {
		waypoint = K_GetWaypointFromIndex(i);

		// The only thing we save for each waypoint is the mobj.
		// Waypoints should NEVER be completely created or destroyed mid-race as a result of this
		WRITEUINT32(save->p, waypoint->mobj->mobjnum);
	}

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveWaypoints(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	if (READUINT32(save->p) != ARCHIVEBLOCK_WAYPOINTS)
		I_Error("Bad $$$.sav at archive block Waypoints!");
	else {
		UINT32 numArchiveWaypoints = READUINT32(save->p);
		size_t numSpawnedWaypoints = K_GetNumWaypoints();

		if (numArchiveWaypoints != numSpawnedWaypoints) {
			I_Error("Bad $$$.sav: More saved waypoints than created!");
		} else {
			waypoint_t *waypoint;
			UINT32 i;
			UINT32 temp;
			for (i = 0U; i < numArchiveWaypoints; i++) {
				waypoint = K_GetWaypointFromIndex(i);
				temp = READUINT32(save->p);
				waypoint->mobj = NULL;
				if (!P_SetTarget(&waypoint->mobj, P_FindNewPosition(temp))) {
					CONS_Debug(DBG_GAMELOGIC, "waypoint mobj not found for %d\n", i);
				}
			}
		}
	}

	TracyCZoneEnd(__zone);
}

static void P_NetArchiveTubeWaypoints(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i, j;

	for (i = 0; i < NUMTUBEWAYPOINTSEQUENCES; i++)
	{
		WRITEUINT16(save->p, numtubewaypoints[i]);
		for (j = 0; j < numtubewaypoints[i]; j++)
		{
			WRITEUINT32(save->p, SaveMobjnum(tubewaypoints[i][j]));
		}
	}

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveTubeWaypoints(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i, j;
	UINT32 mobjnum;

	for (i = 0; i < NUMTUBEWAYPOINTSEQUENCES; i++)
	{
		numtubewaypoints[i] = READUINT16(save->p);
		for (j = 0; j < numtubewaypoints[i]; j++)
		{
			mobjnum = READUINT32(save->p);
			tubewaypoints[i][j] = NULL;
			if (mobjnum != 0)
				P_SetTarget(&tubewaypoints[i][j], P_FindNewPosition(mobjnum));
		}
	}

	TracyCZoneEnd(__zone);
}

// Now save the pointers, tracer and target, but at load time we must
// relink to this; the savegame contains the old position in the pointer
// field copyed in the info field temporarily, but finally we just search
// for the old position and relink to it.
mobj_t *P_FindNewPosition(UINT32 oldposition)
{
	thinker_t *th;
	mobj_t *mobj;

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mobj = (mobj_t *)th;
		if (mobj->mobjnum != oldposition)
			continue;

		return mobj;
	}
	CONS_Debug(DBG_GAMELOGIC, "mobj %d not found\n", oldposition);
	return NULL;
}

static inline mobj_t *LoadMobj(UINT32 mobjnum)
{
	if (mobjnum == 0) return NULL;
	return (mobj_t *)(size_t)mobjnum;
}

static sector_t *LoadSector(UINT32 sector)
{
	if (sector >= numsectors) return NULL;
	return &sectors[sector];
}

static line_t *LoadLine(UINT32 line)
{
	if (line >= numlines) return NULL;
	return &lines[line];
}

static inline player_t *LoadPlayer(UINT32 player)
{
	if (player >= MAXPLAYERS) return NULL;
	return &players[player];
}

static inline pslope_t *LoadSlope(UINT32 slopeid)
{
	pslope_t *p = slopelist;
	if (slopeid > slopecount) return NULL;
	do
	{
		if (p->id == slopeid)
			return p;
	} while ((p = p->next));
	return NULL;
}

static thinker_t* LoadMobjThinker(savebuffer_t *save, actionf_p1 thinker)
{
	mobj_t *mobj;
	UINT32 diff;
	UINT32 diff2;
	UINT32 diff3;
	INT32 i;
	fixed_t z, floorz, ceilingz;
	ffloor_t *floorrover = NULL, *ceilingrover = NULL;
	size_t j;

	diff = READUINT32(save->p);
	if (diff & MD_MORE)
		diff2 = READUINT32(save->p);
	else
		diff2 = 0;

	if (diff2 & MD2_MORE)
		diff3 = READUINT32(save->p);
	else
		diff3 = 0;

	z = READFIXED(save->p); // Force this so 3dfloor problems don't arise.
	floorz = READFIXED(save->p);
	ceilingz = READFIXED(save->p);

	if (diff2 & MD2_FLOORROVER)
	{
		sector_t *sec = LoadSector(READUINT32(save->p));
		UINT16 id = READUINT16(save->p);
		floorrover = P_GetFFloorByID(sec, id);
	}

	if (diff2 & MD2_CEILINGROVER)
	{
		sector_t *sec = LoadSector(READUINT32(save->p));
		UINT16 id = READUINT16(save->p);
		ceilingrover = P_GetFFloorByID(sec, id);
	}

	if (diff & MD_SPAWNPOINT)
	{
		UINT16 spawnpointnum = READUINT16(save->p);

		if (mapthings[spawnpointnum].type == 1713) // NiGHTS Hoop special case
		{
			P_SpawnHoop(&mapthings[spawnpointnum]);
			return NULL;
		}

		mobj = Z_Calloc(sizeof (*mobj), PU_LEVEL, NULL);

		mobj->spawnpoint = &mapthings[spawnpointnum];
		mapthings[spawnpointnum].mobj = mobj;
	}
	else
		mobj = Z_Calloc(sizeof (*mobj), PU_LEVEL, NULL);

	// declare this as a valid mobj as soon as possible.
	mobj->thinker.function.acp1 = thinker;

	mobj->z = z;
	mobj->floorz = floorz;
	mobj->ceilingz = ceilingz;
	mobj->floorrover = floorrover;
	mobj->ceilingrover = ceilingrover;

	if (diff & MD_TYPE)
		mobj->type = READUINT32(save->p);
	else
	{
		for (i = 0; i < NUMMOBJTYPES; i++)
			if (mobj->spawnpoint && mobj->spawnpoint->type == mobjinfo[i].doomednum)
				break;
		if (i == NUMMOBJTYPES)
		{
			if (mobj->spawnpoint)
				CONS_Alert(CONS_ERROR, "Found mobj with unknown map thing type %d\n", mobj->spawnpoint->type);
			else
				CONS_Alert(CONS_ERROR, "Found mobj with unknown map thing type NULL\n");
			I_Error("Netsave corrupted");
		}
		mobj->type = i;
	}
	mobj->info = &mobjinfo[mobj->type];
	if (diff & MD_POS)
	{
		mobj->x = mobj->old_x = READFIXED(save->p);
		mobj->y = mobj->old_y = READFIXED(save->p);
		mobj->angle = mobj->old_angle = READANGLE(save->p);
		mobj->pitch = mobj->old_pitch = READANGLE(save->p);
		mobj->roll = mobj->old_roll = READANGLE(save->p);
	}
	else
	{
		mobj->x = mobj->old_x = mobj->spawnpoint->x << FRACBITS;
		mobj->y = mobj->old_y = mobj->spawnpoint->y << FRACBITS;
		mobj->angle = mobj->old_angle = FixedAngle(mobj->spawnpoint->angle*FRACUNIT);
		mobj->pitch = mobj->old_pitch = FixedAngle(mobj->spawnpoint->pitch*FRACUNIT);
		mobj->roll = mobj->old_roll = FixedAngle(mobj->spawnpoint->roll*FRACUNIT);
	}
	if (diff & MD_MOM)
	{
		mobj->momx = READFIXED(save->p);
		mobj->momy = READFIXED(save->p);
		mobj->momz = READFIXED(save->p);
		mobj->pmomz = READFIXED(save->p);
		mobj->lastmomz = READFIXED(save->p);
	} // otherwise they're zero, and the memset took care of it

	if (diff & MD_RADIUS)
		mobj->radius = READFIXED(save->p);
	else
		mobj->radius = mobj->info->radius;
	if (diff & MD_HEIGHT)
		mobj->height = READFIXED(save->p);
	else
		mobj->height = mobj->info->height;
	if (diff & MD_FLAGS)
		mobj->flags = READUINT32(save->p);
	else
		mobj->flags = mobj->info->flags;
	if (diff & MD_FLAGS2)
		mobj->flags2 = READUINT32(save->p);
	if (diff & MD_HEALTH)
		mobj->health = READINT32(save->p);
	else
		mobj->health = mobj->info->spawnhealth;
	if (diff & MD_RTIME)
		mobj->reactiontime = READINT32(save->p);
	else
		mobj->reactiontime = mobj->info->reactiontime;

	if (diff & MD_STATE)
		mobj->state = &states[READUINT16(save->p)];
	else
		mobj->state = &states[mobj->info->spawnstate];
	if (diff & MD_TICS)
		mobj->tics = READINT32(save->p);
	else
		mobj->tics = mobj->state->tics;
	if (diff & MD_SPRITE) {
		mobj->sprite = READUINT16(save->p);
		if (mobj->sprite == SPR_PLAY)
			mobj->sprite2 = READUINT8(save->p);
	}
	else {
		mobj->sprite = mobj->state->sprite;
		if (mobj->sprite == SPR_PLAY)
			mobj->sprite2 = mobj->state->frame&FF_FRAMEMASK;
	}
	if (diff & MD_FRAME)
	{
		mobj->frame = READUINT32(save->p);
		mobj->anim_duration = READUINT16(save->p);
	}
	else
	{
		mobj->frame = mobj->state->frame;
		mobj->anim_duration = (UINT16)mobj->state->var2;
	}
	if (diff & MD_EFLAGS)
		mobj->eflags = READUINT16(save->p);
	if (diff & MD_PLAYER)
	{
		i = READUINT8(save->p);
		mobj->player = &players[i];
		mobj->player->mo = mobj;
	}
	if (diff & MD_MOVEDIR)
		mobj->movedir = READANGLE(save->p);
	if (diff & MD_MOVECOUNT)
		mobj->movecount = READINT32(save->p);
	if (diff & MD_THRESHOLD)
		mobj->threshold = READINT32(save->p);
	if (diff & MD_LASTLOOK)
		mobj->lastlook = READINT32(save->p);
	else
		mobj->lastlook = -1;
	if (diff & MD_TARGET)
		mobj->target = (mobj_t *)(size_t)READUINT32(save->p);
	if (diff & MD_TRACER)
		mobj->tracer = (mobj_t *)(size_t)READUINT32(save->p);
	if (diff & MD_FRICTION)
		mobj->friction = READFIXED(save->p);
	else
		mobj->friction = ORIG_FRICTION;
	if (diff & MD_MOVEFACTOR)
		mobj->movefactor = READFIXED(save->p);
	else
		mobj->movefactor = FRACUNIT;
	if (diff & MD_FUSE)
		mobj->fuse = READINT32(save->p);
	if (diff & MD_WATERTOP)
		mobj->watertop = READFIXED(save->p);
	if (diff & MD_WATERBOTTOM)
		mobj->waterbottom = READFIXED(save->p);
	if (diff & MD_SCALE)
		mobj->scale = READFIXED(save->p);
	else
		mobj->scale = FRACUNIT;
	if (diff & MD_DSCALE)
		mobj->destscale = READFIXED(save->p);
	else
		mobj->destscale = mobj->scale;
	if (diff2 & MD2_SCALESPEED)
		mobj->scalespeed = READFIXED(save->p);
	else
		mobj->scalespeed = mapobjectscale/12;
	if (diff & MD_ARGS)
	{
		for (j = 0; j < NUM_MAPTHING_ARGS; j++)
			mobj->thing_args[j] = READINT32(save->p);
	}
	if (diff & MD_STRINGARGS)
	{
		for (j = 0; j < NUM_MAPTHING_STRINGARGS; j++)
		{
			size_t len = READINT32(save->p);
			size_t k;

			if (!len)
			{
				Z_Free(mobj->thing_stringargs[j]);
				mobj->thing_stringargs[j] = NULL;
				continue;
			}

			mobj->thing_stringargs[j] = Z_Realloc(mobj->thing_stringargs[j], len + 1, PU_LEVEL, NULL);
			for (k = 0; k < len; k++)
				mobj->thing_stringargs[j][k] = READCHAR(save->p);
			mobj->thing_stringargs[j][len] = '\0';
		}
	}
	if (diff2 & MD2_CUSVAL)
		mobj->cusval = READINT32(save->p);
	if (diff2 & MD2_CVMEM)
		mobj->cvmem = READINT32(save->p);
	if (diff2 & MD2_SKIN)
		mobj->skin = &skins[READUINT8(save->p)];
	if (diff2 & MD2_COLOR)
		mobj->color = READUINT16(save->p);
	if (diff2 & MD2_EXTVAL1)
		mobj->extravalue1 = READINT32(save->p);
	if (diff2 & MD2_EXTVAL2)
		mobj->extravalue2 = READINT32(save->p);
	if (diff2 & MD2_HNEXT)
		mobj->hnext = (mobj_t *)(size_t)READUINT32(save->p);
	if (diff2 & MD2_HPREV)
		mobj->hprev = (mobj_t *)(size_t)READUINT32(save->p);
	if (diff2 & MD2_ITNEXT)
		mobj->itnext = (mobj_t *)(size_t)READUINT32(save->p);
	if (diff2 & MD2_SLOPE)
		mobj->standingslope = P_SlopeById(READUINT16(save->p));
	if (diff2 & MD2_COLORIZED)
		mobj->colorized = READUINT8(save->p);
	if (diff2 & MD2_MIRRORED)
		mobj->mirrored = READUINT8(save->p);
	if (diff2 & MD2_ROLLANGLE)
		mobj->rollangle = READANGLE(save->p);
	if (diff2 & MD2_SHADOWSCALE)
	{
		mobj->shadowscale = READFIXED(save->p);
		mobj->whiteshadow = READUINT8(save->p);
		mobj->shadowcolor = READUINT8(save->p);
	}
	if (diff2 & MD2_RENDERFLAGS)
		mobj->renderflags = READUINT32(save->p);
	if (diff2 & MD2_TID)
		mobj->tid = READINT16(save->p);
	if (diff2 & MD2_SPRITESCALE)
	{
		mobj->spritexscale = READFIXED(save->p);
		mobj->spriteyscale = READFIXED(save->p);
	}
	else
	{
		mobj->spritexscale = mobj->spriteyscale = FRACUNIT;
	}
	if (diff2 & MD2_SPRITEOFFSET)
	{
		mobj->spritexoffset = READFIXED(save->p);
		mobj->spriteyoffset = READFIXED(save->p);
	}
	else
	{
		mobj->spritexoffset = mobj->spriteyoffset = 0;
	}
	if (diff2 & MD2_WORLDOFFSET)
	{
		mobj->sprxoff = READFIXED(save->p);
		mobj->spryoff = READFIXED(save->p);
		mobj->sprzoff = READFIXED(save->p);
	}
	else
	{
		mobj->sprxoff = mobj->spryoff = mobj->sprzoff = 0;
	}
	if (diff2 & MD2_SPECIAL)
	{
		mobj->special = READINT16(save->p);

		for (j = 0; j < NUM_SCRIPT_ARGS; j++)
			mobj->script_args[j] = READINT32(save->p);

		for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
		{
			size_t len = READINT32(save->p);
			size_t k;

			if (!len)
			{
				Z_Free(mobj->script_stringargs[j]);
				mobj->script_stringargs[j] = NULL;
				continue;
			}

			mobj->script_stringargs[j] = Z_Realloc(mobj->script_stringargs[j], len + 1, PU_LEVEL, NULL);
			for (k = 0; k < len; k++)
				mobj->script_stringargs[j][k] = READCHAR(save->p);
			mobj->script_stringargs[j][len] = '\0';
		}
	}
	else if (mobj->spawnpoint)
	{
		P_CopyMapThingSpecialFieldsToMobj(mobj->spawnpoint, mobj);
	}
	if (diff2 & MD2_FLOORSPRITESLOPE)
	{
		pslope_t *slope = (pslope_t *)P_CreateFloorSpriteSlope(mobj);

		slope->zdelta = READFIXED(save->p);
		slope->zangle = READANGLE(save->p);
		slope->xydirection = READANGLE(save->p);

		slope->o.x = READFIXED(save->p);
		slope->o.y = READFIXED(save->p);
		slope->o.z = READFIXED(save->p);

		slope->d.x = READFIXED(save->p);
		slope->d.y = READFIXED(save->p);

		slope->normal.x = READFIXED(save->p);
		slope->normal.y = READFIXED(save->p);
		slope->normal.z = READFIXED(save->p);

		P_UpdateSlopeLightOffset(slope);
	}
	if (diff2 & MD2_HITLAG)
	{
		mobj->hitlag = READINT32(save->p);
	}
	if (diff2 & MD2_WATERSKIP)
	{
		mobj->waterskip = READUINT8(save->p);
	}
	if (diff2 & MD2_DISPOFFSET)
	{
		mobj->dispoffset = READINT32(save->p);
	}
	if (diff2 & MD2_FROZEN)
	{
		mobj->frozen = (boolean)READUINT8(save->p);
	}
	if (diff2 & MD2_TERRAIN)
	{
		mobj->terrain = (terrain_t *)(size_t)READUINT32(save->p);
		mobj->terrainOverlay = (mobj_t *)(size_t)READUINT32(save->p);
	}
	else
	{
		mobj->terrain = NULL;
	}

	if (diff3 & MD3_LIGHTLEVEL)
	{
		mobj->lightlevel = READINT16(save->p);
	}
	if (diff3 & MD3_REAPPEAR)
	{
		mobj->reappear = READUINT32(save->p);
	}
	if (diff3 & MD3_PUNT_REF)
	{
		mobj->punt_ref = (mobj_t *)(size_t)READUINT32(save->p);
	}
	if (diff3 & MD3_OWNER)
	{
		mobj->owner = (mobj_t *)(size_t)READUINT32(save->p);
	}

	// link tid set earlier
	P_AddThingTID(mobj);

	// set sprev, snext, bprev, bnext, subsector
	P_SetThingPosition(mobj);

	mobj->mobjnum = READUINT32(save->p);

	if (mobj->player)
	{
		if (mobj->eflags & MFE_VERTICALFLIP)
			mobj->player->viewz = mobj->z + mobj->height - mobj->player->viewheight;
		else
			mobj->player->viewz = mobj->player->mo->z + mobj->player->viewheight;
	}

	if (mobj->type == MT_SKYBOX && mobj->spawnpoint)
	{
		P_InitSkyboxPoint(mobj, mobj->spawnpoint);
	}
	else if (mobj->type == MT_SPRAYCAN)
	{
		P_SprayCanInit(mobj);
	}

	if (diff2 & MD2_WAYPOINTCAP)
		P_SetTarget(&waypointcap, mobj);

	if (diff2 & MD2_KITEMCAP)
		P_SetTarget(&trackercap, mobj);

	R_AddMobjInterpolator(mobj);

	return &mobj->thinker;
}

static thinker_t* LoadNoEnemiesThinker(savebuffer_t *save, actionf_p1 thinker)
{
	noenemies_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(READUINT32(save->p));
	return &ht->thinker;
}

static thinker_t* LoadBounceCheeseThinker(savebuffer_t *save, actionf_p1 thinker)
{
	bouncecheese_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(READUINT32(save->p));
	ht->sector = LoadSector(READUINT32(save->p));
	ht->speed = READFIXED(save->p);
	ht->distance = READFIXED(save->p);
	ht->floorwasheight = READFIXED(save->p);
	ht->ceilingwasheight = READFIXED(save->p);
	ht->low = READCHAR(save->p);

	if (ht->sector)
		ht->sector->ceilingdata = ht;

	return &ht->thinker;
}

static thinker_t* LoadContinuousFallThinker(savebuffer_t *save, actionf_p1 thinker)
{
	continuousfall_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->speed = READFIXED(save->p);
	ht->direction = READINT32(save->p);
	ht->floorstartheight = READFIXED(save->p);
	ht->ceilingstartheight = READFIXED(save->p);
	ht->destheight = READFIXED(save->p);

	if (ht->sector)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadMarioBlockThinker(savebuffer_t *save, actionf_p1 thinker)
{
	mariothink_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->speed = READFIXED(save->p);
	ht->direction = READINT32(save->p);
	ht->floorstartheight = READFIXED(save->p);
	ht->ceilingstartheight = READFIXED(save->p);
	ht->tag = READINT16(save->p);

	if (ht->sector)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadMarioCheckThinker(savebuffer_t *save, actionf_p1 thinker)
{
	mariocheck_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(READUINT32(save->p));
	ht->sector = LoadSector(READUINT32(save->p));
	return &ht->thinker;
}

static thinker_t* LoadThwompThinker(savebuffer_t *save, actionf_p1 thinker)
{
	thwomp_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(READUINT32(save->p));
	ht->sector = LoadSector(READUINT32(save->p));
	ht->crushspeed = READFIXED(save->p);
	ht->retractspeed = READFIXED(save->p);
	ht->direction = READINT32(save->p);
	ht->floorstartheight = READFIXED(save->p);
	ht->ceilingstartheight = READFIXED(save->p);
	ht->delay = READINT32(save->p);
	ht->tag = READINT16(save->p);
	ht->sound = READUINT16(save->p);

	if (ht->sector)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadFloatThinker(savebuffer_t *save, actionf_p1 thinker)
{
	floatthink_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(READUINT32(save->p));
	ht->sector = LoadSector(READUINT32(save->p));
	ht->tag = READINT16(save->p);
	return &ht->thinker;
}

static thinker_t* LoadEachTimeThinker(savebuffer_t *save, actionf_p1 thinker)
{
	size_t i;
	eachtime_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(READUINT32(save->p));
	for (i = 0; i < MAXPLAYERS; i++)
	{
		ht->playersInArea[i] = READCHAR(save->p);
	}
	ht->triggerOnExit = READCHAR(save->p);
	return &ht->thinker;
}

static thinker_t* LoadRaiseThinker(savebuffer_t *save, actionf_p1 thinker)
{
	raise_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->tag = READINT16(save->p);
	ht->sector = LoadSector(READUINT32(save->p));
	ht->ceilingbottom = READFIXED(save->p);
	ht->ceilingtop = READFIXED(save->p);
	ht->basespeed = READFIXED(save->p);
	ht->extraspeed = READFIXED(save->p);
	ht->shaketimer = READUINT8(save->p);
	ht->flags = READUINT8(save->p);
	return &ht->thinker;
}

static thinker_t* LoadCeilingThinker(savebuffer_t *save, actionf_p1 thinker)
{
	ceiling_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = READUINT8(save->p);
	ht->sector = LoadSector(READUINT32(save->p));
	ht->bottomheight = READFIXED(save->p);
	ht->topheight = READFIXED(save->p);
	ht->speed = READFIXED(save->p);
	ht->delay = READFIXED(save->p);
	ht->delaytimer = READFIXED(save->p);
	ht->crush = READUINT8(save->p);
	ht->texture = READINT32(save->p);
	ht->direction = READINT32(save->p);
	ht->tag = READINT16(save->p);
	ht->origspeed = READFIXED(save->p);
	ht->crushHeight = READFIXED(save->p);
	ht->crushSpeed = READFIXED(save->p);
	ht->returnHeight = READFIXED(save->p);
	ht->returnSpeed = READFIXED(save->p);
	if (ht->sector)
		ht->sector->ceilingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadFloormoveThinker(savebuffer_t *save, actionf_p1 thinker)
{
	floormove_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = READUINT8(save->p);
	ht->crush = READUINT8(save->p);
	ht->sector = LoadSector(READUINT32(save->p));
	ht->direction = READINT32(save->p);
	ht->texture = READINT32(save->p);
	ht->floordestheight = READFIXED(save->p);
	ht->speed = READFIXED(save->p);
	ht->origspeed = READFIXED(save->p);
	ht->delay = READFIXED(save->p);
	ht->delaytimer = READFIXED(save->p);
	ht->tag = READINT16(save->p);
	ht->crushHeight = READFIXED(save->p);
	ht->crushSpeed = READFIXED(save->p);
	ht->returnHeight = READFIXED(save->p);
	ht->returnSpeed = READFIXED(save->p);
	if (ht->sector)
		ht->sector->floordata = ht;
	return &ht->thinker;
}

static thinker_t* LoadLightflashThinker(savebuffer_t *save, actionf_p1 thinker)
{
	lightflash_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->maxlight = READINT32(save->p);
	ht->minlight = READINT32(save->p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadStrobeThinker(savebuffer_t *save, actionf_p1 thinker)
{
	strobe_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->count = READINT32(save->p);
	ht->minlight = READINT16(save->p);
	ht->maxlight = READINT16(save->p);
	ht->darktime = READINT32(save->p);
	ht->brighttime = READINT32(save->p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadGlowThinker(savebuffer_t *save, actionf_p1 thinker)
{
	glow_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->minlight = READINT16(save->p);
	ht->maxlight = READINT16(save->p);
	ht->direction = READINT16(save->p);
	ht->speed = READINT16(save->p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadFireflickerThinker(savebuffer_t *save, actionf_p1 thinker)
{
	fireflicker_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->count = READINT32(save->p);
	ht->resetcount = READINT32(save->p);
	ht->maxlight = READINT16(save->p);
	ht->minlight = READINT16(save->p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadElevatorThinker(savebuffer_t *save, actionf_p1 thinker, boolean setplanedata)
{
	elevator_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = READUINT8(save->p);
	ht->sector = LoadSector(READUINT32(save->p));
	ht->actionsector = LoadSector(READUINT32(save->p));
	ht->direction = READINT32(save->p);
	ht->floordestheight = READFIXED(save->p);
	ht->ceilingdestheight = READFIXED(save->p);
	ht->speed = READFIXED(save->p);
	ht->origspeed = READFIXED(save->p);
	ht->low = READFIXED(save->p);
	ht->high = READFIXED(save->p);
	ht->distance = READFIXED(save->p);
	ht->delay = READFIXED(save->p);
	ht->delaytimer = READFIXED(save->p);
	ht->floorwasheight = READFIXED(save->p);
	ht->ceilingwasheight = READFIXED(save->p);

	if (ht->sector && setplanedata)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadCrumbleThinker(savebuffer_t *save, actionf_p1 thinker)
{
	crumble_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(READUINT32(save->p));
	ht->sector = LoadSector(READUINT32(save->p));
	ht->actionsector = LoadSector(READUINT32(save->p));
	ht->player = LoadPlayer(READUINT32(save->p));
	ht->direction = READINT32(save->p);
	ht->origalpha = READINT32(save->p);
	ht->timer = READINT32(save->p);
	ht->speed = READFIXED(save->p);
	ht->floorwasheight = READFIXED(save->p);
	ht->ceilingwasheight = READFIXED(save->p);
	ht->flags = READUINT8(save->p);

	if (ht->sector)
		ht->sector->floordata = ht;

	return &ht->thinker;
}

static thinker_t* LoadScrollThinker(savebuffer_t *save, actionf_p1 thinker)
{
	scroll_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->dx = READFIXED(save->p);
	ht->dy = READFIXED(save->p);
	ht->affectee = READINT32(save->p);
	ht->control = READINT32(save->p);
	ht->last_height = READFIXED(save->p);
	ht->vdx = READFIXED(save->p);
	ht->vdy = READFIXED(save->p);
	ht->accel = READINT32(save->p);
	ht->exclusive = READINT32(save->p);
	ht->type = READUINT8(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadFrictionThinker(savebuffer_t *save, actionf_p1 thinker)
{
	friction_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->friction = READINT32(save->p);
	ht->movefactor = READINT32(save->p);
	ht->affectee = READINT32(save->p);
	ht->referrer = READINT32(save->p);
	ht->roverfriction = READUINT8(save->p);
	return &ht->thinker;
}

static thinker_t* LoadPusherThinker(savebuffer_t *save, actionf_p1 thinker)
{
	pusher_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = READUINT8(save->p);
	ht->x_mag = READFIXED(save->p);
	ht->y_mag = READFIXED(save->p);
	ht->z_mag = READFIXED(save->p);
	ht->affectee = READINT32(save->p);
	ht->roverpusher = READUINT8(save->p);
	ht->referrer = READINT32(save->p);
	ht->exclusive = READINT32(save->p);
	ht->slider = READINT32(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadLaserThinker(savebuffer_t *save, actionf_p1 thinker)
{
	laserthink_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->tag = READINT16(save->p);
	ht->sourceline = LoadLine(READUINT32(save->p));
	ht->nobosses = READUINT8(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadLightlevelThinker(savebuffer_t *save, actionf_p1 thinker)
{
	lightlevel_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->sourcelevel = READINT16(save->p);
	ht->destlevel = READINT16(save->p);
	ht->fixedcurlevel = READFIXED(save->p);
	ht->fixedpertic = READFIXED(save->p);
	ht->timer = READINT32(save->p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static inline thinker_t* LoadExecutorThinker(savebuffer_t *save, actionf_p1 thinker)
{
	executor_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->line = LoadLine(READUINT32(save->p));
	ht->caller = LoadMobj(READUINT32(save->p));
	ht->sector = LoadSector(READUINT32(save->p));
	ht->timer = READINT32(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadDisappearThinker(savebuffer_t *save, actionf_p1 thinker)
{
	disappear_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->appeartime = READUINT32(save->p);
	ht->disappeartime = READUINT32(save->p);
	ht->offset = READUINT32(save->p);
	ht->timer = READUINT32(save->p);
	ht->affectee = READINT32(save->p);
	ht->sourceline = READINT32(save->p);
	ht->exists = READINT32(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadFadeThinker(savebuffer_t *save, actionf_p1 thinker)
{
	sector_t *ss;
	fade_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->dest_exc = GetNetColormapFromList(READUINT32(save->p));
	ht->sectornum = READUINT32(save->p);
	ht->ffloornum = READUINT32(save->p);
	ht->alpha = READINT32(save->p);
	ht->sourcevalue = READINT16(save->p);
	ht->destvalue = READINT16(save->p);
	ht->destlightlevel = READINT16(save->p);
	ht->speed = READINT16(save->p);
	ht->ticbased = (boolean)READUINT8(save->p);
	ht->timer = READINT32(save->p);
	ht->doexists = READUINT8(save->p);
	ht->dotranslucent = READUINT8(save->p);
	ht->dolighting = READUINT8(save->p);
	ht->docolormap = READUINT8(save->p);
	ht->docollision = READUINT8(save->p);
	ht->doghostfade = READUINT8(save->p);
	ht->exactalpha = READUINT8(save->p);

	ss = LoadSector(ht->sectornum);
	if (ss)
	{
		size_t j = 0; // ss->ffloors is saved as ffloor #0, ss->ffloors->next is #1, etc
		ffloor_t *rover;
		for (rover = ss->ffloors; rover; rover = rover->next)
		{
			if (j == ht->ffloornum)
			{
				ht->rover = rover;
				rover->fadingdata = ht;
				break;
			}
			j++;
		}
	}
	return &ht->thinker;
}

static inline thinker_t* LoadFadeColormapThinker(savebuffer_t *save, actionf_p1 thinker)
{
	fadecolormap_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(READUINT32(save->p));
	ht->source_exc = GetNetColormapFromList(READUINT32(save->p));
	ht->dest_exc = GetNetColormapFromList(READUINT32(save->p));
	ht->ticbased = (boolean)READUINT8(save->p);
	ht->duration = READINT32(save->p);
	ht->timer = READINT32(save->p);
	if (ht->sector)
		ht->sector->fadecolormapdata = ht;
	return &ht->thinker;
}

static inline thinker_t* LoadPlaneDisplaceThinker(savebuffer_t *save, actionf_p1 thinker)
{
	planedisplace_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;

	ht->affectee = READINT32(save->p);
	ht->control = READINT32(save->p);
	ht->last_height = READFIXED(save->p);
	ht->speed = READFIXED(save->p);
	ht->type = READUINT8(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadDynamicLineSlopeThinker(savebuffer_t *save, actionf_p1 thinker)
{
	dynlineplanethink_t* ht = Z_Malloc(sizeof(*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;

	ht->type = READUINT8(save->p);
	ht->slope = LoadSlope(READUINT32(save->p));
	ht->sourceline = LoadLine(READUINT32(save->p));
	ht->extent = READFIXED(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadDynamicVertexSlopeThinker(savebuffer_t *save, actionf_p1 thinker)
{
	size_t i;
	dynvertexplanethink_t* ht = Z_Malloc(sizeof(*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;

	ht->slope = LoadSlope(READUINT32(save->p));
	for (i = 0; i < 3; i++)
		ht->secs[i] = LoadSector(READUINT32(save->p));
	READMEM(save->p, ht->vex, sizeof(ht->vex));
	READMEM(save->p, ht->origsecheights, sizeof(ht->origsecheights));
	READMEM(save->p, ht->origvecheights, sizeof(ht->origvecheights));
	ht->relative = READUINT8(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyrotatetThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polyrotate_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->speed = READINT32(save->p);
	ht->distance = READINT32(save->p);
	ht->turnobjs = READUINT8(save->p);
	return &ht->thinker;
}

static thinker_t* LoadPolymoveThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polymove_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->speed = READINT32(save->p);
	ht->momx = READFIXED(save->p);
	ht->momy = READFIXED(save->p);
	ht->distance = READINT32(save->p);
	ht->angle = READANGLE(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolywaypointThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polywaypoint_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->speed = READINT32(save->p);
	ht->sequence = READINT32(save->p);
	ht->pointnum = READINT32(save->p);
	ht->direction = READINT32(save->p);
	ht->returnbehavior = READUINT8(save->p);
	ht->continuous = READUINT8(save->p);
	ht->stophere = READUINT8(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyslidedoorThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polyslidedoor_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->delay = READINT32(save->p);
	ht->delayCount = READINT32(save->p);
	ht->initSpeed = READINT32(save->p);
	ht->speed = READINT32(save->p);
	ht->initDistance = READINT32(save->p);
	ht->distance = READINT32(save->p);
	ht->initAngle = READUINT32(save->p);
	ht->angle = READUINT32(save->p);
	ht->revAngle = READUINT32(save->p);
	ht->momx = READFIXED(save->p);
	ht->momy = READFIXED(save->p);
	ht->closing = READUINT8(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyswingdoorThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polyswingdoor_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->delay = READINT32(save->p);
	ht->delayCount = READINT32(save->p);
	ht->initSpeed = READINT32(save->p);
	ht->speed = READINT32(save->p);
	ht->initDistance = READINT32(save->p);
	ht->distance = READINT32(save->p);
	ht->closing = READUINT8(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolydisplaceThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polydisplace_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->controlSector = LoadSector(READUINT32(save->p));
	ht->dx = READFIXED(save->p);
	ht->dy = READFIXED(save->p);
	ht->oldHeights = READFIXED(save->p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyrotdisplaceThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polyrotdisplace_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->controlSector = LoadSector(READUINT32(save->p));
	ht->rotscale = READFIXED(save->p);
	ht->turnobjs = READUINT8(save->p);
	ht->oldHeights = READFIXED(save->p);
	return &ht->thinker;
}

static thinker_t* LoadPolyfadeThinker(savebuffer_t *save, actionf_p1 thinker)
{
	polyfade_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = READINT32(save->p);
	ht->sourcevalue = READINT32(save->p);
	ht->destvalue = READINT32(save->p);
	ht->docollision = (boolean)READUINT8(save->p);
	ht->doghostfade = (boolean)READUINT8(save->p);
	ht->ticbased = (boolean)READUINT8(save->p);
	ht->duration = READINT32(save->p);
	ht->timer = READINT32(save->p);
	return &ht->thinker;
}

static void ReadMobjPointer(mobj_t **mobj_p)
{
	*mobj_p = LoadMobj(READUINT32(current_savebuffer->p));
}

static void P_NetUnArchiveThinkers(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	thinker_t *currentthinker;
	thinker_t *next;
	UINT8 tclass;
	UINT8 restoreNum = false;
	UINT32 i;
	UINT32 numloaded = 0;

	if (READUINT32(save->p) != ARCHIVEBLOCK_THINKERS)
		I_Error("Bad $$$.sav at archive block Thinkers");

	// remove all the current thinkers
	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		for (currentthinker = thlist[i].next; currentthinker != &thlist[i]; currentthinker = next)
		{
			next = currentthinker->next;

			currentthinker->references = 0; // Heinous but this is the only place the assertion in P_UnlinkThinkers is wrong

			if (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker || currentthinker->function.acp1 == (actionf_p1)P_NullPrecipThinker)
				P_RemoveSavegameMobj((mobj_t *)currentthinker); // item isn't saved, don't remove it
			else
			{
				(next->prev = currentthinker->prev)->next = next;
				R_DestroyLevelInterpolators(currentthinker);
				Z_Free(currentthinker);
			}
		}
	}

	// we don't want the removed mobjs to come back
	P_InitThinkers();

	P_LoadMobjPointers(ReadMobjPointer);

	// clear sector thinker pointers so they don't point to non-existant thinkers for all of eternity
	for (i = 0; i < numsectors; i++)
	{
		sectors[i].floordata = sectors[i].ceilingdata = sectors[i].lightingdata = sectors[i].fadecolormapdata = NULL;
	}

	// read in saved thinkers
	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		for (;;)
		{
			thinker_t* th = NULL;
			tclass = READUINT8(save->p);

			if (tclass == tc_end)
				break; // leave the saved thinker reading loop
			numloaded++;

			switch (tclass)
			{
				case tc_mobj:
					th = LoadMobjThinker(save, (actionf_p1)P_MobjThinker);
					break;

				case tc_ceiling:
					th = LoadCeilingThinker(save, (actionf_p1)T_MoveCeiling);
					break;

				case tc_crushceiling:
					th = LoadCeilingThinker(save, (actionf_p1)T_CrushCeiling);
					break;

				case tc_floor:
					th = LoadFloormoveThinker(save, (actionf_p1)T_MoveFloor);
					break;

				case tc_flash:
					th = LoadLightflashThinker(save, (actionf_p1)T_LightningFlash);
					break;

				case tc_strobe:
					th = LoadStrobeThinker(save, (actionf_p1)T_StrobeFlash);
					break;

				case tc_glow:
					th = LoadGlowThinker(save, (actionf_p1)T_Glow);
					break;

				case tc_fireflicker:
					th = LoadFireflickerThinker(save, (actionf_p1)T_FireFlicker);
					break;

				case tc_elevator:
					th = LoadElevatorThinker(save, (actionf_p1)T_MoveElevator, true);
					break;

				case tc_continuousfalling:
					th = LoadContinuousFallThinker(save, (actionf_p1)T_ContinuousFalling);
					break;

				case tc_thwomp:
					th = LoadThwompThinker(save, (actionf_p1)T_ThwompSector);
					break;

				case tc_noenemies:
					th = LoadNoEnemiesThinker(save, (actionf_p1)T_NoEnemiesSector);
					break;

				case tc_eachtime:
					th = LoadEachTimeThinker(save, (actionf_p1)T_EachTimeThinker);
					break;

				case tc_raisesector:
					th = LoadRaiseThinker(save, (actionf_p1)T_RaiseSector);
					break;

				case tc_camerascanner:
					th = LoadElevatorThinker(save, (actionf_p1)T_CameraScanner, false);
					break;

				case tc_bouncecheese:
					th = LoadBounceCheeseThinker(save, (actionf_p1)T_BounceCheese);
					break;

				case tc_startcrumble:
					th = LoadCrumbleThinker(save, (actionf_p1)T_StartCrumble);
					break;

				case tc_marioblock:
					th = LoadMarioBlockThinker(save, (actionf_p1)T_MarioBlock);
					break;

				case tc_marioblockchecker:
					th = LoadMarioCheckThinker(save, (actionf_p1)T_MarioBlockChecker);
					break;

				case tc_floatsector:
					th = LoadFloatThinker(save, (actionf_p1)T_FloatSector);
					break;

				case tc_laserflash:
					th = LoadLaserThinker(save, (actionf_p1)T_LaserFlash);
					break;

				case tc_lightfade:
					th = LoadLightlevelThinker(save, (actionf_p1)T_LightFade);
					break;

				case tc_executor:
					th = LoadExecutorThinker(save, (actionf_p1)T_ExecutorDelay);
					restoreNum = true;
					break;

				case tc_disappear:
					th = LoadDisappearThinker(save, (actionf_p1)T_Disappear);
					break;

				case tc_fade:
					th = LoadFadeThinker(save, (actionf_p1)T_Fade);
					break;

				case tc_fadecolormap:
					th = LoadFadeColormapThinker(save, (actionf_p1)T_FadeColormap);
					break;

				case tc_planedisplace:
					th = LoadPlaneDisplaceThinker(save, (actionf_p1)T_PlaneDisplace);
					break;
				case tc_polyrotate:
					th = LoadPolyrotatetThinker(save, (actionf_p1)T_PolyObjRotate);
					break;

				case tc_polymove:
					th = LoadPolymoveThinker(save, (actionf_p1)T_PolyObjMove);
					break;

				case tc_polywaypoint:
					th = LoadPolywaypointThinker(save, (actionf_p1)T_PolyObjWaypoint);
					break;

				case tc_polyslidedoor:
					th = LoadPolyslidedoorThinker(save, (actionf_p1)T_PolyDoorSlide);
					break;

				case tc_polyswingdoor:
					th = LoadPolyswingdoorThinker(save, (actionf_p1)T_PolyDoorSwing);
					break;

				case tc_polyflag:
					th = LoadPolymoveThinker(save, (actionf_p1)T_PolyObjFlag);
					break;

				case tc_polydisplace:
					th = LoadPolydisplaceThinker(save, (actionf_p1)T_PolyObjDisplace);
					break;

				case tc_polyrotdisplace:
					th = LoadPolyrotdisplaceThinker(save, (actionf_p1)T_PolyObjRotDisplace);
					break;

				case tc_polyfade:
					th = LoadPolyfadeThinker(save, (actionf_p1)T_PolyObjFade);
					break;

				case tc_dynslopeline:
					th = LoadDynamicLineSlopeThinker(save, (actionf_p1)T_DynamicSlopeLine);
					break;

				case tc_dynslopevert:
					th = LoadDynamicVertexSlopeThinker(save, (actionf_p1)T_DynamicSlopeVert);
					break;

				case tc_scroll:
					th = LoadScrollThinker(save, (actionf_p1)T_Scroll);
					break;

				case tc_friction:
					th = LoadFrictionThinker(save, (actionf_p1)T_Friction);
					break;

				case tc_pusher:
					th = LoadPusherThinker(save, (actionf_p1)T_Pusher);
					break;

				default:
					I_Error("P_UnarchiveSpecials: Unknown tclass %d in savegame", tclass);
			}
			if (th)
				P_AddThinker(i, th);
		}

		CONS_Debug(DBG_NETPLAY, "%u thinkers loaded in list %d\n", numloaded, i);
	}

	if (restoreNum)
	{
		executor_t *delay = NULL;
		UINT32 mobjnum;
		for (currentthinker = thlist[THINK_MAIN].next; currentthinker != &thlist[THINK_MAIN]; currentthinker = currentthinker->next)
		{
			if (currentthinker->function.acp1 != (actionf_p1)T_ExecutorDelay)
				continue;
			delay = (void *)currentthinker;
			if (!(mobjnum = (UINT32)(size_t)delay->caller))
				continue;
			delay->caller = P_FindNewPosition(mobjnum);
		}
	}

	TracyCZoneEnd(__zone);
}

///////////////////////////////////////////////////////////////////////////////
//
// haleyjd 03/26/06: PolyObject saving code
//
#define PD_FLAGS  0x01
#define PD_TRANS   0x02

static inline void P_ArchivePolyObj(savebuffer_t *save, polyobj_t *po)
{
	TracyCZone(__zone, true);

	UINT8 diff = 0;
	WRITEINT32(save->p, po->id);
	WRITEANGLE(save->p, po->angle);

	WRITEFIXED(save->p, po->spawnSpot.x);
	WRITEFIXED(save->p, po->spawnSpot.y);

	if (po->flags != po->spawnflags)
		diff |= PD_FLAGS;
	if (po->translucency != po->spawntrans)
		diff |= PD_TRANS;

	WRITEUINT8(save->p, diff);

	if (diff & PD_FLAGS)
		WRITEINT32(save->p, po->flags);
	if (diff & PD_TRANS)
		WRITEINT32(save->p, po->translucency);

	TracyCZoneEnd(__zone);
}

static inline void P_UnArchivePolyObj(savebuffer_t *save, polyobj_t *po)
{
	TracyCZone(__zone, true);

	INT32 id;
	UINT32 angle;
	fixed_t x, y;
	UINT8 diff;

	// nullify all polyobject thinker pointers;
	// the thinkers themselves will fight over who gets the field
	// when they first start to run.
	po->thinker = NULL;

	id = READINT32(save->p);

	angle = READANGLE(save->p);

	x = READFIXED(save->p);
	y = READFIXED(save->p);

	diff = READUINT8(save->p);

	if (diff & PD_FLAGS)
		po->flags = READINT32(save->p);
	if (diff & PD_TRANS)
		po->translucency = READINT32(save->p);

	// if the object is bad or isn't in the id hash, we can do nothing more
	// with it, so return now
	if (po->isBad || po != Polyobj_GetForNum(id))
		return;

	// rotate and translate polyobject
	Polyobj_MoveOnLoad(po, angle, x, y);

	TracyCZoneEnd(__zone);
}

static inline void P_ArchivePolyObjects(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i;

	WRITEUINT32(save->p, ARCHIVEBLOCK_POBJS);

	// save number of polyobjects
	WRITEINT32(save->p, numPolyObjects);

	for (i = 0; i < numPolyObjects; ++i)
		P_ArchivePolyObj(save, &PolyObjects[i]);

	TracyCZoneEnd(__zone);
}

static inline void P_UnArchivePolyObjects(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	INT32 i, numSavedPolys;

	if (READUINT32(save->p) != ARCHIVEBLOCK_POBJS)
		I_Error("Bad $$$.sav at archive block Pobjs");

	numSavedPolys = READINT32(save->p);

	if (numSavedPolys != numPolyObjects)
		I_Error("P_UnArchivePolyObjects: polyobj count inconsistency\n");

	for (i = 0; i < numSavedPolys; ++i)
		P_UnArchivePolyObj(save, &PolyObjects[i]);

	TracyCZoneEnd(__zone);
}

static mobj_t *RelinkMobj(mobj_t **ptr)
{
	UINT32 temp = (UINT32)(size_t)*ptr;
	*ptr = NULL;
	return P_SetTarget(ptr, P_FindNewPosition(temp));
}

static void RelinkMobjVoid(mobj_t **ptr)
{
	RelinkMobj(ptr);
}

static void P_RelinkPointers(void)
{
	thinker_t *currentthinker;
	mobj_t *mobj;
	UINT32 temp, i;

	P_LoadMobjPointers(RelinkMobjVoid);

	if (g_endcam.panMobj)
	{
		if (!RelinkMobj(&g_endcam.panMobj))
			CONS_Debug(DBG_GAMELOGIC, "g_endcam.panMobj not found\n");
	}

	// use info field (value = oldposition) to relink mobjs
	for (currentthinker = thlist[THINK_MOBJ].next; currentthinker != &thlist[THINK_MOBJ];
		currentthinker = currentthinker->next)
	{
		if (currentthinker->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mobj = (mobj_t *)currentthinker;

		if (TypeIsNetSynced(mobj->type) == false)
			continue;

		if (mobj->tracer)
		{
			if (!RelinkMobj(&mobj->tracer))
				CONS_Debug(DBG_GAMELOGIC, "tracer not found on %d\n", mobj->type);
		}
		if (mobj->target)
		{
			if (!RelinkMobj(&mobj->target))
				CONS_Debug(DBG_GAMELOGIC, "target not found on %d\n", mobj->type);
		}
		if (mobj->hnext)
		{
			if (!RelinkMobj(&mobj->hnext))
				CONS_Debug(DBG_GAMELOGIC, "hnext not found on %d\n", mobj->type);
		}
		if (mobj->hprev)
		{
			if (!RelinkMobj(&mobj->hprev))
				CONS_Debug(DBG_GAMELOGIC, "hprev not found on %d\n", mobj->type);
		}
		if (mobj->itnext)
		{
			if (!RelinkMobj(&mobj->itnext))
				CONS_Debug(DBG_GAMELOGIC, "itnext not found on %d\n", mobj->type);
		}
		if (mobj->terrain)
		{
			temp = (UINT32)(size_t)mobj->terrain;
			mobj->terrain = K_GetTerrainByIndex(temp);
			if (mobj->terrain == NULL)
			{
				CONS_Debug(DBG_GAMELOGIC, "terrain not found on %d\n", mobj->type);
			}
		}
		if (mobj->terrainOverlay)
		{
			if (!RelinkMobj(&mobj->terrainOverlay))
				CONS_Debug(DBG_GAMELOGIC, "terrainOverlay not found on %d\n", mobj->type);
		}
		if (mobj->punt_ref)
		{
			if (!RelinkMobj(&mobj->punt_ref))
				CONS_Debug(DBG_GAMELOGIC, "punt_ref not found on %d\n", mobj->type);
		}
		if (mobj->owner)
		{
			if (!RelinkMobj(&mobj->owner))
				CONS_Debug(DBG_GAMELOGIC, "owner not found on %d\n", mobj->type);
		}
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (players[i].skybox.viewpoint)
		{
			if (!RelinkMobj(&players[i].skybox.viewpoint))
				CONS_Debug(DBG_GAMELOGIC, "skybox.viewpoint not found on player %d\n", i);
		}
		if (players[i].skybox.centerpoint)
		{
			if (!RelinkMobj(&players[i].skybox.centerpoint))
				CONS_Debug(DBG_GAMELOGIC, "skybox.centerpoint not found on player %d\n", i);
		}
		if (players[i].awayview.mobj)
		{
			if (!RelinkMobj(&players[i].awayview.mobj))
				CONS_Debug(DBG_GAMELOGIC, "awayview.mobj not found on player %d\n", i);
		}
		if (players[i].followmobj)
		{
			if (!RelinkMobj(&players[i].followmobj))
				CONS_Debug(DBG_GAMELOGIC, "followmobj not found on player %d\n", i);
		}
		if (players[i].follower)
		{
			if (!RelinkMobj(&players[i].follower))
				CONS_Debug(DBG_GAMELOGIC, "follower not found on player %d\n", i);
		}
		if (players[i].currentwaypoint)
		{
			temp = (UINT32)(size_t)players[i].currentwaypoint;
			players[i].currentwaypoint = K_GetWaypointFromIndex(temp);
			if (players[i].currentwaypoint == NULL)
			{
				CONS_Debug(DBG_GAMELOGIC, "currentwaypoint not found on player %d\n", i);
			}
		}
		if (players[i].nextwaypoint)
		{
			temp = (UINT32)(size_t)players[i].nextwaypoint;
			players[i].nextwaypoint = K_GetWaypointFromIndex(temp);
			if (players[i].nextwaypoint == NULL)
			{
				CONS_Debug(DBG_GAMELOGIC, "nextwaypoint not found on player %d\n", i);
			}
		}
		if (players[i].respawn.wp)
		{
			temp = (UINT32)(size_t)players[i].respawn.wp;
			players[i].respawn.wp = K_GetWaypointFromIndex(temp);
			if (players[i].respawn.wp == NULL)
			{
				CONS_Debug(DBG_GAMELOGIC, "respawn.wp not found on player %d\n", i);
			}
		}
		if (players[i].hoverhyudoro)
		{
			if (!RelinkMobj(&players[i].hoverhyudoro))
				CONS_Debug(DBG_GAMELOGIC, "hoverhyudoro not found on player %d\n", i);
		}
		if (players[i].stumbleIndicator)
		{
			if (!RelinkMobj(&players[i].stumbleIndicator))
				CONS_Debug(DBG_GAMELOGIC, "stumbleIndicator not found on player %d\n", i);
		}
		if (players[i].wavedashIndicator)
		{
			if (!RelinkMobj(&players[i].wavedashIndicator))
				CONS_Debug(DBG_GAMELOGIC, "wavedashIndicator not found on player %d\n", i);
		}
		if (players[i].trickIndicator)
		{
			if (!RelinkMobj(&players[i].trickIndicator))
				CONS_Debug(DBG_GAMELOGIC, "trickIndicator not found on player %d\n", i);
		}
		if (players[i].whip)
		{
			if (!RelinkMobj(&players[i].whip))
				CONS_Debug(DBG_GAMELOGIC, "whip not found on player %d\n", i);
		}
		if (players[i].hand)
		{
			if (!RelinkMobj(&players[i].hand))
				CONS_Debug(DBG_GAMELOGIC, "hand not found on player %d\n", i);
		}
		if (players[i].ringShooter)
		{
			if (!RelinkMobj(&players[i].ringShooter))
				CONS_Debug(DBG_GAMELOGIC, "ringShooter not found on player %d\n", i);
		}
		if (players[i].flickyAttacker)
		{
			if (!RelinkMobj(&players[i].flickyAttacker))
				CONS_Debug(DBG_GAMELOGIC, "flickyAttacker not found on player %d\n", i);
		}
		if (players[i].powerup.flickyController)
		{
			if (!RelinkMobj(&players[i].powerup.flickyController))
				CONS_Debug(DBG_GAMELOGIC, "powerup.flickyController not found on player %d\n", i);
		}
		if (players[i].powerup.barrier)
		{
			if (!RelinkMobj(&players[i].powerup.barrier))
				CONS_Debug(DBG_GAMELOGIC, "powerup.barrier not found on player %d\n", i);
		}
	}
}

static inline void P_NetArchiveSpecials(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	size_t i, z;

	WRITEUINT32(save->p, ARCHIVEBLOCK_SPECIALS);

	// itemrespawn queue for deathmatch
	i = iquetail;
	while (iquehead != i)
	{
		for (z = 0; z < nummapthings; z++)
		{
			if (&mapthings[z] == itemrespawnque[i])
			{
				WRITEUINT32(save->p, z);
				break;
			}
		}
		WRITEUINT32(save->p, itemrespawntime[i]);
		i = (i + 1) & (ITEMQUESIZE-1);
	}

	// end delimiter
	WRITEUINT32(save->p, 0xffffffff);

	// Sky number
	WRITESTRINGN(save->p, globallevelskytexture, 9);

	// Current global weather type
	WRITEUINT8(save->p, globalweather);

	TracyCZoneEnd(__zone);
}

static void P_NetUnArchiveSpecials(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	char skytex[9];
	size_t i;

	if (READUINT32(save->p) != ARCHIVEBLOCK_SPECIALS)
		I_Error("Bad $$$.sav at archive block Specials");

	// BP: added save itemrespawn queue for deathmatch
	iquetail = iquehead = 0;
	while ((i = READUINT32(save->p)) != 0xffffffff)
	{
		itemrespawnque[iquehead] = &mapthings[i];
		itemrespawntime[iquehead++] = READINT32(save->p);
	}

	READSTRINGN(save->p, skytex, sizeof(skytex));
	if (strcmp(skytex, globallevelskytexture))
		P_SetupLevelSky(skytex, true);

	globalweather = READUINT8(save->p);

	if (globalweather)
	{
		if (curWeather == globalweather)
			curWeather = PRECIP_NONE;

		P_SwitchWeather(globalweather);
	}
	else // PRECIP_NONE
	{
		if (curWeather != PRECIP_NONE)
			P_SwitchWeather(globalweather);
	}

	TracyCZoneEnd(__zone);
}

// =======================================================================
//          Misc
// =======================================================================
static inline void P_ArchiveMisc(savebuffer_t *save)
{
	WRITESTRINGN(save->p, timeattackfolder, sizeof(timeattackfolder));

	// Grand Prix information

	WRITEUINT8(save->p, grandprixinfo.gamespeed);
	WRITEUINT8(save->p, (UINT8)grandprixinfo.encore);
	WRITEUINT8(save->p, (UINT8)grandprixinfo.masterbots);

	WRITEUINT32(save->p, grandprixinfo.specialDamage);

	WRITESTRINGL(save->p, grandprixinfo.cup->name, MAXCUPNAME);

	// Round Queue information

	WRITEUINT8(save->p, roundqueue.position);
	WRITEUINT8(save->p, roundqueue.size);
	WRITEUINT8(save->p, roundqueue.roundnum);

	UINT8 i;
	UINT16 mapnum;
	UINT16 gtnum;

	for (i = 0; i < roundqueue.size; i++)
	{
		mapnum = roundqueue.entries[i].mapnum;

		if (mapnum < nummapheaders && mapheaderinfo[mapnum] != NULL)
		{
			WRITEUINT8(save->p, roundqueue.entries[i].overridden);

			if (roundqueue.entries[i].overridden == true)
			{
				WRITESTRINGL(save->p, mapheaderinfo[mapnum]->lumpname, MAXMAPLUMPNAME);

				gtnum = roundqueue.entries[i].gametype;
				if (gtnum < numgametypes && gametypes[gtnum])
				{
					WRITESTRINGL(save->p, gametypes[roundqueue.entries[i].gametype]->name, MAXGAMETYPELENGTH);
				}
				else
				{
					// Unrecoverable, so we at least try to provide a debugging hint
					const char *badgtstr = va("bad GT %03d on save?", gtnum); // ~20ch vs 32 (MAXGAMETYPELENGTH as of writing)
					WRITESTRINGL(save->p, badgtstr, MAXGAMETYPELENGTH);
				}
			}
			else
			{
				WRITEUINT32(save->p, mapheaderinfo[mapnum]->lumpnamehash);
			}
		}
		else
		{
			// eh, not our problem. provide something that'll almost certainly fail on load
			WRITEUINT8(save->p, false);
			WRITEUINT32(save->p, 0);
		}
	}

	// Rank information

	{
		const gpRank_t *rank = &grandprixinfo.rank;

		WRITEUINT8(save->p, rank->numPlayers);
		WRITEUINT8(save->p, rank->totalPlayers);

		WRITEUINT8(save->p, rank->position);
		WRITEUINT8(save->p, rank->skin);

		WRITEUINT32(save->p, rank->winPoints);
		WRITEUINT32(save->p, rank->totalPoints);

		WRITEUINT32(save->p, rank->laps);
		WRITEUINT32(save->p, rank->totalLaps);

		WRITEUINT32(save->p, (rank->continuesUsed + 1));

		WRITEUINT32(save->p, rank->prisons);
		WRITEUINT32(save->p, rank->totalPrisons);

		WRITEUINT32(save->p, rank->rings);
		WRITEUINT32(save->p, rank->totalRings);

		WRITEUINT8(save->p, (UINT8)rank->specialWon);

		WRITEINT32(save->p, rank->scorePosition);
		WRITEINT32(save->p, rank->scoreGPPoints);
		WRITEINT32(save->p, rank->scoreLaps);
		WRITEINT32(save->p, rank->scorePrisons);
		WRITEINT32(save->p, rank->scoreRings);
		WRITEINT32(save->p, rank->scoreContinues);
		WRITEINT32(save->p, rank->scoreTotal);

		WRITEUINT8(save->p, rank->numLevels);

		for (i = 0; i < rank->numLevels; i++)
		{
			const gpRank_level_t *lvl = &rank->levels[i];

			UINT32 mapHash = 0; // no good default, will all-but-guarantee bad save
			UINT16 id = lvl->id-1; // GAMEMAP BASED AAAGH
			if (id < nummapheaders && mapheaderinfo[id] != NULL)
			{
				mapHash = mapheaderinfo[id]->lumpnamehash;
				//CONS_Printf("wrote map \"%s\" from rank in %u/%u\n", mapheaderinfo[id]->lumpname, i, rank->numLevels);
			}
			WRITEUINT32(save->p, mapHash);

			WRITEINT32(save->p, lvl->event);
			WRITEUINT32(save->p, lvl->time);
			WRITEUINT16(save->p, lvl->totalLapPoints);
			WRITEUINT16(save->p, lvl->totalPrisons);

			UINT8 j;
			for (j = 0; j < rank->numPlayers; j++)
			{
				const gpRank_level_perplayer_t *plr = &lvl->perPlayer[j];

				WRITEUINT8(save->p, plr->position);
				WRITEUINT8(save->p, plr->rings);
				WRITEUINT16(save->p, plr->lapPoints);
				WRITEUINT16(save->p, plr->prisons);
				WRITEUINT8(save->p, (UINT8)plr->gotSpecialPrize);
				WRITESINT8(save->p, (SINT8)plr->grade);
			}
		}
	}

	// Marathon information

	WRITEUINT8(save->p, (marathonmode & ~MA_INIT));

	UINT32 writetime = marathontime;
	if (!(marathonmode & MA_INGAME))
		writetime += TICRATE*5; // live event backup penalty because we don't know how long it takes to get to the next map
	WRITEUINT32(save->p, writetime);
}

void P_GetBackupCupData(savebuffer_t *save)
{
	char testname[sizeof(timeattackfolder)];

	READSTRINGN(save->p, testname, sizeof(testname));

	if (strcmp(testname, timeattackfolder))
	{
		cupsavedata.cup = NULL;
		return;
	}

	// Grand Prix information

	cupsavedata.difficulty = READUINT8(save->p);
	cupsavedata.encore = (boolean)READUINT8(save->p);
	boolean masterbots = (boolean)READUINT8(save->p);

	save->p += 4; // specialDamage

	if (masterbots == true)
		cupsavedata.difficulty = KARTGP_MASTER;

	// Find the relevant cup.
	char cupname[MAXCUPNAME];
	READSTRINGL(save->p, cupname, sizeof(cupname));
	UINT32 hash = quickncasehash(cupname, MAXCUPNAME);

	for (cupsavedata.cup = kartcupheaders; cupsavedata.cup; cupsavedata.cup = cupsavedata.cup->next)
	{
		if (cupsavedata.cup->namehash != hash)
			continue;

		if (strcmp(cupsavedata.cup->name, cupname))
			continue;

		break;
	}

	// Okay, no further! We've got everything we need.
}

static boolean P_UnArchiveSPGame(savebuffer_t *save)
{
	char testname[sizeof(timeattackfolder)];

	READSTRINGN(save->p, testname, sizeof(testname));

	if (strcmp(testname, timeattackfolder))
	{
		CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Corrupt mod ID.\n");
		return false;
	}

	// TODO do not work off grandprixinfo/roundqueue directly
	// This is only strictly necessary if we ever re-add a save
	// select screen or something, for live event backup only
	// it's *fine* and, more importantly, shippable

	memset(&grandprixinfo, 0, sizeof(grandprixinfo));

	grandprixinfo.gp = true;

	// Grand Prix information

	grandprixinfo.gamespeed = READUINT8(save->p);
	grandprixinfo.encore = (boolean)READUINT8(save->p);
	grandprixinfo.masterbots = (boolean)READUINT8(save->p);

	grandprixinfo.specialDamage = READUINT32(save->p);

	// Find the relevant cup.
	char cupname[MAXCUPNAME];
	READSTRINGL(save->p, cupname, sizeof(cupname));
	UINT32 hash = quickncasehash(cupname, MAXCUPNAME);

	for (grandprixinfo.cup = kartcupheaders; grandprixinfo.cup; grandprixinfo.cup = grandprixinfo.cup->next)
	{
		if (grandprixinfo.cup->namehash != hash)
			continue;

		if (strcmp(grandprixinfo.cup->name, cupname))
			continue;

		break;
	}

	if (!grandprixinfo.cup)
	{
		CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Cup \"%s\" is not currently loaded.\n", cupname);
		return false;
	}

	// Round Queue information

	memset(&roundqueue, 0, sizeof(roundqueue));

	G_GPCupIntoRoundQueue(grandprixinfo.cup, GT_RACE, grandprixinfo.encore);

	roundqueue.position = READUINT8(save->p);
	UINT8 size = READUINT8(save->p);
	roundqueue.roundnum = READUINT8(save->p);

	if (roundqueue.size != size)
	{
		CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Cup \"%s\"'s level composition has changed between game launches (%u expected, got %u).\n", cupname, roundqueue.size, size);
		return false;
	}

	if (roundqueue.position == 0 || roundqueue.position > size)
	{
		CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Position %u/%d in the round queue is invalid.\n", roundqueue.position, size);
		return false;
	}

	UINT8 i, j;
	UINT16 mapnum;
	INT32 gtnum;

	for (i = 0; i < roundqueue.size; i++)
	{
		roundqueue.entries[i].overridden = (boolean)READUINT8(save->p);
		if (roundqueue.entries[i].overridden == true)
		{
			if (i >= roundqueue.position)
			{
				CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Cup \"%s\"'s level composition is invalid (has been overridden at entry %u/%u, ahead of the queue head %u).\n", cupname, i, roundqueue.size, roundqueue.position-1);
				return false;
			}

			char mapname[MAXMAPLUMPNAME];
			char gtname[MAXGAMETYPELENGTH];

			READSTRINGL(save->p, mapname, MAXMAPLUMPNAME);
			READSTRINGL(save->p, gtname, MAXGAMETYPELENGTH);

			mapnum = G_MapNumber(mapname);

			if (mapnum < nummapheaders)
			{
				roundqueue.entries[i].mapnum = mapnum;

				gtnum = G_GetGametypeByName(gtname);
				if (gtnum == -1)
				{
					CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Cup \"%s\"'s level composition is invalid (unknown gametype \"%s\" at overridden entry %u/%u).\n", cupname, gtname, i, roundqueue.size);
					return false;
				}

				roundqueue.entries[i].gametype = gtnum;

				// Success, don't fall through to failure
				continue;
			}
		}
		else
		{
			UINT32 val = READUINT32(save->p);

			if (roundqueue.entries[i].rankrestricted && roundqueue.position != i+1)
			{
				// If this is a Sealed Star that hasn't yet been
				// reached, don't be picky about divergance. Just
				// use the base game without question. ~toast 010324
				continue;
			}

			mapnum = roundqueue.entries[i].mapnum;
			if (mapnum < nummapheaders && mapheaderinfo[mapnum] != NULL)
			{
				if (mapheaderinfo[mapnum]->lumpnamehash == val)
				{
					// Success, don't fall through to failure
					continue;
				}
			}
		}

		CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Cup \"%s\"'s level composition has changed between game launches (differs at queue entry %u/%u).\n", cupname, i, roundqueue.size);
		return false;
	}

	// Rank information

	{
		gpRank_t *const rank = &grandprixinfo.rank;

		rank->numPlayers = READUINT8(save->p);
		rank->totalPlayers = READUINT8(save->p);

		rank->position = READUINT8(save->p);
		rank->skin = READUINT8(save->p);

		rank->winPoints = READUINT32(save->p);
		rank->totalPoints = READUINT32(save->p);

		rank->laps = READUINT32(save->p);
		rank->totalLaps = READUINT32(save->p);

		rank->continuesUsed = READUINT32(save->p);

		rank->prisons = READUINT32(save->p);
		rank->totalPrisons = READUINT32(save->p);

		rank->rings = READUINT32(save->p);
		rank->totalRings = READUINT32(save->p);

		rank->specialWon = (boolean)READUINT8(save->p);

		rank->scorePosition = READINT32(save->p);
		rank->scoreGPPoints = READINT32(save->p);
		rank->scoreLaps = READINT32(save->p);
		rank->scorePrisons = READINT32(save->p);
		rank->scoreRings = READINT32(save->p);
		rank->scoreContinues = READINT32(save->p);
		rank->scoreTotal = READINT32(save->p);

		rank->numLevels = READUINT8(save->p);

		if (rank->numLevels > roundqueue.size)
		{
			CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Cup \"%s\"'s level composition has changed between game launches (%u levels ranked VS %u).\n", cupname, rank->numLevels, roundqueue.size);
			return false;
		}

		boolean seeninqueue[ROUNDQUEUE_MAX];
		memset(seeninqueue, 0, sizeof (boolean) * roundqueue.size);

		for (i = 0; i < rank->numLevels; i++)
		{
			gpRank_level_t *const lvl = &rank->levels[i];

			UINT32 mapHash = READUINT32(save->p);

			// Hidden Palace can adjust cup composition, and this level stuff is
			// purely visual anyway, so don't be as strict as the earlier check.
			for (j = 0; j < roundqueue.size; j++)
			{
				// Simple handling to accomodate collisions
				if (seeninqueue[j] == true)
					continue;

				UINT16 id = roundqueue.entries[j].mapnum;

				if (mapheaderinfo[id]->lumpnamehash != mapHash)
					continue;

				lvl->id = id+1;
				seeninqueue[j] = true;
				break;
			}

			if (j == roundqueue.size)
			{
				CONS_Alert(CONS_ERROR, "P_UnArchiveSPGame: Cup \"%s\"'s level composition has changed between game launches (ranked level %u/%u not found in queue).\n", cupname, i, rank->numLevels);
				return false;
			}

			lvl->event = READINT32(save->p);
			lvl->time = READUINT32(save->p);
			lvl->totalLapPoints = READUINT16(save->p);
			lvl->totalPrisons = READUINT16(save->p);

			for (j = 0; j < rank->numPlayers; j++)
			{
				gpRank_level_perplayer_t *const plr = &lvl->perPlayer[j];

				plr->position = READUINT8(save->p);
				plr->rings = READUINT8(save->p);
				plr->lapPoints = READUINT16(save->p);
				plr->prisons = READUINT16(save->p);
				plr->gotSpecialPrize = (boolean)READUINT8(save->p);
				plr->grade = (gp_rank_e)READSINT8(save->p);
			}
		}
	}

	// Marathon information

	marathonmode = READUINT8(save->p);
	marathontime = READUINT32(save->p);

	return true;
}

static void P_NetArchiveMisc(savebuffer_t *save, boolean resending)
{
	TracyCZone(__zone, true);

	size_t i, j;

	WRITEUINT32(save->p, ARCHIVEBLOCK_MISC);

	if (resending)
		WRITEUINT32(save->p, gametic);
	WRITEINT16(save->p, gamemap);

	if (gamestate != GS_LEVEL)
		WRITEINT16(save->p, GS_WAITINGPLAYERS); // nice hack to put people back into waitingplayers
	else
		WRITEINT16(save->p, gamestate);
	WRITEINT16(save->p, gametype);
	WRITEINT16(save->p, g_lastgametype);

	{
		UINT32 pig = 0;
		for (i = 0; i < MAXPLAYERS; i++)
			pig |= (playeringame[i] != 0)<<i;
		WRITEUINT32(save->p, pig);
	}

	for (i = 0; i < MAXUNLOCKABLES;)
	{
		UINT8 btemp = 0;
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			btemp |= (netUnlocked[j+i] << j);
		WRITEUINT8(save->p, btemp);
		i += j;
	}

	WRITEUINT8(save->p, encoremode);

	WRITEUINT8(save->p, mapmusrng);

	WRITEUINT32(save->p, leveltime);
	WRITEINT16(save->p, lastmap);
	WRITEUINT16(save->p, bossdisabled);

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		WRITEUINT16(save->p, g_voteLevels[i][0]);
		WRITEUINT16(save->p, g_voteLevels[i][1]);
	}

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		WRITESINT8(save->p, g_votes[i]);
	}

	WRITESINT8(save->p, g_pickedVote);

	{
		UINT8 globools = 0;
		if (stagefailed)
			globools |= 1;
		if (stoppedclock)
			globools |= (1<<1);
		WRITEUINT8(save->p, globools);
	}

	WRITEUINT32(save->p, bluescore);
	WRITEUINT32(save->p, redscore);

	WRITEUINT16(save->p, skincolor_redteam);
	WRITEUINT16(save->p, skincolor_blueteam);
	WRITEUINT16(save->p, skincolor_redring);
	WRITEUINT16(save->p, skincolor_bluering);

	WRITEINT32(save->p, modulothing);

	WRITEINT16(save->p, autobalance);
	WRITEINT16(save->p, teamscramble);

	for (i = 0; i < MAXPLAYERS; i++)
		WRITEINT16(save->p, scrambleplayers[i]);

	for (i = 0; i < MAXPLAYERS; i++)
		WRITEINT16(save->p, scrambleteams[i]);

	WRITEINT16(save->p, scrambletotal);
	WRITEINT16(save->p, scramblecount);

	WRITEUINT32(save->p, racecountdown);
	WRITEUINT32(save->p, exitcountdown);

	// exitcondition_t
	WRITEUINT8(save->p, g_exit.losing);
	WRITEUINT8(save->p, g_exit.retry);

	WRITEFIXED(save->p, gravity);
	WRITEFIXED(save->p, mapobjectscale);

	// SRB2kart
	WRITEINT32(save->p, numgotboxes);
	WRITEUINT8(save->p, numtargets);
	WRITEUINT8(save->p, maptargets);
	WRITEUINT32(save->p, nummapboxes);

	WRITEUINT8(save->p, battleprisons);
	WRITEUINT32(save->p, g_emeraldWin);

	WRITEUINT8(save->p, gamespeed);
	WRITEUINT8(save->p, numlaps);
	WRITEUINT8(save->p, franticitems);

	WRITESINT8(save->p, speedscramble);
	WRITESINT8(save->p, encorescramble);

	// battleovertime_t
	WRITEUINT16(save->p, battleovertime.enabled);
	WRITEFIXED(save->p, battleovertime.radius);
	WRITEFIXED(save->p, battleovertime.initial_radius);
	WRITEUINT32(save->p, battleovertime.start);
	WRITEFIXED(save->p, battleovertime.x);
	WRITEFIXED(save->p, battleovertime.y);
	WRITEFIXED(save->p, battleovertime.z);

	// battleufo_t
	WRITEINT32(save->p, g_battleufo.previousId);
	WRITEUINT32(save->p, g_battleufo.due);

	WRITEUINT32(save->p, wantedcalcdelay);
	for (i = 0; i < NUMKARTITEMS-1; i++)
		WRITEUINT32(save->p, itemCooldowns[i]);
	WRITEUINT32(save->p, mapreset);

	WRITEUINT8(save->p, spectateGriefed);

	WRITEUINT8(save->p, thwompsactive);
	WRITEUINT8(save->p, lastLowestLap);
	WRITESINT8(save->p, spbplace);
	WRITEUINT8(save->p, rainbowstartavailable);
	WRITEUINT8(save->p, inDuel);

	WRITEUINT32(save->p, introtime);
	WRITEUINT32(save->p, starttime);
	WRITEUINT8(save->p, numbulbs);

	WRITEUINT32(save->p, timelimitintics);
	WRITEUINT32(save->p, extratimeintics);
	WRITEUINT32(save->p, secretextratime);

	WRITEUINT32(save->p, g_pointlimit);

	WRITEUINT32(save->p, g_darkness.start);
	WRITEUINT32(save->p, g_darkness.end);

	WRITEUINT32(save->p, g_musicfade.start);
	WRITEUINT32(save->p, g_musicfade.end);
	WRITEUINT32(save->p, g_musicfade.fade);
	WRITEUINT8(save->p, g_musicfade.ticked);

	WRITEUINT16(save->p, numchallengedestructibles);

	// Is it paused?
	if (paused)
		WRITEUINT8(save->p, 0x2f);
	else
		WRITEUINT8(save->p, 0x2e);

	WRITEUINT32(save->p, livestudioaudience_timer);

	// Only the server uses this, but it
	// needs synched for remote admins anyway.
	WRITEUINT32(save->p, schedule_len);
	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];
		WRITEINT16(save->p, task->basetime);
		WRITEINT16(save->p, task->timer);
		WRITESTRING(save->p, task->command);
	}

	WRITEUINT32(save->p, cht_debug);

	TracyCZoneEnd(__zone);
}

static boolean P_NetUnArchiveMisc(savebuffer_t *save, boolean reloading)
{
	TracyCZone(__zone, true);

	size_t i, j;
	size_t numTasks;

	if (READUINT32(save->p) != ARCHIVEBLOCK_MISC)
		I_Error("Bad $$$.sav at archive block Misc");

	if (reloading)
		gametic = READUINT32(save->p);

	gamemap = READINT16(save->p);
	g_reloadingMap = false;

	// gamemap changed; we assume that its map header is always valid,
	// so make it so
	if (!gamemap || gamemap > nummapheaders || !mapheaderinfo[gamemap-1])
		I_Error("P_NetUnArchiveMisc: Internal map ID %d not found (nummapheaders = %d)", gamemap-1, nummapheaders);

	G_SetGamestate(READINT16(save->p));

	gametype = READINT16(save->p);
	g_lastgametype = READINT16(save->p);

	{
		UINT32 pig = READUINT32(save->p);
		for (i = 0; i < MAXPLAYERS; i++)
		{
			playeringame[i] = (pig & (1<<i)) != 0;
			// playerstate is set in unarchiveplayers
		}
	}

	for (i = 0; i < MAXUNLOCKABLES;)
	{
		UINT8 rtemp = READUINT8(save->p);
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			netUnlocked[j+i] = ((rtemp >> j) & 1);
		i += j;
	}

	encoremode = (boolean)READUINT8(save->p);

	mapmusrng = READUINT8(save->p);

	if (!P_LoadLevel(true, reloading))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Can't load the level!\n"));
		return false;
	}

	// get the time
	leveltime = READUINT32(save->p);
	lastmap = READINT16(save->p);
	bossdisabled = READUINT16(save->p);

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		g_voteLevels[i][0] = READUINT16(save->p);
		g_voteLevels[i][1] = READUINT16(save->p);
	}

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		g_votes[i] = READSINT8(save->p);
	}

	g_pickedVote = READSINT8(save->p);

	{
		UINT8 globools = READUINT8(save->p);
		stagefailed = !!(globools & 1);
		stoppedclock = !!(globools & (1<<1));
	}

	bluescore = READUINT32(save->p);
	redscore = READUINT32(save->p);

	skincolor_redteam = READUINT16(save->p);
	skincolor_blueteam = READUINT16(save->p);
	skincolor_redring = READUINT16(save->p);
	skincolor_bluering = READUINT16(save->p);

	modulothing = READINT32(save->p);

	autobalance = READINT16(save->p);
	teamscramble = READINT16(save->p);

	for (i = 0; i < MAXPLAYERS; i++)
		scrambleplayers[i] = READINT16(save->p);

	for (i = 0; i < MAXPLAYERS; i++)
		scrambleteams[i] = READINT16(save->p);

	scrambletotal = READINT16(save->p);
	scramblecount = READINT16(save->p);

	racecountdown = READUINT32(save->p);
	exitcountdown = READUINT32(save->p);

	// exitcondition_t
	g_exit.losing = READUINT8(save->p);
	g_exit.retry = READUINT8(save->p);

	gravity = READFIXED(save->p);
	mapobjectscale = READFIXED(save->p);

	// SRB2kart
	numgotboxes = READINT32(save->p);
	numtargets = READUINT8(save->p);
	maptargets = READUINT8(save->p);
	nummapboxes = READINT32(save->p);
	battleprisons = (boolean)READUINT8(save->p);
	g_emeraldWin = (tic_t)READUINT32(save->p);

	gamespeed = READUINT8(save->p);
	numlaps = READUINT8(save->p);
	franticitems = (boolean)READUINT8(save->p);

	speedscramble = READSINT8(save->p);
	encorescramble = READSINT8(save->p);

	// battleovertime_t
	battleovertime.enabled = READUINT16(save->p);
	battleovertime.radius = READFIXED(save->p);
	battleovertime.initial_radius = READFIXED(save->p);
	battleovertime.start = READUINT32(save->p);
	battleovertime.x = READFIXED(save->p);
	battleovertime.y = READFIXED(save->p);
	battleovertime.z = READFIXED(save->p);

	// battleufo_t
	g_battleufo.previousId = READINT32(save->p);
	g_battleufo.due = READUINT32(save->p);

	wantedcalcdelay = READUINT32(save->p);
	for (i = 0; i < NUMKARTITEMS-1; i++)
		itemCooldowns[i] = READUINT32(save->p);
	mapreset = READUINT32(save->p);

	spectateGriefed = READUINT8(save->p);

	thwompsactive = (boolean)READUINT8(save->p);
	lastLowestLap = READUINT8(save->p);
	spbplace = READSINT8(save->p);
	rainbowstartavailable = (boolean)READUINT8(save->p);
	inDuel = (boolean)READUINT8(save->p);

	introtime = READUINT32(save->p);
	starttime = READUINT32(save->p);
	numbulbs = READUINT8(save->p);

	timelimitintics = READUINT32(save->p);
	extratimeintics = READUINT32(save->p);
	secretextratime = READUINT32(save->p);

	g_pointlimit = READUINT32(save->p);

	g_darkness.start = READUINT32(save->p);
	g_darkness.end = READUINT32(save->p);

	g_musicfade.start = READUINT32(save->p);
	g_musicfade.end = READUINT32(save->p);
	g_musicfade.fade = READUINT32(save->p);
	g_musicfade.ticked = READUINT8(save->p);

	numchallengedestructibles = READUINT16(save->p);

	// Is it paused?
	if (READUINT8(save->p) == 0x2f)
		paused = true;

	livestudioaudience_timer = READUINT32(save->p);

	// Only the server uses this, but it
	// needs synched for remote admins anyway.
	Schedule_Clear();

	numTasks = READUINT32(save->p);
	for (i = 0; i < numTasks; i++)
	{
		INT16 basetime;
		INT16 timer;
		char command[MAXTEXTCMD];

		basetime = READINT16(save->p);
		timer = READINT16(save->p);
		READSTRING(save->p, command);

		Schedule_Add(basetime, timer, command);
	}

	cht_debug = READUINT32(save->p);

	TracyCZoneEnd(__zone);
	return true;
}

static inline void P_ArchiveLuabanksAndConsistency(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	UINT8 i, banksinuse = NUM_LUABANKS;

	while (banksinuse && !luabanks[banksinuse-1])
		banksinuse--; // get the last used bank

	if (banksinuse)
	{
		WRITEUINT8(save->p, 0xb7); // luabanks marker
		WRITEUINT8(save->p, banksinuse);
		for (i = 0; i < banksinuse; i++)
			WRITEINT32(save->p, luabanks[i]);
	}

	WRITEUINT8(save->p, 0x1d); // consistency marker

	TracyCZoneEnd(__zone);
}

static inline boolean P_UnArchiveLuabanksAndConsistency(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	boolean ret = true;
	switch (READUINT8(save->p))
	{
		case 0xb7: // luabanks marker
			{
				UINT8 i, banksinuse = READUINT8(save->p);
				if (banksinuse > NUM_LUABANKS)
				{
					CONS_Alert(CONS_ERROR, M_GetText("Corrupt Luabanks! (Too many banks in use)\n"));
					ret = false;
					break;
				}
				for (i = 0; i < banksinuse; i++)
					luabanks[i] = READINT32(save->p);
				if (READUINT8(save->p) != 0x1d) // consistency marker
				{
					CONS_Alert(CONS_ERROR, M_GetText("Corrupt Luabanks! (Failed consistency check)\n"));
					ret = false;
					break;
				}
			}
		case 0x1d: // consistency marker
			break;
		default: // anything else is nonsense
			CONS_Alert(CONS_ERROR, M_GetText("Failed consistency check (???)\n"));
			ret = false;
			break;
	}

	TracyCZoneEnd(__zone);
	return ret;
}

static void P_NetArchiveRNG(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	size_t i;

	WRITEUINT32(save->p, ARCHIVEBLOCK_RNG);

	for (i = 0; i < PRNUMSYNCED; i++)
	{
		WRITEUINT32(save->p, P_GetInitSeed(i));
		WRITEUINT32(save->p, P_GetRandSeed(i));
	}

	TracyCZoneEnd(__zone);
}

static inline void P_NetUnArchiveRNG(savebuffer_t *save)
{
	TracyCZone(__zone, true);

	size_t i;

	if (READUINT32(save->p) != ARCHIVEBLOCK_RNG)
		I_Error("Bad $$$.sav at archive block RNG");

	for (i = 0; i < PRNUMSYNCED; i++)
	{
		UINT32 init = READUINT32(save->p);
		UINT32 seed = READUINT32(save->p);

		P_SetRandSeedNet(i, init, seed);
	}

	TracyCZoneEnd(__zone);
}

void P_SaveGame(savebuffer_t *save)
{
	P_ArchiveMisc(save);
	P_ArchivePlayer(save);
	P_ArchiveLuabanksAndConsistency(save);
}

void P_SaveNetGame(savebuffer_t *save, boolean resending)
{
	TracyCZone(__zone, true);

	current_savebuffer = save;

	thinker_t *th;
	mobj_t *mobj;
	UINT32 i = 1; // don't start from 0, it'd be confused with a blank pointer otherwise

	CV_SaveNetVars(&save->p);
	P_NetArchiveMisc(save, resending);

	// Assign the mobjnumber for pointer tracking
	if (gamestate == GS_LEVEL)
	{
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mobj = (mobj_t *)th;
			if (TypeIsNetSynced(mobj->type) == false)
				continue;
			mobj->mobjnum = i++;
		}
	}

	K_SaveEndCamera(save);
	WriteMobjPointer(g_endcam.panMobj);

	P_NetArchivePlayers(save);
	P_NetArchiveParties(save);
	P_NetArchiveRoundQueue(save);
	P_NetArchiveZVote(save);

	if (gamestate == GS_LEVEL)
	{
		P_NetArchiveWorld(save);
		P_ArchivePolyObjects(save);
		P_NetArchiveThinkers(save);
		P_NetArchiveSpecials(save);
		P_NetArchiveColormaps(save);
		P_NetArchiveTubeWaypoints(save);
		P_NetArchiveWaypoints(save);
	}

	ACS_Archive(save);
	LUA_Archive(save, true);

	P_NetArchiveRNG(save);

	P_ArchiveLuabanksAndConsistency(save);

	TracyCZoneEnd(__zone);
}

boolean P_LoadGame(savebuffer_t *save)
{
	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	if (gamestate == GS_VOTING)
		Y_EndVote();
	G_SetGamestate(GS_NULL); // should be changed in P_UnArchiveMisc

	if (!P_UnArchiveSPGame(save))
		goto badloadgame;
	if (!P_UnArchivePlayer(save))
		goto badloadgame;

	if (!P_UnArchiveLuabanksAndConsistency(save))
		goto badloadgame;

	return true;

badloadgame:
	// these are the side effects of P_UnarchiveSPGame
	savedata.lives = 0;
	roundqueue.size = 0;
	grandprixinfo.gp = false;
	marathonmode = 0;

	return false;
}

boolean P_LoadNetGame(savebuffer_t *save, boolean reloading)
{
	TracyCZone(__zone, true);

	current_savebuffer = save;

	save->p += CV_LoadNetVars(save->p);

	if (!P_NetUnArchiveMisc(save, reloading))
		return false;

	K_LoadEndCamera(save);
	ReadMobjPointer(&g_endcam.panMobj);

	P_NetUnArchivePlayers(save);
	P_NetUnArchiveParties(save);
	P_NetUnArchiveRoundQueue(save);
	P_NetUnArchiveZVote(save);

	if (gamestate == GS_LEVEL)
	{
		P_NetUnArchiveWorld(save);
		P_UnArchivePolyObjects(save);
		P_NetUnArchiveThinkers(save);
		P_NetUnArchiveSpecials(save);
		P_NetUnArchiveColormaps(save);
		P_NetUnArchiveTubeWaypoints(save);
		P_NetUnArchiveWaypoints(save);
		P_RelinkPointers();
	}

	ACS_UnArchive(save);
	LUA_UnArchive(save, true);

	P_NetUnArchiveRNG(save);

	// The precipitation would normally be spawned in P_SetupLevel, which is called by
	// P_NetUnArchiveMisc above. However, that would place it up before P_NetUnArchiveThinkers,
	// so the thinkers would be deleted later. Therefore, P_SetupLevel will *not* spawn
	// precipitation when loading a netgame save. Instead, precip has to be spawned here.
	// This is done in P_NetUnArchiveSpecials now.
	boolean ret = P_UnArchiveLuabanksAndConsistency(save);

	TracyCZoneEnd(__zone);
	return ret;
}

boolean P_SaveBufferZAlloc(savebuffer_t *save, size_t alloc_size, INT32 tag, void *user)
{
	I_Assert(save->buffer == NULL);
	save->buffer = (UINT8 *)Z_Malloc(alloc_size, tag, user);

	if (save->buffer == NULL)
	{
		return false;
	}

	save->size = alloc_size;
	save->p = save->buffer;
	save->end = save->buffer + save->size;

	return true;
}

boolean P_SaveBufferFromExisting(savebuffer_t *save, UINT8 *existing_buffer, size_t existing_size)
{
	I_Assert(save->buffer == NULL);

	if (existing_buffer == NULL || existing_size == 0)
	{
		return false;
	}

	save->buffer = existing_buffer;
	save->size = existing_size;

	save->p = save->buffer;
	save->end = save->buffer + save->size;

	return true;
}

boolean P_SaveBufferFromLump(savebuffer_t *save, lumpnum_t lump)
{
	I_Assert(save->buffer == NULL);

	if (lump == LUMPERROR)
	{
		return false;
	}

	save->buffer = (UINT8 *)W_CacheLumpNum(lump, PU_STATIC);

	if (save->buffer == NULL)
	{
		return false;
	}

	save->size = W_LumpLength(lump);

	save->p = save->buffer;
	save->end = save->buffer + save->size;

	return true;
}

boolean P_SaveBufferFromFile(savebuffer_t *save, char const *name)
{
	size_t len = 0;

	I_Assert(save->buffer == NULL);
	len = FIL_ReadFile(name, &save->buffer);

	if (len != 0)
	{
		save->size = len;

		save->p = save->buffer;
		save->end = save->buffer + save->size;
	}

	return len;
}

static void P_SaveBufferInvalidate(savebuffer_t *save)
{
	save->buffer = save->p = save->end = NULL;
	save->size = 0;
}

void P_SaveBufferFree(savebuffer_t *save)
{
	Z_Free(save->buffer);
	P_SaveBufferInvalidate(save);
}

size_t P_SaveBufferRemaining(const savebuffer_t *save)
{
	if (save->p < save->end)
	{
		return save->end - save->p;
	}
	else
	{
		return 0;
	}
}
