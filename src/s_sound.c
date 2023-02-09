// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  s_sound.c
/// \brief System-independent sound and music routines

#include "doomdef.h"
#include "doomstat.h"
#include "command.h"
#include "g_game.h"
#include "m_argv.h"
#include "r_main.h" // R_PointToAngle2() used to calc stereo sep.
#include "r_skins.h" // for skins
#include "i_system.h"
#include "i_sound.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_main.h"
#include "r_sky.h" // skyflatnum
#include "p_local.h" // camera info
#include "fastcmp.h"
#include "m_misc.h" // for tunes command
#include "m_cond.h" // for conditionsets
#include "lua_hook.h" // MusicChange hook
#include "byteptr.h"
#include "k_menu.h" // M_PlayMenuJam
#include "m_random.h" // P_RandomKey

#ifdef HW3SOUND
// 3D Sound Interface
#include "hardware/hw3sound.h"
#else
static boolean S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source, INT32 *vol, INT32 *sep, INT32 *pitch, sfxinfo_t *sfxinfo);
#endif

CV_PossibleValue_t soundvolume_cons_t[] = {{0, "MIN"}, {MAX_VOLUME, "MAX"}, {0, NULL}};
static void SetChannelsNum(void);
static void Command_Tunes_f(void);
static void Command_RestartAudio_f(void);
static void Command_PlaySound(void);
static void Got_PlaySound(UINT8 **p, INT32 playernum);

// Sound system toggles
static void GameSounds_OnChange(void);
static void GameDigiMusic_OnChange(void);

static void PlayMusicIfUnfocused_OnChange(void);
static void PlaySoundIfUnfocused_OnChange(void);

#ifdef HAVE_OPENMPT
static void ModFilter_OnChange(void);
#endif

static lumpnum_t S_GetMusicLumpNum(const char *mname);

static boolean S_CheckQueue(void);

consvar_t cv_samplerate = CVAR_INIT ("samplerate", "22050", 0, CV_Unsigned, NULL); //Alam: For easy hacking?

// stereo reverse
consvar_t stereoreverse = CVAR_INIT ("stereoreverse", "Off", CV_SAVE, CV_OnOff, NULL);

// if true, all sounds are loaded at game startup
static consvar_t precachesound = CVAR_INIT ("precachesound", "Off", CV_SAVE, CV_OnOff, NULL);

// actual general (maximum) sound & music volume, saved into the config
consvar_t cv_soundvolume = CVAR_INIT ("soundvolume", "50", CV_SAVE, soundvolume_cons_t, NULL);
consvar_t cv_digmusicvolume = CVAR_INIT ("musicvolume", "50", CV_SAVE, soundvolume_cons_t, NULL);

// number of channels available
consvar_t cv_numChannels = CVAR_INIT ("snd_channels", "64", CV_SAVE|CV_CALL, CV_Unsigned, SetChannelsNum);

consvar_t surround = CVAR_INIT ("surround", "Off", CV_SAVE, CV_OnOff, NULL);

static void Captioning_OnChange(void)
{
	S_ResetCaptions();
	if (cv_closedcaptioning.value)
		S_StartSound(NULL, sfx_menu1);
}

consvar_t cv_closedcaptioning = CVAR_INIT ("closedcaptioning", "Off", CV_SAVE|CV_CALL, CV_OnOff, Captioning_OnChange);

// Sound system toggles, saved into the config
consvar_t cv_gamedigimusic = CVAR_INIT ("music", "On", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, GameDigiMusic_OnChange);
consvar_t cv_gamesounds = CVAR_INIT ("sounds", "On", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, GameSounds_OnChange);

static CV_PossibleValue_t music_resync_threshold_cons_t[] = {
	{0,    "MIN"},
	{1000, "MAX"},
	{0, NULL}
};

consvar_t cv_music_resync_threshold = CVAR_INIT ("music_resync_threshold", "100", CV_SAVE|CV_CALL, music_resync_threshold_cons_t, I_UpdateSongLagThreshold);
consvar_t cv_music_resync_powerups_only = CVAR_INIT ("music_resync_powerups_only", "No", CV_SAVE|CV_CALL, CV_YesNo, I_UpdateSongLagConditions);

// Window focus sound sytem toggles
consvar_t cv_playmusicifunfocused = CVAR_INIT ("playmusicifunfocused",  "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, PlayMusicIfUnfocused_OnChange);
consvar_t cv_playsoundifunfocused = CVAR_INIT ("playsoundsifunfocused", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, PlaySoundIfUnfocused_OnChange);

#ifdef HAVE_OPENMPT
openmpt_module *openmpt_mhandle = NULL;

static CV_PossibleValue_t interpolationfilter_cons_t[] = {{0, "Default"}, {1, "None"}, {2, "Linear"}, {4, "Cubic"}, {8, "Windowed sinc"}, {0, NULL}};
consvar_t cv_modfilter = CVAR_INIT ("modfilter", "0", CV_SAVE|CV_CALL, interpolationfilter_cons_t, ModFilter_OnChange);
#endif

#define S_MAX_VOLUME 127

// when to clip out sounds
// Does not fit the large outdoor areas.
// added 2-2-98 in 8 bit volume control (before (1200*0x10000))
#define S_CLIPPING_DIST (1536*0x10000)

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
// added 2-2-98 in 8 bit volume control (before (160*0x10000))
#define S_CLOSE_DIST (160*0x10000)

// added 2-2-98 in 8 bit volume control (before remove the +4)
#define S_ATTENUATOR ((S_CLIPPING_DIST-S_CLOSE_DIST)>>(FRACBITS+4))

// Adjustable by menu.
#define NORM_VOLUME snd_MaxVolume

#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128

#define S_PITCH_PERTURB 1
#define S_STEREO_SWING (96*0x10000)

#ifdef SURROUND
#define SURROUND_SEP -128
#endif

// percent attenuation from front to back
#define S_IFRACVOL 30

// the set of channels available
static channel_t *channels = NULL;
static INT32 numofchannels = 0;

caption_t closedcaptions[NUMCAPTIONS];

void S_ResetCaptions(void)
{
	UINT8 i;
	for (i = 0; i < NUMCAPTIONS; i++)
	{
		closedcaptions[i].c = NULL;
		closedcaptions[i].s = NULL;
		closedcaptions[i].t = 0;
		closedcaptions[i].b = 0;
	}
}

//
// Internals.
//
static void S_StopChannel(INT32 cnum);

//
// S_getChannel
//
// If none available, return -1. Otherwise channel #.
//
static INT32 S_getChannel(const void *origin, sfxinfo_t *sfxinfo)
{
	// channel number to use
	INT32 cnum;

	// Find an open channel
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (!channels[cnum].sfxinfo)
			break;

		// Now checks if same sound is being played, rather
		// than just one sound per mobj
		else if (sfxinfo == channels[cnum].sfxinfo && (sfxinfo->pitch & SF_NOMULTIPLESOUND))
		{
			return -1;
		}
		else if (sfxinfo == channels[cnum].sfxinfo && sfxinfo->singularity == true)
		{
			S_StopChannel(cnum);
			break;
		}
		else if (origin && channels[cnum].origin == origin && channels[cnum].sfxinfo == sfxinfo)
		{
			if (sfxinfo->pitch & SF_NOINTERRUPT)
				return -1;
			else
				S_StopChannel(cnum);
			break;
		}
		else if (origin && channels[cnum].origin == origin
			&& channels[cnum].sfxinfo->name != sfxinfo->name
			&& (channels[cnum].sfxinfo->pitch & SF_TOTALLYSINGLE) && (sfxinfo->pitch & SF_TOTALLYSINGLE))
		{
			S_StopChannel(cnum);
			break;
		}
	}

	// None available
	if (cnum == numofchannels)
	{
		// Look for lower priority
		for (cnum = 0; cnum < numofchannels; cnum++)
			if (channels[cnum].sfxinfo->priority <= sfxinfo->priority)
				break;

		if (cnum == numofchannels)
		{
			// No lower priority. Sorry, Charlie.
			return -1;
		}
		else
		{
			// Otherwise, kick out lower priority.
			S_StopChannel(cnum);
		}
	}

	return cnum;
}

void S_RegisterSoundStuff(void)
{
	if (dedicated)
	{
		sound_disabled = true;
		return;
	}

	CV_RegisterVar(&stereoreverse);
	CV_RegisterVar(&precachesound);

	CV_RegisterVar(&surround);
	CV_RegisterVar(&cv_samplerate);
	CV_RegisterVar(&cv_playsoundifunfocused);
	CV_RegisterVar(&cv_playmusicifunfocused);
	CV_RegisterVar(&cv_gamesounds);
	CV_RegisterVar(&cv_gamedigimusic);

	CV_RegisterVar(&cv_music_resync_threshold);
	CV_RegisterVar(&cv_music_resync_powerups_only);

#ifdef HAVE_OPENMPT
	CV_RegisterVar(&cv_modfilter);
#endif

	COM_AddCommand("tunes", Command_Tunes_f);
	COM_AddCommand("restartaudio", Command_RestartAudio_f);
	COM_AddCommand("playsound", Command_PlaySound);
	RegisterNetXCmd(XD_PLAYSOUND, Got_PlaySound);
}

static void SetChannelsNum(void)
{
	// Allocating the internal channels for mixing
	// (the maximum number of sounds rendered
	// simultaneously) within zone memory.
	if (channels)
		S_StopSounds();

	Z_Free(channels);
	channels = NULL;


	if (cv_numChannels.value == 999999999) //Alam_GBC: OH MY ROD!(ROD rimmiced with GOD!)
		CV_StealthSet(&cv_numChannels,cv_numChannels.defaultvalue);

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_SetSourcesNum();
		return;
	}
#endif
	if (cv_numChannels.value)
		channels = (channel_t *)Z_Calloc(cv_numChannels.value * sizeof (channel_t), PU_STATIC, NULL);
	numofchannels = (channels ? cv_numChannels.value : 0);

	S_ResetCaptions();
}


// Retrieve the lump number of sfx
//
lumpnum_t S_GetSfxLumpNum(sfxinfo_t *sfx)
{
	char namebuf[9];
	lumpnum_t sfxlump;

	sprintf(namebuf, "ds%s", sfx->name);

	sfxlump = W_CheckNumForName(namebuf);
	if (sfxlump != LUMPERROR)
		return sfxlump;

	strlcpy(namebuf, sfx->name, sizeof namebuf);

	sfxlump = W_CheckNumForName(namebuf);
	if (sfxlump != LUMPERROR)
		return sfxlump;

	return W_GetNumForName("dsthok");
}

//
// Sound Status
//

boolean S_SoundDisabled(void)
{
	return (
			sound_disabled ||
			( window_notinfocus && ! cv_playsoundifunfocused.value )
	);
}

// Stop all sounds, load level info, THEN start sounds.
void S_StopSounds(void)
{
	INT32 cnum;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSounds();
		return;
	}
#endif

	// kill all playing sounds at start of level
	for (cnum = 0; cnum < numofchannels; cnum++)
		if (channels[cnum].sfxinfo)
			S_StopChannel(cnum);

	S_ResetCaptions();
}

void S_StopSoundByID(void *origin, sfxenum_t sfx_id)
{
	INT32 cnum;

	// Sounds without origin can have multiple sources, they shouldn't
	// be stopped by new sounds.
	if (!origin)
		return;
#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSoundByID(origin, sfx_id);
		return;
	}
#endif
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].sfxinfo == &S_sfx[sfx_id] && channels[cnum].origin == origin)
		{
			S_StopChannel(cnum);
		}
	}
}

void S_StopSoundByNum(sfxenum_t sfxnum)
{
	INT32 cnum;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSoundByNum(sfxnum);
		return;
	}
#endif
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].sfxinfo == &S_sfx[sfxnum])
		{
			S_StopChannel(cnum);
		}
	}
}

void S_StartCaption(sfxenum_t sfx_id, INT32 cnum, UINT16 lifespan)
{
	UINT8 i, set, moveup, start;
	boolean same = false;
	sfxinfo_t *sfx;

	if (!cv_closedcaptioning.value) // no captions at all
		return;

	// check for bogus sound #
	// I_Assert(sfx_id >= 0); -- allowing sfx_None; this shouldn't be allowed directly if S_StartCaption is ever exposed to Lua by itself
	I_Assert(sfx_id < NUMSFX);

	sfx = &S_sfx[sfx_id];

	if (sfx->caption[0] == '/') // no caption for this one
		return;

	start = ((closedcaptions[0].s && (closedcaptions[0].s-S_sfx == sfx_None)) ? 1 : 0);

	if (sfx_id)
	{
		for (i = start; i < (set = NUMCAPTIONS-1); i++)
		{
			same = ((sfx == closedcaptions[i].s) || (closedcaptions[i].s && fastcmp(sfx->caption, closedcaptions[i].s->caption)));
			if (same)
			{
				set = i;
				break;
			}
		}
	}
	else
	{
		set = 0;
		same = (closedcaptions[0].s == sfx);
	}

	moveup = 255;

	if (!same)
	{
		for (i = start; i < set; i++)
		{
			if (!(closedcaptions[i].c || closedcaptions[i].s) || (sfx->priority >= closedcaptions[i].s->priority))
			{
				set = i;
				if (closedcaptions[i].s && (sfx->priority >= closedcaptions[i].s->priority))
					moveup = i;
				break;
			}
		}
		for (i = NUMCAPTIONS-1; i > set; i--)
		{
			if (sfx == closedcaptions[i].s)
			{
				closedcaptions[i].c = NULL;
				closedcaptions[i].s = NULL;
				closedcaptions[i].t = 0;
				closedcaptions[i].b = 0;
			}
		}
	}

	if (moveup != 255)
	{
		for (i = moveup; i < NUMCAPTIONS-1; i++)
		{
			if (!(closedcaptions[i].c || closedcaptions[i].s))
				break;
		}
		for (; i > set; i--)
		{
			closedcaptions[i].c = closedcaptions[i-1].c;
			closedcaptions[i].s = closedcaptions[i-1].s;
			closedcaptions[i].t = closedcaptions[i-1].t;
			closedcaptions[i].b = closedcaptions[i-1].b;
		}
	}

	closedcaptions[set].c = ((cnum == -1) ? NULL : &channels[cnum]);
	closedcaptions[set].s = sfx;
	closedcaptions[set].t = lifespan;
	closedcaptions[set].b = 2; // bob
}

static INT32 S_ScaleVolumeWithSplitscreen(INT32 volume)
{
	fixed_t root = INT32_MAX;

	if (r_splitscreen == 0)
	{
		return volume;
	}

	root = FixedSqrt((r_splitscreen + 1) * (FRACUNIT/3));

	return FixedDiv(
		volume * FRACUNIT,
		root
	) / FRACUNIT;
}

void S_StartSoundAtVolume(const void *origin_p, sfxenum_t sfx_id, INT32 volume)
{
	const mobj_t *origin = (const mobj_t *)origin_p;
	const sfxenum_t actual_id = sfx_id;
	const boolean reverse = (stereoreverse.value ^ encoremode);
	const INT32 initial_volume = (origin ? S_ScaleVolumeWithSplitscreen(volume) : volume);

	sfxinfo_t *sfx;
	INT32 sep, pitch, priority, cnum;
	boolean anyListeners = false;
	boolean itsUs = false;
	INT32 i;

	listener_t listener[MAXSPLITSCREENPLAYERS];
	mobj_t *listenmobj[MAXSPLITSCREENPLAYERS];

	if (S_SoundDisabled() || !sound_started)
		return;

	// Don't want a sound? Okay then...
	if (sfx_id == sfx_None)
		return;

	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *player = &players[displayplayers[i]];

		memset(&listener[i], 0, sizeof (listener[i]));
		listenmobj[i] = NULL;

		if (!player)
		{
			continue;
		}

		if (i == 0 && democam.soundmobj)
		{
			continue;
		}

		if (player->awayviewtics)
		{
			listenmobj[i] = player->awayviewmobj;
		}
		else
		{
			listenmobj[i] = player->mo;
		}

		if (origin && origin == listenmobj[i])
		{
			itsUs = true;
		}
	}

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StartSound(origin, sfx_id);
		return;
	};
#endif

	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *player = &players[displayplayers[i]];

		if (!player)
		{
			continue;
		}

		if (camera[i].chase && !player->awayviewtics)
		{
			listener[i].x = camera[i].x;
			listener[i].y = camera[i].y;
			listener[i].z = camera[i].z;
			listener[i].angle = camera[i].angle;
			anyListeners = true;
		}
		else if (listenmobj[i])
		{
			listener[i].x = listenmobj[i]->x;
			listener[i].y = listenmobj[i]->y;
			listener[i].z = listenmobj[i]->z;
			listener[i].angle = listenmobj[i]->angle;
			anyListeners = true;
		}
	}

	if (origin && anyListeners == false)
	{
		// If a mobj is trying to make a noise, and no one is around to hear it, does it make a sound?
		return;
	}

	// check for bogus sound #
	I_Assert(sfx_id >= 1);
	I_Assert(sfx_id < NUMSFX);

	sfx = &S_sfx[sfx_id];

	if (sfx->skinsound != -1 && origin && (origin->player || origin->skin))
	{
		// redirect player sound to the sound in the skin table
		skin_t *skin = (origin->player ? &skins[origin->player->skin] : ((skin_t *)origin->skin));
		sfx_id = skin->soundsid[sfx->skinsound];
		sfx = &S_sfx[sfx_id];
	}

	// Initialize sound parameters
	pitch = NORM_PITCH;
	priority = NORM_PRIORITY;
	sep = NORM_SEP;

	i = 0; // sensible default

	{
		// Check to see if it is audible, and if not, modify the params
		if (origin && !itsUs)
		{
			boolean audible = false;

			if (r_splitscreen > 0)
			{
				fixed_t recdist = INT32_MAX;
				UINT8 j = 0;

				for (; j <= r_splitscreen; j++)
				{
					fixed_t thisdist = INT32_MAX;

					if (!listenmobj[j])
					{
						continue;
					}

					thisdist = P_AproxDistance(listener[j].x - origin->x, listener[j].y - origin->y);

					if (thisdist >= recdist)
					{
						continue;
					}
				
					recdist = thisdist;
					i = j;
				}
			}

			if (listenmobj[i])
			{
				audible = S_AdjustSoundParams(listenmobj[i], origin, &volume, &sep, &pitch, sfx);
			}

			if (!audible)
			{
				return;
			}
		}

		// This is supposed to handle the loading/caching.
		// For some odd reason, the caching is done nearly
		// each time the sound is needed?

		// cache data if necessary
		// NOTE: set sfx->data NULL sfx->lump -1 to force a reload
		if (!sfx->data)
		{
			sfx->data = I_GetSfx(sfx);

			if (!sfx->data)
			{
				CONS_Alert(CONS_WARNING,
						"Tried to load invalid sfx_%s\n",
						sfx->name);
				return;/* don't play it */
			}
		}

		// increase the usefulness
		if (sfx->usefulness++ < 0)
		{
			sfx->usefulness = -1;
		}

		// Avoid channel reverse if surround
		if (reverse
#ifdef SURROUND
			&& sep != SURROUND_SEP
#endif
			)
		{
			sep = (~sep) & 255;
		}

		// At this point it is determined that a sound can and should be played, so find a free channel to play it on
		cnum = S_getChannel(origin, sfx);

		if (cnum < 0)
		{
			return; // If there's no free channels, there won't be any for anymore players either
		}

		// Handle closed caption input.
		S_StartCaption(actual_id, cnum, MAXCAPTIONTICS);

		// Now that we know we are going to play a sound, fill out this info
		channels[cnum].sfxinfo = sfx;
		channels[cnum].origin = origin;
		channels[cnum].volume = initial_volume;
		channels[cnum].handle = I_StartSound(sfx_id, S_GetSoundVolume(sfx, volume), sep, pitch, priority, cnum);
	}
}

void S_StartSound(const void *origin, sfxenum_t sfx_id)
{
	if (S_SoundDisabled())
		return;

	// the volume is handled 8 bits
#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3S_StartSound(origin, sfx_id);
	else
#endif
		S_StartSoundAtVolume(origin, sfx_id, 255);
}

void S_ReducedVFXSoundAtVolume(const void *origin, sfxenum_t sfx_id, INT32 volume, player_t *owner)
{
	if (S_SoundDisabled())
		return;

	if (cv_reducevfx.value == 1)
	{
		if (owner == NULL)
		{
			return;
		}

		if (P_IsDisplayPlayer(owner) == false)
		{
			return;
		}
	}

	S_StartSoundAtVolume(origin, sfx_id, volume);
}

void S_StopSound(void *origin)
{
	INT32 cnum;

	// Sounds without origin can have multiple sources, they shouldn't
	// be stopped by new sounds.
	if (!origin)
		return;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSound(origin);
		return;
	}
#endif
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
		{
			S_StopChannel(cnum);
		}
	}
}

//
// Updates music & sounds
//
static INT32 actualsfxvolume; // check for change through console
static INT32 actualdigmusicvolume;

void S_UpdateSounds(void)
{
	INT32 cnum, volume, sep, pitch;
	boolean audible = false;
	channel_t *c;
	INT32 i;

	listener_t listener[MAXSPLITSCREENPLAYERS];
	mobj_t *listenmobj[MAXSPLITSCREENPLAYERS];

	// Update sound/music volumes, if changed manually at console
	if (actualsfxvolume != cv_soundvolume.value * USER_VOLUME_SCALE)
		S_SetSfxVolume (cv_soundvolume.value);
	if (actualdigmusicvolume != cv_digmusicvolume.value * USER_VOLUME_SCALE)
		S_SetDigMusicVolume (cv_digmusicvolume.value);

	// We're done now, if we're not in a level.
	if (gamestate != GS_LEVEL)
	{
#ifndef NOMUMBLE
		// Stop Mumble cutting out. I'm sick of it.
		I_UpdateMumble(NULL, listener[0]);
#endif

		goto notinlevel;
	}

	if (dedicated || sound_disabled)
		return;

	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *player = &players[displayplayers[i]];

		memset(&listener[i], 0, sizeof (listener[i]));
		listenmobj[i] = NULL;

		if (!player)
		{
			continue;
		}

		if (i == 0 && democam.soundmobj)
		{
			continue;
		}

		if (player->awayviewtics)
		{
			listenmobj[i] = player->awayviewmobj;
		}
		else
		{
			listenmobj[i] = player->mo;
		}
	}

#ifndef NOMUMBLE
	I_UpdateMumble(players[consoleplayer].mo, listener[0]);
#endif

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_UpdateSources();
		goto notinlevel;
	}
#endif

	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *player = &players[displayplayers[i]];

		if (!player)
		{
			continue;
		}

		if (camera[i].chase && !player->awayviewtics)
		{
			listener[i].x = camera[i].x;
			listener[i].y = camera[i].y;
			listener[i].z = camera[i].z;
			listener[i].angle = camera[i].angle;
		}
		else if (listenmobj[i])
		{
			listener[i].x = listenmobj[i]->x;
			listener[i].y = listenmobj[i]->y;
			listener[i].z = listenmobj[i]->z;
			listener[i].angle = listenmobj[i]->angle;
		}
	}

	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		c = &channels[cnum];

		if (c->sfxinfo)
		{
			if (I_SoundIsPlaying(c->handle))
			{
				// initialize parameters
				volume = c->volume; // 8 bits internal volume precision
				pitch = NORM_PITCH;
				sep = NORM_SEP;

				// check non-local sounds for distance clipping
				//  or modify their params
				if (c->origin)
				{
					boolean itsUs = false;

					for (i = r_splitscreen; i >= 0; i--)
					{
						if (c->origin != listenmobj[i])
							continue;

						itsUs = true;
					}

					if (itsUs == false)
					{
						const mobj_t *origin = c->origin;

						i = 0;

						if (r_splitscreen > 0)
						{
							fixed_t recdist = INT32_MAX;
							UINT8 j = 0;

							for (; j <= r_splitscreen; j++)
							{
								fixed_t thisdist = INT32_MAX;

								if (!listenmobj[j])
								{
									continue;
								}

								thisdist = P_AproxDistance(listener[j].x - origin->x, listener[j].y - origin->y);

								if (thisdist >= recdist)
								{
									continue;
								}

								recdist = thisdist;
								i = j;
							}
						}

						if (listenmobj[i])
						{
							audible = S_AdjustSoundParams(
								listenmobj[i], c->origin,
								&volume, &sep, &pitch,
								c->sfxinfo
							);
						}

						if (audible)
							I_UpdateSoundParams(c->handle, S_GetSoundVolume(c->sfxinfo, volume), sep, pitch);
						else
							S_StopChannel(cnum);
					}
				}
			}
			else
			{
				// if channel is allocated but sound has stopped, free it
				S_StopChannel(cnum);
			}
		}
	}

notinlevel:
	I_UpdateSound();
}

void S_UpdateClosedCaptions(void)
{
	UINT8 i;
	boolean gamestopped = (paused || P_AutoPause());
	for (i = 0; i < NUMCAPTIONS; i++) // update captions
	{
		if (!closedcaptions[i].s)
			continue;

		if (i == 0 && (closedcaptions[0].s-S_sfx == sfx_None) && gamestopped)
			continue;

		if (!(--closedcaptions[i].t))
		{
			closedcaptions[i].c = NULL;
			closedcaptions[i].s = NULL;
		}
		else if (closedcaptions[i].c && !I_SoundIsPlaying(closedcaptions[i].c->handle))
		{
			closedcaptions[i].c = NULL;
			if (closedcaptions[i].t > CAPTIONFADETICS)
				closedcaptions[i].t = CAPTIONFADETICS;
		}
	}
}

void S_SetSfxVolume(INT32 volume)
{
	CV_SetValue(&cv_soundvolume, volume);
	actualsfxvolume = cv_soundvolume.value * USER_VOLUME_SCALE;

#ifdef HW3SOUND
	hws_mode == HWS_DEFAULT_MODE ? I_SetSfxVolume(volume&0x1F) : HW3S_SetSfxVolume(volume&0x1F);
#else
	// now hardware volume
	I_SetSfxVolume(volume);
#endif
}

void S_ClearSfx(void)
{
	size_t i;
	for (i = 1; i < NUMSFX; i++)
		I_FreeSfx(S_sfx + i);
}

static void S_StopChannel(INT32 cnum)
{
	channel_t *c = &channels[cnum];

	if (c->sfxinfo)
	{
		// stop the sound playing
		if (I_SoundIsPlaying(c->handle))
			I_StopSound(c->handle);

		// degrade usefulness of sound data
		c->sfxinfo->usefulness--;
		c->sfxinfo = 0;
	}

	c->origin = NULL;
}

//
// S_CalculateSoundDistance
//
// Calculates the distance between two points for a sound.
// Clips the distance to prevent overflow.
//
fixed_t S_CalculateSoundDistance(fixed_t sx1, fixed_t sy1, fixed_t sz1, fixed_t sx2, fixed_t sy2, fixed_t sz2)
{
	fixed_t approx_dist, adx, ady;

	// calculate the distance to sound origin and clip it if necessary
	adx = abs((sx1>>FRACBITS) - (sx2>>FRACBITS));
	ady = abs((sy1>>FRACBITS) - (sy2>>FRACBITS));

	// From _GG1_ p.428. Approx. euclidian distance fast.
	// Take Z into account
	adx = adx + ady - ((adx < ady ? adx : ady)>>1);
	ady = abs((sz1>>FRACBITS) - (sz2>>FRACBITS));
	approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

	if (approx_dist >= FRACUNIT/2)
		approx_dist = FRACUNIT/2-1;

	approx_dist <<= FRACBITS;

	return FixedDiv(approx_dist, mapobjectscale); // approx_dist
}

INT32 S_GetSoundVolume(sfxinfo_t *sfx, INT32 volume)
{
	if (sfx->volume > 0)
		return (volume * sfx->volume) / 100;

	return volume;
}

//
// Changes volume, stereo-separation, and pitch variables
// from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
boolean S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source, INT32 *vol, INT32 *sep, INT32 *pitch,
	sfxinfo_t *sfxinfo)
{
	const boolean reverse = (stereoreverse.value ^ encoremode);

	fixed_t approx_dist;

	listener_t listensource;
	INT32 i;

	(void)pitch;

	if (!listener)
		return false;

	// Init listensource with default listener
	listensource.x = listener->x;
	listensource.y = listener->y;
	listensource.z = listener->z;
	listensource.angle = listener->angle;

	for (i = 0; i <= r_splitscreen; i++)
	{
		// If listener is a chasecam player, use the camera instead
		if (listener == players[displayplayers[i]].mo && camera[i].chase)
		{
			listensource.x = camera[i].x;
			listensource.y = camera[i].y;
			listensource.z = camera[i].z;
			listensource.angle = camera[i].angle;
			break;
		}
	}

	if (sfxinfo->pitch & SF_OUTSIDESOUND) // Rain special case
	{
		fixed_t x, y, yl, yh, xl, xh, newdist;

		if (R_PointInSubsector(listensource.x, listensource.y)->sector->ceilingpic == skyflatnum)
			approx_dist = 0;
		else
		{
			// Essentially check in a 1024 unit radius of the player for an outdoor area.
			yl = listensource.y - 1024*FRACUNIT;
			yh = listensource.y + 1024*FRACUNIT;
			xl = listensource.x - 1024*FRACUNIT;
			xh = listensource.x + 1024*FRACUNIT;
			approx_dist = 1024*FRACUNIT;
			for (y = yl; y <= yh; y += FRACUNIT*64)
				for (x = xl; x <= xh; x += FRACUNIT*64)
				{
					if (R_PointInSubsector(x, y)->sector->ceilingpic == skyflatnum)
					{
						// Found the outdoors!
						newdist = S_CalculateSoundDistance(listensource.x, listensource.y, 0, x, y, 0);
						if (newdist < approx_dist)
						{
							approx_dist = newdist;
						}
					}
				}
		}
	}
	else
	{
		approx_dist = S_CalculateSoundDistance(listensource.x, listensource.y, listensource.z,
												source->x, source->y, source->z);
	}

	// Ring loss, deaths, etc, should all be heard louder.
	if (sfxinfo->pitch & SF_X8AWAYSOUND)
		approx_dist = FixedDiv(approx_dist,8*FRACUNIT);

	// Combine 8XAWAYSOUND with 4XAWAYSOUND and get.... 32XAWAYSOUND?
	if (sfxinfo->pitch & SF_X4AWAYSOUND)
		approx_dist = FixedDiv(approx_dist,4*FRACUNIT);

	if (sfxinfo->pitch & SF_X2AWAYSOUND)
		approx_dist = FixedDiv(approx_dist,2*FRACUNIT);

	if (approx_dist > S_CLIPPING_DIST)
		return false;

	if (source->x == listensource.x && source->y == listensource.y)
	{
		*sep = NORM_SEP;
	}
	else
	{
		// angle of source to listener
		angle_t angle = R_PointToAngle2(listensource.x, listensource.y, source->x, source->y);

		if (angle > listensource.angle)
			angle = angle - listensource.angle;
		else
			angle = angle + InvAngle(listensource.angle);

		if (reverse)
			angle = InvAngle(angle);

#ifdef SURROUND
		// Produce a surround sound for angle from 105 till 255
		if (surround.value == 1 && (angle > ANG105 && angle < ANG255 ))
			*sep = SURROUND_SEP;
		else
#endif
		{
			angle >>= ANGLETOFINESHIFT;

			// stereo separation
			*sep = 128 - (FixedMul(S_STEREO_SWING, FINESINE(angle))>>FRACBITS);
		}
	}

	// volume calculation
	/* not sure if it should be > (no =), but this matches the old behavior */
	if (approx_dist >= S_CLOSE_DIST)
	{
		// distance effect
		INT32 n = (15 * ((S_CLIPPING_DIST - approx_dist)>>FRACBITS));
		*vol = FixedMul(*vol * FRACUNIT / 255, n) / S_ATTENUATOR;
	}

	return (*vol > 0);
}

// Searches through the channels and checks if a sound is playing
// on the given origin.
INT32 S_OriginPlaying(void *origin)
{
	INT32 cnum;
	if (!origin)
		return false;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		return HW3S_OriginPlaying(origin);
#endif

	for (cnum = 0; cnum < numofchannels; cnum++)
		if (channels[cnum].origin == origin)
			return 1;
	return 0;
}

// Searches through the channels and checks if a given id
// is playing anywhere.
INT32 S_IdPlaying(sfxenum_t id)
{
	INT32 cnum;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		return HW3S_IdPlaying(id);
#endif

	for (cnum = 0; cnum < numofchannels; cnum++)
		if ((size_t)(channels[cnum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	return 0;
}

// Searches through the channels and checks for
// origin x playing sound id y.
INT32 S_SoundPlaying(void *origin, sfxenum_t id)
{
	INT32 cnum;
	if (!origin)
		return 0;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		return HW3S_SoundPlaying(origin, id);
#endif

	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].origin == origin
		 && (size_t)(channels[cnum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	}
	return 0;
}

//
// S_StartSoundName
// Starts a sound using the given name.
#define MAXNEWSOUNDS 10
static sfxenum_t newsounds[MAXNEWSOUNDS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void S_StartSoundName(void *mo, const char *soundname)
{
	INT32 i, soundnum = 0;
	// Search existing sounds...
	for (i = sfx_None + 1; i < NUMSFX; i++)
	{
		if (!S_sfx[i].name)
			continue;
		if (!stricmp(S_sfx[i].name, soundname))
		{
			soundnum = i;
			break;
		}
	}

	if (!soundnum)
	{
		for (i = 0; i < MAXNEWSOUNDS; i++)
		{
			if (newsounds[i] == 0)
				break;
			if (!S_IdPlaying(newsounds[i]))
			{
				S_RemoveSoundFx(newsounds[i]);
				break;
			}
		}

		if (i == MAXNEWSOUNDS)
		{
			CONS_Debug(DBG_GAMELOGIC, "Cannot load another extra sound!\n");
			return;
		}

		soundnum = S_AddSoundFx(soundname, false, 0, false);
		newsounds[i] = soundnum;
	}

	S_StartSound(mo, soundnum);
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_InitSfxChannels(INT32 sfxVolume)
{
	INT32 i;

	if (dedicated)
		return;

	S_SetSfxVolume(sfxVolume);

	SetChannelsNum();

	// Note that sounds have not been cached (yet).
	for (i = 1; i < NUMSFX; i++)
	{
		S_sfx[i].usefulness = -1; // for I_GetSfx()
		S_sfx[i].lumpnum = LUMPERROR;
	}

	// precache sounds if requested by cmdline, or precachesound var true
	if (!sound_disabled && (M_CheckParm("-precachesound") || precachesound.value))
	{
		// Initialize external data (all sounds) at start, keep static.
		CONS_Printf(M_GetText("Loading sounds... "));

		for (i = 1; i < NUMSFX; i++)
			if (S_sfx[i].name)
				S_sfx[i].data = I_GetSfx(&S_sfx[i]);

		CONS_Printf(M_GetText(" pre-cached all sound data\n"));
	}
}

/// ------------------------
/// Music
/// ------------------------

static char      music_name[7]; // up to 6-character name
static void      *music_data;
static UINT16    music_flags;
static boolean   music_looping;
static consvar_t *music_refade_cv;
static int       music_usage;

static char      queue_name[7];
static UINT16    queue_flags;
static boolean   queue_looping;
static UINT32    queue_position;
static UINT32    queue_fadeinms;

static tic_t     pause_starttic;

/// ------------------------
/// Music Definitions
/// ------------------------

musicdef_t *musicdefstart = NULL;
struct cursongcredit cursongcredit; // Currently displayed song credit info
int musicdef_volume;

//
// S_FindMusicDef
//
// Find music def by 6 char name
//
musicdef_t *S_FindMusicDef(const char *name)
{
	musicdef_t *def = musicdefstart;

	while (def)
	{
		if (!stricmp(def->name, name))
		{
			return def;
		}

		def = def->next;
	}

	return NULL;
}

static boolean
MusicDefError
(
		alerttype_t  level,
		const char * description,
		const char * field,
		lumpnum_t    lumpnum,
		int          line
){
	const wadfile_t  * wad  =    wadfiles[WADFILENUM (lumpnum)];
	const lumpinfo_t * lump = &wad->lumpinfo[LUMPNUM (lumpnum)];

	CONS_Alert(level,
			va("%%s|%%s: %s (line %%d)\n", description),
			wad->filename,
			lump->fullname,
			field,
			line
	);

	return false;
}

static boolean
ReadMusicDefFields
(
		lumpnum_t     lumpnum,
		int           line,
		char       *  stoken,
		musicdef_t ** defp
){
	musicdef_t *def;

	char *value;
	char *textline;

	if (!stricmp(stoken, "lump"))
	{
		value = strtok(NULL, " ");
		if (!value)
		{
			return MusicDefError(CONS_WARNING,
					"Field '%'s is missing name.",
					stoken, lumpnum, line);
		}
		else
		{
			def = S_FindMusicDef(value);

			// Nothing found, add to the end.
			if (!def)
			{
				def = Z_Calloc(sizeof (musicdef_t), PU_STATIC, NULL);

				STRBUFCPY(def->name, value);
				strlwr(def->name);
				def->volume = DEFAULT_MUSICDEF_VOLUME;

				def->next = musicdefstart;
				musicdefstart = def;
			}

			(*defp) = def;
		}
	}
	else
	{
		value = strtok(NULL, "");

		if (value)
		{
			// Find the equals sign.
			value = strchr(value, '=');
		}

		if (!value)
		{
			return MusicDefError(CONS_WARNING,
					"Field '%s' is missing value.",
					stoken, lumpnum, line);
		}
		else
		{
			def = (*defp);

			if (!def)
			{
				return MusicDefError(CONS_ERROR,
						"No music definition before field '%s'.",
						stoken, lumpnum, line);
			}

			// Skip the equals sign.
			value++;

			// Now skip funny whitespace.
			value += strspn(value, "\t ");

			textline = value;

			if (!stricmp(stoken, "title"))
			{
				Z_Free(def->title);
				def->title = Z_StrDup(textline);
			}
			else if (!stricmp(stoken, "author"))
			{
				Z_Free(def->author);
				def->author = Z_StrDup(textline);
			}
			else if (!stricmp(stoken, "source"))
			{
				Z_Free(def->source);
				def->source = Z_StrDup(textline);
			}
			else if (!stricmp(stoken, "originalcomposers"))
			{
				Z_Free(def->composers);
				def->composers = Z_StrDup(textline);
			}
			else if (!stricmp(stoken, "volume"))
			{
				def->volume = atoi(textline);
			}
			else
			{
				MusicDefError(CONS_WARNING,
						"Unknown field '%s'.",
						stoken, lumpnum, line);
			}
		}
	}

	return true;
}

static void S_LoadMusicDefLump(lumpnum_t lumpnum)
{
	char *lump;
	char *musdeftext;
	size_t size;

	char *lf;
	char *stoken;

	size_t nlf;
	size_t ncr;

	musicdef_t *def = NULL;
	int line = 1; // for better error msgs

	lump = W_CacheLumpNum(lumpnum, PU_CACHE);
	size = W_LumpLength(lumpnum);

	// Null-terminated MUSICDEF lump.
	musdeftext = malloc(size+1);
	if (!musdeftext)
		I_Error("S_LoadMusicDefs: No more free memory for the parser\n");
	M_Memcpy(musdeftext, lump, size);
	musdeftext[size] = '\0';

	// Find music def
	stoken = musdeftext;
	for (;;)
	{
		lf = strpbrk(stoken, "\r\n");
		if (lf)
		{
			if (*lf == '\n')
				nlf = 1;
			else
				nlf = 0;
			*lf++ = '\0';/* now we can delimit to here */
		}

		stoken = strtok(stoken, " ");
		if (stoken)
		{
			if (! ReadMusicDefFields(lumpnum, line, stoken, &def))
				break;
		}

		if (lf)
		{
			do
			{
				line += nlf;
				ncr = strspn(lf, "\r");/* skip CR */
				lf += ncr;
				nlf = strspn(lf, "\n");
				lf += nlf;
			}
			while (nlf || ncr) ;

			stoken = lf;/* now the next nonempty line */
		}
		else
			break;/* EOF */
	}

	free(musdeftext);
}

void S_LoadMusicDefs(UINT16 wad)
{
	const lumpnum_t wadnum = wad << 16;

	UINT16 lump = 0;

	while (( lump = W_CheckNumForNamePwad("MUSICDEF", wad, lump) ) != INT16_MAX)
	{
		S_LoadMusicDefLump(wadnum | lump);

		lump++;
	}
}

//
// S_InitMusicDefs
//
// Simply load music defs in all wads.
//
void S_InitMusicDefs(void)
{
	UINT16 i;
	for (i = 0; i < numwadfiles; i++)
		S_LoadMusicDefs(i);
}

//
// S_ShowMusicCredit
//
// Display current song's credit on screen
//
void S_ShowMusicCredit(void)
{
	musicdef_t *def = S_FindMusicDef(music_name);

	char credittext[128] = "";
	char *work = NULL;
	size_t len = 128, worklen;

	if (!cv_songcredits.value || demo.rewinding)
		return;

	if (!def) // No definitions
		return;

	if (!def->title)
	{
		return;
	}

	work = va("\x1F %s", def->title);
	worklen = strlen(work);
	if (worklen <= len)
	{
		strncat(credittext, work, len);
		len -= worklen;

#define MUSICCREDITAPPEND(field)\
		if (field)\
		{\
			work = va(" - %s", field);\
			worklen = strlen(work);\
			if (worklen <= len)\
			{\
				strncat(credittext, work, len);\
				len -= worklen;\
			}\
		}

		MUSICCREDITAPPEND(def->author);
		MUSICCREDITAPPEND(def->source);

#undef MUSICCREDITAPPEND
	}

	if (credittext[0] == '\0')
		return;

	cursongcredit.def = def;
	Z_Free(cursongcredit.text);
	cursongcredit.text = Z_StrDup(credittext);
	cursongcredit.anim = 5*TICRATE;
	cursongcredit.x = cursongcredit.old_x = 0;
	cursongcredit.trans = NUMTRANSMAPS;
}

/// ------------------------
/// Music Status
/// ------------------------

boolean S_DigMusicDisabled(void)
{
	return digital_disabled;
}

boolean S_MusicDisabled(void)
{
	return digital_disabled;
}

boolean S_MusicPlaying(void)
{
	return I_SongPlaying();
}

boolean S_MusicPaused(void)
{
	return I_SongPaused();
}

boolean S_MusicNotInFocus(void)
{
	return (
			( window_notinfocus && ! cv_playmusicifunfocused.value )
	);
}

const char *S_MusicType(void)
{
	return I_SongType();
}

const char *S_MusicName(void)
{
	return music_name;
}

boolean S_MusicInfo(char *mname, UINT16 *mflags, boolean *looping)
{
	if (!I_SongPlaying())
		return false;

	strncpy(mname, music_name, 7);
	mname[6] = 0;
	*mflags = music_flags;
	*looping = music_looping;

	return (boolean)mname[0];
}

boolean S_MusicExists(const char *mname)
{
	return W_CheckNumForName(va("O_%s", mname)) != LUMPERROR;
}

/// ------------------------
/// Music Effects
/// ------------------------

boolean S_SpeedMusic(float speed)
{
	return I_SetSongSpeed(speed);
}

/// ------------------------
/// Music Seeking
/// ------------------------

UINT32 S_GetMusicLength(void)
{
	return I_GetSongLength();
}

boolean S_SetMusicLoopPoint(UINT32 looppoint)
{
	return I_SetSongLoopPoint(looppoint);
}

UINT32 S_GetMusicLoopPoint(void)
{
	return I_GetSongLoopPoint();
}

boolean S_SetMusicPosition(UINT32 position)
{
	if (demo.rewinding // Don't mess with music while rewinding!
		|| demo.title) // SRB2Kart: Demos don't interrupt title screen music
		return false;

	return I_SetSongPosition(position);
}

UINT32 S_GetMusicPosition(void)
{
	return I_GetSongPosition();
}

/// ------------------------
/// Music Stacking (Jingles)
/// In this section: mazmazz doesn't know how to do dynamic arrays or struct pointers!
/// ------------------------

char music_stack_nextmusname[7];
boolean music_stack_noposition = false;
UINT32 music_stack_fadeout = 0;
UINT32 music_stack_fadein = 0;
static musicstack_t *music_stacks = NULL;
static musicstack_t *last_music_stack = NULL;

void S_SetStackAdjustmentStart(void)
{
	if (!pause_starttic)
		pause_starttic = gametic;
}

void S_AdjustMusicStackTics(void)
{
	if (pause_starttic)
	{
		musicstack_t *mst;
		for (mst = music_stacks; mst; mst = mst->next)
			mst->tic += gametic - pause_starttic;
		pause_starttic = 0;
	}
}

static void S_ResetMusicStack(void)
{
	musicstack_t *mst, *mst_next;
	for (mst = music_stacks; mst; mst = mst_next)
	{
		mst_next = mst->next;
		Z_Free(mst);
	}
	music_stacks = last_music_stack = NULL;
}

static void S_RemoveMusicStackEntry(musicstack_t *entry)
{
	musicstack_t *mst;
	for (mst = music_stacks; mst; mst = mst->next)
	{
		if (mst == entry)
		{
			// Remove ourselves from the chain and link
			// prev and next together

			if (mst->prev)
				mst->prev->next = mst->next;
			else
				music_stacks = mst->next;

			if (mst->next)
				mst->next->prev = mst->prev;
			else
				last_music_stack = mst->prev;

			break;
		}
	}
	Z_Free(entry);
}

static void S_RemoveMusicStackEntryByStatus(UINT16 status)
{
	musicstack_t *mst, *mst_next;

	if (!status)
		return;

	for (mst = music_stacks; mst; mst = mst_next)
	{
		mst_next = mst->next;
		if (mst->status == status)
			S_RemoveMusicStackEntry(mst);
	}
}

static void S_AddMusicStackEntry(const char *mname, UINT16 mflags, boolean looping, UINT32 position, UINT16 status)
{
	musicstack_t *mst, *new_mst;

	// if the first entry is empty, force master onto it
	if (!music_stacks)
	{
		music_stacks = Z_Calloc(sizeof (*mst), PU_MUSIC, NULL);
		strncpy(music_stacks->musname, (status == JT_MASTER ? mname : (S_CheckQueue() ? queue_name : mapmusname)), 7);
		music_stacks->musflags = (status == JT_MASTER ? mflags : (S_CheckQueue() ? queue_flags : mapmusflags));
		music_stacks->looping = (status == JT_MASTER ? looping : (S_CheckQueue() ? queue_looping : true));
		music_stacks->position = (status == JT_MASTER ? position : (S_CheckQueue() ? queue_position : S_GetMusicPosition()));
		music_stacks->tic = gametic;
		music_stacks->status = JT_MASTER;
		music_stacks->mlumpnum = S_GetMusicLumpNum(music_stacks->musname);
		music_stacks->noposition = S_CheckQueue();

		if (status == JT_MASTER)
			return; // we just added the user's entry here
	}

	// look for an empty slot to park ourselves
	for (mst = music_stacks; mst->next; mst = mst->next);

	// create our new entry
	new_mst = Z_Calloc(sizeof (*new_mst), PU_MUSIC, NULL);
	strncpy(new_mst->musname, mname, 7);
	new_mst->musname[6] = 0;
	new_mst->musflags = mflags;
	new_mst->looping = looping;
	new_mst->position = position;
	new_mst->tic = gametic;
	new_mst->status = status;
	new_mst->mlumpnum = S_GetMusicLumpNum(new_mst->musname);
	new_mst->noposition = false;

	mst->next = new_mst;
	new_mst->prev = mst;
	new_mst->next = NULL;
	last_music_stack = new_mst;
}

static musicstack_t *S_GetMusicStackEntry(UINT16 status, boolean fromfirst, INT16 startindex)
{
	musicstack_t *mst, *start_mst = NULL, *mst_next;

	// if the first entry is empty, force master onto it
	// fixes a memory corruption bug
	if (!music_stacks && status != JT_MASTER)
		S_AddMusicStackEntry(mapmusname, mapmusflags, true, S_GetMusicPosition(), JT_MASTER);

	if (startindex >= 0)
	{
		INT16 i = 0;
		for (mst = music_stacks; mst && i <= startindex; mst = mst->next, i++)
			start_mst = mst;
	}
	else
		start_mst = (fromfirst ? music_stacks : last_music_stack);

	for (mst = start_mst; mst; mst = mst_next)
	{
		mst_next = (fromfirst ? mst->next : mst->prev);

		if (!status || mst->status == status)
		{
			if (P_EvaluateMusicStatus(mst->status, mst->musname))
			{
				if (!S_MusicExists(mst->musname)) // paranoia
					S_RemoveMusicStackEntry(mst); // then continue
				else
					return mst;
			}
			else
				S_RemoveMusicStackEntry(mst); // then continue
		}
	}

	return NULL;
}

void S_RetainMusic(const char *mname, UINT16 mflags, boolean looping, UINT32 position, UINT16 status)
{
	musicstack_t *mst;

	if (!status) // we use this as a null indicator, don't push
	{
		CONS_Alert(CONS_ERROR, "Music stack entry must have a nonzero status.\n");
		return;
	}
	else if (status == JT_MASTER) // enforce only one JT_MASTER
	{
		for (mst = music_stacks; mst; mst = mst->next)
		{
			if (mst->status == JT_MASTER)
			{
				CONS_Alert(CONS_ERROR, "Music stack can only have one JT_MASTER entry.\n");
				return;
			}
		}
	}
	else // remove any existing status
		S_RemoveMusicStackEntryByStatus(status);

	S_AddMusicStackEntry(mname, mflags, looping, position, status);
}

boolean S_RecallMusic(UINT16 status, boolean fromfirst)
{
	UINT32 newpos = 0;
	boolean mapmuschanged = false;
	musicstack_t *result;
	musicstack_t *entry;

	if (demo.rewinding // Don't mess with music while rewinding!
		|| demo.title) // SRB2Kart: Demos don't interrupt title screen music
		return false;

	entry = Z_Calloc(sizeof (*result), PU_MUSIC, NULL);

	if (status)
		result = S_GetMusicStackEntry(status, fromfirst, -1);
	else
		result = S_GetMusicStackEntry(JT_NONE, false, -1);

	if (result && !S_MusicExists(result->musname))
	{
		Z_Free(entry);
		return false; // music doesn't exist, so don't do anything
	}

	// make a copy of result, since we make modifications to our copy
	if (result)
	{
		*entry = *result;
		strncpy(entry->musname, result->musname, 7);
	}

	// no result, just grab mapmusname
	if (!result || !entry->musname[0] || ((status == JT_MASTER || (music_stacks ? !music_stacks->status : false)) && !entry->status))
	{
		strncpy(entry->musname, mapmusname, 7);
		entry->musflags = mapmusflags;
		entry->looping = true;
		entry->position = mapmusposition;
		entry->tic = gametic;
		entry->status = JT_MASTER;
		entry->mlumpnum = S_GetMusicLumpNum(entry->musname);
		entry->noposition = false; // don't set this until we do the mapmuschanged check, below. Else, this breaks some resumes.
	}

	if (entry->status == JT_MASTER)
	{
		mapmuschanged = strnicmp(entry->musname, mapmusname, 7);
		if (mapmuschanged)
		{
			strncpy(entry->musname, mapmusname, 7);
			entry->musflags = mapmusflags;
			entry->looping = true;
			entry->position = mapmusposition;
			entry->tic = gametic;
			entry->status = JT_MASTER;
			entry->mlumpnum = S_GetMusicLumpNum(entry->musname);
			entry->noposition = true;
		}
		S_ResetMusicStack();
	}
	else if (!entry->status)
	{
		Z_Free(entry);
		return false;
	}

	if (strncmp(entry->musname, S_MusicName(), 7)) // don't restart music if we're already playing it
	{
		if (music_stack_fadeout)
			S_ChangeMusicEx(entry->musname, entry->musflags, entry->looping, 0, music_stack_fadeout, 0);
		else
		{
			S_ChangeMusicEx(entry->musname, entry->musflags, entry->looping, 0, 0, music_stack_fadein);

			if (!entry->noposition && !music_stack_noposition) // HACK: Global boolean to toggle position resuming, e.g., de-superize
			{
				UINT32 poslapse = 0;

				// To prevent the game from jumping past the end of the music, we need
				// to check if we can get the song's length. Otherwise, if the lapsed resume time goes
				// over a LOOPPOINT, mixer_sound.c will be unable to calculate the new resume position.
				if (S_GetMusicLength())
					poslapse = (UINT32)((float)(gametic - entry->tic)/(float)TICRATE*(float)MUSICRATE);

				newpos = entry->position + poslapse;
			}

			// If the newly recalled music lumpnum does not match the lumpnum that we stored in stack,
			// then discard the new position. That way, we will not recall an invalid position
			// when the music is replaced or digital/MIDI is toggled.
			if (newpos > 0 && S_MusicPlaying() && S_GetMusicLumpNum(entry->musname) == entry->mlumpnum)
				S_SetMusicPosition(newpos);
			else
			{
				S_StopFadingMusic();
				S_SetInternalMusicVolume(100);
			}
		}
		music_stack_noposition = false;
		music_stack_fadeout = 0;
		music_stack_fadein = JINGLEPOSTFADE;
	}

	Z_Free(entry);
	return true;
}

/// ------------------------
/// Music Playback
/// ------------------------

static lumpnum_t S_GetMusicLumpNum(const char *mname)
{
	if (S_MusicExists(mname))
		return W_GetNumForName(va("o_%s", mname));
	else
		return LUMPERROR;
}

static boolean S_LoadMusic(const char *mname)
{
	lumpnum_t mlumpnum;
	void *mdata;

	if (S_MusicDisabled())
		return false;

	mlumpnum = S_GetMusicLumpNum(mname);

	if (mlumpnum == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, "Music %.6s could not be loaded: lump not found!\n", mname);
		return false;
	}

	// load & register it
	mdata = W_CacheLumpNum(mlumpnum, PU_MUSIC);

	if (I_LoadSong(mdata, W_LumpLength(mlumpnum)))
	{
		strncpy(music_name, mname, 7);
		music_name[6] = 0;
		music_data = mdata;
		return true;
	}
	else
	{
		CONS_Alert(CONS_ERROR, "Music %.6s could not be loaded: engine failure!\n", mname);
		return false;
	}
}

static void S_UnloadMusic(void)
{
	I_UnloadSong();

#ifndef HAVE_SDL //SDL uses RWOPS
	Z_ChangeTag(music_data, PU_CACHE);
#endif
	music_data = NULL;

	music_name[0] = 0;
	music_flags = 0;
	music_looping = false;

	music_refade_cv = 0;
	music_usage = 0;
}

static boolean S_PlayMusic(boolean looping, UINT32 fadeinms)
{
	//musicdef_t *def;

	if (S_MusicDisabled())
		return false;

	I_UpdateSongLagConditions();

	if ((!fadeinms && !I_PlaySong(looping)) ||
		(fadeinms && !I_FadeInPlaySong(fadeinms, looping)))
	{
		CONS_Alert(CONS_ERROR, "Music %.6s could not be played: engine failure!\n", music_name);
		S_UnloadMusic();
		return false;
	}

#if 0
	/* set loop point from MUSICDEF */
	for (def = musicdefstart; def; def = def->next)
	{
		if (strcasecmp(def->name, music_name) == 0)
		{
			if (def->loop_ms)
				S_SetMusicLoopPoint(def->loop_ms);
			break;
		}
	}
#endif

	S_InitMusicVolume(); // switch between digi and sequence volume

	if (S_MusicNotInFocus())
		I_SetMusicVolume(0);

	return true;
}

static void S_QueueMusic(const char *mmusic, UINT16 mflags, boolean looping, UINT32 position, UINT32 fadeinms)
{
	strncpy(queue_name, mmusic, 7);
	queue_flags = mflags;
	queue_looping = looping;
	queue_position = position;
	queue_fadeinms = fadeinms;
}

static boolean S_CheckQueue(void)
{
	return queue_name[0];
}

static void S_ClearQueue(void)
{
	queue_name[0] = queue_flags = queue_looping = queue_position = queue_fadeinms = 0;
}

static void S_ChangeMusicToQueue(void)
{
	S_ChangeMusicEx(queue_name, queue_flags, queue_looping, queue_position, 0, queue_fadeinms);
	S_ClearQueue();
}

void S_ChangeMusicEx(const char *mmusic, UINT16 mflags, boolean looping, UINT32 position, UINT32 prefadems, UINT32 fadeinms)
{
	char newmusic[7];

	struct MusicChange hook_param = {
		newmusic,
		&mflags,
		&looping,
		&position,
		&prefadems,
		&fadeinms
	};

	if (S_MusicDisabled()
		|| demo.rewinding // Don't mess with music while rewinding!
		|| demo.title) // SRB2Kart: Demos don't interrupt title screen music
		return;

	strncpy(newmusic, mmusic, 7);

	if (LUA_HookMusicChange(music_name, &hook_param))
		return;

	newmusic[6] = 0;

 	// No Music (empty string)
	if (newmusic[0] == 0)
 	{
		if (prefadems)
			I_FadeSong(0, prefadems, &S_StopMusic);
		else
			S_StopMusic();
		return;
	}

	if (prefadems) // queue music change for after fade // allow even if the music is the same
		// && S_MusicPlaying() // Let the delay happen even if we're not playing music
	{
		CONS_Debug(DBG_DETAILED, "Now fading out song %s\n", music_name);
		S_QueueMusic(newmusic, mflags, looping, position, fadeinms);
		I_FadeSong(0, prefadems, S_ChangeMusicToQueue);
		return;
	}
	else if (strnicmp(music_name, newmusic, 6) || (mflags & MUSIC_FORCERESET))
 	{
		CONS_Debug(DBG_DETAILED, "Now playing song %s\n", newmusic);

		S_StopMusic();

		if (!S_LoadMusic(newmusic))
			return;

		music_flags = mflags;
		music_looping = looping;

		musicdef_volume = DEFAULT_MUSICDEF_VOLUME;

		{
			musicdef_t *def = S_FindMusicDef(music_name);

			if (def)
			{
				musicdef_volume = def->volume;
			}
		}

		if (!S_PlayMusic(looping, fadeinms))
			return;

		if (position)
			I_SetSongPosition(position);

		I_SetSongTrack(mflags & MUSIC_TRACKMASK);
	}
	else if (fadeinms) // let fades happen with same music
	{
		I_SetSongPosition(position);
		I_FadeSong(100, fadeinms, NULL);
 	}
	else // reset volume to 100 with same music
	{
		I_StopFadingSong();
		I_FadeSong(100, 500, NULL);
	}
}

void S_ChangeMusicSpecial (const char *mmusic)
{
	if (cv_resetspecialmusic.value)
		S_ChangeMusic(mmusic, MUSIC_FORCERESET, true);
	else
		S_ChangeMusicInternal(mmusic, true);
}

void S_StopMusic(void)
{
	if (!I_SongPlaying()
		|| demo.rewinding // Don't mess with music while rewinding!
		|| demo.title) // SRB2Kart: Demos don't interrupt title screen music
		return;

	if (strcasecmp(music_name, mapmusname) == 0)
		mapmusresume = I_GetSongPosition();

	if (I_SongPaused())
		I_ResumeSong();

	S_SpeedMusic(1.0f);
	I_StopSong();
	S_UnloadMusic(); // for now, stopping also means you unload the song

	if (cv_closedcaptioning.value)
	{
		if (closedcaptions[0].s-S_sfx == sfx_None)
		{
			if (gamestate != wipegamestate)
			{
				closedcaptions[0].c = NULL;
				closedcaptions[0].s = NULL;
				closedcaptions[0].t = 0;
				closedcaptions[0].b = 0;
			}
			else
				closedcaptions[0].t = CAPTIONFADETICS;
		}
	}
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseAudio(void)
{
	if (I_SongPlaying() && !I_SongPaused())
		I_PauseSong();

	S_SetStackAdjustmentStart();
}

void S_ResumeAudio(void)
{
	if (S_MusicNotInFocus())
		return;

	if (I_SongPlaying() && I_SongPaused())
		I_ResumeSong();

	S_AdjustMusicStackTics();
}

void S_SetMusicVolume(INT32 digvolume)
{
	if (digvolume < 0)
		digvolume = cv_digmusicvolume.value;

	CV_SetValue(&cv_digmusicvolume, digvolume);
	actualdigmusicvolume = cv_digmusicvolume.value * USER_VOLUME_SCALE;
	I_SetMusicVolume(digvolume);
}

void
S_SetRestoreMusicFadeInCvar (consvar_t *cv)
{
	music_refade_cv = cv;
}

int
S_GetRestoreMusicFadeIn (void)
{
	if (music_refade_cv)
		return music_refade_cv->value;
	else
		return 0;
}

void
S_SetMusicUsage (int type)
{
	music_usage = type;
	I_UpdateSongLagConditions();
}

int
S_MusicUsage (void)
{
	return music_usage;
}

/// ------------------------
/// Music Fading
/// ------------------------

void S_SetInternalMusicVolume(INT32 volume)
{
	I_SetInternalMusicVolume(min(max(volume, 0), 100));
}

void S_StopFadingMusic(void)
{
	I_StopFadingSong();
}

boolean S_FadeMusicFromVolume(UINT8 target_volume, INT16 source_volume, UINT32 ms)
{
	if (demo.rewinding // Don't mess with music while rewinding!
		|| demo.title) // SRB2Kart: Demos don't interrupt title screen music
		return false;

	if (source_volume < 0)
		return I_FadeSong(target_volume, ms, NULL);
	else
		return I_FadeSongFromVolume(target_volume, source_volume, ms, NULL);
}

boolean S_FadeOutStopMusic(UINT32 ms)
{
	if (demo.rewinding // Don't mess with music while rewinding!
		|| demo.title) // SRB2Kart: Demos don't interrupt title screen music
		return false;

	return I_FadeSong(0, ms, &S_StopMusic);
}

/// ------------------------
/// Init & Others
/// ------------------------

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_StartEx(boolean reset)
{
	(void)reset;

	if (mapmusflags & MUSIC_RELOADRESET)
	{
		UINT32 i = P_RandomKey(PR_MUSICSELECT, 2);
		strncpy(mapmusname, mapheaderinfo[gamemap-1]->musname[i], 7);
		mapmusname[6] = 0;
		mapmusflags = (mapheaderinfo[gamemap-1]->mustrack & MUSIC_TRACKMASK);
		mapmusposition = mapheaderinfo[gamemap-1]->muspos;
		mapmusresume = 0;
	}

	S_StopMusic(); // Starting ambience should always be restarted, if playing.

	if (leveltime < (starttime + (TICRATE/2))) // SRB2Kart
	{
		;
	}
	else
		S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);

	S_ResetMusicStack();
	music_stack_noposition = false;
	music_stack_fadeout = 0;
	music_stack_fadein = JINGLEPOSTFADE;
}

static inline void PrintMusicDefField(const char *label, const char *field)
{
	if (field)
	{
		CONS_Printf("%s%s\n", label, field);
	}
}

static void PrintSongAuthors(const musicdef_t *def)
{
	PrintMusicDefField("Title:  ", def->title);
	PrintMusicDefField("Author: ", def->author);

	CONS_Printf("\n");

	PrintMusicDefField("Original Source:    ", def->source);
	PrintMusicDefField("Original Composers: ", def->composers);
}

// TODO: fix this function, needs better support for map names
static void Command_Tunes_f(void)
{
	const char *tunearg;
	UINT16 track = 0;
	UINT32 position = 0;
	const size_t argc = COM_Argc();

	if (argc < 2) //tunes slot ...
	{
		CONS_Printf("tunes <name/num> [track] [speed] [position] / <-show> / <-default> / <-none>:\n");
		CONS_Printf(M_GetText("Play an arbitrary music lump. If a map number is used, 'MAP##M' is played.\n"));
		CONS_Printf(M_GetText("If the format supports multiple songs, you can specify which one to play.\n\n"));
		CONS_Printf(M_GetText("* With \"-show\", shows the currently playing tune and track.\n"));
		CONS_Printf(M_GetText("* With \"-default\", returns to the default music for the map.\n"));
		CONS_Printf(M_GetText("* With \"-none\", any music playing will be stopped.\n"));
		return;
	}

	tunearg = COM_Argv(1);
	track = 0;

	if (!strcasecmp(tunearg, "-show"))
	{
		const musicdef_t *def = S_FindMusicDef(mapmusname);

		CONS_Printf(M_GetText("The current tune is: %s [track %d]\n"),
			mapmusname, (mapmusflags & MUSIC_TRACKMASK));

		if (def != NULL)
		{
			PrintSongAuthors(def);
		}
		return;
	}
	if (!strcasecmp(tunearg, "-none"))
	{
		S_StopMusic();
		return;
	}
	else if (!strcasecmp(tunearg, "-default"))
	{
		tunearg = mapheaderinfo[gamemap-1]->musname[0];
		track = mapheaderinfo[gamemap-1]->mustrack;
	}

	if (strlen(tunearg) > 6) // This is automatic -- just show the error just in case
		CONS_Alert(CONS_NOTICE, M_GetText("Music name too long - truncated to six characters.\n"));

	if (argc > 2)
		track = (UINT16)atoi(COM_Argv(2))-1;

	strncpy(mapmusname, tunearg, 7);

	if (argc > 4)
		position = (UINT32)atoi(COM_Argv(4));

	mapmusname[6] = 0;
	mapmusflags = (track & MUSIC_TRACKMASK);
	mapmusposition = position;
	mapmusresume = 0;

	S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);

	if (argc > 3)
	{
		float speed = (float)atof(COM_Argv(3));
		if (speed > 0.0f)
			S_SpeedMusic(speed);
	}
}

static void Command_RestartAudio_f(void)
{
	if (dedicated)  // No point in doing anything if game is a dedicated server.
		return;

	S_StopMusic();
	S_StopSounds();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_StartupSound();
	I_InitMusic();

// These must be called or no sound and music until manually set.

	I_SetSfxVolume(cv_soundvolume.value);
	S_SetMusicVolume(cv_digmusicvolume.value);

	S_StartSound(NULL, sfx_strpst);

	if (Playing()) // Gotta make sure the player is in a level
		P_RestoreMusic(&players[consoleplayer]);
	else
		S_ChangeMusicInternal("titles", looptitle);
}

static void Command_PlaySound(void)
{
	const char *sound;
	const size_t argc = COM_Argc();
	sfxenum_t sfx = NUMSFX;
	UINT8 buf[4];
	UINT8 *buf_p = buf;

	if (argc < 2)
	{
		CONS_Printf("playsound <name/num>: Plays a sound effect for the entire server.\n");
		return;
	}

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf("This can only be used by the server host.\n");
		return;
	}

	sound = COM_Argv(1);
	if (*sound >= '0' && *sound <= '9')
	{
		sfx = atoi(sound);
	}
	else
	{
		for (sfx = 0; sfx < sfxfree; sfx++)
		{
			if (S_sfx[sfx].name && fasticmp(sound, S_sfx[sfx].name))
				break;
		}
	}

	if (sfx < 0 || sfx >= NUMSFX)
	{
		CONS_Printf("Could not find sound effect named \"sfx_%s\".\n", sound);
		return;
	}

	WRITEINT32(buf_p, sfx);
	SendNetXCmd(XD_PLAYSOUND, buf, buf_p - buf);
}

static void Got_PlaySound(UINT8 **cp, INT32 playernum)
{
	INT32 sound_id = READINT32(*cp);

	if (playernum != serverplayer && !IsPlayerAdmin(playernum)) // hacked client, or disasterous bug
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal playsound received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (sound_id < 0 || sound_id >= NUMSFX)
	{
		// bad sound effect, ignore
		return;
	}

	S_StartSound(NULL, sound_id);
}

void GameSounds_OnChange(void)
{
	if (M_CheckParm("-nosound") || M_CheckParm("-noaudio"))
		return;

	if (sound_disabled)
	{
		sound_disabled = false;
		I_StartupSound(); // will return early if initialised
		S_InitSfxChannels(cv_soundvolume.value);
		S_StartSound(NULL, sfx_strpst);
	}
	else
	{
		sound_disabled = true;
		S_StopSounds();
	}
}

void GameDigiMusic_OnChange(void)
{
	if (M_CheckParm("-nomusic") || M_CheckParm("-noaudio"))
		return;
	else if (M_CheckParm("-nodigmusic"))
		return;

	if (digital_disabled)
	{
		digital_disabled = false;
		I_StartupSound(); // will return early if initialised
		I_InitMusic();

		if (Playing())
		{
			P_RestoreMusic(&players[consoleplayer]);
		}
		else if (gamestate == GS_TITLESCREEN)
		{
			S_ChangeMusicInternal("_title", looptitle);
		}
		else
		{
			M_PlayMenuJam();
		}
	}
	else
	{
		digital_disabled = true;
		S_StopMusic();
	}
}

static void PlayMusicIfUnfocused_OnChange(void)
{
	if (window_notinfocus)
	{
		if (cv_playmusicifunfocused.value)
			I_SetMusicVolume(0);
		else
			S_InitMusicVolume();
	}
}

static void PlaySoundIfUnfocused_OnChange(void)
{
	if (!cv_gamesounds.value)
		return;

	if (window_notinfocus && !cv_playsoundifunfocused.value)
		S_StopSounds();
}

#ifdef HAVE_OPENMPT
void ModFilter_OnChange(void)
{
	if (openmpt_mhandle)
		openmpt_module_set_render_param(openmpt_mhandle, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, cv_modfilter.value);
}
#endif
