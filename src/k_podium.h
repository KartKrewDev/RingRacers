// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_podium.h
/// \brief Grand Prix podium cutscene

#ifndef __K_PODIUM__
#define __K_PODIUM__

#include "doomtype.h"
#include "d_event.h"
#include "p_mobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------
	boolean K_PodiumSequence(void);

		Returns whenver or not we are in the podium
		cutscene mode.

	Input Arguments:-
		N/A

	Return:-
		true if we're in GS_CEREMONY, otherwise false.
--------------------------------------------------*/

boolean K_PodiumSequence(void);


/*--------------------------------------------------
	boolean K_StartCeremony(void);

		Loads the podium map and changes the gamestate
		to the podium cutscene mode.

	Input Arguments:-
		N/A

	Return:-
		true if successful, otherwise false. Can fail
		if there is no podium map defined.
--------------------------------------------------*/

boolean K_StartCeremony(void);


/*--------------------------------------------------
	void K_CeremonyTicker(boolean run);

		Ticker function to be ran during the podium
		cutscene mode gamestate. Handles updating
		the camera.

	Input Arguments:-
		run - Set to true when we're running a
			new game frame.

	Return:-
		N/A
--------------------------------------------------*/

void K_CeremonyTicker(boolean run);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_PODIUM__
