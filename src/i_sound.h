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

#ifndef __I_SOUND__
#define __I_SOUND__

#include "doomdef.h"
#include "sounds.h"
#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

/**	\brief Sound subsystem runing and waiting
*/
extern UINT8 sound_started;

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
INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority, INT32 channel);

/**	\brief	Stops a sound channel.

	\param	handle	stop sfx handle

	\return	void
*/
void I_StopSound(INT32 handle);

/**	\brief Some digital sound drivers need this.
*/
void I_UpdateSound(void);

/**	\brief	Called by S_*() functions to see if a channel is still playing.

	\param	handle	sfx handle

	\return	0 if no longer playing, 1 if playing.
*/
boolean I_SoundIsPlaying(INT32 handle);

/**	\brief	Updates the sfx handle

	\param	handle	sfx handle
	\param	vol	volume
	\param	sep	separation
	\param	pitch	ptich

	\return	void
*/
void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch);

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
boolean I_SongPlaying(void);
boolean I_SongPaused(void);

/// ------------------------
//  MUSIC EFFECTS
/// ------------------------

boolean I_SetSongSpeed(float speed);

/// ------------------------
//  MUSIC SEEKING
/// ------------------------

UINT32 I_GetSongLength(void);

boolean I_SetSongLoopPoint(UINT32 looppoint);
UINT32 I_GetSongLoopPoint(void);

boolean I_SetSongPosition(UINT32 position);
UINT32 I_GetSongPosition(void);

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
boolean I_LoadSong(char *data, size_t len);

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
boolean I_PlaySong(boolean looping);

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

boolean I_SetSongTrack(INT32 track);

void I_SetMasterVolume(int volume);

/// ------------------------
/// MUSIC FADING
/// ------------------------

void I_SetInternalMusicVolume(UINT8 volume);
void I_StopFadingSong(void);
boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, void (*callback)(void));
boolean I_FadeSong(UINT8 target_volume, UINT32 ms, void (*callback)(void));
boolean I_FadeOutStopSong(UINT32 ms);
boolean I_FadeInPlaySong(UINT32 ms, boolean looping);

// AUDIO INPUT (Microphones)
boolean I_SoundInputIsEnabled(void);
boolean I_SoundInputSetEnabled(boolean enabled);
UINT32 I_SoundInputDequeueSamples(void *data, UINT32 len);
UINT32 I_SoundInputRemainingSamples(void);

// VOICE CHAT

/// Queue a frame of samples of voice data from a player. Voice format is MONO F32 SYSTEM ENDIANNESS.
/// If there is too much data being queued, old samples will be truncated
void I_QueueVoiceFrameFromPlayer(INT32 playernum, void *data, UINT32 len, boolean terminal);

void I_SetPlayerVoiceProperties(INT32 playernum, float volume, float sep);

/// Reset the voice queue for the given player. Use when server connection ends
void I_ResetVoiceQueue(INT32 playernum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
