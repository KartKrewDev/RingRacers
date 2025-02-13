// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../command.h"
#include "../s_sound.h"
#include "../i_sound.h"

//
// CD MUSIC I/O
//

UINT8 cdaudio_started = 0;

consvar_t cd_volume = CVAR_INIT ("cd_volume","18",CV_SAVE,soundvolume_cons_t, NULL);
consvar_t cdUpdate  = CVAR_INIT ("cd_update","1",CV_SAVE, NULL, NULL);


void I_InitCD(void){}

void I_StopCD(void){}

void I_PauseCD(void){}

void I_ResumeCD(void){}

void I_ShutdownCD(void){}

void I_UpdateCD(void){}

void I_PlayCD(UINT8 track, UINT8 looping)
{
        (void)track;
        (void)looping;
}

boolean I_SetVolumeCD(INT32 volume)
{
        (void)volume;
        return false;
}
