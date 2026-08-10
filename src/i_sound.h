// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_sound.h
/// \brief System interface, sound, music

#ifndef I_SOUND_H
#define I_SOUND_H

#include "doomdef.h"
#include "sounds.h"
#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

/**	\brief Sound subsystem runing and waiting
*/
extern uint8_t sound_started;

/**	\brief info of samplerate
*/
extern consvar_t cv_samplerate;
//extern consvar_t cv_rndsoundpitch;

/**	\brief	The I_GetSfx function

	\param	sfx	sfx to setup

	\return	data for sfx
*/
void *I_GetSfx(sfxinfo_t *sfx);

/**	\brief	The I_FreeSfx function

	\param	sfx	sfx to be freed up

	\return	void
*/
void I_FreeSfx(sfxinfo_t *sfx);

/**	\brief	Returns total bytes allocated for loaded sound data on the heap.
*/
size_t I_GetSoundMemUsage(void);

/**	\brief Init at program start...
*/
void I_StartupSound(void);

/**	\brief ... shut down and relase at program termination.
*/
void I_ShutdownSound(void);

/** \brief Update instance of AVRecorder for audio capture.
*/
void I_UpdateAudioRecorder(void);

/// ------------------------
///  SFX I/O
/// ------------------------

/**	\brief	Starts a sound in a particular sound channel.
	\param	id	sfxid
	\param	vol	volume for sound
	\param	sep	left-right balancle
	\param	pitch	not used
	\param	priority	not used

	\return	sfx handle
*/
int32_t I_StartSound(sfxenum_t id, uint8_t vol, uint8_t sep, uint8_t pitch, uint8_t priority, int32_t channel);

/**	\brief	Stops a sound channel.

	\param	handle	stop sfx handle

	\return	void
*/
void I_StopSound(int32_t handle);

/**	\brief Some digital sound drivers need this.
*/
void I_UpdateSound(void);

/**	\brief	Called by S_*() functions to see if a channel is still playing.

	\param	handle	sfx handle

	\return	0 if no longer playing, 1 if playing.
*/
dboolean I_SoundIsPlaying(int32_t handle);

/**	\brief	Updates the sfx handle

	\param	handle	sfx handle
	\param	vol	volume
	\param	sep	separation
	\param	pitch	ptich

	\return	void
*/
void I_UpdateSoundParams(int32_t handle, uint8_t vol, uint8_t sep, uint8_t pitch);

/**	\brief	The I_SetSfxVolume function

	\param	volume	volume to set at

	\return	void
*/
void I_SetSfxVolume(int volume);

void I_SetVoiceVolume(int volume);

/// ------------------------
//  MUSIC SYSTEM
/// ------------------------

/** \brief Init the music systems
*/
void I_InitMusic(void);

/** \brief Shutdown the music systems
*/
void I_ShutdownMusic(void);

/// ------------------------
//  MUSIC PROPERTIES
/// ------------------------

const char *I_SongType(void);
dboolean I_SongPlaying(void);
dboolean I_SongPaused(void);

/// ------------------------
//  MUSIC EFFECTS
/// ------------------------

dboolean I_SetSongSpeed(float speed);

/// ------------------------
//  MUSIC SEEKING
/// ------------------------

uint32_t I_GetSongLength(void);

dboolean I_SetSongLoopPoint(uint32_t looppoint);
uint32_t I_GetSongLoopPoint(void);

dboolean I_SetSongPosition(uint32_t position);
uint32_t I_GetSongPosition(void);

void I_UpdateSongLagThreshold (void);
void I_UpdateSongLagConditions (void);

/// ------------------------
//  MUSIC PLAYBACK
/// ------------------------

/**	\brief	Registers a song handle to song data.

	\param	data	pointer to song data
	\param	len	len of data

	\return	song handle

	\todo Remove this
*/
dboolean I_LoadSong(char *data, size_t len);

/**	\brief	See ::I_LoadSong, then think backwards

	\param	handle	song handle

	\sa I_LoadSong
	\todo remove midi handle
*/
void I_UnloadSong(void);

/**	\brief	Called by anything that wishes to start music

	\param	handle	Song handle
	\param	looping	looping it if true

	\return	if true, it's playing the song

	\todo pass music name, not handle
*/
dboolean I_PlaySong(dboolean looping);

/**	\brief	Stops a song over 3 seconds

	\param	handle	Song handle
	\return	void

	/todo drop handle
*/
void I_StopSong(void);

/**	\brief	PAUSE game handling.

	\param	handle	song handle

	\return	void
*/
void I_PauseSong(void);

/**	\brief	RESUME game handling

	\param	handle	song handle

	\return	void
*/
void I_ResumeSong(void);

/**	\brief	Sets the volume of the Music mixing channel. Distinguished from the song's individual volume. The scale of
            the volume is determined by the interface implementation.

	\param	volume	volume to set at

	\return	void
*/
void I_SetMusicVolume(int volume);

/** \brief Sets the current song's volume, independent of the overall music channel volume. The volume scale is 0-100,
 * as a linear gain multiplier. This is distinguished from SetMusicVolume which may or may not be linear.
*/
void I_SetCurrentSongVolume(int volume);

// TODO refactor fades to control Song Volume exclusively in tandem with RR musicdef volume multiplier.

dboolean I_SetSongTrack(int32_t track);

void I_SetMasterVolume(int volume);

/// ------------------------
/// MUSIC FADING
/// ------------------------

void I_SetInternalMusicVolume(uint8_t volume);
void I_StopFadingSong(void);
dboolean I_FadeSongFromVolume(uint8_t target_volume, uint8_t source_volume, uint32_t ms, void (*callback)(void));
dboolean I_FadeSong(uint8_t target_volume, uint32_t ms, void (*callback)(void));
dboolean I_FadeOutStopSong(uint32_t ms);
dboolean I_FadeInPlaySong(uint32_t ms, dboolean looping);

// AUDIO INPUT (Microphones)
dboolean I_SoundInputIsEnabled(void);
dboolean I_SoundInputSetEnabled(dboolean enabled);
uint32_t I_SoundInputDequeueSamples(void *data, uint32_t len);
uint32_t I_SoundInputRemainingSamples(void);

// VOICE CHAT

/// Queue a frame of samples of voice data from a player. Voice format is MONO F32 SYSTEM ENDIANNESS.
/// If there is too much data being queued, old samples will be truncated
void I_QueueVoiceFrameFromPlayer(int32_t playernum, void *data, uint32_t len, dboolean terminal);

void I_SetPlayerVoiceProperties(int32_t playernum, float volume, float sep);

/// Reset the voice queue for the given player. Use when server connection ends
void I_ResetVoiceQueue(int32_t playernum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
