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
/// \file  call-funcs.hpp
/// \brief Action Code Script: CallFunc instructions

#ifndef __SRB2_ACS_CALL_FUNCS_HPP__
#define __SRB2_ACS_CALL_FUNCS_HPP__

#include "acsvm.hpp"

/*--------------------------------------------------
	bool CallFunc_???(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

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

bool CallFunc_Random(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ThingCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TagWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PolyWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CameraWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DialogueWaitDismiss(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DialogueWaitText(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ChangeFloor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ChangeCeiling(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_LineSide(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ClearLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_EndPrint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GameType(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GameSpeed(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Timer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_IsNetworkGame(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SectorSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_AmbientSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineBlocking(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ThingSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_EndPrintBold(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerTeam(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerNumber(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ActivatorTID(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_EndLog(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_strcmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_strcasecmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_CountEnemies(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CountPushables(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_HaveUnlockableTrigger(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_HaveUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerSkin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerSkinRealName(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerLosing(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerExiting(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_LowestLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_EncoreMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PrisonBreak(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TimeAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_FreePlay(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GrandPrix(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PositionStart(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetGrabbedSprayCan(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CheckTutorialChallenge(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_PodiumPosition(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PodiumFinish(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_SetLineRenderStyle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_MapWarp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_AddBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_StopLevelExit(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ExitLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicPlay(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicStopAll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicRemap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicDim(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_DialogueSetSpeaker(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DialogueSetCustomSpeaker(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DialogueNewText(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DialogueAutoDismiss(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_Freeze(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_GetLineProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetSideProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetSideProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_GetLineUserProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetSideUserProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetSectorUserProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetThingUserProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_AddMessage(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_AddMessageForPlayer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ClearPersistentMessages(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ClearPersistentMessageForPlayer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

#endif // __SRB2_ACS_CALL_FUNCS_HPP__
