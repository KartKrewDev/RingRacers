// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2016 by James Haley, David Hill, et al. (Team Eternity)
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  thread.hpp
/// \brief Action Code Script: Thread definition

#ifndef __SRB2_ACS_THREAD_HPP__
#define __SRB2_ACS_THREAD_HPP__

#include "acsvm.hpp"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../p_tick.h"
#include "../r_defs.h"
#include "../r_state.h"
#include "../p_spec.h"

namespace srb2::acs {

//
// Special global script types.
//
enum acs_scriptType_e
{
	ACS_ST_OPEN			=  1, // OPEN: Runs once when the level starts.
	ACS_ST_RESPAWN		=  2, // RESPAWN: Runs when a player respawns.
	ACS_ST_DEATH		=  3, // DEATH: Runs when a player dies.
	ACS_ST_ENTER		=  4, // ENTER: Runs when a player enters the game; both on start of the level, and when un-spectating.
	ACS_ST_LAP			=  5, // LAP: Runs when a player's lap increases from crossing the finish line.
	ACS_ST_POSITION		=  6, // POSITION: Runs when the POSITION period ends.
	ACS_ST_OVERTIME		=  7, // OVERTIME: Runs when Overtime starts in timed game modes.
	ACS_ST_UFO			=  8, // UFO: Runs when the UFO Catcher is destroyed in a Special Stage.
	ACS_ST_EMERALD		=  9, // EMERALD: Runs when the Chaos Emerald is collected in a Special Stage.
	ACS_ST_GAMEOVER		= 10, // GAMEOVER: Runs when the level ends due to a losing condition and no player has an extra life.
	ACS_ST_FINISH		= 11, // FINISH: Runs when a player finishes
};

//
// Script "waiting on tag" types.
//
enum acs_tagType_e
{
	ACS_TAGTYPE_POLYOBJ,
	ACS_TAGTYPE_SECTOR,
	ACS_TAGTYPE_CAMERA,
	ACS_TAGTYPE_DIALOGUE,
};

class ThreadInfo : public ACSVM::ThreadInfo
{
public:
	UINT32 thread_era;			// If equal to thinker_era, mobj pointers are safe.
	mobj_t *mo;					// Object that activated this thread.
	line_t *line;				// Linedef that activated this thread.
	UINT8 side;					// Front / back side of said linedef.
	sector_t *sector;			// Sector that activated this thread.
	polyobj_t *po;				// Polyobject that activated this thread.
	bool fromLineSpecial;		// Called from P_ProcessLineSpecial.
	UINT32 dialogue_era;		// Prevents overlapping dialogue scripts.

	ThreadInfo() :
		thread_era { thinker_era },
		mo{ nullptr },
		line{ nullptr },
		side{ 0 },
		sector{ nullptr },
		po{ nullptr },
		fromLineSpecial{ false },
		dialogue_era { 0 }
	{
	}

	ThreadInfo(const ThreadInfo &info) :
		thread_era { thinker_era },
		mo{ nullptr },
		line{ info.line },
		side{ info.side },
		sector{ info.sector },
		po{ info.po },
		fromLineSpecial{ info.fromLineSpecial },
		dialogue_era { info.dialogue_era }
	{
		P_SetTarget(&mo, info.mo);
	}

	ThreadInfo(const activator_t *activator) :
		thread_era { thinker_era },
		mo{ nullptr },
		line{ activator->line },
		side{ activator->side },
		sector{ activator->sector },
		po{ activator->po },
		fromLineSpecial{ static_cast<bool>(activator->fromLineSpecial) },
		dialogue_era { 0 }
	{
		P_SetTarget(&mo, activator->mo);
	}

	~ThreadInfo()
	{
		if (thread_era == thinker_era)
		{
			P_SetTarget(&mo, nullptr);
		}
	}

	ThreadInfo &operator = (const ThreadInfo &info)
	{
		thread_era = thinker_era;
		P_SetTarget(&mo, info.mo);
		line = info.line;
		side = info.side;
		sector = info.sector;
		po = info.po;
		dialogue_era = info.dialogue_era;

		return *this;
	}
};

class Thread : public ACSVM::Thread
{
public:
	ThreadInfo info;

	explicit Thread(ACSVM::Environment *env_) : ACSVM::Thread{env_} {}

	virtual ACSVM::ThreadInfo const *getInfo() const { return &info; }

	virtual void start(
		ACSVM::Script *script, ACSVM::MapScope *map,
		const ACSVM::ThreadInfo *info, const ACSVM::Word *argV, ACSVM::Word argC
	);

	virtual void stop();

	virtual void loadState(ACSVM::Serial &serial);

	virtual void saveState(ACSVM::Serial &serial) const;
};

}

#endif // __SRB2_ACS_THREAD_HPP__
