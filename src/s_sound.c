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
/// \file  s_sound.c
/// \brief System-independent sound and music routines

#include "doomdef.h"
#include "doomstat.h"
#include "command.h"
#include "console.h" // con_startup
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
#include "m_random.h" // M_RandomKey
#include "i_time.h"
#include "v_video.h" // V_ThinStringWidth
#include "music.h"
#include "y_inter.h" // Y_PlayIntermissionMusic
#include "f_finale.h" // F_PlayTitleScreenMusic

extern consvar_t cv_mastervolume;

CV_PossibleValue_t soundmixingbuffersize_cons_t[] = {
	{256, "256"},
	{512, "512"},
	{1024, "1024"},
	{2048, "2048"},
	{4096, "4096"},
	{0, NULL}
};

static boolean S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source, INT32 *vol, INT32 *sep, INT32 *pitch, sfxinfo_t *sfxinfo);

static void Command_Tunes_f(void);
static void Command_RestartAudio_f(void);
static void Command_PlaySound(void);
static void Got_PlaySound(const UINT8 **p, INT32 playernum);
static void Command_MusicDef_f(void);

void Captioning_OnChange(void);
void Captioning_OnChange(void)
{
	S_ResetCaptions();
	if (cv_closedcaptioning.value)
		S_StartSound(NULL, sfx_menu1);
}

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

	COM_AddDebugCommand("tunes", Command_Tunes_f);
	COM_AddDebugCommand("restartaudio", Command_RestartAudio_f);
	COM_AddDebugCommand("playsound", Command_PlaySound);
	RegisterNetXCmd(XD_PLAYSOUND, Got_PlaySound);
	COM_AddDebugCommand("musicdef", Command_MusicDef_f);
}

void SetChannelsNum(void);
void SetChannelsNum(void)
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
			( window_notinfocus && ! (cv_bgaudio.value & 2) ) ||
			(g_fast_forward > 0) ||
			staffsync
	);
}

boolean S_VoiceDisabled(void)
{
	return (
			g_voice_disabled ||
			(g_fast_forward > 0)
	);
}

// Stop all sounds, load level info, THEN start sounds.
void S_StopSounds(void)
{
	INT32 cnum;

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
	// (The above comment predates this codebase using git and cannot be BLAME'd)
	// ...yeah, but if it's being stopped by ID, it's clearly an intentful effect. ~toast 090623
#if 0
	if (!origin)
		return;
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
		boolean camaway = false;

		memset(&listener[i], 0, sizeof (listener[i]));
		listenmobj[i] = NULL;

		if (!player)
		{
			continue;
		}

		if (player->awayview.tics)
		{
			listenmobj[i] = player->awayview.mobj;
		}
		else
		{
			listenmobj[i] = player->mo;
			if (player->exiting)
				camaway = true;
		}

		if (origin && origin == listenmobj[i] && !camera[i].freecam && !camaway)
		{
			itsUs = true;
		}
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *player = &players[displayplayers[i]];

		if (!player)
		{
			continue;
		}

		if (camera[i].chase && !player->awayview.tics)
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
		skin_t *skin = (origin->player ? skins[origin->player->skin] : ((skin_t *)origin->skin));
		if (R_CanShowSkinInDemo(skin->skinnum) == false)
			return;
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
	S_StartSoundAtVolume(origin, sfx_id, 255);
}

void S_ReducedVFXSoundAtVolume(const void *origin, sfxenum_t sfx_id, INT32 volume, const player_t *owner)
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
static INT32 actualmastervolume;
static INT32 actualvoicevolume;

static boolean PointIsLeft(float ax, float ay, float bx, float by)
{
	// return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x) > 0;
	return ax * by - ay * bx > 0;
}

void S_UpdateSounds(void)
{
	INT32 cnum, volume, sep, pitch;
	boolean audible = false;
	channel_t *c;
	INT32 i;

	listener_t listener[MAXSPLITSCREENPLAYERS];
	mobj_t *listenmobj[MAXSPLITSCREENPLAYERS];

	// Update sound/music volumes, if changed manually at console
	if (actualsfxvolume != cv_soundvolume.value)
		S_SetSfxVolume();
	if (actualdigmusicvolume != cv_digmusicvolume.value)
		S_SetMusicVolume();
	if (actualmastervolume != cv_mastervolume.value)
		S_SetMasterVolume();
	if (actualvoicevolume != cv_voicevolume.value)
		S_SetVoiceVolume();

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

		if (player->awayview.tics)
		{
			listenmobj[i] = player->awayview.mobj;
		}
		else
		{
			listenmobj[i] = player->mo;
		}
	}

#ifndef NOMUMBLE
	I_UpdateMumble(players[consoleplayer].mo, listener[0]);
#endif

	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *player = &players[displayplayers[i]];

		if (!player)
		{
			continue;
		}

		if (camera[i].chase && !player->awayview.tics)
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
						if (camera[i].freecam)
							continue;

						if (c->origin != listenmobj[i])
							continue;

						if (listenmobj[i]->player && listenmobj[i]->player->exiting)
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

void S_UpdateVoicePositionalProperties(void)
{
	int i;

	if (gamestate != GS_LEVEL)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			I_SetPlayerVoiceProperties(i, 1.0f, 0.0f);
		}
		return;
	}

	player_t *consoleplr = &players[consoleplayer];
	listener_t listener = {0};
	mobj_t *listenmobj = NULL;

	if (consoleplr)
	{
		if (consoleplr->awayview.tics)
		{
			listenmobj = consoleplr->awayview.mobj;
		}
		else
		{
			listenmobj = consoleplr->mo;
		}

		if (camera[0].chase && !consoleplr->awayview.tics)
		{
			listener.x = camera[0].x;
			listener.y = camera[0].y;
			listener.z = camera[0].z;
			listener.angle = camera[0].angle;
		}
		else if (listenmobj)
		{
			listener.x = listenmobj->x;
			listener.y = listenmobj->y;
			listener.z = listenmobj->z;
			listener.angle = listenmobj->angle;
		}
	}

	float playerdistances[MAXPLAYERS];
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *plr = &players[i];
		mobj_t *mo = plr->mo;
		if (plr->spectator || !mo)
		{
			playerdistances[i] = 0.f;
			continue;
		}
		float px = FixedToFloat(mo->x - listener.x);
		float py = FixedToFloat(mo->y - listener.y);
		float pz = FixedToFloat(mo->z - listener.z);
		playerdistances[i] = sqrtf(px * px + py * py + pz * pz);
	}

	// Positional voice audio
	boolean voice_proximity_enabled = cv_voice_proximity.value == 1;
	float voice_distanceattenuation_distance = FixedToFloat(cv_voice_distanceattenuation_distance.value) * FixedToFloat(mapheaderinfo[gamemap-1]->mobj_scale);
	float voice_distanceattenuation_teamdistance = FixedToFloat(cv_voice_distanceattenuation_teamdistance.value) * FixedToFloat(mapheaderinfo[gamemap-1]->mobj_scale);
	float voice_distanceattenuation_factor = FixedToFloat(cv_voice_distanceattenuation_factor.value);
	float voice_stereopanning_factor = FixedToFloat(cv_voice_stereopanning_factor.value);
	float voice_concurrentattenuation_min = max(0, min(MAXPLAYERS, cv_voice_concurrentattenuation_min.value));
	float voice_concurrentattenuation_max = max(0, min(MAXPLAYERS, cv_voice_concurrentattenuation_max.value));
	voice_concurrentattenuation_min = min(voice_concurrentattenuation_max, voice_concurrentattenuation_min);
	float voice_concurrentattenuation_factor = FixedToFloat(cv_voice_concurrentattenuation_factor.value);

	// Derive concurrent speaker attenuation
	float speakingplayers = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (S_IsPlayerVoiceActive(i))
		{
			float attenuation_distance = voice_distanceattenuation_distance;
			if (G_GametypeHasTeams() && players[i].team == players[consoleplayer].team)
			{
				attenuation_distance = voice_distanceattenuation_teamdistance;
			}

			if (attenuation_distance > 0)
			{
				speakingplayers += 1.f - (playerdistances[i] / attenuation_distance);
			}
			else
			{
				// invalid distance attenuation
				speakingplayers += 1.f;
			}
		}
	}
	speakingplayers = min(voice_concurrentattenuation_max, max(0, speakingplayers - voice_concurrentattenuation_min));
	float speakingplayerattenuation = 1.f - (speakingplayers * (voice_concurrentattenuation_factor / voice_concurrentattenuation_max));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			I_SetPlayerVoiceProperties(i, speakingplayerattenuation, 0.0f);
			continue;
		}

		if (i == consoleplayer)
		{
			I_SetPlayerVoiceProperties(i, speakingplayerattenuation, 0.0f);
			continue;
		}

		player_t *plr = &players[i];
		if (plr->spectator)
		{
			I_SetPlayerVoiceProperties(i, speakingplayerattenuation, 0.0f);
			continue;
		}

		mobj_t *plrmobj = plr->mo;

		if (!plrmobj)
		{
			I_SetPlayerVoiceProperties(i, 1.0f, 0.0f);
			continue;
		}

		float lx = FixedToFloat(listener.x);
		float ly = FixedToFloat(listener.y);
		float lz = FixedToFloat(listener.z);
		float px = FixedToFloat(plrmobj->x) - lx;
		float py = FixedToFloat(plrmobj->y) - ly;
		float pz = FixedToFloat(plrmobj->z) - lz;

		float ldirx = cosf(ANG2RAD(listener.angle));
		float ldiry = sinf(ANG2RAD(listener.angle));
		float pdistance = sqrtf(px * px + py * py + pz * pz);
		float p2ddistance = sqrtf(px * px + py * py);
		float pdirx = px / p2ddistance;
		float pdiry = py / p2ddistance;
		float angle = acosf(pdirx * ldirx + pdiry * ldiry);
		angle = (
			PointIsLeft(ldirx, ldiry, pdirx, pdiry)
			^ stereoreverse.value
			^ encoremode
		) ? -angle : angle;

		float plrvolume = 1.0f;

		float attenuation_distance = voice_distanceattenuation_distance;
		if (G_GametypeHasTeams() && players[i].team == players[consoleplayer].team)
		{
			attenuation_distance = voice_distanceattenuation_teamdistance;
		}

		if (voice_proximity_enabled && attenuation_distance > 0 && voice_distanceattenuation_factor >= 0 && voice_distanceattenuation_factor <= 1.0f)
		{
			float invfactor = 1.0f - voice_distanceattenuation_factor;
			float distfactor = max(0.f, min(attenuation_distance, pdistance)) / attenuation_distance;
			plrvolume = max(0.0f, min(1.0f, 1.0f - (invfactor * distfactor)));
		}

		float fsep = sinf(angle) * max(0.0f, min(1.0f, voice_stereopanning_factor));
		I_SetPlayerVoiceProperties(i, plrvolume * speakingplayerattenuation, fsep);
	}
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

void S_SetSfxVolume(void)
{
	actualsfxvolume = cv_soundvolume.value;

	// now hardware volume
	I_SetSfxVolume(actualsfxvolume);
}

void S_SetVoiceVolume(void)
{
	actualvoicevolume = cv_voicevolume.value;

	I_SetVoiceVolume(actualvoicevolume);
}

void S_SetMasterVolume(void)
{
	actualmastervolume = cv_mastervolume.value;

	I_SetMasterVolume(actualmastervolume);
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

	if (source->thinker.function.acp1 == (actionf_p1)P_MobjThinker && P_MobjIsReappearing(source))
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
		INT64 x, y, yl, yh, xl, xh;
		fixed_t newdist;

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

	for (cnum = 0; cnum < numofchannels; cnum++)
		if ((size_t)(channels[cnum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	return 0;
}

// Searches through the channels and checks for
// origin x playing sound id y.
INT32 S_SoundPlaying(const void *origin, sfxenum_t id)
{
	INT32 cnum;
	if (!origin)
		return 0;

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
void S_InitSfxChannels(void)
{
	extern consvar_t precachesound;

	INT32 i;

	if (dedicated)
		return;

	S_SetSfxVolume();

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

void S_AttemptToRestoreMusic(void)
{
	switch (gamestate)
	{
		case GS_LEVEL:
			if (musiccountdown != 1)
			{
				P_LoadLevelMusic();
				Music_Play("level");
				break;
			}
			// FALLTHRU
		case GS_INTERMISSION:
			Y_PlayIntermissionMusic();
			break;
		case GS_CEREMONY:
			Music_Play("level");
			break;
		case GS_TITLESCREEN:
			F_PlayTitleScreenMusic();
			break;
		case GS_MENU:
			M_PlayMenuJam();
			break;
		case GS_CREDITS:
			Music_Loop("credits", true);
			Music_Play("credits");
			break;
		default:
			break;
	}
}

/// ------------------------
/// Music Definitions
/// ------------------------

musicdef_t *musicdefstart = NULL;
struct cursongcredit cursongcredit; // Currently displayed song credit info
char *g_realsongcredit;
struct soundtest soundtest; // Sound Test (sound test)

static void S_InsertMusicAtSoundTestSequenceTail(const char *musname, UINT16 map, UINT8 altref, musicdef_t ***tail)
{
	UINT8 i = 0;
	musicdef_t *def = S_FindMusicDef(musname, &i);

	if (def == NULL)
		return;

	if (def->sequence.id == soundtest.sequence.id)
		return;

	def->sequence.id = soundtest.sequence.id;
	def->sequence.map = map;
	def->sequence.altref = altref;

	// So what we're doing here is to avoid iterating
	// for every insertion, we dereference the pointer
	// to get **tail from S_PopulateSoundTestSequence,
	// then dereference that to get the musicdef_t *.
	// We do it this way so that soundtest.sequence.next
	// can be handled natively without special cases.
	// I have officially lost my MIND. ~toast 270323
	*(*tail) = def;
	*tail = &def->sequence.next;
}

static void S_InsertMapIntoSoundTestSequence(UINT16 map, musicdef_t ***tail)
{
	UINT8 i;

	if (mapheaderinfo[map]->positionmus[0])
	{
		S_InsertMusicAtSoundTestSequenceTail(mapheaderinfo[map]->positionmus, map, 0, tail);
	}

	for (i = 0; i < mapheaderinfo[map]->musname_size; i++)
	{
		S_InsertMusicAtSoundTestSequenceTail(mapheaderinfo[map]->musname[i], map, i, tail);
	}

	for (i = 0; i < mapheaderinfo[map]->associatedmus_size; i++)
	{
		S_InsertMusicAtSoundTestSequenceTail(mapheaderinfo[map]->associatedmus[i], map, ALTREF_REQUIRESBEATEN, tail);
	}

	for (i = 0; i < mapheaderinfo[map]->encoremusname_size; i++)
	{
		S_InsertMusicAtSoundTestSequenceTail(mapheaderinfo[map]->encoremusname[i], map, i+MAXMUSNAMES, tail);
	}
}

void S_PopulateSoundTestSequence(void)
{
	UINT16 i;
	musicdef_t **tail;

	// First, increment the sequence and wipe the HEAD.
	// This invalidates all existing musicdefs without us
	// having to iterate through everything all the time,
	// and offers a very convenient checking mechanism.
	// ...preventing id 0 protects against inconsistencies
	// caused by newly Calloc'd music definitions.
	soundtest.sequence.id = (soundtest.sequence.id + 1) & 255;
	if (soundtest.sequence.id == 0)
		soundtest.sequence.id = 1;

	// Prepare shuffle material.
	soundtest.sequence.shuffleinfo = 0;
	soundtest.sequence.shufflenext = NULL;

	soundtest.sequence.next = NULL;

	tail = &soundtest.sequence.next;

	// We iterate over all tutorial maps.
	for (i = 0; i < nummapheaders; i++)
	{
		if (!mapheaderinfo[i])
			continue;

		if (mapheaderinfo[i]->cup != NULL)
			continue;

		if ((mapheaderinfo[i]->typeoflevel & TOL_TUTORIAL) == 0)
			continue;

		S_InsertMapIntoSoundTestSequence(i, &tail);
	}

	// Next, we iterate over all cups.
	{
		cupheader_t *cup;
		for (cup = kartcupheaders; cup; cup = cup->next)
		{
			for (i = 0; i < CUPCACHE_MAX; i++)
			{
				if (cup->cachedlevels[i] >= nummapheaders)
					continue;

				if (!mapheaderinfo[cup->cachedlevels[i]])
					continue;

				if (mapheaderinfo[cup->cachedlevels[i]]->cup != cup)
					continue;

				S_InsertMapIntoSoundTestSequence(cup->cachedlevels[i], &tail);
			}
		}
	}

	// Then, we iterate over all remaining non-cupped maps.
	for (i = 0; i < nummapheaders; i++)
	{
		if (!mapheaderinfo[i])
			continue;

		if (mapheaderinfo[i]->cup != NULL)
			continue;

		if (mapheaderinfo[i]->typeoflevel & TOL_TUTORIAL)
			continue;

		S_InsertMapIntoSoundTestSequence(i, &tail);
	}

	// Okay, guarantee the list ends on NULL! This stops
	// that pointing to either invalid memory in general,
	// or valid memory that is already somewhere else in
	// the sound test sequence (way more likely).
	// (We do this here so that inserting unimportant,
	// mapless musicdefs does not get overwritten, like it
	// would be if this were done after the below block.)
	*tail = NULL;

	// Finally, we insert all important musicdefs at the head,
	// and all others at the tail.
	// It's being added to the sequence in reverse order...
	// but because musicdefstart is ALSO populated in reverse,
	// the reverse of the reverse is the right way around!
	{
		musicdef_t *def;

		for (def = musicdefstart; def; def = def->next)
		{
			if (def->sequence.id == soundtest.sequence.id)
				continue;

			if (def->important == false)
				continue;

			def->sequence.id = soundtest.sequence.id;
			def->sequence.map = NEXTMAP_INVALID;
			def->sequence.altref = 0;

			def->sequence.next = soundtest.sequence.next;
			soundtest.sequence.next = def;
		}

		for (def = musicdefstart; def; def = def->next)
		{
			// This is the simplest set of checks,
			// so let's wipe the shuffle data here.
			def->sequence.shuffleinfo = 0;
			def->sequence.shufflenext = NULL;

			if (def->sequence.id == soundtest.sequence.id)
				continue;

			def->sequence.id = soundtest.sequence.id;
			def->sequence.map = NEXTMAP_INVALID;
			def->sequence.altref = 0;

			def->sequence.next = *tail;
			*tail = def;
		}
	}
}

static boolean S_SoundTestDefLocked(musicdef_t *def)
{
	// temporary - i'd like to find a way to conditionally hide
	// specific musicdefs that don't have any map associated.
	if (def->sequence.map >= nummapheaders || !mapheaderinfo[def->sequence.map])
		return false;

	mapheader_t *header = mapheaderinfo[def->sequence.map];

	// Visitation required?
	if (!(header->menuflags & LF2_NOVISITNEEDED)
	&& !(header->records.mapvisited & MV_VISITED))
		return true;

	if (def->sequence.altref != 0)
	{
		if ((def->sequence.altref == ALTREF_REQUIRESBEATEN))
		{
			// Associated music only when completed
			if (!(header->records.mapvisited & MV_BEATEN))
				return true;
		}
		else if (def->sequence.altref < MAXMUSNAMES)
		{
			// Alt music requires unlocking the alt
			if ((header->cache_muslock[def->sequence.altref - 1] < MAXUNLOCKABLES)
			&& gamedata->unlocked[header->cache_muslock[def->sequence.altref - 1]] == false)
				return true;
		}
		else if (def->sequence.altref < MAXMUSNAMES*2)
		{
			// Encore!
			if (M_SecretUnlocked(SECRET_ENCORE, true) == false)
				return true;

			// Side B of the same CD
			if (def->sequence.altref > MAXMUSNAMES
			&& (header->cache_muslock[def->sequence.altref - (1 + MAXMUSNAMES)] < MAXUNLOCKABLES)
			&& gamedata->unlocked[header->cache_muslock[def->sequence.altref - (1 + MAXMUSNAMES)]] == false)
				return true;
		}
	}

	// Finally, do a full-fat map check.
	return M_MapLocked(def->sequence.map+1);
}

void S_UpdateSoundTestDef(boolean reverse, boolean dotracks, boolean skipnull)
{
	musicdef_t *newdef = NULL;

	if (reverse == false)
	{
		// Track update
		if (dotracks == true && soundtest.current != NULL
			&& soundtest.currenttrack < soundtest.current->numtracks-1)
		{
			soundtest.currenttrack++;
			goto updatetrackonly;
		}

		if (soundtest.shuffle == true && soundtest.sequence.shuffleinfo == 0)
		{
			// The shuffle data isn't initialised.
			// Count the valid set of musicdefs we can randomly select from!
			// This will later liberally be passed to M_RandomKey.

			newdef = soundtest.sequence.next;
			while (newdef != NULL)
			{
				if (S_SoundTestDefLocked(newdef) == false)
				{
					newdef->sequence.shuffleinfo = 0;
					soundtest.sequence.shuffleinfo++;
				}
				else
				{
					// Don't permit if it gets unlocked before shuffle count gets reset
					newdef->sequence.shuffleinfo = (size_t)-1;
				}
				newdef->sequence.shufflenext = NULL;

				newdef = newdef->sequence.next;
			}
			soundtest.sequence.shufflenext = NULL;
		}

		if (soundtest.shuffle == true)
		{
			// Do we have it cached..?
			newdef = soundtest.current != NULL
				? soundtest.current->sequence.shufflenext
				: soundtest.sequence.shufflenext;

			if (newdef != NULL)
				;
			else if (soundtest.sequence.shuffleinfo != 0)
			{
				// Nope, not cached. Grab a random entry and hunt for it.
				size_t shuffleseek = M_RandomKey(soundtest.sequence.shuffleinfo);
				size_t shuffleseekcopy = shuffleseek;

				// Since these are sequential, we can sometimes
				// get a small benefit by starting partway down the list.
				if (
					soundtest.current != NULL
					&& soundtest.current->sequence.shuffleinfo != 0
					&& soundtest.current->sequence.shuffleinfo <= shuffleseek
				)
				{
					newdef = soundtest.current;
					shuffleseek -= (soundtest.current->sequence.shuffleinfo - 1);
				}
				else
				{
					newdef = soundtest.sequence.next;
				}

				// ...yeah, though, this is basically O(n). I could provide a
				// great many excuses, but the basic impetus is that I saw
				// a thread on an open-source software development forum where,
				// since 2014, a parade of users have been asking for the same
				// basic QoL feature and been consecutively berated by one developer
				// extremely against the idea of implmenting something imperfect.
				// I have enough self-awareness as a programmer to recognise that
				// that is a chronic case of "PROGRAMMER BRAIN". Sometimes you
				// just need to do a feature "badly" because it's more important
				// for it to exist at all than to channel mathematical elegance.
				// ~toast 220923

				for (; newdef != NULL; newdef = newdef->sequence.next)
				{
					if (newdef->sequence.shuffleinfo != 0)
						continue;

					if (S_SoundTestDefLocked(newdef) == true)
						continue;

					if (shuffleseek != 0)
					{
						shuffleseek--;
						continue;
					}
					break;
				}

				if (newdef == NULL)
				{
					// Fell short!? Try again later
					soundtest.sequence.shuffleinfo = 0;
				}
				else
				{
					// Don't select the same entry twice
					if (soundtest.sequence.shuffleinfo)
						soundtest.sequence.shuffleinfo--;

					// One-indexed so the first shuffled entry has a valid shuffleinfo
					newdef->sequence.shuffleinfo = shuffleseekcopy+1;

					// Link it to the end of the chain
					if (soundtest.current && soundtest.current->sequence.shuffleinfo != 0)
					{
						soundtest.current->sequence.shufflenext = newdef;
					}
					else
					{
						soundtest.sequence.shufflenext = newdef;
					}
				}
			}
		}
		else
		{
			// Just blaze through the musicdefs
			newdef = (soundtest.current != NULL)
				? soundtest.current->sequence.next
				: soundtest.sequence.next;
			while (newdef != NULL && S_SoundTestDefLocked(newdef))
				newdef = newdef->sequence.next;

			if (newdef == NULL && skipnull == true)
			{
				newdef = soundtest.sequence.next;
				while (newdef != NULL && S_SoundTestDefLocked(newdef))
					newdef = newdef->sequence.next;
			}
		}
	}
	else
	{
		// Everything in this case is doing a full-on O(n) search
		// for the previous entry in one of two singly linked lists.
		// I know there are better solutions. It basically boils
		// down to the fact that this code only runs on direct user
		// input on a menu, never in the background, and therefore
		// is straight up less important than the forwards direction.

		musicdef_t *def, *lastdef = NULL;

		// Track update
		if (dotracks == true && soundtest.current != NULL
			&& soundtest.currenttrack > 0)
		{
			soundtest.currenttrack--;
			goto updatetrackonly;
		}

		if (soundtest.shuffle && soundtest.current != NULL)
		{
			// Basically identical structure to the sequence.next case... templates might be cool one day

			if (soundtest.sequence.shufflenext == soundtest.current)
				;
			else for (def = soundtest.sequence.shufflenext; def; def = def->sequence.shufflenext)
			{
				if (!S_SoundTestDefLocked(def))
				{
					lastdef = def;
				}

				if (def->sequence.shufflenext != soundtest.current)
				{
					continue;
				}

				newdef = lastdef;
				break;
			}

			goto updatecurrent;
		}

		soundtest.shuffle = false;
		soundtest.sequence.shuffleinfo = 0;

		if (soundtest.current == soundtest.sequence.next
			&& skipnull == false)
		{
			goto updatecurrent;
		}

		for (def = soundtest.sequence.next; def; def = def->sequence.next)
		{
			if (!S_SoundTestDefLocked(def))
			{
				lastdef = def;
			}

			if (def->sequence.next != soundtest.current)
			{
				continue;
			}

			newdef = lastdef;
			break;
		}
	}

updatecurrent:
	soundtest.current = newdef;
	soundtest.currenttrack =
		(reverse == true && dotracks == true && newdef != NULL)
			? newdef->numtracks-1
			: 0;

	if (newdef == NULL)
	{
		CV_SetValue(&cv_soundtest, 0);
	}

updatetrackonly:
	if (soundtest.playing == true)
	{
		S_SoundTestPlay();
	}
}

const char *S_SoundTestTune(UINT8 invert)
{
	return soundtest.tune ^ invert ? "stereo_fade" : "stereo";
}

boolean S_SoundTestCanSequenceFade(void)
{
	return soundtest.current->basenoloop[soundtest.currenttrack] == false;
}

static void S_SoundTestReconfigure(const char *tune)
{
	Music_Remap(tune, soundtest.current->name[soundtest.currenttrack]);
	Music_Play(tune);
}

void S_SoundTestPlay(void)
{
	UINT32 sequencemaxtime = 0;

	if (soundtest.current == NULL)
	{
		S_SoundTestStop();
		return;
	}

	soundtest.playing = true;
	soundtest.tune = (soundtest.autosequence == true && S_SoundTestCanSequenceFade() == true);

	S_SoundTestReconfigure("stereo");
	S_SoundTestReconfigure("stereo_fade");

	// Assuming this song is now actually playing
	sequencemaxtime = I_GetSongLength();

	if (sequencemaxtime == 0)
	{
		S_SoundTestStop();
		return;
	}

	// Does song have default loop?
	if (S_SoundTestCanSequenceFade() == true)
	{
		// I'd personally like songs in sequence to last between 3 and 6 minutes.
		if (sequencemaxtime < 3*60*1000)
		{
			const UINT32 looppoint = I_GetSongLoopPoint();
			const UINT32 loopduration =
				(looppoint < sequencemaxtime)
				? sequencemaxtime - looppoint
				: 0;

			if (!loopduration)
				;
			else do
			{
				sequencemaxtime += loopduration;
			} while (sequencemaxtime < 4*1000);
			// If the track is EXTREMELY short, keep adding until about 4s!
		}
	}

	// Only the last track fades out... but we still use stereo_fade to handle stopping.
	if (soundtest.currenttrack == soundtest.current->numtracks-1)
	{
		Music_SetFadeOut("stereo_fade", 5000);
	}
	else
	{
		Music_SetFadeOut("stereo_fade", 0);
	}

	Music_DelayEnd(
		S_SoundTestCanSequenceFade() ? "stereo_fade" : "stereo",
		(TICRATE*sequencemaxtime)/1000 // ms to TICRATE conversion
	);

	Music_Suspend(S_SoundTestTune(1));
}

void S_SoundTestStop(void)
{
	if (soundtest.playing == false)
	{
		return;
	}

	soundtest.tune = 0;

	soundtest.playing = false;
	soundtest.autosequence = false;
	soundtest.shuffle = false;
	soundtest.sequence.shuffleinfo = 0;

	Music_Stop("stereo");
	Music_Stop("stereo_fade");
}

void S_SoundTestTogglePause(void)
{
	if (soundtest.playing == false)
	{
		return;
	}

	const char *tune = S_SoundTestTune(0);
	if (Music_Paused(tune))
	{
		Music_UnPause(tune);
	}
	else
	{
		Music_Pause(tune);
	}
}

void S_TickSoundTest(void)
{
	if (soundtest.playing == false || soundtest.current == NULL)
	{
		// Nothing worth discussing.
		return;
	}

	if (I_SongPlaying() == false)
	{
		// We stopped for some reason. Accomodate this.
		goto handlenextsong;
	}

	if (Music_DurationLeft(S_SoundTestTune(0)) == 0)
	{
		goto handlenextsong;
	}

	return;

handlenextsong:
	// If the song's stopped while not in autosequence, stop visibly playing.
	if (soundtest.autosequence == false)
	{
		S_SoundTestStop();
		return;
	}

	// Okay, this is autosequence in action.
	S_UpdateSoundTestDef(false, true, true);
}

//
// S_FindMusicDef
//
// Find music def by 6 char name
//
musicdef_t *S_FindMusicDef(const char *name, UINT8 *i)
{
	UINT32 hash;
	musicdef_t *def;

	if (!name || !name[0])
		return NULL;

	hash = quickncasehash (name, 6);

	for (def = musicdefstart; def; def = def->next)
	{
		for (*i = 0; *i < def->numtracks; (*i)++)
		{
			if (hash != def->hash[*i])
				continue;

			if (stricmp(def->name[*i], name))
				continue;

			return def;
		}
	}

	*i = 0;
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
		value = strtok(NULL, " ,");
		if (!value)
		{
			return MusicDefError(CONS_WARNING,
					"Field '%'s is missing name.",
					stoken, lumpnum, line);
		}
		else
		{
			UINT8 i = 0;

			def = S_FindMusicDef(value, &i);

			// Nothing found, add to the end.
			if (!def)
			{
				def = Z_Calloc(sizeof (musicdef_t), PU_STATIC, NULL);

				do {
					if (i >= MAXDEFTRACKS)
						break;
					if (value[0] == '\\')
					{
						def->basenoloop[i] = true;
						value++;
					}
					STRBUFCPY(def->name[i], value);
					strlwr(def->name[i]);
					def->hash[i] = quickncasehash (def->name[i], 6);
					i++;
				} while ((value = strtok(NULL," ,")) != NULL);

				if (value != NULL)
				{
					return MusicDefError(CONS_ERROR,
							"Extra tracks for field '%s' beyond 3 discarded.", // MAXDEFTRACKS
							stoken, lumpnum, line);
				}

				def->numtracks = i;
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
			else if (!stricmp(stoken, "important"))
			{
				textline[0] = toupper(textline[0]);
				def->important = (textline[0] == 'Y' || textline[0] == 'T' || textline[0] == '1');
			}
			else if (!stricmp(stoken, "contentidunsafe"))
			{
				textline[0] = toupper(textline[0]);
				def->contentidunsafe = (textline[0] == 'Y' || textline[0] == 'T' || textline[0] == '1');
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
	S_PopulateSoundTestSequence();
}

//
// S_LoadMusicCredit
//
// Load the current song's credit into memory
//
void S_LoadMusicCredit(void)
{
	UINT8 i = 0;
	musicdef_t *def = S_FindMusicDef(Music_CurrentSong(), &i);

	char credittext[128] = "";
	char *work = NULL;
	size_t len = 128, worklen;
	INT32 widthused = (3*BASEVIDWIDTH/4) - 7, workwidth;

	S_UnloadMusicCredit();

	if (!def) // No definitions
		return;

	if (!def->title)
		return;

	work = va("\x1F %s", def->title);
	worklen = strlen(work);
	if (worklen <= len)
	{
		strncat(credittext, work, len);
		len -= worklen;

		if (def->numtracks > 1)
		{
			work = va(" (%c)", i+'A');
			worklen = strlen(work);
			if (worklen <= len)
			{
				strncat(credittext, work, len);
				len -= worklen;
			}
		}

		widthused -= V_ThinStringWidth(credittext, 0);

#define MUSICCREDITAPPEND(field, force)\
		if (field)\
		{\
			work = va(" - %s", field);\
			worklen = strlen(work);\
			if (worklen <= len)\
			{\
				workwidth = V_ThinStringWidth(work, 0);\
				if (force || widthused >= workwidth)\
				{\
					strncat(credittext, work, len);\
					len -= worklen;\
					widthused -= workwidth;\
				}\
			}\
		}

		MUSICCREDITAPPEND(def->author, true);
		MUSICCREDITAPPEND(def->source, false);

#undef MUSICCREDITAPPEND
	}

	if (credittext[0] == '\0')
		return;

	g_realsongcredit = Z_StrDup(credittext);
}

void S_UnloadMusicCredit(void)
{
	Z_Free(g_realsongcredit);
	g_realsongcredit = NULL;
}

//
// S_ShowMusicCredit
//
// Display current song's credit on screen
//
void S_ShowMusicCredit(void)
{
	if (!cv_songcredits.value)
		return;

	if (!g_realsongcredit)
	{
		// Like showing a blank credit.
		S_StopMusicCredit();
		return;
	}

	Z_Free(cursongcredit.text);
	cursongcredit.text = Z_StrDup(g_realsongcredit);
	cursongcredit.anim = 5*TICRATE;
	cursongcredit.x = cursongcredit.old_x = 0;
	cursongcredit.trans = NUMTRANSMAPS;
	cursongcredit.use_credits_offset = (gamestate == GS_CREDITS || (demo.playback && demo.attract == DEMO_ATTRACT_CREDITS));
}

void S_StopMusicCredit(void)
{
	Z_Free(cursongcredit.text);
	memset(&cursongcredit,0,sizeof(struct cursongcredit));
}

/// ------------------------
/// Music Status
/// ------------------------

boolean S_MusicDisabled(void)
{
	return digital_disabled;
}

boolean S_MusicNotInFocus(void)
{
	return (
			( window_notinfocus && ! (cv_bgaudio.value & 1) )
	);
}

/// ------------------------
/// Music Playback
/// ------------------------

//
// Stop and resume music, during game PAUSE.
//
void S_PauseAudio(void)
{
	Music_PauseAll();
}

void S_ResumeAudio(void)
{
	if (S_MusicNotInFocus())
		return;

	Music_UnPauseAll();
}

void S_SetMusicVolume(void)
{
	actualdigmusicvolume = cv_digmusicvolume.value;
	I_SetMusicVolume(actualdigmusicvolume);
}

/// ------------------------
/// Init & Others
/// ------------------------

static inline void PrintMusicDefField(const char *label, const char *field)
{
	if (field)
	{
		CONS_Printf("%s%s\n", label, field);
	}
}

static void PrintSongAuthors(const musicdef_t *def, UINT8 i)
{
	if (def->numtracks > 1)
	{
		PrintMusicDefField("Title:  ", va("%s (%c)", def->title, i+'A'));
	}
	else
	{
		PrintMusicDefField("Title:  ", def->title);
	}
	PrintMusicDefField("Author: ", def->author);

	CONS_Printf("\n");

	PrintMusicDefField("Original Source:    ", def->source);
	PrintMusicDefField("Original Composers: ", def->composers);
}

static void PrintMusicDef(const char *song)
{
	UINT8 i = 0;
	const musicdef_t *def = S_FindMusicDef(song, &i);

	if (def != NULL)
	{
		PrintSongAuthors(def, i);
	}
}

// TODO: fix this function, needs better support for map names
static void Command_Tunes_f(void)
{
	const char *tunearg;
	const size_t argc = COM_Argc();

	if (argc < 2) //tunes slot ...
	{
		CONS_Printf("tunes <name> [speed] [position] / <-show> / <-showdefault> / <-default> / <-none>:\n");
		CONS_Printf(M_GetText("Play an arbitrary music lump.\n\n"));
		CONS_Printf(M_GetText("* With \"-show\", shows the currently playing tune and track.\n"));
		CONS_Printf(M_GetText("* With \"-showdefault\", shows the current music for the level.\n"));
		CONS_Printf(M_GetText("* With \"-default\", returns to the default music for the map.\n"));
		CONS_Printf(M_GetText("* With \"-none\", any music playing will be stopped.\n"));
		return;
	}

	tunearg = COM_Argv(1);

	if (!strcasecmp(tunearg, "-show"))
	{
		CONS_Printf(M_GetText("The current tune is: %s\n"), Music_CurrentSong());
		PrintMusicDef(Music_CurrentSong());
		return;
	}

	if (!strcasecmp(tunearg, "-showdefault"))
	{
		CONS_Printf(M_GetText("The default tune is: %s\n"), Music_Song("level"));
		PrintMusicDef(Music_Song("level"));
		return;
	}

	S_SoundTestStop();

	if (!strcasecmp(tunearg, "-none"))
	{
		Music_Remap("stereo", "");
		Music_Play("stereo");
		return;
	}

	if (!strcasecmp(tunearg, "-default"))
	{
		Music_Stop("stereo");
		return;
	}

	Music_Remap("stereo", tunearg);
	Music_Loop("stereo", true);
	Music_Play("stereo");

	if (argc > 3)
		Music_Seek("stereo", atoi(COM_Argv(3)));

	if (argc > 2)
	{
		float speed = (float)atof(COM_Argv(2));
		if (speed > 0.0f)
			I_SetSongSpeed(speed);
	}
}

static void Command_RestartAudio_f(void)
{
	if (dedicated)  // No point in doing anything if game is a dedicated server.
		return;

	Music_StopAll();
	S_StopSounds();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_StartupSound();
	I_InitMusic();

// These must be called or no sound and music until manually set.

	S_SetSfxVolume();
	S_SetMusicVolume();
	S_SetMasterVolume();

	S_AttemptToRestoreMusic();
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

static void Got_PlaySound(const UINT8 **cp, INT32 playernum)
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

static void Command_MusicDef_f(void)
{
	const char *arg1 = COM_Argv(1);
	const char *arg2 = COM_Argv(2);

	enum {
		CMD_VOLUME,
		CMD_SHOW,
	} cmd;

	musicdef_t *def;

	if (!stricmp(arg1, "-volume"))
	{
		cmd = CMD_VOLUME;
	}
	else if (!stricmp(arg1, "-show"))
	{
		cmd = CMD_SHOW;
	}
	else
	{
		CONS_Printf(
				"\nmusicdef -volume <volume>\n"
				"    Change the volume for the current song.\n"
				"    Changes are saved while the game is open.\n"
				"    Hint: turn on devmode music too!\n"
				"\nmusicdef -show\n"
				"    Print a list of changed musicdefs.\n"
		);
		return;
	}

	switch (cmd)
	{
		case CMD_VOLUME:
			if (!strcmp(arg2, ""))
			{
				CONS_Printf("musicdef %s: missing argument\n", arg1);
				return;
			}

			// This command uses the current musicdef
			{
				UINT8 i = 0;

				def = S_FindMusicDef(Music_CurrentSong(), &i);
				def->debug_volume = atoi(arg2);
				I_SetCurrentSongVolume(def->debug_volume);

				CONS_Printf("Changed %s", def->name[0]);

				for (i = 1; i < def->numtracks; ++i)
				{
					CONS_Printf(", %s", def->name[i]);
				}

				CONS_Printf("\n");
			}
			break;

		case CMD_SHOW:
			for (def = musicdefstart; def; def = def->next)
			{
				if (def->debug_volume != 0)
				{
					UINT8 i;

					CONS_Printf("Lump %s", def->name[0]);

					for (i = 1; i < def->numtracks; ++i)
					{
						CONS_Printf(", %s", def->name[i]);
					}

					CONS_Printf(
							"\n"
							"Volume = %d\n"
							"\n",
							def->debug_volume
					);
				}
			}
			break;

		default:
			I_Assert(false);
	}
}

void GameSounds_OnChange(void);
void GameSounds_OnChange(void)
{
	if (M_CheckParm("-nosound") || M_CheckParm("-noaudio"))
		return;

	if (cv_gamesounds.value != sound_disabled)
		return;

	if (sound_disabled)
	{
		sound_disabled = false;
		I_StartupSound(); // will return early if initialised
		S_InitSfxChannels();
		S_StartSound(NULL, sfx_strpst);
	}
	else
	{
		sound_disabled = true;
		S_StopSounds();
	}
}

void GameDigiMusic_OnChange(void);
void GameDigiMusic_OnChange(void)
{
	if (M_CheckParm("-nomusic") || M_CheckParm("-noaudio"))
		return;
	else if (M_CheckParm("-nodigmusic"))
		return;

	if (cv_gamedigimusic.value != digital_disabled)
		return;

	if (digital_disabled)
	{
		digital_disabled = false;
		I_StartupSound(); // will return early if initialised
		I_InitMusic();
	}
	else
	{
		digital_disabled = true;
		I_UnloadSong();
		Music_Flip();
	}
}

void VoiceSelfDeafen_OnChange(void);
void weaponPrefChange(INT32 ssplayer);
void VoiceSelfDeafen_OnChange(void)
{
	if (M_CheckParm("-novoice") || M_CheckParm("-noaudio"))
		return;

	g_voice_disabled = cv_voice_selfdeafen.value;

	weaponPrefChange(0);
}

void BGAudio_OnChange(void);
void BGAudio_OnChange(void)
{
	if (window_notinfocus)
	{
		if (cv_bgaudio.value & 1)
			I_SetMusicVolume(0);
		else
			S_SetMusicVolume();
	}

	if (!cv_gamesounds.value)
		return;

	if (window_notinfocus && !(cv_bgaudio.value & 2))
		S_StopSounds();

	if (window_notinfocus && !(cv_bgaudio.value & 4))
		g_voice_disabled = true;
}


boolean S_SoundInputIsEnabled(void)
{
	return I_SoundInputIsEnabled();
}

boolean S_SoundInputSetEnabled(boolean enabled)
{
	return I_SoundInputSetEnabled(enabled);
}

UINT32 S_SoundInputDequeueSamples(void *data, UINT32 len)
{
	return I_SoundInputDequeueSamples(data, len);
}

UINT32 S_SoundInputRemainingSamples(void)
{
	return I_SoundInputRemainingSamples();
}

static INT32 g_playerlastvoiceactive[MAXPLAYERS];

void S_QueueVoiceFrameFromPlayer(INT32 playernum, void *data, UINT32 len, boolean terminal)
{
	if (dedicated)
	{
		return;
	}
	if (cv_voice_selfdeafen.value != 1 && !g_voice_disabled)
	{
		I_QueueVoiceFrameFromPlayer(playernum, data, len, terminal);
	}
}

void S_SetPlayerVoiceActive(INT32 playernum)
{
	g_playerlastvoiceactive[playernum] = I_GetTime();
}

boolean S_IsPlayerVoiceActive(INT32 playernum)
{
	return I_GetTime() - g_playerlastvoiceactive[playernum] < 5;
}

void S_ResetVoiceQueue(INT32 playernum)
{
	if (dedicated)
	{
		return;
	}
	I_ResetVoiceQueue(playernum);
	g_playerlastvoiceactive[playernum] = 0;
}
