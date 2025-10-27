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
/// \file  s_sound.h
/// \brief The not so system specific sound interface

#ifndef __S_SOUND__
#define __S_SOUND__

#include "sounds.h"
#include "m_fixed.h"
#include "command.h"
#include "tables.h" // angle_t

#ifdef __cplusplus
extern "C" {
#endif

// mask used to indicate sound origin is player item pickup
#define PICKUP_SOUND 0x8000

//
#define SOUND_VOLUME_RANGE 100
#define MAX_SOUND_VOLUME 100

#define DEFAULT_MUSICDEF_VOLUME 100

extern consvar_t stereoreverse;
extern consvar_t cv_soundvolume, cv_closedcaptioning, cv_digmusicvolume;
extern consvar_t cv_voicevolume;

extern consvar_t surround;
extern consvar_t cv_numChannels;
extern CV_PossibleValue_t soundmixingbuffersize_cons_t[];
extern consvar_t cv_soundmixingbuffersize;

extern consvar_t cv_gamedigimusic;

extern consvar_t cv_gamesounds;
extern consvar_t cv_bgaudio;
extern consvar_t cv_streamersafemusic;

extern consvar_t cv_voice_selfdeafen;
extern consvar_t cv_voice_mode;
extern consvar_t cv_voice_selfmute;
extern consvar_t cv_voice_loopback;
extern consvar_t cv_voice_denoise;
extern consvar_t cv_voice_inputamp;
extern consvar_t cv_voice_activationthreshold;
extern consvar_t cv_voice_proximity;
extern consvar_t cv_voice_distanceattenuation_distance;
extern consvar_t cv_voice_distanceattenuation_teamdistance;
extern consvar_t cv_voice_distanceattenuation_factor;
extern consvar_t cv_voice_stereopanning_factor;
extern consvar_t cv_voice_concurrentattenuation_factor;
extern consvar_t cv_voice_concurrentattenuation_min;
extern consvar_t cv_voice_concurrentattenuation_max;
extern float g_local_voice_last_peak;
extern boolean g_local_voice_detected;

typedef enum
{
	SF_TOTALLYSINGLE =  1, // Only play one of these sounds at a time...GLOBALLY
	SF_NOMULTIPLESOUND =  2, // Like SF_NOINTERRUPT, but doesnt care what the origin is
	SF_OUTSIDESOUND  =  4, // Volume is adjusted depending on how far away you are from 'outside'
	SF_X4AWAYSOUND   =  8, // Hear it from 4x the distance away
	SF_X8AWAYSOUND   = 16, // Hear it from 8x the distance away
	SF_NOINTERRUPT   = 32, // Only play this sound if it isn't already playing on the origin
	SF_X2AWAYSOUND   = 64, // Hear it from 2x the distance away
} soundflags_t;

struct listener_t {
	fixed_t x, y, z;
	angle_t angle;
};

struct channel_t
{
	// sound information (if null, channel avail.)
	sfxinfo_t *sfxinfo;

	// origin of sound
	const void *origin;

	// initial volume of sound, which is applied after distance and direction
	INT32 volume;

	// handle of the sound being played
	INT32 handle;

};

struct caption_t {
	channel_t *c;
	sfxinfo_t *s;
	UINT16 t;
	UINT8 b;
};

#define NUMCAPTIONS 8
#define MAXCAPTIONTICS (2*TICRATE)
#define CAPTIONFADETICS 20

extern caption_t closedcaptions[NUMCAPTIONS];
void S_StartCaption(sfxenum_t sfx_id, INT32 cnum, UINT16 lifespan);
void S_ResetCaptions(void);

// register sound vars and commands at game startup
void S_RegisterSoundStuff(void);

//
// Initializes sound stuff, including volume
// Sets channels, SFX, allocates channel buffer, sets S_sfx lookup.
//
void S_InitSfxChannels(void);

//
// Per level startup code.
// Kills playing sounds at start of level, determines music if any, changes music.
//
void S_StopSounds(void);
void S_ClearSfx(void);
void S_InitLevelMusic(boolean reset);

//
// Basically a W_GetNumForName that adds "ds" at the beginning of the string. Returns a lumpnum.
//
lumpnum_t S_GetSfxLumpNum(sfxinfo_t *sfx);

//
// Sound Status
//

boolean S_SoundDisabled(void);

boolean S_VoiceDisabled(void);

//
// Start sound for thing at <origin> using <sound_id> from sounds.h
//
void S_StartSound(const void *origin, sfxenum_t sound_id);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(const void *origin, sfxenum_t sound_id, INT32 volume);

// Will start a sound, but only if VFX reduce is off or the owner isn't a display player.
void S_ReducedVFXSoundAtVolume(const void *origin, sfxenum_t sfx_id, INT32 volume, const player_t *owner);
#define S_ReducedVFXSound(a, b, c) S_ReducedVFXSoundAtVolume(a, b, 255, c)

// Stop sound for thing at <origin>
void S_StopSound(void *origin);

//
// Music Status
//

boolean S_DigMusicDisabled(void);
boolean S_MusicDisabled(void);
boolean S_MusicNotInFocus(void);


#define MAXDEFTRACKS 3
#define ALTREF_REQUIRESBEATEN UINT8_MAX

struct soundtestsequence_t
{
	UINT8 id;
	UINT16 map;
	UINT8 altref;
	musicdef_t *next;

	size_t shuffleinfo;
	musicdef_t *shufflenext;
};

// Music credits
struct musicdef_t
{
	char name[MAXDEFTRACKS][7];
	UINT32 hash[MAXDEFTRACKS];
	boolean basenoloop[MAXDEFTRACKS];
	UINT8 numtracks;
	char *title;
	char *author;
	char *source;
	char *composers;
	int volume;
	int debug_volume;
	boolean important;
	boolean contentidunsafe;
	musicdef_t *next;
	soundtestsequence_t sequence;
};

// For HUD, doesn't always appear
extern struct cursongcredit
{
	char *text;
	UINT16 anim;
	UINT8 trans;
	fixed_t x;
	fixed_t old_x;
	boolean use_credits_offset;
} cursongcredit;

// For menu, always appears
extern char *g_realsongcredit;

extern struct soundtest
{
	UINT8 tune;							// Tune used for music system

	boolean playing; 					// Music is playing?
	boolean justopened;					// Menu visual assist

	INT32 menutick;						// Menu visual timer

	musicdef_t *current;				// Current selected music definition
	SINT8 currenttrack;					// Current selected music track for definition

	soundtestsequence_t sequence;		// Sequence head

	boolean autosequence;				// In auto sequence mode?
	boolean shuffle;					// In shuffle mode;
} soundtest;

void S_PopulateSoundTestSequence(void);
void S_UpdateSoundTestDef(boolean reverse, boolean dotracks, boolean skipnull);
void S_SoundTestPlay(void);
void S_SoundTestStop(void);
void S_SoundTestTogglePause(void);
void S_TickSoundTest(void);
const char *S_SoundTestTune(UINT8 invert);
boolean S_SoundTestCanSequenceFade(void);

extern musicdef_t *musicdefstart;

void S_LoadMusicDefs(UINT16 wadnum);
void S_InitMusicDefs(void);
musicdef_t *S_FindMusicDef(const char *name, UINT8 *i);
void S_LoadMusicCredit(void);
void S_UnloadMusicCredit(void);
void S_ShowMusicCredit(void);
void S_StopMusicCredit(void);

//
// Music Playback
//

// Stop and resume music, during game PAUSE.
void S_PauseAudio(void);
void S_ResumeAudio(void);

// Enable and disable sound effects
void S_EnableSound(void);
void S_DisableSound(void);

// Attempt to restore music based on gamestate.
void S_AttemptToRestoreMusic(void);

//
// Updates music & sounds
//
void S_UpdateSounds(void);
void S_UpdateClosedCaptions(void);
void S_UpdateVoicePositionalProperties(void);

FUNCMATH fixed_t S_CalculateSoundDistance(fixed_t px1, fixed_t py1, fixed_t pz1, fixed_t px2, fixed_t py2, fixed_t pz2);

INT32 S_GetSoundVolume(sfxinfo_t *sfx, INT32 volume);

void S_SetSfxVolume(void);
void S_SetMusicVolume(void);
void S_SetMasterVolume(void);
void S_SetVoiceVolume(void);

INT32 S_OriginPlaying(void *origin);
INT32 S_IdPlaying(sfxenum_t id);
INT32 S_SoundPlaying(const void *origin, sfxenum_t id);

void S_StartSoundName(void *mo, const  char *soundname);

void S_StopSoundByID(void *origin, sfxenum_t sfx_id);
void S_StopSoundByNum(sfxenum_t sfxnum);

#define S_StartAttackSound S_StartSound
#define S_StartScreamSound S_StartSound

boolean S_SoundInputIsEnabled(void);
boolean S_SoundInputSetEnabled(boolean enabled);
UINT32 S_SoundInputDequeueSamples(void *data, UINT32 len);
UINT32 S_SoundInputRemainingSamples(void);

void S_QueueVoiceFrameFromPlayer(INT32 playernum, void *data, UINT32 len, boolean terminal);
void S_SetPlayerVoiceActive(INT32 playernum);
boolean S_IsPlayerVoiceActive(INT32 playernum);
void S_ResetVoiceQueue(INT32 playernum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
