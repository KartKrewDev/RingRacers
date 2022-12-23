// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2016 by James Haley, David Hill, et al. (Team Eternity)
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  interface.cpp
/// \brief Action Code Script: Interface for the rest of SRB2's game logic

extern "C" {
#include "interface.h"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../r_defs.h"
#include "../g_game.h"
#include "../i_system.h"
}

#include <cmath>
#include <memory>

#include <ACSVM/Code.hpp>
#include <ACSVM/CodeData.hpp>
#include <ACSVM/Environment.hpp>
#include <ACSVM/Error.hpp>
#include <ACSVM/Module.hpp>
#include <ACSVM/Scope.hpp>
#include <ACSVM/Script.hpp>
#include <ACSVM/Serial.hpp>
#include <ACSVM/Thread.hpp>
#include <Util/Floats.hpp>

#include "environment.hpp"
#include "thread.hpp"

#include "../cxxutil.hpp"

using namespace srb2::acs;

using std::size_t;

/*--------------------------------------------------
	void ACS_Init(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Init(void)
{
#if 0
	// Initialize ACS on engine start-up.
	ACSEnv = new Environment();
	I_AddExitFunc(ACS_Shutdown);
#endif
}

/*--------------------------------------------------
	void ACS_Shutdown(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Shutdown(void)
{
#if 0
	// Delete ACS environment.
	delete ACSEnv;
	ACSEnv = nullptr;
#endif
}

/*--------------------------------------------------
	void ACS_LoadLevelScripts(size_t mapID)

		See header file for description.
--------------------------------------------------*/
void ACS_LoadLevelScripts(size_t mapID)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *hub = NULL;
	ACSVM::MapScope *map = NULL;

	std::vector<ACSVM::Module *> modules;

	// Just some notes on how Hexen's scopes work, if anyone
	// intends to implement proper hub logic:

	// The integer is an ID for which hub / map it is,
	// and instead sets active according to which ones
	// should run, since you can go between them.

	// But I didn't intend on implementing these features,
	// since hubs aren't planned for Ring Racers (although
	// they might be useful for SRB2), and I intentionally
	// avoided implementing global ACS (since Lua would be
	// a better language to do that kind of code).

	// Since we literally only are using map scope, we can
	// just free everything between every level. But if
	// hubs are to be implemented, this logic would need
	// to be far more sophisticated.

	// Reset hub scope, even if we are not using it.
	hub = global->getHubScope(0);
	hub->reset();
	hub->active = true;

	// Start up new map scope.
	map = hub->getMapScope(0); // This is where you'd put in mapID if you add hub support.
	map->reset();
	map->active = true;

	// Insert BEHAVIOR lump into the list.
	{
		ACSVM::ModuleName name = ACSVM::ModuleName(
			env->getString( mapheaderinfo[mapID]->lumpname ),
			nullptr,
			mapheaderinfo[mapID]->lumpnum
		);

		modules.push_back(env->getModule(name));
	}

	if (modules.empty() == false)
	{
		// Register the modules with map scope.
		map->addModules(modules.data(), modules.size());
	}
}

/*--------------------------------------------------
	void ACS_RunPlayerEnterScript(player_t *player)

		See header file for description.
--------------------------------------------------*/
void ACS_RunPlayerEnterScript(player_t *player)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	ACSVM::MapScope::ScriptStartInfo scriptInfo;
	ThreadInfo info;

	P_SetTarget(&info.mo, player->mo);

	scriptInfo.info = &info;

	map->scriptStartTypeForced(ACS_ST_ENTER, scriptInfo);
}

/*--------------------------------------------------
	void ACS_RunLevelStartScripts(void)

		See header file for description.
--------------------------------------------------*/
void ACS_RunLevelStartScripts(void)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	map->scriptStartType(ACS_ST_OPEN, {});

	for (int i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];
		if (player->spectator == true)
		{
			continue;
		}

		ACS_RunPlayerEnterScript(player);
	}
}

/*--------------------------------------------------
	void ACS_RunLapScript(mobj_t *mo, line_t *line)

		See header file for description.
--------------------------------------------------*/
void ACS_RunLapScript(mobj_t *mo, line_t *line)
{
	Environment *env = &ACSEnv;

	ACSVM::GlobalScope *const global = env->getGlobalScope(0);
	ACSVM::HubScope *const hub = global->getHubScope(0);
	ACSVM::MapScope *const map = hub->getMapScope(0);

	ACSVM::MapScope::ScriptStartInfo scriptInfo;
	ThreadInfo info;

	P_SetTarget(&info.mo, mo);
	info.line = line;

	scriptInfo.info = &info;

	map->scriptStartTypeForced(ACS_ST_LAP, scriptInfo);
}

/*--------------------------------------------------
	void ACS_Tick(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Tick(void)
{
	Environment *env = &ACSEnv;

	if (env->hasActiveThread() == true)
	{
		env->exec();
	}
}
