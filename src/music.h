// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

//
// A brief description of "tunes".
//
// Tunes are how the game identifies music. A tune may be
// remapped to any music lump. For example, the "level" tune
// represents the level music, but the actual song that plays
// is different between levels.
//
// Tunes store info, such as for how long they will play (in
// the case of "invinc" and "grow"), or whether they fade in
// or out and for how long they fade.
//
// Tunes are given a priority. Only the highest priority tune
// is heard, even if the others haven't ended. Tunes with the
// same priority are sorted by picking the last one that begun
// playing.
//

#ifndef MUSIC_H
#define MUSIC_H

#include "doomtype.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Tuneflags, for Music_AddTune
//

// inclusive fade out
#define TN_INCLUSIVEFADE  0x01

// should the tune use the level volume?
#define TN_USEMAPVOLUME   0x02

// sync tune to game logic
#define TN_SYNCMUSIC      0x04

// show tune credit (only on first play)
#define TN_MUSICCRED      0x08

// allow the game to slow down the music in encore mode
#define TN_VAPES          0x10
#define TN_NIGHTCOREABLE  0x20
#define TN_CHANGEPITCH    (TN_VAPES | TN_NIGHTCOREABLE)

// looping?
#define TN_LOOPING        0x40

//
// Get the currently playing tune.
//


// Returns the song name for the currently playing tune.
// Returns empty string if no tune is playing.
const char *Music_CurrentSong(void);

// Returns the id of the currently playing tune. Returns empty
// string if no tune is playing.
const char *Music_CurrentId(void);


//
// Actions that take effect immediately.
//

// Add a new tune to the tunes list.
void Music_AddTune(const char* id, int priority, int tuneflags);

// Begin playing a tune, duration is infinite. If the tune was
// already playing, this resets its current position (seeks
// back to the start.)
void Music_Play(const char *id);

// Set fade out duration. Mostly to fix a last minute bug
// with Stereo Mode.
void Music_SetFadeOut(const char* id, int fade_out);

// Set fade in duration. Done for parity with the BLUA music
// functions.
void Music_SetFadeIn(const char* id, int fade_in, boolean resume);

// Postpone the end of this tune until N tics from now. The
// tune should already be playing before calling this.
void Music_DelayEnd(const char *id, tic_t duration);

// Stop playing a tune.
void Music_Stop(const char *id);
void Music_StopAll(void);

// Pause a tune. This effectively extends its duration,
// relative to game time. Unpausing will resume the tune
// exactly where it left off.
void Music_Pause(const char *id);
void Music_UnPause(const char *id);
void Music_PauseAll(void);
void Music_UnPauseAll(void);

// Suspend a tune. The manager will switch to a tune that is
// not suspended. Upon unsuspending, the tune resumes from
// the position it would have reached normally (so the
// duration is not extended like with pausing).
void Music_Suspend(const char *id);
void Music_UnSuspend(const char *id);


//
// Change properties. May be called before calling Music_Play.
// These take effect on the next tick.
//


// Seek to a specific time in milliseconds in the tune.
void Music_Seek(const char *id, UINT32 set);

// Remap a tune to another song. Use the lump name, with the
// 'O_' at the beginning removed. song is case insensitive.
void Music_Remap(const char *id, const char *song);

// Set whether a tune should loop.
void Music_Loop(const char *id, boolean loop);

// Temporarily exemplify a tune from batch operations, such
// as Music_StopAll.
void Music_BatchExempt(const char *id);

// Set the volume for level context. TODO: this should be
// done on a more selective basis, rather than globally.
void Music_LevelVolume(int volume);

// Reset volume back to normal. This will fade it as if the
// music is resuming after another tune ended.
void Music_ResetLevelVolume(void);


//
// Query properties.
//

// Returns true if the tune exists.
boolean Music_TuneExists(const char* id);

// Returns true if the tune is configured to loop.
boolean Music_CanLoop(const char *id);

// Returns true if the tune does not play indefinitely, i.e.
// has a limited duration.
boolean Music_CanEnd(const char *id);

// Returns true if the tune is playing. This does not
// necessarily mean it is audible, because it has to be at the
// highest priority to be heard.
boolean Music_Playing(const char *id);

// Returns true if the tune is paused.
boolean Music_Paused(const char *id);

// Returns true if the tune is suspended.
boolean Music_Suspended(const char *id);

// Returns the number of tics elapsed since the start of the
// tune.
tic_t Music_Elapsed(const char *id);

// Returns the number of tics remaining until the tune ends.
tic_t Music_DurationLeft(const char *id);

// Returns the total duration of the tune, in tics.
tic_t Music_TotalDuration(const char *id);

// Returns the number of milliseconds a tune is configured to
// fade for.
unsigned int Music_FadeOutDuration(const char *id);

// Returns the song name mapped to a tune.
const char *Music_Song(const char *id);


//
// Low level program state.
//


// Loads certain data structures used for the lifetime of the
// program. Only needs to be called once, before any other
// functions.
void Music_Init(void);

// Call this every tic to update the music.
void Music_Tick(void);

// Flips the internal state so music is reloaded next tick.
// This is required when disabling music during runtime so
// the music plays again when re-enabled.
void Music_Flip(void);

// Resets all non-dynamic tunes to default values.
// Keeps ACS music remapping from playing havoc after a map.
void Music_TuneReset(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // MUSIC_H
