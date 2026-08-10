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

uint8_t cdaudio_started = 0;

consvar_t cd_volume = CVAR_INIT ("cd_volume","18",CV_SAVE,soundvolume_cons_t, NULL);
consvar_t cdUpdate  = CVAR_INIT ("cd_update","1",CV_SAVE, NULL, NULL);


void I_InitCD(void){}

void I_StopCD(void){}

void I_PauseCD(void){}

void I_ResumeCD(void){}

void I_ShutdownCD(void){}

void I_UpdateCD(void){}

void I_PlayCD(uint8_t track, uint8_t looping)
{
        (void)track;
        (void)looping;
}

dboolean I_SetVolumeCD(int32_t volume)
{
        (void)volume;
        return false;
}
