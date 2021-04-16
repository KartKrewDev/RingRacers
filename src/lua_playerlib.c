// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2020 by Sonic Team Junior.
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
	INT32 i = -1;
	INT32 temp = -1;
	INT32 iter = 0;

	if (lua_gettop(L) < 2)
	{
		//return luaL_error(L, "Don't call displayplayers.iterate() directly, use it as 'for player in displayplayers.iterate do <block> end'.");
		lua_pushcfunction(L, lib_iterateDisplayplayers);
		return 1;
	}
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
	{
		temp = (INT32)(*((player_t **)luaL_checkudata(L, 1, META_PLAYER)) - players);	// get the player # of the last iterated player.

		// @FIXME:
		// I didn't quite find a better way for this; Here, we go back to which player in displayplayers we last iterated to resume the for loop below for this new function call
		// I don't understand enough about how the Lua stacks work to get this to work in possibly a single line.
		// So anyone feel free to correct this!

		for (; iter < MAXSPLITSCREENPLAYERS; iter++)
		{
			if (displayplayers[iter] == temp)
			{
				i = iter;
				break;
			}
		}
	}

	for (i++; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (i > splitscreen || !playeringame[displayplayers[i]])
			return 0;	// Stop! There are no more players for us to go through. There will never be a player gap in displayplayers.

		LUA_PushUserdata(L, &players[displayplayers[i]], META_PLAYER);
		lua_pushinteger(L, i);	// push this to recall what number we were on for the next function call. I suppose this also means you can retrieve the splitscreen player number with 'for p, n in displayplayers.iterate'!
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
		if (i > splitscreen)
			return 0;
		if (!playeringame[displayplayers[i]])
			return 0;
		LUA_PushUserdata(L, &players[displayplayers[i]], META_PLAYER);
		return 1;
	}

	field = luaL_checkstring(L, 2);
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateDisplayplayers);
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
	else if (fastcmp(field,"respawn"))
		LUA_PushUserdata(L, &plr->respawn, META_RESPAWN);
	else if (fastcmp(field,"playerstate"))
		lua_pushinteger(L, plr->playerstate);
	else if (fastcmp(field,"viewz"))
		lua_pushfixed(L, plr->viewz);
	else if (fastcmp(field,"viewheight"))
		lua_pushfixed(L, plr->viewheight);
	else if (fastcmp(field,"viewrollangle"))
		lua_pushangle(L, plr->viewrollangle);
	else if (fastcmp(field,"aiming"))
		lua_pushangle(L, plr->aiming);
	else if (fastcmp(field,"drawangle"))
		lua_pushangle(L, plr->drawangle);
	else if (fastcmp(field,"karthud"))
		LUA_PushUserdata(L, plr->karthud, META_KARTHUD);
	else if (fastcmp(field,"ktemp_nocontrol"))
		lua_pushinteger(L, plr->ktemp_nocontrol);
	else if (fastcmp(field,"ktemp_carry"))
		lua_pushinteger(L, plr->ktemp_carry);
	else if (fastcmp(field,"ktemp_dye"))
		lua_pushinteger(L, plr->ktemp_dye);
	else if (fastcmp(field,"ktemp_position"))
		lua_pushinteger(L, plr->ktemp_position);
	else if (fastcmp(field,"ktemp_oldposition"))
		lua_pushinteger(L, plr->ktemp_oldposition);
	else if (fastcmp(field,"ktemp_positiondelay"))
		lua_pushinteger(L, plr->ktemp_positiondelay);
	else if (fastcmp(field,"distancetofinish"))
		lua_pushinteger(L, plr->distancetofinish);
	else if (fastcmp(field,"airtime"))
		lua_pushinteger(L, plr->airtime);
	else if (fastcmp(field,"ktemp_flashing"))
		lua_pushinteger(L, plr->ktemp_flashing);
	else if (fastcmp(field,"ktemp_spinouttimer"))
		lua_pushinteger(L, plr->ktemp_spinouttimer);
	else if (fastcmp(field,"ktemp_instashield"))
		lua_pushinteger(L, plr->ktemp_instashield);
	else if (fastcmp(field,"ktemp_wipeoutslow"))
		lua_pushinteger(L, plr->ktemp_wipeoutslow);
	else if (fastcmp(field,"ktemp_justbumped"))
		lua_pushinteger(L, plr->ktemp_justbumped);
	else if (fastcmp(field,"tumbleBounces"))
		lua_pushinteger(L, plr->tumbleBounces);
	else if (fastcmp(field,"tumbleHeight"))
		lua_pushinteger(L, plr->tumbleHeight);
	else if (fastcmp(field,"ktemp_drift"))
		lua_pushinteger(L, plr->ktemp_drift);
	else if (fastcmp(field,"ktemp_driftcharge"))
		lua_pushinteger(L, plr->ktemp_driftcharge);
	else if (fastcmp(field,"ktemp_driftboost"))
		lua_pushinteger(L, plr->ktemp_driftboost);
	else if (fastcmp(field,"ktemp_aizdriftstraft"))
		lua_pushinteger(L, plr->ktemp_aizdriftstrat);
	else if (fastcmp(field,"aizdrifttilt"))
		lua_pushinteger(L, plr->aizdrifttilt);
	else if (fastcmp(field,"aizdriftturn"))
		lua_pushinteger(L, plr->aizdriftturn);
	else if (fastcmp(field,"ktemp_offroad"))
		lua_pushinteger(L, plr->ktemp_offroad);
	else if (fastcmp(field,"ktemp_waterskip"))
		lua_pushinteger(L, plr->ktemp_waterskip);
	else if (fastcmp(field,"ktemp_tiregrease"))
		lua_pushinteger(L, plr->ktemp_tiregrease);
	else if (fastcmp(field,"ktemp_springstars"))
		lua_pushinteger(L, plr->ktemp_springstars);
	else if (fastcmp(field,"ktemp_springcolor"))
		lua_pushinteger(L, plr->ktemp_springcolor);
	else if (fastcmp(field,"ktemp_dashpadcooldown"))
		lua_pushinteger(L, plr->ktemp_dashpadcooldown);
	else if (fastcmp(field,"ktemp_spindash"))
		lua_pushinteger(L, plr->ktemp_spindash);
	else if (fastcmp(field,"ktemp_spindashspeed"))
		lua_pushinteger(L, plr->ktemp_spindashspeed);
	else if (fastcmp(field,"ktemp_spindashboost"))
		lua_pushinteger(L, plr->ktemp_spindashboost);
	else if (fastcmp(field,"ktemp_numboosts"))
		lua_pushinteger(L, plr->ktemp_numboosts);
	else if (fastcmp(field,"ktemp_boostpower"))
		lua_pushinteger(L, plr->ktemp_boostpower);
	else if (fastcmp(field,"ktemp_speedboost"))
		lua_pushinteger(L, plr->ktemp_speedboost);
	else if (fastcmp(field,"ktemp_accelboost"))
		lua_pushinteger(L, plr->ktemp_accelboost);
	else if (fastcmp(field,"ktemp_handleboost"))
		lua_pushinteger(L, plr->ktemp_handleboost);
	else if (fastcmp(field,"ktemp_boostangle"))
		lua_pushinteger(L, plr->ktemp_boostangle);
	else if (fastcmp(field,"ktemp_draftpower"))
		lua_pushinteger(L, plr->ktemp_draftpower);
	else if (fastcmp(field,"ktemp_draftleeway"))
		lua_pushinteger(L, plr->ktemp_draftleeway);
	else if (fastcmp(field,"ktemp_lastdraft"))
		lua_pushinteger(L, plr->ktemp_lastdraft);
	else if (fastcmp(field,"ktemp_itemroulette"))
		lua_pushinteger(L, plr->ktemp_itemroulette);
	else if (fastcmp(field,"ktemp_roulettetype"))
		lua_pushinteger(L, plr->ktemp_roulettetype);
	else if (fastcmp(field,"ktemp_itemtype"))
		lua_pushinteger(L, plr->ktemp_itemtype);
	else if (fastcmp(field,"ktemp_itemamount"))
		lua_pushinteger(L, plr->ktemp_itemamount);
	else if (fastcmp(field,"ktemp_throwdir"))
		lua_pushinteger(L, plr->ktemp_throwdir);
	else if (fastcmp(field,"ktemp_sadtimer"))
		lua_pushinteger(L, plr->ktemp_sadtimer);
	else if (fastcmp(field,"rings"))
		lua_pushinteger(L, plr->rings);
	else if (fastcmp(field,"ktemp_pickuprings"))
		lua_pushinteger(L, plr->ktemp_pickuprings);
	else if (fastcmp(field,"ktemp_ringdelay"))
		lua_pushinteger(L, plr->ktemp_ringdelay);
	else if (fastcmp(field,"ktemp_ringboost"))
		lua_pushinteger(L, plr->ktemp_ringboost);
	else if (fastcmp(field,"ktemp_sparkleanim"))
		lua_pushinteger(L, plr->ktemp_sparkleanim);
	else if (fastcmp(field,"ktemp_superring"))
		lua_pushinteger(L, plr->ktemp_superring);
	else if (fastcmp(field,"ktemp_curshield"))
		lua_pushinteger(L, plr->ktemp_curshield);
	else if (fastcmp(field,"ktemp_bubblecool"))
		lua_pushinteger(L, plr->ktemp_bubblecool);
	else if (fastcmp(field,"ktemp_bubbleblowup"))
		lua_pushinteger(L, plr->ktemp_bubbleblowup);
	else if (fastcmp(field,"ktemp_flamedash"))
		lua_pushinteger(L, plr->ktemp_flamedash);
	else if (fastcmp(field,"ktemp_flamemeter"))
		lua_pushinteger(L, plr->ktemp_flamemeter);
	else if (fastcmp(field,"ktemp_flamelength"))
		lua_pushinteger(L, plr->ktemp_flamelength);
	else if (fastcmp(field,"ktemp_hyudorotimer"))
		lua_pushinteger(L, plr->ktemp_hyudorotimer);
	else if (fastcmp(field,"ktemp_stealingtimer"))
		lua_pushinteger(L, plr->ktemp_stealingtimer);
	else if (fastcmp(field,"ktemp_sneakertimer"))
		lua_pushinteger(L, plr->ktemp_sneakertimer);
	else if (fastcmp(field,"ktemp_numsneakers"))
		lua_pushinteger(L, plr->ktemp_numsneakers);
	else if (fastcmp(field,"ktemp_floorboost"))
		lua_pushinteger(L, plr->ktemp_floorboost);
	else if (fastcmp(field,"ktemp_growshrinktimer"))
		lua_pushinteger(L, plr->ktemp_growshrinktimer);
	else if (fastcmp(field,"ktemp_rocketsneakertimer"))
		lua_pushinteger(L, plr->ktemp_rocketsneakertimer);
	else if (fastcmp(field,"ktemp_invincibilitytimer"))
		lua_pushinteger(L, plr->ktemp_invincibilitytimer);
	else if (fastcmp(field,"ktemp_eggmanexplode"))
		lua_pushinteger(L, plr->ktemp_eggmanexplode);
	else if (fastcmp(field,"ktemp_eggmanblame"))
		lua_pushinteger(L, plr->ktemp_eggmanblame);
	else if (fastcmp(field,"ktemp_bananadrag"))
		lua_pushinteger(L, plr->ktemp_bananadrag);
	else if (fastcmp(field,"ktemp_lastjawztarget"))
		lua_pushinteger(L, plr->ktemp_lastjawztarget);
	else if (fastcmp(field,"ktemp_jawztargetdelay"))
		lua_pushinteger(L, plr->ktemp_jawztargetdelay);
	else if (fastcmp(field,"glanceDir"))
		lua_pushinteger(L, plr->glanceDir);
	else if (fastcmp(field,"trickpanel"))
		lua_pushinteger(L, plr->trickpanel);
	else if (fastcmp(field,"trickmomx"))
		lua_pushfixed(L, plr->trickmomx);
	else if (fastcmp(field,"trickmomy"))
		lua_pushfixed(L, plr->trickmomy);
	else if (fastcmp(field,"trickmomz"))
		lua_pushfixed(L, plr->trickmomz);
	else if (fastcmp(field,"roundscore"))
		plr->roundscore = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_emeralds"))
		lua_pushinteger(L, plr->ktemp_emeralds);
	else if (fastcmp(field,"bumpers"))
		lua_pushinteger(L, plr->bumpers);
	else if (fastcmp(field,"karmadelay"))
		lua_pushinteger(L, plr->karmadelay);
	else if (fastcmp(field,"spheres"))
		lua_pushinteger(L, plr->spheres);
	else if (fastcmp(field,"pflags"))
		lua_pushinteger(L, plr->pflags);
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
	else if (fastcmp(field,"availabilities"))
		lua_pushinteger(L, plr->availabilities);
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
	//
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
	else if (fastcmp(field,"ctfteam"))
		lua_pushinteger(L, plr->ctfteam);
	else if (fastcmp(field,"checkskip"))
		lua_pushinteger(L, plr->checkskip);
	else if (fastcmp(field,"starpostnum"))
		lua_pushinteger(L, plr->starpostnum);
	else if (fastcmp(field,"lastsidehit"))
		lua_pushinteger(L, plr->lastsidehit);
	else if (fastcmp(field,"lastlinehit"))
		lua_pushinteger(L, plr->lastlinehit);
	else if (fastcmp(field,"onconveyor"))
		lua_pushinteger(L, plr->onconveyor);
	else if (fastcmp(field,"awayviewmobj"))
		LUA_PushUserdata(L, plr->awayviewmobj, META_MOBJ);
	else if (fastcmp(field,"awayviewtics"))
		lua_pushinteger(L, plr->awayviewtics);
	else if (fastcmp(field,"awayviewaiming"))
		lua_pushangle(L, plr->awayviewaiming);

	else if (fastcmp(field,"spectator"))
		lua_pushboolean(L, plr->spectator);
	else if (fastcmp(field,"bot"))
		lua_pushboolean(L, plr->bot);
	else if (fastcmp(field,"jointime"))
		lua_pushinteger(L, plr->jointime);
	else if (fastcmp(field,"splitscreenindex"))
		lua_pushinteger(L, plr->splitscreenindex);
	else if (fastcmp(field,"quittime"))
		lua_pushinteger(L, plr->quittime);
#ifdef HWRENDER
	else if (fastcmp(field,"fovadd"))
		lua_pushfixed(L, plr->fovadd);
#endif
	else if (fastcmp(field,"ping"))
		lua_pushinteger(L, playerpingtable[( plr - players )]);
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

	if (hook_cmd_running)
		return luaL_error(L, "Do not alter player_t in BuildCMD code!");

	if (fastcmp(field,"mo")) {
		mobj_t *newmo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		plr->mo->player = NULL; // remove player pointer from old mobj
		(newmo->player = plr)->mo = newmo; // set player pointer for new mobj, and set new mobj as the player's mobj
	}
	else if (fastcmp(field,"cmd"))
		return NOSET;
	else if (fastcmp(field,"respawn"))
		return NOSET;
	else if (fastcmp(field,"playerstate"))
		plr->playerstate = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"viewz"))
		plr->viewz = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"viewheight"))
		plr->viewheight = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"viewrollangle"))
		plr->viewrollangle = luaL_checkangle(L, 3);
	else if (fastcmp(field,"aiming")) {
		UINT8 i;
		plr->aiming = luaL_checkangle(L, 3);
		for (i = 0; i <= r_splitscreen; i++)
		{
			if (plr == &players[displayplayers[i]])
			{
				localaiming[i] = plr->aiming;
				break;
			}
		}
	}
	else if (fastcmp(field,"drawangle"))
		plr->drawangle = luaL_checkangle(L, 3);
	else if (fastcmp(field,"pflags"))
		plr->pflags = luaL_checkinteger(L, 3);
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
	else if (fastcmp(field,"availabilities"))
		return NOSET;
	else if (fastcmp(field,"score"))
		plr->score = luaL_checkinteger(L, 3);
	// SRB2kart
	else if (fastcmp(field,"ktemp_nocontrol"))
		plr->ktemp_nocontrol = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_carry"))
		plr->ktemp_carry = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_dye"))
		plr->ktemp_dye = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_position"))
		plr->ktemp_position = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_oldposition"))
		plr->ktemp_oldposition = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_positiondelay"))
		plr->ktemp_positiondelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"distancetofinish"))
		return NOSET;
	else if (fastcmp(field,"airtime"))
		plr->airtime = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_flashing"))
		plr->ktemp_flashing = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_spinouttimer"))
		plr->ktemp_spinouttimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_instashield"))
		plr->ktemp_instashield = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_wipeoutslow"))
		plr->ktemp_wipeoutslow = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_justbumped"))
		plr->ktemp_justbumped = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tumbleBounces"))
		plr->tumbleBounces = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tumbleHeight"))
		plr->tumbleHeight = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_drift"))
		plr->ktemp_drift = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_driftcharge"))
		plr->ktemp_driftcharge = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_driftboost"))
		plr->ktemp_driftboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_aizdriftstraft"))
		plr->ktemp_aizdriftstrat = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aizdrifttilt"))
		plr->aizdrifttilt = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aizdriftturn"))
		plr->aizdriftturn = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_offroad"))
		plr->ktemp_offroad = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_waterskip"))
		plr->ktemp_waterskip = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_tiregrease"))
		plr->ktemp_tiregrease = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_springstars"))
		plr->ktemp_springstars = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_springcolor"))
		plr->ktemp_springcolor = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_dashpadcooldown"))
		plr->ktemp_dashpadcooldown = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_spindash"))
		plr->ktemp_spindash = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_spindashspeed"))
		plr->ktemp_spindashspeed = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_spindashboost"))
		plr->ktemp_spindashboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_numboosts"))
		plr->ktemp_numboosts = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_boostpower"))
		plr->ktemp_boostpower = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_speedboost"))
		plr->ktemp_speedboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_accelboost"))
		plr->ktemp_accelboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_handleboost"))
		plr->ktemp_handleboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_boostangle"))
		plr->ktemp_boostangle = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_draftpower"))
		plr->ktemp_draftpower = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_draftleeway"))
		plr->ktemp_draftleeway = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_lastdraft"))
		plr->ktemp_lastdraft = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_itemroulette"))
		plr->ktemp_itemroulette = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_roulettetype"))
		plr->ktemp_roulettetype = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_itemtype"))
		plr->ktemp_itemtype = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_itemamount"))
		plr->ktemp_itemamount = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_throwdir"))
		plr->ktemp_throwdir = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_sadtimer"))
		plr->ktemp_sadtimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rings"))
		plr->rings = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_pickuprings"))
		plr->ktemp_pickuprings = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_ringdelay"))
		plr->ktemp_ringdelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_ringboost"))
		plr->ktemp_ringboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_sparkleanim"))
		plr->ktemp_sparkleanim = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_superring"))
		plr->ktemp_superring = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_curshield"))
		plr->ktemp_curshield = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_bubblecool"))
		plr->ktemp_bubblecool = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_bubbleblowup"))
		plr->ktemp_bubbleblowup = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_flamedash"))
		plr->ktemp_flamedash = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_flamemeter"))
		plr->ktemp_flamemeter = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_flamelength"))
		plr->ktemp_flamelength = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_hyudorotimer"))
		plr->ktemp_hyudorotimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_stealingtimer"))
		plr->ktemp_stealingtimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_sneakertimer"))
		plr->ktemp_sneakertimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_numsneakers"))
		plr->ktemp_numsneakers = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_floorboost"))
		plr->ktemp_floorboost = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_growshrinktimer"))
		plr->ktemp_growshrinktimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_rocketsneakertimer"))
		plr->ktemp_rocketsneakertimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_invincibilitytimer"))
		plr->ktemp_invincibilitytimer = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_eggmanexplode"))
		plr->ktemp_eggmanexplode = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_eggmanblame"))
		plr->ktemp_eggmanblame = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_bananadrag"))
		plr->ktemp_bananadrag = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_lastjawztarget"))
		plr->ktemp_lastjawztarget = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ktemp_jawztargetdelay"))
		plr->ktemp_jawztargetdelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"glanceDir"))
		plr->glanceDir = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"trickpanel"))
		plr->trickpanel = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"trickmomx"))
		plr->trickmomx = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"trickmomy"))
		plr->trickmomy = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"trickmomz"))
		plr->trickmomz = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"roundscore"))
		lua_pushinteger(L, plr->roundscore);
	else if (fastcmp(field,"ktemp_emeralds"))
		plr->ktemp_emeralds = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bumpers"))
		plr->bumpers = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"karmadelay"))
		plr->karmadelay = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spheres"))
		plr->spheres = luaL_checkinteger(L, 3);
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
	else if (fastcmp(field,"follower"))	// it's probably best we don't allow the follower mobj to change.
		return NOSET;
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
	else if (fastcmp(field,"ctfteam"))
		plr->ctfteam = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"checkskip"))
		plr->checkskip = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"starpostnum"))
		plr->starpostnum = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastsidehit"))
		plr->lastsidehit = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastlinehit"))
		plr->lastlinehit = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"onconveyor"))
		plr->onconveyor = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"awayviewmobj"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->awayviewmobj, mo);
	}
	else if (fastcmp(field,"awayviewtics"))
	{
		plr->awayviewtics = (INT32)luaL_checkinteger(L, 3);
		if (plr->awayviewtics && !plr->awayviewmobj) // awayviewtics must ALWAYS have an awayviewmobj set!!
			P_SetTarget(&plr->awayviewmobj, plr->mo); // but since the script might set awayviewmobj immediately AFTER setting awayviewtics, use player mobj as filler for now.
	}
	else if (fastcmp(field,"awayviewaiming"))
		plr->awayviewaiming = luaL_checkangle(L, 3);
	else if (fastcmp(field,"spectator"))
		plr->spectator = lua_toboolean(L, 3);
	else if (fastcmp(field,"bot"))
		return NOSET;
	else if (fastcmp(field,"jointime"))
		plr->jointime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"splitscreenindex"))
		return NOSET;
#ifdef HWRENDER
	else if (fastcmp(field,"fovadd"))
		plr->fovadd = luaL_checkfixed(L, 3);
#endif
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
		return luaL_error(L, "Do not alter player_t in BuildCMD code!");
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

// Same shit for player.respawn variable... Why is everything in different sub-variables again now???
#define RNOFIELD luaL_error(L, LUA_QL("respawnvars_t") " has no field named " LUA_QS, field)
#define RUNIMPLEMENTED luaL_error(L, LUA_QL("respawnvars_t") " unimplemented field " LUA_QS " cannot be read or set.", field)
// @TODO: Waypoints in Lua possibly maybe? No don't count on me to do it...

static int respawn_get(lua_State *L)
{
	respawnvars_t *rsp = *((respawnvars_t **)luaL_checkudata(L, 1, META_RESPAWN));
	const char *field = luaL_checkstring(L, 2);
	if (!rsp)
		return LUA_ErrInvalid(L, "player_t");

	if (fastcmp(field,"state"))
		lua_pushinteger(L, rsp->state);
	else if (fastcmp(field,"waypoint"))
		return RUNIMPLEMENTED;
	else if (fastcmp(field,"pointx"))
		lua_pushfixed(L, rsp->pointx);
	else if (fastcmp(field,"pointy"))
		lua_pushfixed(L, rsp->pointy);
	else if (fastcmp(field,"pointz"))
		lua_pushfixed(L, rsp->pointz);
	else if (fastcmp(field,"flip"))
		lua_pushboolean(L, rsp->flip);
	else if (fastcmp(field,"timer"))
		lua_pushinteger(L, rsp->timer);
	else if (fastcmp(field,"distanceleft"))
		lua_pushinteger(L, rsp->distanceleft);	// Can't possibly foresee any problem when pushing UINT32 to Lua's INT32 hahahahaha, get ready for dumb hacky shit on high distances.
	else if (fastcmp(field,"dropdash"))
		lua_pushinteger(L, rsp->dropdash);
	else
		return RNOFIELD;

	return 1;
}

static int respawn_set(lua_State *L)
{
	respawnvars_t *rsp = *((respawnvars_t **)luaL_checkudata(L, 1, META_RESPAWN));
	const char *field = luaL_checkstring(L, 2);
	if (!rsp)
		return LUA_ErrInvalid(L, "respawnvars_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter player_t in CMD building code!");

	if (fastcmp(field,"state"))
		rsp->state = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"waypoint"))
		return RUNIMPLEMENTED;
	else if (fastcmp(field,"pointx"))
		rsp->pointx = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"pointy"))
		rsp->pointy = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"pointz"))
		rsp->pointz = luaL_checkfixed(L, 3);
	else if (fastcmp(field,"flip"))
		rsp->flip = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"timer"))
		rsp->timer = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"distanceleft"))
		rsp->distanceleft = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dropdash"))
		rsp->dropdash = (tic_t)luaL_checkinteger(L, 3);
	else
		return RNOFIELD;

	return 0;
}

#undef RNOFIELD
#undef RUNIMPLEMENTED


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

	luaL_newmetatable(L, META_RESPAWN);
		lua_pushcfunction(L, respawn_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, respawn_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	luaL_newmetatable(L, META_TICCMD);
		lua_pushcfunction(L, ticcmd_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, ticcmd_set);
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
