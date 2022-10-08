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
/// \file  k_acs.h
/// \brief Action Code Script implementation using ACSVM

#ifndef __K_ACS__
#define __K_ACS__

#include "doomtype.h"
#include "doomdef.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "p_polyobj.h"
#include "d_player.h"

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

//
// Special global script types.
//
typedef enum
{
	ACS_ST_OPEN			=  1, // OPEN: Runs once when the level starts.
	ACS_ST_RESPAWN		=  2, // RESPAWN: Runs when a player respawns.
	ACS_ST_DEATH		=  3, // DEATH: Runs when a player dies.
	ACS_ST_ENTER		=  4, // ENTER: Runs when a player enters the game; both on start of the level, and when un-spectating.
} acs_scriptType_e;

//
// Script "waiting on tag" types.
//
typedef enum
{
	ACS_TAGTYPE_POLYOBJ,
	ACS_TAGTYPE_SECTOR,
} acs_tagType_e;

//
// Thread activator info
//
typedef struct
{
	mobj_t *mo;				// Object that activated this thread.
	line_t *line;			// Linedef that activated this thread.
	UINT8 side;				// Front / back side of said linedef.
	sector_t *sector;		// Sector that activated this thread.
	polyobj_t *po;			// Polyobject that activated this thread.
} acs_threadinfo_t;

/*--------------------------------------------------
	ACSVM_Environment *ACS_GetEnvironment(void);

		Returns the global ACS environment. This contains
		all of the information about the ACS VM state.

	Input Arguments:-
		None

	Return:-
		The ACS environment object.
--------------------------------------------------*/

ACSVM_Environment *ACS_GetEnvironment(void);


/*--------------------------------------------------
	void ACS_Init(void);

		Initializes the ACS environment. Handles creating
		the VM, initializing its hooks, storing the
		pointer for future reference, and adding the
		shutdown function.
--------------------------------------------------*/

void ACS_Init(void);


/*--------------------------------------------------
	void ACS_Shutdown(void);

		Frees the ACS environment, for when the game
		is exited.
--------------------------------------------------*/

void ACS_Shutdown(void);


/*--------------------------------------------------
	void ACS_LoadLevelScripts(size_t mapID);

		Resets the ACS hub and map scopes to remove
		existing running scripts, and inserts the new
		level's ACS modules (BEHAVIOR lump) into
		the environment.

	Input Arguments:-
		mapID: The map's number to read the BEHAVIOR
			lump of.

	Return:-
		None
--------------------------------------------------*/

void ACS_LoadLevelScripts(size_t mapID);


/*--------------------------------------------------
	void ACS_RunPlayerEnterScript(player_t *player);

		Runs the map's special script for a player
		entering the game.

	Input Arguments:-
		player: The player to run the script for.

	Return:-
		None
--------------------------------------------------*/

void ACS_RunPlayerEnterScript(player_t *player);


/*--------------------------------------------------
	void ACS_RunLevelStartScripts(void);

		Runs the map's special scripts for opening
		the level, and for all players to enter
		the game.
--------------------------------------------------*/

void ACS_RunLevelStartScripts(void);


/*--------------------------------------------------
	void ACS_Tick(void);

		Executes all of the ACS environment's
		currently active threads.
--------------------------------------------------*/

void ACS_Tick(void);


/*--------------------------------------------------
	bool ACS_CF_???(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);

		These are the actual CallFuncs ran when ACS
		is executed. Which CallFuncs are executed
		is based on the indices from the compiled
		data. ACS_EnvConstruct is where the link
		between the byte code and the actual function
		is made.

	Input Arguments:-
		thread: The ACS execution thread this action
			is running on.
		argV: An array of the action's arguments.
		argC: The length of the argument array.

	Return:-
		Returns true if this function pauses the
		thread's execution. Otherwise returns false.
--------------------------------------------------*/

bool ACS_CF_Random(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_ThingCount(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_TagWait(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_PolyWait(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_ChangeFloor(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_ChangeCeiling(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_LineSide(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_ClearLineSpecial(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_PlayerCount(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_GameType(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_GameSpeed(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_SectorSound(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_AmbientSound(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_SetLineTexture(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_SetLineSpecial(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_ThingSound(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_EndPrintBold(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_EndLog(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);


#endif // __K_ACS__
