// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by John "JTE" Muniz.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_playerlib.c
/// \brief player object library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "p_mobj.h"
#include "d_player.h"
#include "g_game.h"
#include "p_local.h"
#include "d_clisrv.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "lua_hook.h" // hook_cmd_running errors
#include "k_profiles.h" // GetPrettyRRID

boolean constplayer = false;

static int lib_iteratePlayers(lua_State *L)
{
	INT32 i = -1;

	if (lua_gettop(L) < 2)
	{
		//return luaL_error(L, "Don't call players.iterate() directly, use it as 'for player in players.iterate do <block> end'.");
		lua_pushcfunction(L, lib_iteratePlayers);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = (INT32)(*((player_t **)luaL_checkudata(L, 1, META_PLAYER)) - players);

	i++;

	if (i == serverplayer)
	{
		return LUA_PushServerPlayer(L);
	}

	for (; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		LUA_PushUserdata(L, &players[i], META_PLAYER);
		return 1;
	}

	return 0;
}

static int lib_getPlayer(lua_State *L)
{
	const char *field;
	// i -> players[i]
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		lua_Integer i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= MAXPLAYERS)
			return luaL_error(L, "players[] index %d out of range (0 - %d)", i, MAXPLAYERS-1);
		if (i == serverplayer)
			return LUA_PushServerPlayer(L);
		if (!playeringame[i])
			return 0;
		LUA_PushUserdata(L, &players[i], META_PLAYER);
		return 1;
	}

	field = luaL_checkstring(L, 2);
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iteratePlayers);
		return 1;
	}
	return 0;
}

// #players -> MAXPLAYERS
static int lib_lenPlayer(lua_State *L)
{
	lua_pushinteger(L, MAXPLAYERS);
	return 1;
}

// Same deal as the three functions above but for displayplayers

static int lib_iterateDisplayplayers(lua_State *L)
{
	INT32 i = lua_tonumber(L, lua_upvalueindex(1));

	if (lua_gettop(L) < 2)
	{
		lua_pushcclosure(L, lib_iterateDisplayplayers, 1);
		return 1;
	}

	if (i <= r_splitscreen)
	{
		if (!playeringame[displayplayers[i]])
			return 0;

		// Return player and splitscreen index.
		LUA_PushUserdata(L, &players[displayplayers[i]], META_PLAYER);
		lua_pushnumber(L, i);

		// Update splitscreen index value for next iteration.
		lua_pushnumber(L, i + 1);
		lua_pushvalue(L, -1);
		lua_replace(L, lua_upvalueindex(1));
		lua_pop(L, 1);

		return 2;
	}
	return 0;
}

static int lib_getDisplayplayers(lua_State *L)
{
	const char *field;
	// i -> players[i]
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		lua_Integer i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= MAXSPLITSCREENPLAYERS)
			return luaL_error(L, "displayplayers[] index %d out of range (0 - %d)", i, MAXSPLITSCREENPLAYERS-1);
		if (i > r_splitscreen)
			return 0;
		if (!playeringame[displayplayers[i]])
			return 0;
		LUA_PushUserdata(L, &players[displayplayers[i]], META_PLAYER);
		return 1;
	}

	field = luaL_checkstring(L, 2);
	if (fastcmp(field,"iterate"))
	{
		lua_pushcclosure(L, lib_iterateDisplayplayers, 1);
		return 1;
	}
	return 0;
}

// #displayplayers -> MAXSPLITSCREENPLAYERS
static int lib_lenDisplayplayers(lua_State *L)
{
	lua_pushinteger(L, MAXSPLITSCREENPLAYERS);
	return 1;
}

static int player_get(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	const char *field = luaL_checkstring(L, 2);

	if (!plr) {
		if (fastcmp(field,"valid")) {
			lua_pushboolean(L, false);
			return 1;
		}
		return LUA_ErrInvalid(L, "player_t");
	}

	if (fastcmp(field,"valid"))
		lua_pushboolean(L, true);
	else if (fastcmp(field,"name"))
		lua_pushstring(L, player_names[plr-players]);
	else if (fastcmp(field,"mo"))
		LUA_PushUserdata(L, plr->mo, META_MOBJ);
	else if (fastcmp(field,"cmd"))
		LUA_PushUserdata(L, &plr->cmd, META_TICCMD);
	else if (fastcmp(field,"oldcmd"))
		LUA_PushUserdata(L, &plr->oldcmd, META_TICCMD);
	else if (fastcmp(field,"respawn"))
		LUA_PushUserdata(L, &plr->respawn, META_RESPAWN);
	else if (fastcmp(field,"playerstate"))
		lua_pushinteger(L, plr->playerstate);
	else if (fastcmp(field,"viewz"))
		lua_pushfixed(L, plr->viewz);
	else if (fastcmp(field,"viewheight"))
		lua_pushfixed(L, plr->viewheight);
	else if (fastcmp(field,"skybox"))
		LUA_PushUserdata(L, &plr->skybox, META_SKYBOX);
	else if (fastcmp(field,"viewrollangle"))
		lua_pushangle(L, plr->viewrollangle);
	else if (fastcmp(field, "tilt"))
		lua_pushangle(L, plr->tilt);
	else if (fastcmp(field,"aiming"))
		lua_pushangle(L, plr->aiming);
	else if (fastcmp(field,"drawangle"))
		lua_pushangle(L, plr->drawangle);
	else if (fastcmp(field,"karthud"))
		LUA_PushUserdata(L, plr->karthud, META_KARTHUD);
	else if (fastcmp(field,"nocontrol"))
		lua_pushinteger(L, plr->nocontrol);
	else if (fastcmp(field,"carry"))
		lua_pushinteger(L, plr->carry);
	else if (fastcmp(field,"dye"))
		lua_pushinteger(L, plr->dye);
	else if (fastcmp(field,"position"))
		lua_pushinteger(L, plr->position);
	else if (fastcmp(field,"oldposition"))
		lua_pushinteger(L, plr->oldposition);
	else if (fastcmp(field,"positiondelay"))
		lua_pushinteger(L, plr->positiondelay);
	else if (fastcmp(field,"leaderpenalty"))
		lua_pushinteger(L, plr->leaderpenalty);
	else if (fastcmp(field,"teamposition"))
		lua_pushinteger(L, plr->teamposition);
	else if (fastcmp(field,"teamimportance"))
		lua_pushinteger(L, plr->teamimportance);
	else if (fastcmp(field,"distancetofinish"))
		lua_pushinteger(L, plr->distancetofinish);
	else if (fastcmp(field,"distancetofinishprev"))
		lua_pushinteger(L, plr->distancetofinishprev);
	else if (fastcmp(field,"lastpickupdistance"))
		lua_pushinteger(L, plr->lastpickupdistance);
	else if (fastcmp(field,"lastpickuptype"))
		lua_pushinteger(L, plr->lastpickuptype);
	else if (fastcmp(field,"currentwaypoint"))
		LUA_PushUserdata(L, plr->currentwaypoint, META_WAYPOINT);
	else if (fastcmp(field,"nextwaypoint"))
		LUA_PushUserdata(L, plr->nextwaypoint, META_WAYPOINT);
	else if (fastcmp(field,"ringshooter"))
		LUA_PushUserdata(L, plr->ringShooter, META_MOBJ);
	else if (fastcmp(field,"airtime"))
		lua_pushinteger(L, plr->airtime);
	else if (fastcmp(field,"lastairtime"))
		lua_pushinteger(L, plr->lastairtime);
	else if (fastcmp(field,"bigwaypointgap"))
		lua_pushinteger(L, plr->bigwaypointgap);
	else if (fastcmp(field,"flashing"))
		lua_pushinteger(L, plr->flashing);
	else if (fastcmp(field,"spinouttimer"))
		lua_pushinteger(L, plr->spinouttimer);
	else if (fastcmp(field,"spinouttype"))
		lua_pushinteger(L, plr->spinouttype);
	else if (fastcmp(field,"instashield"))
		lua_pushinteger(L, plr->instashield);
	else if (fastcmp(field,"nullhitlag"))
		lua_pushinteger(L, plr->nullHitlag);
	else if (fastcmp(field,"wipeoutslow"))
		lua_pushinteger(L, plr->wipeoutslow);
	else if (fastcmp(field,"justbumped"))
		lua_pushinteger(L, plr->justbumped);
	else if (fastcmp(field,"noebrakemagnet"))
		lua_pushinteger(L, plr->noEbrakeMagnet);
	else if (fastcmp(field,"wallspikedampen"))
		lua_pushinteger(L, plr->wallSpikeDampen);
	else if (fastcmp(field,"tumblebounces"))
		lua_pushinteger(L, plr->tumbleBounces);
	else if (fastcmp(field,"tumbleheight"))
		lua_pushinteger(L, plr->tumbleHeight);
	else if (fastcmp(field,"stunned"))
		lua_pushinteger(L, plr->stunned);
	else if (fastcmp(field,"flybot"))
		LUA_PushUserdata(L, plr->flybot, META_MOBJ);
	else if (fastcmp(field,"justdi"))
		lua_pushinteger(L, plr->justDI);
	else if (fastcmp(field,"flipdi"))
		lua_pushboolean(L, plr->flipDI);
	else if (fastcmp(field,"cangrabitems"))
		lua_pushinteger(L, plr->cangrabitems);
	else if (fastcmp(field,"analoginput"))
		lua_pushboolean(L, plr->analoginput);
	else if (fastcmp(field,"transfer"))
		lua_pushfixed(L, plr->transfer);
	else if (fastcmp(field,"markedfordeath"))
		lua_pushboolean(L, plr->markedfordeath);
	else if (fastcmp(field,"mfdfinish"))
		lua_pushboolean(L, plr->mfdfinish);
	else if (fastcmp(field,"incontrol"))
		lua_pushboolean(L, plr->incontrol);
	else if (fastcmp(field,"progressivethrust"))
		lua_pushinteger(L, plr->progressivethrust);
	else if (fastcmp(field,"ringvisualwarning"))
		lua_pushinteger(L, plr->ringvisualwarning);
	else if (fastcmp(field,"bailcharge"))
		lua_pushinteger(L, plr->bailcharge);
	else if (fastcmp(field,"baildrop"))
		lua_pushinteger(L, plr->baildrop);
	else if (fastcmp(field,"bailhitlag"))
		lua_pushboolean(L, plr->bailhitlag);
	else if (fastcmp(field,"dotrickfx"))
		lua_pushboolean(L, plr->dotrickfx);
	else if (fastcmp(field,"stingfx"))
		lua_pushboolean(L, plr->stingfx);
	else if (fastcmp(field,"bumperinflate"))
		lua_pushinteger(L, plr->bumperinflate);
	else if (fastcmp(field,"ringboxdelay"))
		lua_pushinteger(L, plr->ringboxdelay);
	else if (fastcmp(field,"ringboxaward"))
		lua_pushinteger(L, plr->ringboxaward);
	else if (fastcmp(field,"lastringboost"))
		lua_pushinteger(L, plr->lastringboost);
	else if (fastcmp(field,"amps"))
		lua_pushinteger(L, plr->amps);
	else if (fastcmp(field,"recentamps"))
		lua_pushinteger(L, plr->recentamps);
	else if (fastcmp(field,"amppickup"))
		lua_pushinteger(L, plr->amppickup);
	else if (fastcmp(field,"ampspending"))
		lua_pushinteger(L, plr->ampspending);
	else if (fastcmp(field,"itemflags"))
		lua_pushinteger(L, plr->itemflags);
	else if (fastcmp(field,"outrun"))
		lua_pushfixed(L, plr->outrun);
	else if (fastcmp(field,"drift"))
		lua_pushinteger(L, plr->drift);
	else if (fastcmp(field,"driftcharge"))
		lua_pushinteger(L, plr->driftcharge);
	else if (fastcmp(field,"driftboost"))
		lua_pushinteger(L, plr->driftboost);
	else if (fastcmp(field,"strongdriftboost"))
		lua_pushinteger(L, plr->strongdriftboost);
	else if (fastcmp(field,"gateboost"))
		lua_pushinteger(L, plr->gateBoost);
	else if (fastcmp(field,"gatesound"))
		lua_pushinteger(L, plr->gateSound);
	else if (fastcmp(field,"startboost"))
		lua_pushinteger(L, plr->startboost);
	else if (fastcmp(field,"neostartboost"))
		lua_pushinteger(L, plr->neostartboost);
	else if (fastcmp(field,"dropdashboost"))
		lua_pushinteger(L, plr->dropdashboost);
	else if (fastcmp(field,"aciddropdashboost"))
		lua_pushinteger(L, plr->aciddropdashboost);
	else if (fastcmp(field,"aizdriftstrat"))
		lua_pushinteger(L, plr->aizdriftstrat);
	else if (fastcmp(field,"aizdriftextend"))
		lua_pushinteger(L, plr->aizdriftextend);
	else if (fastcmp(field,"aizdrifttilt"))
		lua_pushinteger(L, plr->aizdrifttilt);
	else if (fastcmp(field,"aizdriftturn"))
		lua_pushinteger(L, plr->aizdriftturn);
	else if (fastcmp(field,"underwatertilt"))
		lua_pushinteger(L, plr->underwatertilt);
	else if (fastcmp(field,"offroad"))
		lua_pushfixed(L, plr->offroad);
	else if (fastcmp(field,"tiregrease"))
		lua_pushinteger(L, plr->tiregrease);
	else if (fastcmp(field,"springstars"))
		lua_pushinteger(L, plr->springstars);
	else if (fastcmp(field,"springcolor"))
		lua_pushinteger(L, plr->springcolor);
	else if (fastcmp(field,"dashpadcooldown"))
		lua_pushinteger(L, plr->dashpadcooldown);
	else if (fastcmp(field,"spindash"))
		lua_pushinteger(L, plr->spindash);
	else if (fastcmp(field,"spindashspeed"))
		lua_pushinteger(L, plr->spindashspeed);
	else if (fastcmp(field,"spindashboost"))
		lua_pushinteger(L, plr->spindashboost);
	else if (fastcmp(field,"ringboostinprogress"))
		lua_pushinteger(L, plr->ringboostinprogress);
	else if (fastcmp(field,"fastfall"))
		lua_pushfixed(L, plr->fastfall);
	else if (fastcmp(field,"fastfallbase"))
		lua_pushfixed(L, plr->fastfallBase);
	else if (fastcmp(field,"numboosts"))
		lua_pushinteger(L, plr->numboosts);
	else if (fastcmp(field,"boostpower"))
		lua_pushinteger(L, plr->boostpower);
	else if (fastcmp(field,"speedboost"))
		lua_pushinteger(L, plr->speedboost);
	else if (fastcmp(field,"accelboost"))
		lua_pushinteger(L, plr->accelboost);
	else if (fastcmp(field,"handleboost"))
		lua_pushinteger(L, plr->handleboost);
	else if (fastcmp(field,"boostangle"))
		lua_pushangle(L, plr->boostangle);
	else if (fastcmp(field,"stonedrag"))
		lua_pushfixed(L, plr->stonedrag);
	else if (fastcmp(field,"draftpower"))
		lua_pushinteger(L, plr->draftpower);
	else if (fastcmp(field,"draftleeway"))
		lua_pushinteger(L, plr->draftleeway);
	else if (fastcmp(field,"lastdraft"))
		lua_pushinteger(L, plr->lastdraft);
	else if (fastcmp(field,"tripwirestate"))
		lua_pushinteger(L, plr->tripwireState);
	else if (fastcmp(field,"tripwirepass"))
		lua_pushinteger(L, plr->tripwirePass);
	else if (fastcmp(field,"fakeboost"))
		lua_pushinteger(L, plr->fakeBoost);
	else if (fastcmp(field,"subsonicleniency"))
		lua_pushinteger(L, plr->subsonicleniency);
	else if (fastcmp(field,"tripwireleniency"))
		lua_pushinteger(L, plr->tripwireLeniency);
	else if (fastcmp(field,"tripwireairleniency"))
		lua_pushinteger(L, plr->tripwireAirLeniency);
	else if (fastcmp(field,"tripwirerebounddelay"))
		lua_pushinteger(L, plr->tripwireReboundDelay);
	else if (fastcmp(field,"shrinklaserdelay"))
		lua_pushinteger(L, plr->shrinkLaserDelay);
	else if (fastcmp(field,"eggmantransferdelay"))
		lua_pushinteger(L, plr->eggmanTransferDelay);
	else if (fastcmp(field,"wavedash"))
		lua_pushinteger(L, plr->wavedash);
	else if (fastcmp(field,"wavedashleft"))
		lua_pushinteger(L, plr->wavedashleft);
	else if (fastcmp(field,"wavedashright"))
		lua_pushinteger(L, plr->wavedashright);
	else if (fastcmp(field,"wavedashdelay"))
		lua_pushinteger(L, plr->wavedashdelay);
	else if (fastcmp(field,"wavedashboost"))
		lua_pushinteger(L, plr->wavedashboost);
	else if (fastcmp(field,"overdrive"))
		lua_pushinteger(L, plr->overdrive);
	else if (fastcmp(field,"overshield"))
		lua_pushinteger(L, plr->overshield);
	else if (fastcmp(field,"wavedashpower"))
		lua_pushinteger(L, plr->wavedashpower);
	else if (fastcmp(field,"overdrivepower"))
		lua_pushfixed(L, plr->overdrivepower);
	else if (fastcmp(field,"overdriveready"))
		lua_pushinteger(L, plr->overdriveready);
	else if (fastcmp(field,"overdrivelenient"))
		lua_pushboolean(L, plr->overdrivelenient);
	else if (fastcmp(field,"trickcharge"))
		lua_pushinteger(L, plr->trickcharge);
	else if (fastcmp(field,"infinitether"))
		lua_pushinteger(L, plr->infinitether);
	else if (fastcmp(field,"finalfailsafe"))
		lua_pushinteger(L, plr->finalfailsafe);
	else if (fastcmp(field,"freeringshootercooldown"))
		lua_pushinteger(L, plr->freeRingShooterCooldown);
	else if (fastcmp(field,"lastsafelap"))
		lua_pushinteger(L, plr->lastsafelap);
	else if (fastcmp(field,"lastsafecheatcheck"))
		lua_pushinteger(L, plr->lastsafecheatcheck);
	else if (fastcmp(field,"ignoreairtimeleniency"))
		lua_pushinteger(L, plr->ignoreAirtimeLeniency);
	else if (fastcmp(field,"bubbledrag"))
		lua_pushboolean(L, plr->bubbledrag);
	else if (fastcmp(field,"topaccel"))
		lua_pushinteger(L, plr->topAccel);
	else if (fastcmp(field,"vortexboost"))
		lua_pushinteger(L, plr->vortexBoost);
	else if (fastcmp(field,"instawhipcharge"))
		lua_pushinteger(L, plr->instaWhipCharge);
	else if (fastcmp(field,"pitblame"))
		lua_pushinteger(L, plr->pitblame);
	else if (fastcmp(field,"defenselockout"))
		lua_pushinteger(L, plr->defenseLockout);
	else if (fastcmp(field,"instawhipchargelockout"))
		lua_pushinteger(L, plr->instaWhipChargeLockout);
	else if (fastcmp(field,"oldguard"))
		lua_pushboolean(L, plr->oldGuard);
	else if (fastcmp(field,"powerupvfxtimer"))
		lua_pushinteger(L, plr->powerupVFXTimer);
	else if (fastcmp(field,"preventfailsafe"))
		lua_pushinteger(L, plr->preventfailsafe);
	else if (fastcmp(field,"tripwireunstuck"))
		lua_pushinteger(L, plr->tripwireUnstuck);
	else if (fastcmp(field,"bumpunstuck"))
		lua_pushinteger(L, plr->bumpUnstuck);
	else if (fastcmp(field,"handtimer"))
		lua_pushinteger(L, plr->handtimer);
	else if (fastcmp(field,"besthanddirection"))
		lua_pushangle(L, plr->besthanddirection);
	else if (fastcmp(field,"itemroulette"))
		LUA_PushUserdata(L, &plr->itemRoulette, META_ITEMROULETTE);
	else if (fastcmp(field,"itemtype"))
		lua_pushinteger(L, plr->itemtype);
	else if (fastcmp(field,"itemamount"))
		lua_pushinteger(L, plr->itemamount);
	else if (fastcmp(field,"backupitemtype"))
		lua_pushinteger(L, plr->backupitemtype);
	else if (fastcmp(field,"backupitemamount"))
		lua_pushinteger(L, plr->backupitemamount);
	else if (fastcmp(field,"throwdir"))
		lua_pushinteger(L, plr->throwdir);
	else if (fastcmp(field,"sadtimer"))
		lua_pushinteger(L, plr->sadtimer);
	else if (fastcmp(field,"rings"))
		lua_pushinteger(L, plr->rings);
	else if (fastcmp(field,"hudrings"))
		lua_pushinteger(L, plr->hudrings);
	else if (fastcmp(field,"pickuprings"))
		lua_pushinteger(L, plr->pickuprings);
	else if (fastcmp(field,"ringdelay"))
		lua_pushinteger(L, plr->ringdelay);
	else if (fastcmp(field,"ringboost"))
		lua_pushinteger(L, plr->ringboost);
	else if (fastcmp(field,"momentboost"))
		lua_pushinteger(L, plr->momentboost);
	else if (fastcmp(field,"sparkleanim"))
		lua_pushinteger(L, plr->sparkleanim);
	else if (fastcmp(field,"superring"))
		lua_pushinteger(L, plr->superring);
	else if (fastcmp(field,"superringdisplay"))
		lua_pushinteger(L, plr->superringdisplay);
	else if (fastcmp(field,"superringpeak"))
		lua_pushinteger(L, plr->superringpeak);
	else if (fastcmp(field,"superringalert"))
		lua_pushinteger(L, plr->superringalert);
	else if (fastcmp(field,"nextringaward"))
		lua_pushinteger(L, plr->nextringaward);
	else if (fastcmp(field,"ringvolume"))
		lua_pushinteger(L, plr->ringvolume);
	else if (fastcmp(field,"ringtransparency"))
		lua_pushinteger(L, plr->ringtransparency);
	else if (fastcmp(field,"ringburst"))
		lua_pushinteger(L, plr->ringburst);
	else if (fastcmp(field,"curshield"))
		lua_pushinteger(L, plr->curshield);
	else if (fastcmp(field,"bubblecool"))
		lua_pushinteger(L, plr->bubblecool);
	else if (fastcmp(field,"bubbleblowup"))
		lua_pushinteger(L, plr->bubbleblowup);
	else if (fastcmp(field,"flamedash"))
		lua_pushinteger(L, plr->flamedash);
	else if (fastcmp(field,"counterdash"))
		lua_pushinteger(L, plr->counterdash);
	else if (fastcmp(field,"flamemeter"))
		lua_pushinteger(L, plr->flamemeter);
	else if (fastcmp(field,"flamelength"))
		lua_pushinteger(L, plr->flamelength);
	else if (fastcmp(field,"lightningcharge"))
		lua_pushinteger(L, plr->lightningcharge);
	else if (fastcmp(field,"ballhogcharge"))
		lua_pushinteger(L, plr->ballhogcharge);
	else if (fastcmp(field,"ballhogburst"))
		lua_pushinteger(L, plr->ballhogburst);
	else if (fastcmp(field,"ballhogtap"))
		lua_pushinteger(L, plr->ballhogtap);
	else if (fastcmp(field,"ballhogreticule"))
		LUA_PushUserdata(L, plr->ballhogreticule, META_MOBJ);
	else if (fastcmp(field,"hyudorotimer"))
		lua_pushinteger(L, plr->hyudorotimer);
	else if (fastcmp(field,"stealingtimer"))
		lua_pushinteger(L, plr->stealingtimer);
	else if (fastcmp(field,"hoverhyudoro"))
		LUA_PushUserdata(L, plr->hoverhyudoro, META_MOBJ);
	else if (fastcmp(field,"sneakertimer"))
		lua_pushinteger(L, plr->sneakertimer);
	else if (fastcmp(field,"numsneakers"))
		lua_pushinteger(L, plr->numsneakers);
	else if (fastcmp(field,"panelsneakertimer"))
		lua_pushinteger(L, plr->panelsneakertimer);
	else if (fastcmp(field,"numpanelsneakers"))
		lua_pushinteger(L, plr->numpanelsneakers);
	else if (fastcmp(field,"weaksneakertimer"))
		lua_pushinteger(L, plr->weaksneakertimer);
	else if (fastcmp(field,"numweaksneakers"))
		lua_pushinteger(L, plr->numweaksneakers);
	else if (fastcmp(field,"floorboost"))
		lua_pushinteger(L, plr->floorboost);
	else if (fastcmp(field,"growshrinktimer"))
		lua_pushinteger(L, plr->growshrinktimer);
	else if (fastcmp(field,"rocketsneakertimer"))
		lua_pushinteger(L, plr->rocketsneakertimer);
	else if (fastcmp(field,"invincibilitytimer"))
		lua_pushinteger(L, plr->invincibilitytimer);
	else if (fastcmp(field,"invincibilityextensions"))
		lua_pushinteger(L, plr->invincibilityextensions);
	else if (fastcmp(field,"loneliness"))
		lua_pushinteger(L, plr->loneliness);
	else if (fastcmp(field,"eggmanexplode"))
		lua_pushinteger(L, plr->eggmanexplode);
	else if (fastcmp(field,"eggmanblame"))
		lua_pushinteger(L, plr->eggmanblame);
	else if (fastcmp(field,"bananadrag"))
		lua_pushinteger(L, plr->bananadrag);
	else if (fastcmp(field,"lastjawztarget"))
		lua_pushinteger(L, plr->lastjawztarget);
	else if (fastcmp(field,"jawztargetdelay"))
		lua_pushinteger(L, plr->jawztargetdelay);
	else if (fastcmp(field,"confirmvictim"))
		lua_pushinteger(L, plr->confirmVictim);
	else if (fastcmp(field,"confirmvictimdelay"))
		lua_pushinteger(L, plr->confirmVictimDelay);
	else if (fastcmp(field,"glancedir"))
		lua_pushinteger(L, plr->glanceDir);
	else if (fastcmp(field,"breathtimer"))
		lua_pushinteger(L, plr->breathTimer);
	else if (fastcmp(field,"trickpanel"))
		lua_pushinteger(L, plr->trickpanel);
	else if (fastcmp(field,"tricktime"))
		lua_pushinteger(L, plr->tricktime);
	else if (fastcmp(field,"trickboostpower"))
		lua_pushfixed(L, plr->trickboostpower);
	else if (fastcmp(field,"trickboostdecay"))
		lua_pushinteger(L, plr->trickboostdecay);
	else if (fastcmp(field,"trickboost"))
		lua_pushinteger(L, plr->trickboost);
	else if (fastcmp(field,"tricklock"))
		lua_pushinteger(L, plr->tricklock);
	else if (fastcmp(field,"dashringpulltics"))
		lua_pushinteger(L, plr->dashRingPullTics);
	else if (fastcmp(field,"dashringpushtics"))
		lua_pushinteger(L, plr->dashRingPushTics);
	else if (fastcmp(field,"pullup"))
		lua_pushboolean(L, plr->pullup);
	else if (fastcmp(field,"finalized"))
		lua_pushboolean(L, plr->finalized);
	else if (fastcmp(field,"ebrakefor"))
		lua_pushinteger(L, plr->ebrakefor);
	else if (fastcmp(field,"roundscore"))
		lua_pushinteger(L, plr->roundscore);
	else if (fastcmp(field,"emeralds"))
		lua_pushinteger(L, plr->emeralds);
	else if (fastcmp(field,"karmadelay"))
		lua_pushinteger(L, plr->karmadelay);
	else if (fastcmp(field,"spheres"))
		lua_pushinteger(L, plr->spheres);
	else if (fastcmp(field,"spheredigestion"))
		lua_pushinteger(L, plr->spheredigestion);
	else if (fastcmp(field,"pflags"))
		lua_pushinteger(L, plr->pflags);
	else if (fastcmp(field,"pflags2"))
		lua_pushinteger(L, plr->pflags2);
	else if (fastcmp(field,"panim"))
		lua_pushinteger(L, plr->panim);
	else if (fastcmp(field,"flashcount"))
		lua_pushinteger(L, plr->flashcount);
	else if (fastcmp(field,"flashpal"))
		lua_pushinteger(L, plr->flashpal);
	else if (fastcmp(field,"skincolor"))
		lua_pushinteger(L, plr->skincolor);
	else if (fastcmp(field,"skin"))
		lua_pushinteger(L, plr->skin);
	else if (fastcmp(field,"fakeskin"))
		lua_pushinteger(L, plr->fakeskin);
	else if (fastcmp(field,"lastfakeskin"))
		lua_pushinteger(L, plr->lastfakeskin);
	else if (fastcmp(field,"score"))
		lua_pushinteger(L, plr->score);
	// SRB2kart
	else if (fastcmp(field,"kartspeed"))
		lua_pushinteger(L, plr->kartspeed);
	else if (fastcmp(field,"kartweight"))
		lua_pushinteger(L, plr->kartweight);
	else if (fastcmp(field,"followerskin"))
		lua_pushinteger(L, plr->followerskin);
	else if (fastcmp(field,"followerready"))
		lua_pushboolean(L, plr->followerready);
	else if (fastcmp(field,"followercolor"))
		lua_pushinteger(L, plr->followercolor);
	else if (fastcmp(field,"follower"))
		LUA_PushUserdata(L, plr->follower, META_MOBJ);
	else if (fastcmp(field,"prefskin"))
		lua_pushinteger(L, plr->prefskin);
	else if (fastcmp(field,"prefcolor"))
		lua_pushinteger(L, plr->prefcolor);
	else if (fastcmp(field,"preffollower"))
		lua_pushinteger(L, plr->preffollower);
	else if (fastcmp(field,"preffollowercolor"))
		lua_pushinteger(L, plr->preffollowercolor);
	//

	// rideroids
	else if (fastcmp(field,"rideroid"))
		lua_pushboolean(L, plr->rideroid);
	else if (fastcmp(field,"rdnodepull"))
		lua_pushboolean(L, plr->rdnodepull);
	else if (fastcmp(field,"rideroidangle"))
		lua_pushinteger(L, plr->rideroidangle);
	else if (fastcmp(field,"rideroidspeed"))
		lua_pushinteger(L, plr->rideroidspeed);
	else if (fastcmp(field,"rideroidrollangle"))
		lua_pushinteger(L, plr->rideroidrollangle);
	else if (fastcmp(field,"rdaddmomx"))
		lua_pushinteger(L, plr->rdaddmomx);
	else if (fastcmp(field,"rdaddmomy"))
		lua_pushinteger(L, plr->rdaddmomy);
	else if (fastcmp(field,"rdaddmomz"))
		lua_pushinteger(L, plr->rdaddmomz);

	// bungee
	else if (fastcmp(field,"bungee"))
		lua_pushinteger(L, plr->bungee);

	// dlz hover
	else if (fastcmp(field,"lasthover"))
		lua_pushinteger(L, plr->lasthover);

	// dlz rocket
	else if (fastcmp(field,"dlzrocket"))
		lua_pushinteger(L, plr->dlzrocket);
	else if (fastcmp(field,"dlzrocketangle"))
		lua_pushinteger(L, plr->dlzrocketangle);
	else if (fastcmp(field,"dlzrocketanglev"))
		lua_pushinteger(L, plr->dlzrocketanglev);
	else if (fastcmp(field,"dlzrocketspd"))
		lua_pushinteger(L, plr->dlzrocketspd);

	// seasaws
	else if (fastcmp(field,"seasaw"))
		lua_pushboolean(L, plr->seasaw);
	else if (fastcmp(field,"seasawcooldown"))
		lua_pushinteger(L, plr->seasawcooldown);
	else if (fastcmp(field,"seasawdist"))
		lua_pushinteger(L, plr->seasawdist);
	else if (fastcmp(field,"seasawangle"))
		lua_pushinteger(L, plr->seasawangle);
	else if (fastcmp(field,"seasawangleadd"))
		lua_pushinteger(L, plr->seasawangleadd);
	else if (fastcmp(field,"seasawmoreangle"))
		lua_pushinteger(L, plr->seasawmoreangle);
	else if (fastcmp(field,"seasawdir"))
		lua_pushboolean(L, plr->seasawdir);

	// turbine
	else if (fastcmp(field,"turbine"))
		lua_pushinteger(L, plr->turbine);
	else if (fastcmp(field,"turbineangle"))
		lua_pushinteger(L, plr->turbineangle);
	else if (fastcmp(field,"turbineheight"))
		lua_pushinteger(L, plr->turbineheight);
	else if (fastcmp(field,"turbinespd"))
		lua_pushinteger(L, plr->turbinespd);

	//clouds
	else if (fastcmp(field,"cloud"))
		lua_pushinteger(L, plr->cloud);
	else if (fastcmp(field,"cloudlaunch"))
		lua_pushinteger(L, plr->cloudlaunch);
	else if (fastcmp(field,"cloudbuf"))
		lua_pushinteger(L, plr->cloudbuf);

	//tulips
	else if (fastcmp(field,"tulip"))
		lua_pushinteger(L, plr->tulip);
	else if (fastcmp(field,"tuliplaunch"))
		lua_pushinteger(L, plr->tuliplaunch);
	else if (fastcmp(field,"tulipbuf"))
		lua_pushinteger(L, plr->tulipbuf);

	else if (fastcmp(field,"charflags"))
		lua_pushinteger(L, plr->charflags);
	else if (fastcmp(field,"followitem"))
		lua_pushinteger(L, plr->followitem);
	else if (fastcmp(field,"followmobj"))
		LUA_PushUserdata(L, plr->followmobj, META_MOBJ);
	else if (fastcmp(field,"lives"))
		lua_pushinteger(L, plr->lives);
	else if (fastcmp(field,"xtralife"))
		lua_pushinteger(L, plr->xtralife);
	else if (fastcmp(field,"speed"))
		lua_pushfixed(L, plr->speed);
	else if (fastcmp(field,"lastspeed"))
		lua_pushfixed(L, plr->lastspeed);
	else if (fastcmp(field,"deadtimer"))
		lua_pushinteger(L, plr->deadtimer);
	else if (fastcmp(field,"exiting"))
		lua_pushinteger(L, plr->exiting);
	else if (fastcmp(field,"cmomx"))
		lua_pushfixed(L, plr->cmomx);
	else if (fastcmp(field,"cmomy"))
		lua_pushfixed(L, plr->cmomy);
	else if (fastcmp(field,"rmomx"))
		lua_pushfixed(L, plr->rmomx);
	else if (fastcmp(field,"rmomy"))
		lua_pushfixed(L, plr->rmomy);
	else if (fastcmp(field,"totalring"))
		lua_pushinteger(L, plr->totalring);
	else if (fastcmp(field,"realtime"))
		lua_pushinteger(L, plr->realtime);
	else if (fastcmp(field,"laps"))
		lua_pushinteger(L, plr->laps);
	else if (fastcmp(field,"latestlap"))
		lua_pushinteger(L, plr->latestlap);
	else if (fastcmp(field,"exp"))
		lua_pushinteger(L, plr->exp);
	else if (fastcmp(field,"gradingfactor"))
		lua_pushfixed(L, plr->gradingfactor);
	else if (fastcmp(field,"gradingpointnum"))
		lua_pushinteger(L, plr->gradingpointnum);
	else if (fastcmp(field,"checkpointid"))
		lua_pushinteger(L, plr->checkpointId);
	else if (fastcmp(field,"team"))
		lua_pushinteger(L, plr->team);
	else if (fastcmp(field,"checkskip"))
		lua_pushinteger(L, plr->checkskip);
	else if (fastcmp(field,"cheatchecknum"))
		lua_pushinteger(L, plr->cheatchecknum);
	else if (fastcmp(field,"duelscore"))
		lua_pushinteger(L, plr->duelscore);
	else if (fastcmp(field,"lastsidehit"))
		lua_pushinteger(L, plr->lastsidehit);
	else if (fastcmp(field,"lastlinehit"))
		lua_pushinteger(L, plr->lastlinehit);
	else if (fastcmp(field,"timeshit"))
		lua_pushinteger(L, plr->timeshit);
	else if (fastcmp(field,"timeshitprev"))
		lua_pushinteger(L, plr->timeshitprev);
	else if (fastcmp(field,"onconveyor"))
		lua_pushinteger(L, plr->onconveyor);
	else if (fastcmp(field,"awayviewmobj")) // FIXME: struct
		LUA_PushUserdata(L, plr->awayview.mobj, META_MOBJ);
	else if (fastcmp(field,"awayviewtics")) // FIXME: struct
		lua_pushinteger(L, plr->awayview.tics);

	else if (fastcmp(field,"spectator"))
		lua_pushboolean(L, plr->spectator);
	else if (fastcmp(field,"spectatewait"))
		lua_pushinteger(L, plr->spectatewait);
	else if (fastcmp(field,"bot"))
		lua_pushboolean(L, plr->bot);
	else if (fastcmp(field,"botvars"))
		LUA_PushUserdata(L, &plr->botvars, META_BOTVARS);
	else if (fastcmp(field,"jointime"))
		lua_pushinteger(L, plr->jointime);
	else if (fastcmp(field,"spectatorreentry"))
		lua_pushinteger(L, plr->spectatorReentry);
	else if (fastcmp(field,"griefvalue"))
		lua_pushinteger(L, plr->griefValue);
	else if (fastcmp(field,"griefstrikes"))
		lua_pushinteger(L, plr->griefStrikes);
	else if (fastcmp(field,"griefwarned"))
		lua_pushinteger(L, plr->griefWarned);
	else if (fastcmp(field,"typing_timer"))
		lua_pushinteger(L, plr->typing_timer);
	else if (fastcmp(field,"typing_duration"))
		lua_pushinteger(L, plr->typing_duration);
	else if (fastcmp(field,"kickstartaccel"))
		lua_pushinteger(L, plr->kickstartaccel);
	else if (fastcmp(field,"autoring"))
		lua_pushboolean(L, plr->autoring);
	else if (fastcmp(field,"stairjank"))
		lua_pushinteger(L, plr->stairjank);
	else if (fastcmp(field,"topdriftheld"))
		lua_pushinteger(L, plr->topdriftheld);
	else if (fastcmp(field,"topinfirst"))
		lua_pushinteger(L, plr->topinfirst);
	else if (fastcmp(field,"splitscreenindex"))
		lua_pushinteger(L, plr->splitscreenindex);
	else if (fastcmp(field,"stumbleindicator"))
		LUA_PushUserdata(L, plr->stumbleIndicator, META_MOBJ);
	else if (fastcmp(field,"wavedashindicator"))
		LUA_PushUserdata(L, plr->wavedashIndicator, META_MOBJ);
	else if (fastcmp(field,"trickindicator"))
		LUA_PushUserdata(L, plr->trickIndicator, META_MOBJ);
	else if (fastcmp(field,"whip"))
		LUA_PushUserdata(L, plr->whip, META_MOBJ);
	else if (fastcmp(field,"hand"))
		LUA_PushUserdata(L, plr->hand, META_MOBJ);
	else if (fastcmp(field,"flickyattacker"))
		LUA_PushUserdata(L, plr->flickyAttacker, META_MOBJ);
	else if (fastcmp(field,"stoneshoe"))
		LUA_PushUserdata(L, plr->stoneShoe, META_MOBJ);
	else if (fastcmp(field,"toxomistercloud"))
		LUA_PushUserdata(L, plr->toxomisterCloud, META_MOBJ);
#ifdef HWRENDER
	else if (fastcmp(field,"fovadd"))
		lua_pushfixed(L, plr->fovadd);
#endif
	else if (fastcmp(field,"ping"))
		lua_pushinteger(L, playerpingtable[( plr - players )]);
	else if (fastcmp(field, "publickey"))
		lua_pushstring(L, GetPrettyRRID(plr->public_key, false));
	else if (fastcmp(field, "loop"))
		LUA_PushUserdata(L, &plr->loop, META_SONICLOOPVARS);
	else if (fastcmp(field, "powerup"))
		LUA_PushUserdata(L, &plr->powerup, META_POWERUPVARS);
	else if (fastcmp(field, "icecube"))
		LUA_PushUserdata(L, &plr->icecube, META_ICECUBEVARS);
	else if (fastcmp(field,"darkness_start"))
		lua_pushinteger(L, plr->darkness_start);
	else if (fastcmp(field,"darkness_end"))
		lua_pushinteger(L, plr->darkness_end);
	else {
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, plr);
		lua_rawget(L, -2);
		if (!lua_istable(L, -1)) { // no extra values table
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no extvars table or field named '%s'; returning nil.\n"), "player_t", field);
			return 0;
		}
		lua_getfield(L, -1, field);
		if (lua_isnil(L, -1)) // no value for this field
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "player_t", field);
	}

	return 1;
}

#define NOSET luaL_error(L, LUA_QL("player_t") " field " LUA_QS " should not be set directly.", field)
static int player_set(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	const char *field = luaL_checkstring(L, 2);
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter player_t in CMD building code!");
	if (constplayer)
		return luaL_error(L, "Do not alter player_t while modifying the roulette!");

	if (fastcmp(field,"mo")) {
		mobj_t *newmo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		plr->mo->player = NULL; // remove player pointer from old mobj
		(newmo->player = plr)->mo = newmo; // set player pointer for new mobj, and set new mobj as the player's mobj
	}
	else if (fastcmp(field,"cmd"))
		return NOSET;
	else if (fastcmp(field,"oldcmd"))
		return NOSET;
	else if (fastcmp(field,"respawn"))
		return NOSET;
	else if (fastcmp(field,"playerstate"))
		plr->playerstate = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"viewz"))
		plr->viewz = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"viewheight"))
		plr->viewheight = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"skybox"))
		return NOSET;
	else if (fastcmp(field,"viewrollangle"))
		plr->viewrollangle = luaL_checkangle(L, 3);
	else if (fastcmp(field,"tilt"))
		plr->tilt = luaL_checkangle(L, 3);
	else if (fastcmp(field,"aiming")) {
		UINT8 i;
		plr->aiming = luaL_checkangle(L, 3);
		for (i = 0; i <= r_splitscreen; i++)
		{
			if (plr == &players[displayplayers[i]])
			{
				localaiming[i] = plr->aiming;
			}
		}
	}
	else if (fastcmp(field,"drawangle"))
		plr->drawangle = luaL_checkangle(L, 3);
	else if (fastcmp(field,"karthud"))
		return NOSET;
	else if (fastcmp(field,"pflags"))
		plr->pflags = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"pflags2"))
	{
		// It's a really bad idea to let Lua modify the voicechat-related flags.
		// If we notice they're modified in any way, don't set anything.
		UINT32 newflags = luaL_checkinteger(L, 3);
		UINT32 forbiddenFlags = PF2_SELFMUTE|PF2_SELFDEAFEN|PF2_SERVERMUTE|PF2_SERVERDEAFEN|PF2_SERVERTEMPMUTE;
		
		if ((newflags & forbiddenFlags) != (plr->pflags2 & forbiddenFlags))
			return luaL_error(L, "player_t's pflags2 member can't"
				"have their voice chat-related flags modified.");
		
		plr->pflags2 = newflags;
	}
	else if (fastcmp(field,"panim"))
		plr->panim = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flashcount"))
		plr->flashcount = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flashpal"))
		plr->flashpal = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"skincolor"))
	{
		UINT16 newcolor = luaL_checkinteger(L,3);
		if (newcolor >= numskincolors)
			return luaL_error(L, "player.skincolor %d out of range (0 - %d).", newcolor, numskincolors-1);
		plr->skincolor = newcolor;
	}
	else if (fastcmp(field,"skin"))
		return NOSET;
	else if (fastcmp(field,"fakeskin"))
		return NOSET;
	else if (fastcmp(field,"lastfakeskin"))
		return NOSET;
	else if (fastcmp(field,"score"))
		plr->score = luaL_checkinteger(L, 3);
	// SRB2kart
	else if (fastcmp(field,"nocontrol"))
		plr->nocontrol = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"carry"))
		plr->carry = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dye"))
		plr->dye = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"position"))
		plr->position = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"oldposition"))
		plr->oldposition = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"positiondelay"))
		plr->positiondelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"leaderpenalty"))
		plr->leaderpenalty = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"teamposition"))
		plr->teamposition = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"teamimportance"))
		plr->teamimportance = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"distancetofinish"))
		plr->distancetofinish = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"distancetofinishprev"))
		plr->distancetofinishprev = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"lastpickupdistance"))
		plr->lastpickupdistance = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastpickuptype"))
		plr->lastpickuptype = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"currentwaypoint"))
		plr->currentwaypoint = *((waypoint_t **)luaL_checkudata(L, 3, META_WAYPOINT));
	else if (fastcmp(field,"nextwaypoint"))
		plr->nextwaypoint = *((waypoint_t **)luaL_checkudata(L, 3, META_WAYPOINT));
	else if (fastcmp(field,"ringshooter"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->ringShooter, mo);
	}
	else if (fastcmp(field,"airtime"))
		plr->airtime = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastairtime"))
		plr->lastairtime = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bigwaypointgap"))
		plr->bigwaypointgap = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flashing"))
		plr->flashing = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spinouttimer"))
		plr->spinouttimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spinouttype"))
		plr->spinouttype = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"instashield"))
		plr->instashield = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"nullhitlag"))
		plr->nullHitlag = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wipeoutslow"))
		plr->wipeoutslow = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"justbumped"))
		plr->justbumped = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"noebrakemagnet"))
		plr->noEbrakeMagnet = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wallspikedampen"))
		plr->wallSpikeDampen = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tumblebounces"))
		plr->tumbleBounces = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tumbleheight"))
		plr->tumbleHeight = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"stunned"))
		plr->stunned = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flybot"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->flybot, mo);
	}
	else if (fastcmp(field,"justdi"))
		plr->justDI = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flipdi"))
		plr->flipDI = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"cangrabitems"))
		plr->cangrabitems = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"incontrol"))
		plr->incontrol = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"progressivethrust"))
		plr->progressivethrust = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringvisualwarning"))
		plr->ringvisualwarning = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bailcharge"))
		plr->bailcharge = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"baildrop"))
		plr->baildrop = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bailhitlag"))
		plr->bailhitlag = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"analoginput"))
		plr->analoginput = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"transfer"))
		plr->transfer = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"markedfordeath"))
		plr->markedfordeath = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"mfdfinish"))
		plr->mfdfinish = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"dotrickfx"))
		plr->dotrickfx = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"stingfx"))
		plr->stingfx = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"bumperinflate"))
		plr->bumperinflate = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringboxdelay"))
		plr->ringboxdelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringboxaward"))
		plr->ringboxaward = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastringboost"))
		plr->lastringboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"amps"))
		plr->amps = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"recentamps"))
		plr->recentamps = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"amppickup"))
		plr->amppickup = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ampspending"))
		plr->ampspending = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"itemflags"))
		plr->itemflags = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"outrun"))
		plr->outrun = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"drift"))
		plr->drift = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"driftcharge"))
		plr->driftcharge = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"driftboost"))
		plr->driftboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"strongdriftboost"))
		plr->strongdriftboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"gateboost"))
		plr->gateBoost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"gatesound"))
		plr->gateSound = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"startboost"))
		plr->startboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"neostartboost"))
		plr->neostartboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dropdashboost"))
		plr->dropdashboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aciddropdashboost"))
		plr->aciddropdashboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aizdriftstrat"))
		plr->aizdriftstrat = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aizdriftextend"))
		plr->aizdriftextend = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aizdrifttilt"))
		plr->aizdrifttilt = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aizdriftturn"))
		plr->aizdriftturn = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"underwatertilt"))
		plr->underwatertilt = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"offroad"))
		plr->offroad = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"tiregrease"))
		plr->tiregrease = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"springstars"))
		plr->springstars = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"springcolor"))
		plr->springcolor = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dashpadcooldown"))
		plr->dashpadcooldown = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spindash"))
		plr->spindash = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spindashspeed"))
		plr->spindashspeed = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spindashboost"))
		plr->spindashboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringboostinprogress"))
		plr->ringboostinprogress = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"fastfall"))
		plr->fastfall = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"fastfallbase"))
		plr->fastfallBase = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"numboosts"))
		plr->numboosts = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"boostpower"))
		plr->boostpower = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"speedboost"))
		plr->speedboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"accelboost"))
		plr->accelboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"handleboost"))
		plr->handleboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"boostangle"))
		plr->boostangle = luaL_checkangle(L, 3);
	else if (fastcmp(field,"stonedrag"))
		plr->stonedrag = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"draftpower"))
		plr->draftpower = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"draftleeway"))
		plr->draftleeway = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastdraft"))
		plr->lastdraft = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tripwirestate"))
		plr->tripwireState = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tripwirepass"))
		plr->tripwirePass = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"fakeboost"))
		plr->fakeBoost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"subsonicleniency"))
		plr->subsonicleniency = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tripwireleniency"))
		plr->tripwireLeniency = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tripwireairleniency"))
		plr->tripwireAirLeniency = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tripwirerebounddelay"))
		plr->tripwireReboundDelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"shrinklaserdelay"))
		plr->shrinkLaserDelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"eggmantransferdelay"))
		plr->eggmanTransferDelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wavedash"))
		plr->wavedash = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wavedashleft"))
		plr->wavedashleft = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wavedashright"))
		plr->wavedashright = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wavedashdelay"))
		plr->wavedashdelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wavedashboost"))
		plr->wavedashboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"overdrive"))
		plr->overdrive = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"overshield"))
		plr->overshield = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"wavedashpower"))
		plr->wavedashpower = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"overdrivepower"))
		plr->overdrivepower = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"overdriveready"))
		plr->overdriveready = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"overdrivelenient"))
		plr->overdrivelenient = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"trickcharge"))
		plr->trickcharge = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"infinitether"))
		plr->infinitether = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"finalfailsafe"))
		plr->finalfailsafe = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"freeringshootercooldown"))
		plr->freeRingShooterCooldown = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastsafelap"))
		plr->lastsafelap = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastsafecheatcheck"))
		plr->lastsafecheatcheck = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ignoreairtimeleniency"))
		plr->ignoreAirtimeLeniency = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bubbledrag"))
		plr->bubbledrag = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"topaccel"))
		plr->topAccel = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"vortexboost"))
		plr->vortexBoost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"instawhipcharge"))
		plr->instaWhipCharge = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"pitblame"))
		plr->pitblame = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"defenselockout"))
		plr->defenseLockout = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"instawhipchargelockout"))
		plr->instaWhipChargeLockout = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"oldguard"))
		plr->oldGuard = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"powerupvfxtimer"))
		plr->powerupVFXTimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"preventfailsafe"))
		plr->preventfailsafe = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tripwireunstuck"))
		plr->tripwireUnstuck = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bumpunstuck"))
		plr->bumpUnstuck = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"handtimer"))
		plr->handtimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"besthanddirection"))
		plr->besthanddirection = luaL_checkangle(L, 3);
	else if (fastcmp(field,"itemroulette"))
		return NOSET;
	else if (fastcmp(field,"itemtype"))
		plr->itemtype = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"itemamount"))
		plr->itemamount = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"backupitemtype"))
		plr->backupitemtype = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"backupitemamount"))
		plr->backupitemamount = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"throwdir"))
		plr->throwdir = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"sadtimer"))
		plr->sadtimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rings"))
		plr->rings = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"hudrings"))
		plr->hudrings = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"pickuprings"))
		plr->pickuprings = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringdelay"))
		plr->ringdelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringboost"))
		plr->ringboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"sparkleanim"))
		plr->sparkleanim = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"superring"))
		plr->superring = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"superringdisplay"))
		plr->superringdisplay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"superringpeak"))
		plr->superringpeak = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"superringalert"))
		plr->superringalert = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"nextringaward"))
		plr->nextringaward = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringvolume"))
		plr->ringvolume = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringtransparency"))
		plr->ringtransparency = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringburst"))
		plr->ringburst = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"curshield"))
		plr->curshield = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bubblecool"))
		plr->bubblecool = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bubbleblowup"))
		plr->bubbleblowup = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flamedash"))
		plr->flamedash = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"counterdash"))
		plr->counterdash = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flamemeter"))
		plr->flamemeter = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flamelength"))
		plr->flamelength = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lightningcharge"))
		plr->lightningcharge = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ballhogcharge"))
		plr->ballhogcharge = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ballhogburst"))
		plr->ballhogburst = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ballhogtap"))
		plr->ballhogtap = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ballhogreticule"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->ballhogreticule, mo);
	}
	else if (fastcmp(field,"hyudorotimer"))
		plr->hyudorotimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"stealingtimer"))
		plr->stealingtimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"hoverhyudoro"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->hoverhyudoro, mo);
	}
	else if (fastcmp(field,"sneakertimer"))
		plr->sneakertimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"numsneakers"))
		plr->numsneakers = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"panelsneakertimer"))
		plr->panelsneakertimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"numpanelsneakers"))
		plr->numpanelsneakers = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"weaksneakertimer"))
		plr->weaksneakertimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"numweaksneakers"))
		plr->numweaksneakers = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"floorboost"))
		plr->floorboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"growshrinktimer"))
		plr->growshrinktimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rocketsneakertimer"))
		plr->rocketsneakertimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"invincibilitytimer"))
		plr->invincibilitytimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"invincibilityextensions"))
		plr->invincibilityextensions = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"loneliness"))
		plr->loneliness = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"eggmanexplode"))
		plr->eggmanexplode = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"eggmanblame"))
		plr->eggmanblame = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bananadrag"))
		plr->bananadrag = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastjawztarget"))
		plr->lastjawztarget = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"jawztargetdelay"))
		plr->jawztargetdelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"confirmvictim"))
		plr->confirmVictim = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"confirmvictimdelay"))
		plr->confirmVictimDelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"glancedir"))
		plr->glanceDir = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"breathtimer"))
		plr->breathTimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"trickpanel"))
		plr->trickpanel = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tricktime"))
		plr->tricktime = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"trickboostpower"))
		plr->trickboostpower = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"trickboostdecay"))
		plr->trickboostdecay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"trickboost"))
		plr->trickboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tricklock"))
		plr->tricklock = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dashringpulltics"))
		plr->dashRingPullTics = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dashringpushtics"))
		plr->dashRingPushTics = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"pullup"))
		plr->pullup = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"finalized"))
		plr->finalized = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"ebrakefor"))
		plr->ebrakefor = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"roundscore"))
		plr->roundscore = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"emeralds"))
		plr->emeralds = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"karmadelay"))
		plr->karmadelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spheres"))
		plr->spheres = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spheredigestion"))
		plr->spheredigestion = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"kartspeed"))
		plr->kartspeed = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"kartweight"))
		plr->kartweight = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"followerskin"))
		plr->followerskin = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"followercolor"))
		plr->followercolor = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"followerready"))
		plr->followerready = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"follower"))
		return NOSET; // it's probably best we don't allow the follower mobj to change.
	else if (fastcmp(field,"prefskin"))
		return NOSET; // don't allow changing user preferences
	else if (fastcmp(field,"prefcolor"))
		return NOSET; // don't allow changing user preferences
	else if (fastcmp(field,"preffollower"))
		return NOSET; // don't allow changing user preferences
	else if (fastcmp(field,"preffollowercolor"))
		return NOSET; // don't allow changing user preferences

	// time to add to the endless elseif list!!!!
	// rideroids
	else if (fastcmp(field,"rideroid"))
		plr->rideroid = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"rdnodepull"))
		plr->rdnodepull = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"rideroidangle"))
		plr->rideroidangle = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rideroidspeed"))
		plr->rideroidspeed = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rideroidrollangle"))
		plr->rideroidrollangle = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rdaddmomx"))
		plr->rdaddmomx = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"rdaddmomy"))
		plr->rdaddmomy = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"rdaddmomz"))
		plr->rdaddmomz = luaL_checkfixed(L, 3);

	// bungee
	else if (fastcmp(field,"bungee"))
		plr->bungee = luaL_checkinteger(L, 3);

	// dlz hover
	else if (fastcmp(field,"lasthover"))
		plr->lasthover = luaL_checkinteger(L, 3);

	// dlz rocket
	else if (fastcmp(field,"dlzrocket"))
		plr->dlzrocket = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dlzrocketangle"))
		plr->dlzrocketangle = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dlzrocketanglev"))
		plr->dlzrocketanglev = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dlzrocketspd"))
		plr->dlzrocketspd = luaL_checkfixed(L, 3);

	// seasaws
	else if (fastcmp(field,"seasaw"))
		plr->seasaw = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"seasawcooldown"))
		plr->seasawcooldown = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"seasawdist"))
		plr->seasawdist = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"seasawangle"))
		plr->seasawangle = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"seasawangleadd"))
		plr->seasawangleadd = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"seasawmoreangle"))
		plr->seasawmoreangle = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"seasawdir"))
		plr->seasawdir = luaL_checkboolean(L, 3);

	// turbines
	else if (fastcmp(field,"turbine"))
		plr->turbine = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"turbineangle"))
		plr->turbineangle = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"turbineheight"))
		plr->turbineheight = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"turbinespd"))
		plr->turbinespd = luaL_checkinteger(L, 3);

	// clouds
	else if (fastcmp(field,"cloud"))
		plr->cloud = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"cloudlaunch"))
		plr->cloudlaunch = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"cloudbuf"))
		plr->cloudbuf = luaL_checkinteger(L, 3);

	// tulips
	else if (fastcmp(field,"tulip"))
		plr->tulip = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tuliplaunch"))
		plr->tuliplaunch = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tulipbuf"))
		plr->tulipbuf = luaL_checkinteger(L, 3);

	//
	else if (fastcmp(field,"charflags"))
		plr->charflags = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"followitem"))
		plr->followitem = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"followmobj"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->followmobj, mo);
	}
	else if (fastcmp(field,"lives"))
		plr->lives = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"xtralife"))
		plr->xtralife = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"speed"))
		plr->speed = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"lastspeed"))
		plr->lastspeed = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"deadtimer"))
		plr->deadtimer = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"exiting"))
		plr->exiting = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"cmomx"))
		plr->cmomx = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"cmomy"))
		plr->cmomy = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"rmomx"))
		plr->rmomx = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"rmomy"))
		plr->rmomy = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"totalring"))
		plr->totalring = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"realtime"))
		plr->realtime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"laps"))
		plr->laps = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"latestlap"))
		plr->latestlap = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"exp"))
		plr->exp = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"gradingfactor"))
		plr->gradingfactor = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"gradingpointnum"))
		plr->gradingpointnum = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"checkpointid"))
		plr->checkpointId = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"team"))
		G_AssignTeam(plr, (UINT8)luaL_checkinteger(L, 3));
	else if (fastcmp(field,"checkskip"))
		plr->checkskip = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"cheatchecknum"))
		plr->cheatchecknum = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"duelscore"))
		plr->duelscore = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastsidehit"))
		plr->lastsidehit = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastlinehit"))
		plr->lastlinehit = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"timeshit"))
		plr->timeshit = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"timeshitprev"))
		plr->timeshitprev = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"onconveyor"))
		plr->onconveyor = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"awayviewmobj")) // FIXME: struct
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->awayview.mobj, mo);
	}
	else if (fastcmp(field,"awayviewtics")) // FIXME: struct
	{
		plr->awayview.tics = (INT32)luaL_checkinteger(L, 3);
		if (plr->awayview.tics && !plr->awayview.mobj) // awayviewtics must ALWAYS have an awayviewmobj set!!
			P_SetTarget(&plr->awayview.mobj, plr->mo); // but since the script might set awayviewmobj immediately AFTER setting awayviewtics, use player mobj as filler for now.
	}
	else if (fastcmp(field,"spectator"))
		plr->spectator = lua_toboolean(L, 3);
	else if (fastcmp(field,"spectatewait"))
		plr->spectatewait = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bot"))
		return NOSET;
	else if (fastcmp(field,"botvars"))
		return NOSET;
	else if (fastcmp(field,"jointime"))
		return NOSET;
	else if (fastcmp(field,"spectatorreentry"))
		plr->spectatorReentry = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"griefvalue"))
		plr->griefValue = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"griefstrikes"))
		plr->griefStrikes = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"griefwarned"))
		plr->griefWarned = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"typing_timer"))
		plr->typing_timer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"typing_duration"))
		plr->typing_duration = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"kickstartaccel"))
		plr->kickstartaccel = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"autoring"))
		plr->autoring = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"stairjank"))
		plr->stairjank = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"topdriftheld"))
		plr->topdriftheld = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"topinfirst"))
		plr->topinfirst = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"splitscreenindex"))
		return NOSET;
	else if (fastcmp(field,"stumbleindicator"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->stumbleIndicator, mo);
	}
	else if (fastcmp(field,"wavedashindicator"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->wavedashIndicator, mo);
	}
	else if (fastcmp(field,"trickindicator"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->trickIndicator, mo);
	}
	else if (fastcmp(field,"whip"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->whip, mo);
	}
	else if (fastcmp(field,"hand"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->hand, mo);
	}
	else if (fastcmp(field,"flickyattacker"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->flickyAttacker, mo);
	}
	else if (fastcmp(field,"stoneshoe"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->stoneShoe, mo);
	}
	else if (fastcmp(field,"toxomistercloud"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->toxomisterCloud, mo);
	}
#ifdef HWRENDER
	else if (fastcmp(field,"fovadd"))
		plr->fovadd = luaL_checkfixed(L, 3);
#endif
	else if (fastcmp(field, "loop"))
		return NOSET;
	else if (fastcmp(field, "powerup"))
		return NOSET;
	else if (fastcmp(field, "icecube"))
		return NOSET;
	else if (fastcmp(field, "darkness_start"))
		plr->darkness_start = luaL_checkinteger(L, 3);
	else if (fastcmp(field, "darkness_end"))
		plr->darkness_end = luaL_checkinteger(L, 3);
	else {
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, plr);
		lua_rawget(L, -2);
		if (lua_isnil(L, -1)) {
			// This index doesn't have a table for extra values yet, let's make one.
			lua_pop(L, 1);
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; adding it as Lua data.\n"), "player_t", field);
			lua_newtable(L);
			lua_pushlightuserdata(L, plr);
			lua_pushvalue(L, -2); // ext value table
			lua_rawset(L, -4); // LREG_EXTVARS table
		}
		lua_pushvalue(L, 3); // value to store
		lua_setfield(L, -2, field);
		lua_pop(L, 2);
	}

	return 0;
}

#undef NOSET

static int player_num(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (!plr)
		return luaL_error(L, "accessed player_t doesn't exist anymore.");
	lua_pushinteger(L, plr-players);
	return 1;
}

// karthud, ks -> karthud[ks]
static int karthud_get(lua_State *L)
{
	INT32 *karthud = *((INT32 **)luaL_checkudata(L, 1, META_KARTHUD));
	karthudtype_t ks = luaL_checkinteger(L, 2);
	if (ks >= NUMKARTHUD)
		return luaL_error(L, LUA_QL("karthudtype_t") " cannot be %u", ks);
	lua_pushinteger(L, karthud[ks]);
	return 1;
}

// karthud, ks, value -> karthud[ks] = value
static int karthud_set(lua_State *L)
{
	INT32 *karthud = *((INT32 **)luaL_checkudata(L, 1, META_KARTHUD));
	karthudtype_t ks = luaL_checkinteger(L, 2);
	INT32 i = (INT32)luaL_checkinteger(L, 3);
	if (ks >= NUMKARTHUD)
		return luaL_error(L, LUA_QL("karthudtype_t") " cannot be %u", ks);
	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter player_t in CMD building code!");
	karthud[ks] = i;
	return 0;
}

// #karthud -> NUMKARTHUD
static int karthud_len(lua_State *L)
{
	lua_pushinteger(L, NUMKARTHUD);
	return 1;
}

// player.cmd get/set
#define NOFIELD luaL_error(L, LUA_QL("ticcmd_t") " has no field named " LUA_QS, field)
#define NOSET luaL_error(L, LUA_QL("ticcmd_t") " field " LUA_QS " cannot be set.", field)

static int ticcmd_get(lua_State *L)
{
	ticcmd_t *cmd = *((ticcmd_t **)luaL_checkudata(L, 1, META_TICCMD));
	const char *field = luaL_checkstring(L, 2);
	if (!cmd)
		return LUA_ErrInvalid(L, "player_t");

	if (fastcmp(field,"forwardmove"))
		lua_pushinteger(L, cmd->forwardmove);
	else if (fastcmp(field,"turning"))
		lua_pushinteger(L, cmd->turning);
	else if (fastcmp(field,"throwdir"))
		lua_pushinteger(L, cmd->throwdir);
	else if (fastcmp(field,"aiming"))
		lua_pushinteger(L, cmd->aiming);
	else if (fastcmp(field,"buttons"))
		lua_pushinteger(L, cmd->buttons);
	else if (fastcmp(field,"latency"))
		lua_pushinteger(L, cmd->latency);
	else if (fastcmp(field,"flags"))
		lua_pushinteger(L, cmd->flags);
	else
		return NOFIELD;

	return 1;
}

static int ticcmd_set(lua_State *L)
{
	ticcmd_t *cmd = *((ticcmd_t **)luaL_checkudata(L, 1, META_TICCMD));
	const char *field = luaL_checkstring(L, 2);
	if (!cmd)
		return LUA_ErrInvalid(L, "ticcmd_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");

	if (fastcmp(field,"forwardmove"))
		cmd->forwardmove = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"turning"))
		cmd->turning = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"throwdir"))
		cmd->throwdir = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aiming"))
		cmd->aiming = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"buttons"))
		cmd->buttons = (UINT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"latency"))
		return NOSET;
	else if (fastcmp(field,"flags"))
		return NOSET;
	else
		return NOFIELD;

	return 0;
}

#undef NOFIELD

enum sonicloopvars {
	sonicloopvars_radius = 0,
	sonicloopvars_revolution,
	sonicloopvars_min_revolution,
	sonicloopvars_max_revolution,
	sonicloopvars_yaw,
	sonicloopvars_origin_x,
	sonicloopvars_origin_y,
	sonicloopvars_origin_z,
	sonicloopvars_origin_shift_x,
	sonicloopvars_origin_shift_y,
	sonicloopvars_shift_x,
	sonicloopvars_shift_y,
	sonicloopvars_flip,
	sonicloopvars_camera,
};

static const char *const sonicloopvars_opt[] = {
	"radius",
	"revolution",
	"min_revolution",
	"max_revolution",
	"yaw",
	"origin_x",
	"origin_y",
	"origin_z",
	"origin_shift_x",
	"origin_shift_y",
	"shift_x",
	"shift_y",
	"flip",
	"camera",
	NULL
};

static int sonicloopvars_get(lua_State *L)
{
	sonicloopvars_t *sonicloopvars = *((sonicloopvars_t **)luaL_checkudata(L, 1, META_SONICLOOPVARS));
	enum sonicloopvars field = luaL_checkoption(L, 2, NULL, sonicloopvars_opt);

	// This should always be valid.
	I_Assert(sonicloopvars != NULL);

	switch (field)
	{
	case sonicloopvars_radius:
		lua_pushfixed(L, sonicloopvars->radius);
		break;
	case sonicloopvars_revolution:
		lua_pushfixed(L, sonicloopvars->revolution);
		break;
	case sonicloopvars_min_revolution:
		lua_pushfixed(L, sonicloopvars->min_revolution);
		break;
	case sonicloopvars_max_revolution:
		lua_pushfixed(L, sonicloopvars->max_revolution);
		break;
	case sonicloopvars_yaw:
		lua_pushangle(L, sonicloopvars->yaw);
		break;
	case sonicloopvars_origin_x:
		lua_pushfixed(L, sonicloopvars->origin.x);
		break;
	case sonicloopvars_origin_y:
		lua_pushfixed(L, sonicloopvars->origin.y);
		break;
	case sonicloopvars_origin_z:
		lua_pushfixed(L, sonicloopvars->origin.z);
		break;
	case sonicloopvars_origin_shift_x:
		lua_pushfixed(L, sonicloopvars->origin_shift.x);
		break;
	case sonicloopvars_origin_shift_y:
		lua_pushfixed(L, sonicloopvars->origin_shift.y);
		break;
	case sonicloopvars_shift_x:
		lua_pushfixed(L, sonicloopvars->shift.x);
		break;
	case sonicloopvars_shift_y:
		lua_pushfixed(L, sonicloopvars->shift.y);
		break;
	case sonicloopvars_flip:
		lua_pushboolean(L, sonicloopvars->flip);
		break;
	case sonicloopvars_camera:
		LUA_PushUserdata(L, &sonicloopvars->camera, META_SONICLOOPCAMVARS);
		break;
	}
	return 1;
}

static int sonicloopvars_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("sonicloopvars_t") " struct cannot be edited by Lua.");
}

enum sonicloopcamvars {
	sonicloopcamvars_enter_tic = 0,
	sonicloopcamvars_exit_tic,
	sonicloopcamvars_zoom_in_speed,
	sonicloopcamvars_zoom_out_speed,
	sonicloopcamvars_dist,
	sonicloopcamvars_pan,
	sonicloopcamvars_pan_speed,
	sonicloopcamvars_pan_accel,
	sonicloopcamvars_pan_back,
};

static const char *const sonicloopcamvars_opt[] = {
	"enter_tic",
	"exit_tic",
	"zoom_in_speed",
	"zoom_out_speed",
	"dist",
	"pan",
	"pan_speed",
	"pan_accel",
	"pan_back",
	NULL
};

static int sonicloopcamvars_get(lua_State *L)
{
	sonicloopcamvars_t *sonicloopcamvars = *((sonicloopcamvars_t **)luaL_checkudata(L, 1, META_SONICLOOPCAMVARS));
	enum sonicloopcamvars field = luaL_checkoption(L, 2, NULL, sonicloopcamvars_opt);

	// This should always be valid.
	I_Assert(sonicloopcamvars != NULL);

	switch (field)
	{
	case sonicloopcamvars_enter_tic:
		lua_pushinteger(L, sonicloopcamvars->enter_tic);
		break;
	case sonicloopcamvars_exit_tic:
		lua_pushinteger(L, sonicloopcamvars->exit_tic);
		break;
	case sonicloopcamvars_zoom_in_speed:
		lua_pushinteger(L, sonicloopcamvars->zoom_in_speed);
		break;
	case sonicloopcamvars_zoom_out_speed:
		lua_pushinteger(L, sonicloopcamvars->zoom_out_speed);
		break;
	case sonicloopcamvars_dist:
		lua_pushfixed(L, sonicloopcamvars->dist);
		break;
	case sonicloopcamvars_pan:
		lua_pushangle(L, sonicloopcamvars->pan);
		break;
	case sonicloopcamvars_pan_speed:
		lua_pushfixed(L, sonicloopcamvars->pan_speed);
		break;
	case sonicloopcamvars_pan_accel:
		lua_pushinteger(L, sonicloopcamvars->pan_accel);
		break;
	case sonicloopcamvars_pan_back:
		lua_pushinteger(L, sonicloopcamvars->pan_back);
		break;
	}
	return 1;
}

static int sonicloopcamvars_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("sonicloopcamvars_t") " struct cannot be edited by Lua.");
}

#define NOFIELD luaL_error(L, LUA_QL("powerupvars_t") " has no field named " LUA_QS, field)

enum powerupvars {
	powerupvars_supertimer = 0,
	powerupvars_barriertimer,
	powerupvars_rhythmbadgetimer,
	powerupvars_flickycontroller,
	powerupvars_barrier,
};

static const char *const powerupvars_opt[] = {
	"supertimer",
	"barriertimer",
	"rhythmbadgetimer",
	"flickycontroller",
	"barrier",
	NULL
};

static int powerupvars_get(lua_State *L)
{
	powerupvars_t *powerupvars = *((powerupvars_t **)luaL_checkudata(L, 1, META_POWERUPVARS));
	enum powerupvars field = luaL_checkoption(L, 2, NULL, powerupvars_opt);
	
	if (!powerupvars)
		return LUA_ErrInvalid(L, "powerupvars_t");
	
	switch (field)
	{
		case powerupvars_supertimer:
			lua_pushinteger(L, powerupvars->superTimer);
			break;
		case powerupvars_barriertimer:
			lua_pushinteger(L, powerupvars->barrierTimer);
			break;
		case powerupvars_rhythmbadgetimer:
			lua_pushinteger(L, powerupvars->rhythmBadgeTimer);
			break;
		case powerupvars_flickycontroller:
			LUA_PushUserdata(L, powerupvars->flickyController, META_MOBJ);
			break;
		case powerupvars_barrier:
			LUA_PushUserdata(L, powerupvars->barrier, META_MOBJ);
			break;
		default:
			return NOFIELD;
	}

	return 1;
}

static int powerupvars_set(lua_State *L)
{
	powerupvars_t *powerupvars = *((powerupvars_t **)luaL_checkudata(L, 1, META_POWERUPVARS));
	enum powerupvars field = luaL_checkoption(L, 2, powerupvars_opt[0], powerupvars_opt);

	if (!powerupvars)
		return LUA_ErrInvalid(L, "powerupvars_t");

	if (hud_running)
		return luaL_error(L, "Do not alter powerupvars_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter powerupvars_t in CMD building code!");
	
	mobj_t *mo = NULL;
	if (!lua_isnil(L, 3) && lua_isuserdata(L, 3))
		mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	
	switch (field)
	{
		case powerupvars_supertimer:
			powerupvars->superTimer = luaL_checkinteger(L, 3);
			break;
		case powerupvars_barriertimer:
			powerupvars->barrierTimer = luaL_checkinteger(L, 3);
			break;
		case powerupvars_rhythmbadgetimer:
			powerupvars->rhythmBadgeTimer = luaL_checkinteger(L, 3);
			break;
		case powerupvars_flickycontroller:
			P_SetTarget(&powerupvars->flickyController, mo);
			break;
		case powerupvars_barrier:
			P_SetTarget(&powerupvars->barrier, mo);
			break;
		default:
			return NOFIELD;
	}

	return 0;
}

#undef NOFIELD

#define NOFIELD luaL_error(L, LUA_QL("skybox_t") " has no field named " LUA_QS, field)

enum icecubevars {
	icecubevars_hitat = 0,
	icecubevars_frozen,
	icecubevars_wiggle,
	icecubevars_frozenat,
	icecubevars_shaketimer,
};

static const char *const icecubevars_opt[] = {
	"hitat",
	"frozen",
	"wiggle",
	"frozenat",
	"shaketimer",
	NULL
};

static int icecubevars_get(lua_State *L)
{
	icecubevars_t *icecubevars = *((icecubevars_t **)luaL_checkudata(L, 1, META_ICECUBEVARS));
	enum icecubevars field = luaL_checkoption(L, 2, NULL, icecubevars_opt);
	
	if (!icecubevars)
		return LUA_ErrInvalid(L, "icecubevars_t");
	
	switch (field)
	{
		case icecubevars_hitat:
			lua_pushinteger(L, icecubevars->hitat);
			break;
		case icecubevars_frozen:
			lua_pushboolean(L, icecubevars->frozen);
			break;
		case icecubevars_wiggle:
			lua_pushinteger(L, icecubevars->wiggle);
			break;
		case icecubevars_frozenat:
			lua_pushinteger(L, icecubevars->frozenat);
			break;
		case icecubevars_shaketimer:
			lua_pushinteger(L, icecubevars->shaketimer);
			break;
		default:
			return NOFIELD;
	}

	return 1;
}

static int icecubevars_set(lua_State *L)
{
	icecubevars_t *icecubevars = *((icecubevars_t **)luaL_checkudata(L, 1, META_ICECUBEVARS));
	enum icecubevars field = luaL_checkoption(L, 2, icecubevars_opt[0], icecubevars_opt);

	if (!icecubevars)
		return LUA_ErrInvalid(L, "icecubevars_t");

	if (hud_running)
		return luaL_error(L, "Do not alter icecubevars_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter icecubevars_t in CMD building code!");
	
	switch (field)
	{
		case icecubevars_hitat:
			icecubevars->hitat = luaL_checkinteger(L, 3);
			break;
		case icecubevars_frozen:
			icecubevars->frozen = luaL_checkboolean(L, 3);
			break;
		case icecubevars_wiggle:
			icecubevars->wiggle = luaL_checkinteger(L, 3);
			break;
		case icecubevars_frozenat:
			icecubevars->frozenat = luaL_checkinteger(L, 3);
			break;
		case icecubevars_shaketimer:
			icecubevars->shaketimer = luaL_checkinteger(L, 3);
			break;
		default:
			return NOFIELD;
	}

	return 0;
}

#undef NOFIELD

#define NOFIELD luaL_error(L, LUA_QL("skybox_t") " has no field named " LUA_QS, field)

enum skybox {
	skybox_viewpoint = 0,
	skybox_centerpoint,
};

static const char *const skybox_opt[] = {
	"viewpoint",
	"centerpoint",
	NULL
};

static int skybox_get(lua_State *L)
{
	skybox_t *skybox = *((skybox_t **)luaL_checkudata(L, 1, META_SKYBOX));
	enum skybox field = luaL_checkoption(L, 2, NULL, skybox_opt);
	
	if (!skybox)
		return LUA_ErrInvalid(L, "skybox_t");
	
	switch (field)
	{
		case skybox_viewpoint:
			LUA_PushUserdata(L, skybox->viewpoint, META_MOBJ);
			break;
		case skybox_centerpoint:
			LUA_PushUserdata(L, skybox->centerpoint, META_MOBJ);
			break;
		default:
			return NOFIELD;
	}

	return 1;
}

static int skybox_set(lua_State *L)
{
	skybox_t *skybox = *((skybox_t **)luaL_checkudata(L, 1, META_SKYBOX));
	enum skybox field = luaL_checkoption(L, 2, skybox_opt[0], skybox_opt);

	if (!skybox)
		return LUA_ErrInvalid(L, "skybox_t");

	if (hud_running)
		return luaL_error(L, "Do not alter skybox_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter skybox_t in CMD building code!");
	
	mobj_t *mo = NULL;
	if (!lua_isnil(L, 3) && lua_isuserdata(L, 3))
		mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	
	switch (field)
	{
		case skybox_viewpoint:
			P_SetTarget(&skybox->viewpoint, mo);
			break;
		case skybox_centerpoint:
			P_SetTarget(&skybox->centerpoint, mo);
			break;
		default:
			return NOFIELD;
	}

	return 0;
}

#undef NOFIELD

int LUA_PlayerLib(lua_State *L)
{
	luaL_newmetatable(L, META_PLAYER);
		lua_pushcfunction(L, player_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, player_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, player_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	luaL_newmetatable(L, META_KARTHUD);
		lua_pushcfunction(L, karthud_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, karthud_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, karthud_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	luaL_newmetatable(L, META_TICCMD);
		lua_pushcfunction(L, ticcmd_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, ticcmd_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	luaL_newmetatable(L, META_SONICLOOPVARS);
		lua_pushcfunction(L, sonicloopvars_get);
		lua_setfield(L, -2, "__index");
		
		lua_pushcfunction(L, sonicloopvars_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	luaL_newmetatable(L, META_SONICLOOPCAMVARS);
		lua_pushcfunction(L, sonicloopcamvars_get);
		lua_setfield(L, -2, "__index");
		
		lua_pushcfunction(L, sonicloopcamvars_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_POWERUPVARS);
		lua_pushcfunction(L, powerupvars_get);
		lua_setfield(L, -2, "__index");
		
		lua_pushcfunction(L, powerupvars_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_ICECUBEVARS);
		lua_pushcfunction(L, icecubevars_get);
		lua_setfield(L, -2, "__index");
		
		lua_pushcfunction(L, icecubevars_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_SKYBOX);
		lua_pushcfunction(L, skybox_get);
		lua_setfield(L, -2, "__index");
		
		lua_pushcfunction(L, skybox_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getPlayer);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_lenPlayer);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "players");

	// push displayplayers in the same fashion
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getDisplayplayers);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_lenDisplayplayers);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "displayplayers");

	return 0;
}
