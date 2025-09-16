// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  deh_soc.c
/// \brief Load SOC file and change tables and text

#include "modp_b64/modp_b64.h"

#include "doomdef.h"
#include "d_main.h" // for srb2home
#include "g_game.h"
#include "sounds.h"
#include "info.h"
#include "d_think.h"
#include "m_argv.h"
#include "z_zone.h"
#include "w_wad.h"
#include "y_inter.h"
#include "k_menu.h"
#include "m_misc.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "i_system.h"
#include "p_setup.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_draw.h"
#include "r_picformats.h"
#include "r_things.h" // R_Char2Frame
#include "r_sky.h"
#include "s_sound.h"
#include "fastcmp.h"
#include "lua_script.h" // Reluctantly included for LUA_EvalMath
#include "d_clisrv.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
#endif

#include "m_cond.h"
#include "m_pw_hash.h"

#include "dehacked.h"
#include "deh_soc.h"
#include "deh_lua.h" // included due to some LUA_SetLuaAction hack smh
#include "deh_tables.h"

// SRB2Kart
#include "filesrch.h" // refreshdirmenu
#include "k_follower.h"
#include "doomstat.h" // MAXMUSNAMES
#include "discord.h"

// Loops through every constant and operation in word and performs its calculations, returning the final value.
fixed_t get_number(const char *word)
{
	return LUA_EvalMath(word);

	/*// DESPERATELY NEEDED: Order of operations support! :x
	fixed_t i = find_const(&word);
	INT32 o;
	while(*word) {
		o = operation_pad(&word);
		if (o != -1)
			i = OPERATIONS[o].v(i,find_const(&word));
		else
			break;
	}
	return i;*/
}

#define PARAMCHECK(n) do { if (!params[n]) { deh_warning("Too few parameters, need %d", n); return; }} while (0)

#define EXTENDEDPARAMCHECK(spos, n) do { if (!spos || !(*spos)) { deh_warning("Missing extended parameter, need at least %d", n); return; }} while (0)

/* ======================================================================== */
// Load a dehacked file format
/* ======================================================================== */
/* a sample to see
                   Thing 1 (Player)       {           // MT_PLAYER
INT32 doomednum;     ID # = 3232              -1,             // doomednum
INT32 spawnstate;    Initial frame = 32       "PLAY",         // spawnstate
INT32 spawnhealth;   Hit points = 3232        100,            // spawnhealth
INT32 seestate;      First moving frame = 32  "PLAY_RUN1",    // seestate
INT32 seesound;      Alert sound = 32         sfx_None,       // seesound
INT32 reactiontime;  Reaction time = 3232     0,              // reactiontime
INT32 attacksound;   Attack sound = 32        sfx_None,       // attacksound
INT32 painstate;     Injury frame = 32        "PLAY_PAIN",    // painstate
INT32 painchance;    Pain chance = 3232       255,            // painchance
INT32 painsound;     Pain sound = 32          sfx_plpain,     // painsound
INT32 meleestate;    Close attack frame = 32  "NULL",         // meleestate
INT32 missilestate;  Far attack frame = 32    "PLAY_ATK1",    // missilestate
INT32 deathstate;    Death frame = 32         "PLAY_DIE1",    // deathstate
INT32 xdeathstate;   Exploding frame = 32     "PLAY_XDIE1",   // xdeathstate
INT32 deathsound;    Death sound = 32         sfx_pldeth,     // deathsound
INT32 speed;         Speed = 3232             0,              // speed
INT32 radius;        Width = 211812352        16*FRACUNIT,    // radius
INT32 height;        Height = 211812352       56*FRACUNIT,    // height
INT32 dispoffset;    DispOffset = 0           0,              // dispoffset
INT32 mass;          Mass = 3232              100,            // mass
INT32 damage;        Missile damage = 3232    0,              // damage
INT32 activesound;   Action sound = 32        sfx_None,       // activesound
INT32 flags;         Bits = 3232              MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
INT32 raisestate;    Respawn frame = 32       S_NULL          // raisestate
                                         }, */

#ifdef HWRENDER
static INT32 searchvalue(const char *s)
{
	while (s[0] != '=' && s[0])
		s++;
	if (s[0] == '=')
		return atoi(&s[1]);
	else
	{
		deh_warning("No value found");
		return 0;
	}
}

static float searchfvalue(const char *s)
{
	while (s[0] != '=' && s[0])
		s++;
	if (s[0] == '=')
		return (float)atof(&s[1]);
	else
	{
		deh_warning("No value found");
		return 0;
	}
}
#endif

// These are for clearing all of various things
void clear_emblems(void)
{
	INT32 i;

	for (i = 0; i < MAXEMBLEMS; ++i)
	{
		Z_Free(emblemlocations[i].level);
		emblemlocations[i].level = NULL;

		Z_Free(emblemlocations[i].stringVar);
		emblemlocations[i].stringVar = NULL;

		Z_Free(emblemlocations[i].stringVar2);
		emblemlocations[i].stringVar2 = NULL;
	}

	memset(&emblemlocations, 0, sizeof(emblemlocations));
	numemblems = 0;
}

void clear_unlockables(void)
{
	INT32 i;

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		Z_Free(unlockables[i].stringVar);
		unlockables[i].stringVar = NULL;

		Z_Free(unlockables[i].icon);
		unlockables[i].icon = NULL;
	}

	memset(&unlockables, 0, sizeof(unlockables));
}

void clear_conditionsets(void)
{
	UINT16 i;
	for (i = 0; i < MAXCONDITIONSETS; ++i)
		M_ClearConditionSet(i);
}

void clear_levels(void)
{
	// This is potentially dangerous but if we're resetting these headers,
	// we may as well try to save some memory, right?
	while (nummapheaders > 0)
	{
		nummapheaders--;

		if (!mapheaderinfo[nummapheaders])
			continue;

		// Custom map header info
		// (no need to set num to 0, we're freeing the entire header shortly)
		Z_Free(mapheaderinfo[nummapheaders]->customopts);

		P_DeleteHeaderFollowers(nummapheaders);

		Patch_Free(mapheaderinfo[nummapheaders]->thumbnailPic);
		Patch_Free(mapheaderinfo[nummapheaders]->minimapPic);

		Z_Free(mapheaderinfo[nummapheaders]->lumpname);

		Z_Free(mapheaderinfo[nummapheaders]);
		mapheaderinfo[nummapheaders] = NULL;
	}

	// Clear out the cache
	{
		cupheader_t *cup = kartcupheaders;
		UINT8 i;

		while (cup)
		{
			for (i = 0; i < CUPCACHE_MAX; i++)
			{
				cup->cachedlevels[i] = NEXTMAP_INVALID;
			}
			cup = cup->next;
		}
	}

	if (gamedata && gamedata->spraycans)
	{
		UINT16 i;
		for (i = 0; i < gamedata->numspraycans; i++)
			gamedata->spraycans[i].map = NEXTMAP_INVALID;
	}

	// Exit the current gamemap as a safeguard
	if (Playing())
		COM_BufAddText("exitgame"); // Command_ExitGame_f() but delayed
}

// TODO: Figure out how to do undolines for this....
// TODO: Warnings for running out of freeslots
void readfreeslots(MYFILE *f)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word,*type;
	char *tmp;
	int i;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			type = strtok(s, "_");
			if (type)
				strupr(type);
			else
				break;

			word = strtok(NULL, "\n");
			if (word)
				strupr(word);
			else
				break;

			// TODO: Check for existing freeslot mobjs/states/etc. and make errors.
			// TODO: Out-of-slots warnings/errors.
			// TODO: Name too long (truncated) warnings.
			if (fastcmp(type, "SFX"))
			{
				CONS_Printf("Sound sfx_%s allocated.\n",word);
				S_AddSoundFx(word, false, 0, false);
			}
			else if (fastcmp(type, "SPR"))
			{
				for (i = SPR_FIRSTFREESLOT; i <= SPR_LASTFREESLOT; i++)
				{
					if (used_spr[(i-SPR_FIRSTFREESLOT)/8] & (1<<(i%8)))
					{
						if (!sprnames[i][4] && memcmp(sprnames[i],word,4)==0)
							sprnames[i][4] = (char)f->wad;
						continue; // Already allocated, next.
					}
					// Found a free slot!
					strncpy(sprnames[i],word,4);
					//sprnames[i][4] = 0;
					CONS_Printf("Sprite SPR_%s allocated.\n",word);
					used_spr[(i-SPR_FIRSTFREESLOT)/8] |= 1<<(i%8); // Okay, this sprite slot has been named now.
					break;
				}
				if (i > SPR_LASTFREESLOT)
					I_Error("Out of Sprite Freeslots while allocating \"%s\"\nLoad less addons to fix this.", word);

			}
			else if (fastcmp(type, "S"))
			{
				for (i = 0; i < NUMSTATEFREESLOTS; i++)
					if (!FREE_STATES[i]) {
						CONS_Printf("State S_%s allocated.\n",word);
						FREE_STATES[i] = Z_Malloc(strlen(word)+1, PU_STATIC, NULL);
						strcpy(FREE_STATES[i],word);
						freeslotusage[0][0]++;
						break;
					}
				if (i == NUMSTATEFREESLOTS)
					I_Error("Out of State Freeslots while allocating \"%s\"\nLoad less addons to fix this.", word);

			}
			else if (fastcmp(type, "MT"))
			{
				for (i = 0; i < NUMMOBJFREESLOTS; i++)
					if (!FREE_MOBJS[i]) {
						CONS_Printf("MobjType MT_%s allocated.\n",word);
						FREE_MOBJS[i] = Z_Malloc(strlen(word)+1, PU_STATIC, NULL);
						strcpy(FREE_MOBJS[i],word);
						freeslotusage[1][0]++;
						break;
					}
				if (i == NUMMOBJFREESLOTS)
					I_Error("Out of Mobj Freeslots while allocating \"%s\"\nLoad less addons to fix this.", word);

			}
			else if (fastcmp(type, "SKINCOLOR"))
			{
				for (i = 0; i < NUMCOLORFREESLOTS; i++)
					if (!FREE_SKINCOLORS[i]) {
						CONS_Printf("Skincolor SKINCOLOR_%s allocated.\n",word);
						FREE_SKINCOLORS[i] = Z_Malloc(strlen(word)+1, PU_STATIC, NULL);
						strcpy(FREE_SKINCOLORS[i],word);
						skincolors[SKINCOLOR_FIRSTFREESLOT+i].cache_spraycan = UINT16_MAX;
						numskincolors++;
						break;
					}
				if (i == NUMCOLORFREESLOTS)
					I_Error("Out of Skincolor Freeslots while allocating \"%s\"\nLoad less addons to fix this.", word);
			}
			else if (fastcmp(type, "SPR2"))
			{
				// Search if we already have an SPR2 by that name...
				for (i = SPR2_FIRSTFREESLOT; i < (int)free_spr2; i++)
					if (memcmp(spr2names[i],word,4) == 0)
						break;
				// We found it? (Two mods using the same SPR2 name?) Then don't allocate another one.
				if (i < (int)free_spr2)
					continue;
				// Copy in the spr2 name and increment free_spr2.
				if (free_spr2 < NUMPLAYERSPRITES) {
					strncpy(spr2names[free_spr2],word,4);
					spr2defaults[free_spr2] = 0;
					spr2names[free_spr2++][4] = 0;
				} else
					I_Error("Out of SPR2 Freeslots while allocating \"%s\"\nLoad less addons to fix this.", word);
			}
			else if (fastcmp(type, "TOL"))
			{
				// Search if we already have a typeoflevel by that name...
				for (i = 0; TYPEOFLEVEL[i].name; i++)
					if (fastcmp(word, TYPEOFLEVEL[i].name))
						break;

				// We found it? Then don't allocate another one.
				if (TYPEOFLEVEL[i].name)
					continue;

				// We don't, so freeslot it.
				if (lastcustomtol == (UINT32)MAXTOL) // Unless you have way too many, since they're flags.
					I_Error("Out of Typeoflevel Freeslots while allocating \"%s\"\nLoad less addons to fix this.", word);
				else
				{
					G_AddTOL(lastcustomtol, word);
					lastcustomtol <<= 1;
				}
			}
			else if (fastcmp(type, "PRECIP"))
			{
				// Search if we already have a PRECIP by that name...
				for (i = PRECIP_FIRSTFREESLOT; i < (int)precip_freeslot; i++)
					if (fastcmp(word, precipprops[i].name))
						break;

				// We found it? Then don't allocate another one.
				if (i < (int)precip_freeslot)
					continue;

				// We don't, so allocate a new one.
				if (precip_freeslot < MAXPRECIP)
				{
					precipprops[i].name = Z_StrDup(word);
					precip_freeslot++;
				} else
					I_Error("Out of Precipitation Freeslots while allocating \"%s\"\nLoad less addons to fix this.", word);
			}
			else
				deh_warning("Freeslots: unknown enum class '%s' for '%s_%s'", type, type, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readthing(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word, *word2;
	char *tmp;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			word2 = strtok(NULL, " = ");
			if (word2)
				strupr(word2);
			else
				break;
			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';

			if (fastcmp(word, "MAPTHINGNUM") || fastcmp(word, "DOOMEDNUM"))
			{
				mobjinfo[num].doomednum = (INT32)atoi(word2);
			}
			else if (fastcmp(word, "SPAWNSTATE"))
			{
				mobjinfo[num].spawnstate = get_number(word2);
			}
			else if (fastcmp(word, "SPAWNHEALTH"))
			{
				mobjinfo[num].spawnhealth = (INT32)get_number(word2);
			}
			else if (fastcmp(word, "SEESTATE"))
			{
				mobjinfo[num].seestate = get_number(word2);
			}
			else if (fastcmp(word, "SEESOUND"))
			{
				mobjinfo[num].seesound = get_number(word2);
			}
			else if (fastcmp(word, "REACTIONTIME"))
			{
				mobjinfo[num].reactiontime = (INT32)get_number(word2);
			}
			else if (fastcmp(word, "ATTACKSOUND"))
			{
				mobjinfo[num].attacksound = get_number(word2);
			}
			else if (fastcmp(word, "PAINSTATE"))
			{
				mobjinfo[num].painstate = get_number(word2);
			}
			else if (fastcmp(word, "PAINCHANCE"))
			{
				mobjinfo[num].painchance = (INT32)get_number(word2);
			}
			else if (fastcmp(word, "PAINSOUND"))
			{
				mobjinfo[num].painsound = get_number(word2);
			}
			else if (fastcmp(word, "MELEESTATE"))
			{
				mobjinfo[num].meleestate = get_number(word2);
			}
			else if (fastcmp(word, "MISSILESTATE"))
			{
				mobjinfo[num].missilestate = get_number(word2);
			}
			else if (fastcmp(word, "DEATHSTATE"))
			{
				mobjinfo[num].deathstate = get_number(word2);
			}
			else if (fastcmp(word, "DEATHSOUND"))
			{
				mobjinfo[num].deathsound = get_number(word2);
			}
			else if (fastcmp(word, "XDEATHSTATE"))
			{
				mobjinfo[num].xdeathstate = get_number(word2);
			}
			else if (fastcmp(word, "SPEED"))
			{
				mobjinfo[num].speed = get_number(word2);
			}
			else if (fastcmp(word, "RADIUS"))
			{
				mobjinfo[num].radius = get_number(word2);
			}
			else if (fastcmp(word, "HEIGHT"))
			{
				mobjinfo[num].height = get_number(word2);
			}
			else if (fastcmp(word, "DISPOFFSET"))
			{
				mobjinfo[num].dispoffset = get_number(word2);
			}
			else if (fastcmp(word, "MASS"))
			{
				mobjinfo[num].mass = (INT32)get_number(word2);
			}
			else if (fastcmp(word, "DAMAGE"))
			{
				mobjinfo[num].damage = (INT32)get_number(word2);
			}
			else if (fastcmp(word, "ACTIVESOUND"))
			{
				mobjinfo[num].activesound = get_number(word2);
			}
			else if (fastcmp(word, "FLAGS"))
			{
				mobjinfo[num].flags = (INT32)get_number(word2);
			}
			else if (fastcmp(word, "RAISESTATE"))
			{
				mobjinfo[num].raisestate = get_number(word2);
			}
			else
				deh_warning("Thing %d: unknown word '%s'", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readskincolor(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word = s;
	char *word2;
	char *tmp;

	Color_cons_t[num].value = num;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;

			if (fastcmp(word, "NAME"))
			{
				char truncword[sizeof(skincolors[num].name)];
				size_t namesize = sizeof(truncword);
				UINT16 dupecheck;

				deh_strlcpy(truncword, word2, namesize, va("Skincolor %d: name", num)); // truncate here to check for dupes
				dupecheck = R_GetColorByName(truncword);
				if (truncword[0] != '\0' && (!stricmp(truncword, skincolors[SKINCOLOR_NONE].name) || (dupecheck && dupecheck != num)))
				{
					size_t lastchar = strlen(truncword);
					char oldword[sizeof(truncword)];
					char dupenum = '1';

					strlcpy(oldword, truncword, lastchar+1);
					lastchar--;
					if (lastchar == namesize-2) // exactly max length, replace last character with 0
						truncword[lastchar] = '0';
					else // append 0
					{
						strcat(truncword, "0");
						lastchar++;
					}

					while (R_GetColorByName(truncword))
					{
						truncword[lastchar] = dupenum;
						if (dupenum == '9')
							dupenum = 'A';
						else if (dupenum == 'Z') // give up :?
							break;
						else
							dupenum++;
					}

					deh_warning("Skincolor %d: name %s is a duplicate of another skincolor's name - renamed to %s", num, oldword, truncword);
				}

				strlcpy(skincolors[num].name, truncword, namesize); // already truncated
			}
			else if (fastcmp(word, "RAMP"))
			{
				UINT8 i;
				tmp = strtok(word2,",");
				for (i = 0; i < COLORRAMPSIZE; i++) {
					skincolors[num].ramp[i] = (UINT8)get_number(tmp);
					if ((tmp = strtok(NULL,",")) == NULL)
						break;
				}
				skincolor_modified[num] = true;
			}
			else if (fastcmp(word, "INVCOLOR"))
			{
				UINT16 v = (UINT16)get_number(word2);
				if (v < numskincolors)
					skincolors[num].invcolor = v;
				else
					skincolors[num].invcolor = SKINCOLOR_GREEN;
			}
			else if (fastcmp(word, "INVSHADE"))
			{
				skincolors[num].invshade = get_number(word2)%COLORRAMPSIZE;
			}
			else if (fastcmp(word, "CHATCOLOR"))
			{
				skincolors[num].chatcolor = get_number(word2);
			}
			else if (fastcmp(word, "ACCESSIBLE"))
			{
				if (num > FIRSTSUPERCOLOR)
					skincolors[num].accessible = (boolean)(atoi(word2) || word2[0] == 'T' || word2[0] == 'Y');
			}
			else
				deh_warning("Skincolor %d: unknown word '%s'", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

#ifdef HWRENDER
void readlight(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *tmp;
	INT32 value;
	float fvalue;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			fvalue = searchfvalue(s);
			value = searchvalue(s);

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			if (fastcmp(word, "TYPE"))
			{
				lspr[num].type = (UINT16)value;
			}
			else if (fastcmp(word, "OFFSETX"))
			{
				lspr[num].light_xoffset = fvalue;
			}
			else if (fastcmp(word, "OFFSETY"))
			{
				lspr[num].light_yoffset = fvalue;
			}
			else if (fastcmp(word, "CORONACOLOR"))
			{
				lspr[num].corona_color = value;
			}
			else if (fastcmp(word, "CORONARADIUS"))
			{
				lspr[num].corona_radius = fvalue;
			}
			else if (fastcmp(word, "DYNAMICCOLOR"))
			{
				lspr[num].dynamic_color = value;
			}
			else if (fastcmp(word, "DYNAMICRADIUS"))
			{
				lspr[num].dynamic_radius = fvalue;

				/// \note Update the sqrradius! unnecessary?
				lspr[num].dynamic_sqrradius = fvalue * fvalue;
			}
			else
				deh_warning("Light %d: unknown word '%s'", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}
#endif // HWRENDER

void readsprite2(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word, *word2;
	char *tmp;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			word2 = strtok(NULL, " = ");
			if (word2)
				strupr(word2);
			else
				break;
			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';

			if (fastcmp(word, "DEFAULT"))
				spr2defaults[num] = get_number(word2);
			else
				deh_warning("Sprite2 %s: unknown word '%s'", spr2names[num], word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readgametype(MYFILE *f, char *gtname)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2, *word2lwr = NULL;
	char *tmp;
	INT32 i, j;

	gametype_t *newgametype = NULL;

	UINT32 newgtrules = 0;
	UINT32 newgttol = 0;
	INT32 newgtpointlimit = 0;
	INT32 newgttimelimit = 0;
	UINT8 newgtinttype = 0;
	SINT8 newgtspeed = KARTSPEED_AUTO; // KARTSPEED_EASY
	char gtconst[MAXLINELEN];
	char gppic[9];
	char gppicmini[9];

	// Empty strings.
	gtconst[0] = gppic[0] = gppicmini[0] = '\0';

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			word2 = strtok(NULL, " = ");
			if (word2)
			{
				if (!word2lwr)
					word2lwr = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
				strcpy(word2lwr, word2);
				strupr(word2);
			}
			else
				break;

			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';
			i = atoi(word2);

			// Game type rules
			if (fastcmp(word, "RULES"))
			{
				// GTR_
				newgtrules = (UINT32)get_number(word2);
			}
			// Identifier
			else if (fastcmp(word, "IDENTIFIER"))
			{
				// GT_
				strlcpy(gtconst, word2, MAXLINELEN);
			}
			else if (fastcmp(word, "GPPIC"))
			{
				strlcpy(gppic, word2, 9);
			}
			else if (fastcmp(word, "GPPICMINI"))
			{
				strlcpy(gppicmini, word2, 9);
			}
			// Point and time limits
			else if (fastcmp(word, "DEFAULTPOINTLIMIT"))
				newgtpointlimit = (INT32)i;
			else if (fastcmp(word, "DEFAULTTIMELIMIT"))
				newgttimelimit = (INT32)i;
			// Intermission type
			else if (fastcmp(word, "INTERMISSIONTYPE"))
			{
				// Case sensitive
				newgtinttype = (int)get_number(word2lwr);
			}
			else if (fastcmp(word, "GAMESPEED"))
			{
				if (fasticmp(word2, "EASY"))
				{
					newgtspeed = KARTSPEED_EASY;
				}
				else if (fasticmp(word2, "NORMAL"))
				{
					newgtspeed = KARTSPEED_NORMAL;
				}
				else if (fasticmp(word2, "HARD"))
				{
					newgtspeed = KARTSPEED_HARD;
				}
				else if (fasticmp(word2, "ANY"))
				{
					newgtspeed = KARTSPEED_AUTO;
				}
				else
				{
					deh_warning("readgametype %s: unknown gamespeed name %s\n", gtname, word2);
				}
			}
			// Type of level
			else if (fastcmp(word, "TYPEOFLEVEL"))
			{
				if (i) // it's just a number
					newgttol = (UINT32)i;
				else
				{
					UINT32 tol = 0;
					tmp = strtok(word2,",");
					do {
						for (i = 0; TYPEOFLEVEL[i].name; i++)
							if (fasticmp(tmp, TYPEOFLEVEL[i].name))
								break;
						if (!TYPEOFLEVEL[i].name)
							deh_warning("readgametype %s: unknown typeoflevel flag %s\n", gtname, tmp);
						tol |= TYPEOFLEVEL[i].flag;
					} while((tmp = strtok(NULL,",")) != NULL);
					newgttol = tol;
				}
			}
			// The SOC probably provided gametype rules as words,
			// instead of using the RULES keyword.
			// Like for example "NOSPECTATORSPAWN = TRUE".
			// This is completely valid, and looks better anyway.
			else
			{
				UINT32 wordgt = 0;
				for (j = 0; GAMETYPERULE_LIST[j]; j++)
					if (fastcmp(word, GAMETYPERULE_LIST[j])) {
						wordgt |= (1<<j);
						if (i || word2[0] == 'T' || word2[0] == 'Y')
							newgtrules |= wordgt;
						break;
					}
				if (!wordgt)
					deh_warning("readgametype %s: unknown word '%s'", gtname, word);
			}
		}
	} while (!myfeof(f)); // finish when the line is empty

	// Free strings.
	Z_Free(s);
	if (word2lwr)
		Z_Free(word2lwr);

	// Ran out of gametype slots
	if (numgametypes == GT_LASTFREESLOT)
	{
		I_Error("Out of Gametype Freeslots while allocating \"%s\"\nLoad less addons to fix this.", gtname);
	}

	if (gtname[0] == '\0')
	{
		deh_warning("Custom gametype must have a name");
		return;
	}

	if (strlen(gtname) >= MAXGAMETYPELENGTH)
	{
		deh_warning("Custom gametype \"%s\"'s name must be %d long at most", gtname, MAXGAMETYPELENGTH-1);
		return;
	}

	for (i = 0; i < numgametypes; i++)
		if (fastcmp(gtname, gametypes[i]->name))
			break;

	if (i < numgametypes)
	{
		deh_warning("Custom gametype \"%s\"'s name is already in use", gtname);
		return;
	}

	// Add the new gametype
	newgametype = Z_Calloc(sizeof (gametype_t), PU_STATIC, NULL);
	if (!newgametype)
	{
		I_Error("Out of memory allocating gametype \"%s\"", gtname);
	}

	if (gtconst[0] == '\0')
		strncpy(gtconst, gtname, MAXLINELEN);

	newgametype->name = Z_StrDup((const char *)gtname);
	newgametype->rules = newgtrules;
	newgametype->constant = G_PrepareGametypeConstant((const char *)gtconst);
	strlcpy(newgametype->gppic, gppic, 9);
	strlcpy(newgametype->gppicmini, gppicmini, 9);
	newgametype->tol = newgttol;
	newgametype->intermission = newgtinttype;
	newgametype->pointlimit = newgtpointlimit;
	newgametype->timelimit = newgttimelimit;
	newgametype->speed = newgtspeed;

	gametypes[numgametypes++] = newgametype;

	CONS_Printf("Added gametype %s\n", gtname);
}

static mapheader_lighting_t *usemaplighting(INT32 mapnum, const char *word)
{
	if (fastncmp(word, "ENCORE", 6))
	{
		mapheaderinfo[mapnum]->use_encore_lighting = true;

		return &mapheaderinfo[mapnum]->lighting_encore;
	}
	else
	{
		return &mapheaderinfo[mapnum]->lighting;
	}
}

void readlevelheader(MYFILE *f, char * name)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);

	char *word;
	char *word2;
	//char *word3; // Non-uppercase version of word2

	char *tmp;
	INT32 i;

	INT32 num = G_MapNumber(name);

	if (num >= nummapheaders)
	{
		P_AllocMapHeader((INT16)(num = nummapheaders));
	}
	else if (f->wad > mainwads)
	{
		// only mark as a major mod if it replaces an already-existing mapheaderinfo
		G_SetGameModified(multiplayer, true);
	}

	if (mapheaderinfo[num]->lumpname == NULL)
	{
		mapheaderinfo[num]->lumpnamehash = quickncasehash(name, MAXMAPLUMPNAME);

		// Check to see if we have any custom map record data that we could substitute in.
		unloaded_mapheader_t *unloadedmap, *prev = NULL;
		for (unloadedmap = unloadedmapheaders; unloadedmap; prev = unloadedmap, unloadedmap = unloadedmap->next)
		{
			if (unloadedmap->lumpnamehash != mapheaderinfo[num]->lumpnamehash)
				continue;

			if (strcasecmp(name, unloadedmap->lumpname) != 0)
				continue;

			// Copy in mapvisited, time, lap, etc.
			unloadedmap->records.mapvisited &= MV_MAX;
			M_Memcpy(&mapheaderinfo[num]->records, &unloadedmap->records, sizeof(recorddata_t));

			// Reuse the zone-allocated lumpname string.
			mapheaderinfo[num]->lumpname = unloadedmap->lumpname;

			// Remove this entry from the chain.
			if (prev)
			{
				prev->next = unloadedmap->next;
			}
			else
			{
				unloadedmapheaders = unloadedmap->next;
			}

			// Finally, free.
			Z_Free(unloadedmap);

			break;
		}

		if (mapheaderinfo[num]->lumpname == NULL)
		{
			// If there was no string to reuse, dup our own.
			mapheaderinfo[num]->lumpname = Z_StrDup(name);
		}
	}

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Set / reset word, because some things (Lua.) move it
			word = s;

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;
			i = atoi(word2); // used for numerical settings

			// The following support mixed/lower case.

			if (fastcmp(word, "LEVELNAME"))
			{
				deh_strlcpy(mapheaderinfo[num]->lvlttl, word2,
					sizeof(mapheaderinfo[num]->lvlttl), va("Level header %d: levelname", num));
				continue;
			}

			if (fastcmp(word, "ZONETITLE"))
			{
				deh_strlcpy(mapheaderinfo[num]->zonttl, word2,
					sizeof(mapheaderinfo[num]->zonttl), va("Level header %d: zonetitle", num));
				continue;
			}

			if (fastcmp(word, "MENUTITLE"))
			{
				deh_strlcpy(mapheaderinfo[num]->menuttl, word2,
					sizeof(mapheaderinfo[num]->menuttl), va("Level header %d: menutitle", num));
				continue;
			}

			// Lua custom options also go above, contents may be case sensitive.
			if (fastncmp(word, "LUA.", 4))
			{
				UINT8 j;
				customoption_t *modoption;

				// Note: we actualy strlwr word here, so things are made a little easier for Lua
				strlwr(word);
				word += 4; // move past "lua."

				// ... and do a simple name sanity check; the name must start with a letter
				if (*word < 'a' || *word > 'z')
				{
					deh_warning("Level header %d: invalid custom option name \"%s\"", num, word);
					continue;
				}

				// Sanity limit of 128 params
				if (mapheaderinfo[num]->numCustomOptions == 128)
				{
					deh_warning("Level header %d: too many custom parameters", num);
					continue;
				}
				j = mapheaderinfo[num]->numCustomOptions++;

				mapheaderinfo[num]->customopts =
					Z_Realloc(mapheaderinfo[num]->customopts,
						sizeof(customoption_t) * mapheaderinfo[num]->numCustomOptions, PU_STATIC, NULL);

				// Newly allocated
				modoption = &mapheaderinfo[num]->customopts[j];

				strncpy(modoption->option, word,  31);
				modoption->option[31] = '\0';
				strncpy(modoption->value,  word2, 255);
				modoption->value[255] = '\0';
				continue;
			}

			// Now go to uppercase
			strupr(word2);

			// List of followers that are be freed in this map
			if (fastcmp(word, "FOLLOWERS"))
			{
				if (fastcmp(word2, "NONE"))
					P_DeleteHeaderFollowers(num);
				else if (fastcmp(word2, "DEFAULT"))
					P_SetDefaultHeaderFollowers(num);
				else
				{
					INT16 tmpfollowers[MAXHEADERFOLLOWERS];
					mapheaderinfo[num]->numFollowers = 0;
					tmp = strtok(word2,",");
					// get up to the first MAXHEADERFOLLOWERS followers
					do {
						if (mapheaderinfo[num]->numFollowers == MAXHEADERFOLLOWERS) // never going to get above that number
						{
							deh_warning("Level header %d: too many followers\n", num);
							break;
						}

						{
							i = K_FollowerAvailable(tmp);
							if (i == -1)
							{
								deh_warning("Level header %d: unknown follower selection %s\n", num, tmp);
								continue;
							}
							tmpfollowers[mapheaderinfo[num]->numFollowers] = i;
							mapheaderinfo[num]->numFollowers++;
						}
					} while ((tmp = strtok(NULL,",")) != NULL);

					if (mapheaderinfo[num]->numFollowers)
					{
						size_t newsize = sizeof(INT16) * mapheaderinfo[num]->numFollowers;
						mapheaderinfo[num]->followers = Z_Realloc(mapheaderinfo[num]->followers, newsize, PU_STATIC, NULL);
						// now we add them to the list!
						M_Memcpy(mapheaderinfo[num]->followers, tmpfollowers, newsize);
					}
					else
						deh_warning("Level header %d: no valid follower types found\n", num);
				}
			}

			// Strings that can be truncated
			else if (fastcmp(word, "RELEVANTSKIN"))
			{
				deh_strlcpy(mapheaderinfo[num]->relevantskin, word2,
					sizeof(mapheaderinfo[num]->relevantskin), va("Level header %d: relevantskin", num));
			}
			else if (fastcmp(word, "SCRIPTNAME"))
			{
				deh_strlcpy(mapheaderinfo[num]->scriptname, word2,
					sizeof(mapheaderinfo[num]->scriptname), va("Level header %d: scriptname", num));
			}
			else if (fastcmp(word, "RUNSOC"))
			{
				deh_strlcpy(mapheaderinfo[num]->runsoc, word2,
					sizeof(mapheaderinfo[num]->runsoc), va("Level header %d: runsoc", num));
			}
			else if (fastcmp(word, "ACT"))
			{
				if (i >= 0 && i <= 99) // 0 for no act number
					mapheaderinfo[num]->actnum = (UINT8)i;
				else
					deh_warning("Level header %d: invalid act number %d", num, i);
			}
			else if (fastcmp(word, "TYPEOFLEVEL"))
			{
				if (i || isdigit(word2[0])) // it's just a number
					mapheaderinfo[num]->typeoflevel = (UINT32)i;
				else
				{
					UINT32 tol = 0;
					tmp = strtok(word2,",");
					do {
						for (i = 0; TYPEOFLEVEL[i].name; i++)
							if (fastcmp(tmp, TYPEOFLEVEL[i].name))
								break;
						if (!TYPEOFLEVEL[i].name)
							deh_warning("Level header %d: unknown typeoflevel flag %s\n", num, tmp);
						tol |= TYPEOFLEVEL[i].flag;
					} while((tmp = strtok(NULL,",")) != NULL);
					mapheaderinfo[num]->typeoflevel = tol;
				}
			}
			else if (fastcmp(word, "KEYWORDS"))
			{
				deh_strlcpy(mapheaderinfo[num]->keywords, word2,
						sizeof(mapheaderinfo[num]->keywords), va("Level header %d: keywords", num));
			}
			else if (fastcmp(word, "MUSIC"))
			{
				if (fastcmp(word2, "NONE"))
				{
					mapheaderinfo[num]->musname[0][0] = 0; // becomes empty string
					mapheaderinfo[num]->musname_size = 0;
				}
				else
				{
					UINT8 j = 0; // i was declared elsewhere
					tmp = strtok(word2, ",");
					do {
						if (j >= MAXMUSNAMES)
							break;
						deh_strlcpy(mapheaderinfo[num]->musname[j], tmp,
							sizeof(mapheaderinfo[num]->musname[j]), va("Level header %d: music", num));
						j++;
					} while ((tmp = strtok(NULL,",")) != NULL);

					if (tmp != NULL)
						deh_warning("Level header %d: additional music slots past %d discarded", num, MAXMUSNAMES);
					mapheaderinfo[num]->musname_size = j;
				}
			}
			else if (fastcmp(word, "ENCOREMUSIC"))
			{
				if (fastcmp(word2, "NONE"))
				{
					mapheaderinfo[num]->encoremusname[0][0] = 0; // becomes empty string
					mapheaderinfo[num]->encoremusname_size = 0;
				}
				else
				{
					UINT8 j = 0; // i was declared elsewhere
					tmp = strtok(word2, ",");
					do {
						if (j >= MAXMUSNAMES)
							break;
						deh_strlcpy(mapheaderinfo[num]->encoremusname[j], tmp,
							sizeof(mapheaderinfo[num]->encoremusname[j]), va("Level header %d: encore music", num));
						j++;
					} while ((tmp = strtok(NULL,",")) != NULL);

					if (tmp != NULL)
						deh_warning("Level header %d: additional music slots past %d discarded", num, MAXMUSNAMES);
					mapheaderinfo[num]->encoremusname_size = j;
				}
			}
			else if (fastcmp(word, "ASSOCIATEDMUSIC"))
			{
				if (fastcmp(word2, "NONE"))
				{
					mapheaderinfo[num]->associatedmus[0][0] = 0; // becomes empty string
					mapheaderinfo[num]->associatedmus_size = 0;
				}
				else
				{
					UINT8 j = 0; // i was declared elsewhere
					tmp = strtok(word2, ",");
					do {
						if (j >= MAXMUSNAMES)
							break;
						deh_strlcpy(mapheaderinfo[num]->associatedmus[j], tmp,
							sizeof(mapheaderinfo[num]->associatedmus[j]), va("Level header %d: associated music", num));
						j++;
					} while ((tmp = strtok(NULL,",")) != NULL);

					if (tmp != NULL)
						deh_warning("Level header %d: additional associated music slots past %d discarded", num, MAXMUSNAMES);
					mapheaderinfo[num]->associatedmus_size = j;
				}
			}
			else if (fastcmp(word, "POSITIONMUSIC"))
			{
				if (fastcmp(word2, "NONE"))
				{
					mapheaderinfo[num]->positionmus[0] = 0; // becomes empty string
				}
				else
				{
					deh_strlcpy(mapheaderinfo[num]->positionmus, word2,
						sizeof(mapheaderinfo[num]->positionmus), va("Level header %d: POSITION!! music", num));
				}
			}
			else if (fastcmp(word, "MUSICTRACK"))
				mapheaderinfo[num]->mustrack = ((UINT16)i - 1);
			else if (fastcmp(word, "MUSICPOS"))
				mapheaderinfo[num]->muspos = (UINT32)get_number(word2);
			else if (fastcmp(word, "WEATHER"))
				mapheaderinfo[num]->weather = get_precip(word2);
			else if (fastcmp(word, "SKYTEXTURE"))
				deh_strlcpy(mapheaderinfo[num]->skytexture, word2,
					sizeof(mapheaderinfo[num]->skytexture), va("Level header %d: sky texture", num));
			else if (fastcmp(word, "PRECUTSCENENUM"))
				mapheaderinfo[num]->precutscenenum = (UINT8)i;
			else if (fastcmp(word, "CUTSCENENUM"))
				mapheaderinfo[num]->cutscenenum = (UINT8)i;
			else if (fastcmp(word, "PALETTE"))
				mapheaderinfo[num]->palette = (UINT16)i;
			else if (fastcmp(word, "ENCOREPAL"))
				mapheaderinfo[num]->encorepal = (UINT16)i;
			else if (fastcmp(word, "NUMLAPS"))
				mapheaderinfo[num]->numlaps = (UINT8)i;
			else if (fastcmp(word, "LAPSPERSECTION"))
				mapheaderinfo[num]->lapspersection = max((UINT8)i, 1u);
			else if (fastcmp(word, "SKYBOXSCALE"))
				mapheaderinfo[num]->skybox_scalex = mapheaderinfo[num]->skybox_scaley = mapheaderinfo[num]->skybox_scalez = (INT16)i;
			else if (fastcmp(word, "SKYBOXSCALEX"))
				mapheaderinfo[num]->skybox_scalex = (INT16)i;
			else if (fastcmp(word, "SKYBOXSCALEY"))
				mapheaderinfo[num]->skybox_scaley = (INT16)i;
			else if (fastcmp(word, "SKYBOXSCALEZ"))
				mapheaderinfo[num]->skybox_scalez = (INT16)i;
			else if (fastcmp(word, "DARKNESS"))
				mapheaderinfo[num]->darkness = FloatToFixed(atof(word2));
			else if (fastcmp(word, "LEVELFLAGS"))
				mapheaderinfo[num]->levelflags = get_number(word2);
			else if (fastcmp(word, "MENUFLAGS"))
				mapheaderinfo[num]->menuflags = get_number(word2);
			// SRB2Kart
			else if (fastcmp(word, "MOBJSCALE"))
				mapheaderinfo[num]->mobj_scale = get_number(word2);
			else if (fastcmp(word, "DEFAULTWAYPOINTRADIUS"))
				mapheaderinfo[num]->default_waypoint_radius = get_number(word2);
			else if (fastcmp(word, "LIGHTCONTRAST") || fastcmp(word, "ENCORELIGHTCONTRAST"))
			{
				usemaplighting(num, word)->light_contrast = (UINT8)i;
			}
			else if (fastcmp(word, "SPRITEBACKLIGHT") || fastcmp(word, "ENCORESPRITEBACKLIGHT"))
			{
				usemaplighting(num, word)->sprite_backlight = (SINT8)i;
			}
			else if (fastcmp(word, "LIGHTANGLE") || fastcmp(word, "ENCORELIGHTANGLE"))
			{
				mapheader_lighting_t *lighting = usemaplighting(num, word);

				if (fastcmp(word2, "EVEN"))
				{
					lighting->use_light_angle = false;
					lighting->light_angle = 0;
				}
				else
				{
					lighting->use_light_angle = true;
					lighting->light_angle = FixedAngle(FloatToFixed(atof(word2)));
				}
			}
			// Individual triggers for level flags, for ease of use (and 2.0 compatibility)
			else if (fastcmp(word, "NOZONE"))
			{
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->levelflags |= LF_NOZONE;
				else
					mapheaderinfo[num]->levelflags &= ~LF_NOZONE;
			}
			else if (fastcmp(word, "SECTIONRACE"))
			{
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->levelflags |= LF_SECTIONRACE;
				else
					mapheaderinfo[num]->levelflags &= ~LF_SECTIONRACE;
			}
			else if (fastcmp(word, "SUBTRACTNUM"))
			{
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->levelflags |= LF_SUBTRACTNUM;
				else
					mapheaderinfo[num]->levelflags &= ~LF_SUBTRACTNUM;
			}

			// Individual triggers for menu flags
			else if (fastcmp(word, "HIDDEN"))
			{
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->menuflags |= LF2_HIDEINMENU;
				else
					mapheaderinfo[num]->menuflags &= ~LF2_HIDEINMENU;
			}
			else if (fastcmp(word, "NOTIMEATTACK") || fastcmp(word, "NORECORDATTACK"))
			{ // RECORDATTACK is an accepted alias
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->menuflags |= LF2_NOTIMEATTACK;
				else
					mapheaderinfo[num]->menuflags &= ~LF2_NOTIMEATTACK;
			}
			else if (fastcmp(word, "FINISHNEEDED"))
			{
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->menuflags |= LF2_FINISHNEEDED;
				else
					mapheaderinfo[num]->menuflags &= ~LF2_FINISHNEEDED;
			}
			else if (fastcmp(word, "NOVISITNEEDED"))
			{
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->menuflags |= LF2_NOVISITNEEDED;
				else
					mapheaderinfo[num]->menuflags &= ~LF2_NOVISITNEEDED;
			}
			else if (fastcmp(word, "GRAVITY"))
				mapheaderinfo[num]->gravity = FLOAT_TO_FIXED(atof(word2));
			else if (fastcmp(word, "DESTROYOBJECTSFORCHALLENGES"))
			{
				if (fastcmp(word2, "NONE"))
				{
					mapheaderinfo[num]->destroyforchallenge_size = 0;
				}
				else
				{
					UINT8 j = 0; // i was declared elsewhere
					tmp = strtok(word2, ",");
					do {
						if (j >= MAXDESTRUCTIBLES)
							break;
						mapheaderinfo[num]->destroyforchallenge[j] = get_mobjtype(tmp);
						j++;
					} while ((tmp = strtok(NULL,",")) != NULL);

					if (tmp != NULL)
						deh_warning("Level header %d: additional destructibles past %d discarded", num, MAXDESTRUCTIBLES);
					mapheaderinfo[num]->destroyforchallenge_size = j;
				}
			}
			else if (fastcmp(word, "LOBBYSIZE"))
			{
				if (fastcmp(word2, "DUEL"))
				{
					mapheaderinfo[num]->playerLimit = 2;
				}
				else if (fastcmp(word2, "SMALL"))
				{
					mapheaderinfo[num]->playerLimit = 5;
				}
				else if (fastcmp(word2, "MEDIUM"))
				{
					mapheaderinfo[num]->playerLimit = 10;
				}
				else if (fastcmp(word2, "LARGE"))
				{
					mapheaderinfo[num]->playerLimit = 16;
				}
				else
				{
					deh_warning("Level header %d: invalid lobby size '%s'", num, word2);
				}
			}
			else if (fastcmp(word, "CAMHEIGHT") || fastcmp(word, "CAMERAHEIGHT"))
			{
				fixed_t camheight = FloatToFixed(atof(word2));

				if (camheight < 0)
				{
					deh_warning("Level header %d: invalid camera height %s", num, word2);
					continue;
				}

				mapheaderinfo[num]->cameraHeight = camheight;;
			}
			else if (fastcmp(word, "NOCOMMS") || fastcmp(word, "NOCOMM"))
			{
				if (i || word2[0] == 'T' || word2[0] == 'Y')
					mapheaderinfo[num]->levelflags |= LF_NOCOMMS;
				else
					mapheaderinfo[num]->levelflags &= ~LF_NOCOMMS;
			}
			else
				deh_warning("Level header %d: unknown word '%s'", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

static void readcutscenescene(MYFILE *f, INT32 num, INT32 scenenum)
{
	char *s = Z_Calloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2;
	INT32 i;
	UINT16 usi;
	UINT8 picid;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			if (fastcmp(word, "SCENETEXT"))
			{
				char *scenetext = NULL;
				char *buffer;
				const int bufferlen = 4096;

				for (i = 0; i < MAXLINELEN; i++)
				{
					if (s[i] == '=')
					{
						scenetext = &s[i+2];
						break;
					}
				}

				if (!scenetext)
				{
					Z_Free(cutscenes[num]->scene[scenenum].text);
					cutscenes[num]->scene[scenenum].text = NULL;
					continue;
				}

				for (i = 0; i < MAXLINELEN; i++)
				{
					if (s[i] == '\0')
					{
						s[i] = '\n';
						s[i+1] = '\0';
						break;
					}
				}

				buffer = Z_Malloc(4096, PU_STATIC, NULL);
				strcpy(buffer, scenetext);

				strcat(buffer,
					myhashfgets(scenetext, bufferlen
					- strlen(buffer) - 1, f));

				// A cutscene overwriting another one...
				Z_Free(cutscenes[num]->scene[scenenum].text);

				cutscenes[num]->scene[scenenum].text = Z_StrDup(buffer);

				Z_Free(buffer);

				continue;
			}

			word2 = strtok(NULL, " = ");
			if (word2)
				strupr(word2);
			else
				break;

			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';
			i = atoi(word2);
			usi = (UINT16)i;


			if (fastcmp(word, "NUMBEROFPICS"))
			{
				cutscenes[num]->scene[scenenum].numpics = (UINT8)i;
			}
			else if (fastncmp(word, "PIC", 3))
			{
				picid = (UINT8)atoi(word + 3);
				if (picid > 8 || picid == 0)
				{
					deh_warning("CutSceneScene %d: unknown word '%s'", num, word);
					continue;
				}
				--picid;

				if (fastcmp(word+4, "NAME"))
				{
					strncpy(cutscenes[num]->scene[scenenum].picname[picid], word2, 8);
				}
				else if (fastcmp(word+4, "HIRES"))
				{
					cutscenes[num]->scene[scenenum].pichires[picid] = (UINT8)(i || word2[0] == 'T' || word2[0] == 'Y');
				}
				else if (fastcmp(word+4, "DURATION"))
				{
					cutscenes[num]->scene[scenenum].picduration[picid] = usi;
				}
				else if (fastcmp(word+4, "XCOORD"))
				{
					cutscenes[num]->scene[scenenum].xcoord[picid] = usi;
				}
				else if (fastcmp(word+4, "YCOORD"))
				{
					cutscenes[num]->scene[scenenum].ycoord[picid] = usi;
				}
				else
					deh_warning("CutSceneScene %d: unknown word '%s'", num, word);
			}
			else if (fastcmp(word, "MUSIC"))
			{
				strncpy(cutscenes[num]->scene[scenenum].musswitch, word2, 7);
				cutscenes[num]->scene[scenenum].musswitch[6] = 0;
			}
			else if (fastcmp(word, "MUSICTRACK"))
			{
				cutscenes[num]->scene[scenenum].musswitchflags = ((UINT16)i) & MUSIC_TRACKMASK;
			}
			else if (fastcmp(word, "MUSICPOS"))
			{
				cutscenes[num]->scene[scenenum].musswitchposition = (UINT32)get_number(word2);
			}
			else if (fastcmp(word, "MUSICLOOP"))
			{
				cutscenes[num]->scene[scenenum].musicloop = (UINT8)(i || word2[0] == 'T' || word2[0] == 'Y');
			}
			else if (fastcmp(word, "TEXTXPOS"))
			{
				cutscenes[num]->scene[scenenum].textxpos = usi;
			}
			else if (fastcmp(word, "TEXTYPOS"))
			{
				cutscenes[num]->scene[scenenum].textypos = usi;
			}
			else if (fastcmp(word, "FADEINID"))
			{
				cutscenes[num]->scene[scenenum].fadeinid = (UINT8)i;
			}
			else if (fastcmp(word, "FADEOUTID"))
			{
				cutscenes[num]->scene[scenenum].fadeoutid = (UINT8)i;
			}
			else if (fastcmp(word, "FADECOLOR"))
			{
				cutscenes[num]->scene[scenenum].fadecolor = (UINT8)i;
			}
			else
				deh_warning("CutSceneScene %d: unknown word '%s'", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readcutscene(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2;
	char *tmp;
	INT32 value;

	// Allocate memory for this cutscene if we don't yet have any
	if (!cutscenes[num])
		cutscenes[num] = Z_Calloc(sizeof (cutscene_t), PU_STATIC, NULL);

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			word2 = strtok(NULL, " ");
			if (word2)
				value = atoi(word2);
			else
			{
				deh_warning("No value for token %s", word);
				continue;
			}

			if (fastcmp(word, "NUMSCENES"))
			{
				cutscenes[num]->numscenes = value;
			}
			else if (fastcmp(word, "SCENE"))
			{
				if (1 <= value && value <= 128)
				{
					readcutscenescene(f, num, value - 1);
				}
				else
					deh_warning("Scene number %d out of range (1 - 128)", value);

			}
			else
				deh_warning("Cutscene %d: unknown word '%s', Scene <num> expected.", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

static void readtextpromptpage(MYFILE *f, INT32 num, INT32 pagenum)
{
	char *s = Z_Calloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2;
	INT32 i;
	UINT16 usi;
	UINT8 picid;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			if (fastcmp(word, "PAGETEXT"))
			{
				char *pagetext = NULL;
				char *buffer;
				const int bufferlen = 4096;

				for (i = 0; i < MAXLINELEN; i++)
				{
					if (s[i] == '=')
					{
						pagetext = &s[i+2];
						break;
					}
				}

				if (!pagetext)
				{
					Z_Free(textprompts[num]->page[pagenum].text);
					textprompts[num]->page[pagenum].text = NULL;
					continue;
				}

				for (i = 0; i < MAXLINELEN; i++)
				{
					if (s[i] == '\0')
					{
						s[i] = '\n';
						s[i+1] = '\0';
						break;
					}
				}

				buffer = Z_Malloc(4096, PU_STATIC, NULL);
				strcpy(buffer, pagetext);

				// \todo trim trailing whitespace before the #
				// and also support # at the end of a PAGETEXT with no line break

				strcat(buffer,
					myhashfgets(pagetext, bufferlen
					- strlen(buffer) - 1, f));

				// A text prompt overwriting another one...
				Z_Free(textprompts[num]->page[pagenum].text);

				textprompts[num]->page[pagenum].text = Z_StrDup(buffer);

				Z_Free(buffer);

				continue;
			}

			word2 = strtok(NULL, " = ");
			if (word2)
				strupr(word2);
			else
				break;

			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';
			i = atoi(word2);
			usi = (UINT16)i;

			// copypasta from readcutscenescene
			if (fastcmp(word, "NUMBEROFPICS"))
			{
				textprompts[num]->page[pagenum].numpics = (UINT8)i;
			}
			else if (fastcmp(word, "PICMODE"))
			{
				UINT8 picmode = 0; // PROMPT_PIC_PERSIST
				if (usi == 1 || word2[0] == 'L') picmode = PROMPT_PIC_LOOP;
				else if (usi == 2 || word2[0] == 'D' || word2[0] == 'H') picmode = PROMPT_PIC_DESTROY;
				textprompts[num]->page[pagenum].picmode = picmode;
			}
			else if (fastcmp(word, "PICTOLOOP"))
				textprompts[num]->page[pagenum].pictoloop = (UINT8)i;
			else if (fastcmp(word, "PICTOSTART"))
				textprompts[num]->page[pagenum].pictostart = (UINT8)i;
			else if (fastcmp(word, "PICSMETAPAGE"))
			{
				if (usi && usi <= textprompts[num]->numpages)
				{
					UINT8 metapagenum = usi - 1;

					textprompts[num]->page[pagenum].numpics = textprompts[num]->page[metapagenum].numpics;
					textprompts[num]->page[pagenum].picmode = textprompts[num]->page[metapagenum].picmode;
					textprompts[num]->page[pagenum].pictoloop = textprompts[num]->page[metapagenum].pictoloop;
					textprompts[num]->page[pagenum].pictostart = textprompts[num]->page[metapagenum].pictostart;

					for (picid = 0; picid < MAX_PROMPT_PICS; picid++)
					{
						strncpy(textprompts[num]->page[pagenum].picname[picid], textprompts[num]->page[metapagenum].picname[picid], 8);
						textprompts[num]->page[pagenum].pichires[picid] = textprompts[num]->page[metapagenum].pichires[picid];
						textprompts[num]->page[pagenum].picduration[picid] = textprompts[num]->page[metapagenum].picduration[picid];
						textprompts[num]->page[pagenum].xcoord[picid] = textprompts[num]->page[metapagenum].xcoord[picid];
						textprompts[num]->page[pagenum].ycoord[picid] = textprompts[num]->page[metapagenum].ycoord[picid];
					}
				}
			}
			else if (fastncmp(word, "PIC", 3))
			{
				picid = (UINT8)atoi(word + 3);
				if (picid > MAX_PROMPT_PICS || picid == 0)
				{
					deh_warning("textpromptscene %d: unknown word '%s'", num, word);
					continue;
				}
				--picid;

				if (fastcmp(word+4, "NAME"))
				{
					strncpy(textprompts[num]->page[pagenum].picname[picid], word2, 8);
				}
				else if (fastcmp(word+4, "HIRES"))
				{
					textprompts[num]->page[pagenum].pichires[picid] = (UINT8)(i || word2[0] == 'T' || word2[0] == 'Y');
				}
				else if (fastcmp(word+4, "DURATION"))
				{
					textprompts[num]->page[pagenum].picduration[picid] = usi;
				}
				else if (fastcmp(word+4, "XCOORD"))
				{
					textprompts[num]->page[pagenum].xcoord[picid] = usi;
				}
				else if (fastcmp(word+4, "YCOORD"))
				{
					textprompts[num]->page[pagenum].ycoord[picid] = usi;
				}
				else
					deh_warning("textpromptscene %d: unknown word '%s'", num, word);
			}
			else if (fastcmp(word, "MUSIC"))
			{
				strncpy(textprompts[num]->page[pagenum].musswitch, word2, 7);
				textprompts[num]->page[pagenum].musswitch[6] = 0;
			}
			else if (fastcmp(word, "MUSICTRACK"))
			{
				textprompts[num]->page[pagenum].musswitchflags = ((UINT16)i) & MUSIC_TRACKMASK;
			}
			else if (fastcmp(word, "MUSICLOOP"))
			{
				textprompts[num]->page[pagenum].musicloop = (UINT8)(i || word2[0] == 'T' || word2[0] == 'Y');
			}
			// end copypasta from readcutscenescene
			else if (fastcmp(word, "NAME"))
			{
				if (*word2 != '\0')
				{
					INT32 j;

					// HACK: Add yellow control char now
					// so the drawing function doesn't call it repeatedly
					char name[34];
					name[0] = '\x82'; // color yellow
					name[1] = 0;
					strncat(name, word2, 33);
					name[33] = 0;

					// Replace _ with ' '
					for (j = 0; j < 32 && name[j]; j++)
					{
						if (name[j] == '_')
							name[j] = ' ';
					}

					strncpy(textprompts[num]->page[pagenum].name, name, 32);
				}
				else
					*textprompts[num]->page[pagenum].name = '\0';
			}
			else if (fastcmp(word, "ICON"))
				strncpy(textprompts[num]->page[pagenum].iconname, word2, 8);
			else if (fastcmp(word, "ICONALIGN"))
				textprompts[num]->page[pagenum].rightside = (i || word2[0] == 'R');
			else if (fastcmp(word, "ICONFLIP"))
				textprompts[num]->page[pagenum].iconflip = (i || word2[0] == 'T' || word2[0] == 'Y');
			else if (fastcmp(word, "LINES"))
				textprompts[num]->page[pagenum].lines = usi;
			else if (fastcmp(word, "BACKCOLOR"))
			{
				INT32 backcolor;
				if      (i == 0 || fastcmp(word2, "WHITE")) backcolor = 0;
				else if (i == 1 || fastcmp(word2, "GRAY") || fastcmp(word2, "GREY") ||
					fastcmp(word2, "BLACK")) backcolor = 1;
				else if (i == 2 || fastcmp(word2, "SEPIA")) backcolor = 2;
				else if (i == 3 || fastcmp(word2, "BROWN")) backcolor = 3;
				else if (i == 4 || fastcmp(word2, "PINK")) backcolor = 4;
				else if (i == 5 || fastcmp(word2, "RASPBERRY")) backcolor = 5;
				else if (i == 6 || fastcmp(word2, "RED")) backcolor = 6;
				else if (i == 7 || fastcmp(word2, "CREAMSICLE")) backcolor = 7;
				else if (i == 8 || fastcmp(word2, "ORANGE")) backcolor = 8;
				else if (i == 9 || fastcmp(word2, "GOLD")) backcolor = 9;
				else if (i == 10 || fastcmp(word2, "YELLOW")) backcolor = 10;
				else if (i == 11 || fastcmp(word2, "EMERALD")) backcolor = 11;
				else if (i == 12 || fastcmp(word2, "GREEN")) backcolor = 12;
				else if (i == 13 || fastcmp(word2, "CYAN") || fastcmp(word2, "AQUA")) backcolor = 13;
				else if (i == 14 || fastcmp(word2, "STEEL")) backcolor = 14;
				else if (i == 15 || fastcmp(word2, "PERIWINKLE")) backcolor = 15;
				else if (i == 16 || fastcmp(word2, "BLUE")) backcolor = 16;
				else if (i == 17 || fastcmp(word2, "PURPLE")) backcolor = 17;
				else if (i == 18 || fastcmp(word2, "LAVENDER")) backcolor = 18;
				else if (i >= 256 && i < 512) backcolor = i; // non-transparent palette index
				else if (i < 0) backcolor = INT32_MAX; // CONS_BACKCOLOR user-configured
				else backcolor = 1; // default gray
				textprompts[num]->page[pagenum].backcolor = backcolor;
			}
			else if (fastcmp(word, "ALIGN"))
			{
				UINT8 align = 0; // left
				if (usi == 1 || word2[0] == 'R') align = 1;
				else if (usi == 2 || word2[0] == 'C' || word2[0] == 'M') align = 2;
				textprompts[num]->page[pagenum].align = align;
			}
			else if (fastcmp(word, "VERTICALALIGN"))
			{
				UINT8 align = 0; // top
				if (usi == 1 || word2[0] == 'B') align = 1;
				else if (usi == 2 || word2[0] == 'C' || word2[0] == 'M') align = 2;
				textprompts[num]->page[pagenum].verticalalign = align;
			}
			else if (fastcmp(word, "TEXTSPEED"))
				textprompts[num]->page[pagenum].textspeed = get_number(word2);
			else if (fastcmp(word, "TEXTSFX"))
				textprompts[num]->page[pagenum].textsfx = get_number(word2);
			else if (fastcmp(word, "HIDEHUD"))
			{
				UINT8 hidehud = 0;
				if ((word2[0] == 'F' && (word2[1] == 'A' || !word2[1])) || word2[0] == 'N') hidehud = 0; // false
				else if (usi == 1 || word2[0] == 'T' || word2[0] == 'Y') hidehud = 1; // true (hide appropriate HUD elements)
				else if (usi == 2 || word2[0] == 'A' || (word2[0] == 'F' && word2[1] == 'O')) hidehud = 2; // force (hide all HUD elements)
				textprompts[num]->page[pagenum].hidehud = hidehud;
			}
			else if (fastcmp(word, "METAPAGE"))
			{
				if (usi && usi <= textprompts[num]->numpages)
				{
					UINT8 metapagenum = usi - 1;

					strncpy(textprompts[num]->page[pagenum].name, textprompts[num]->page[metapagenum].name, 32);
					strncpy(textprompts[num]->page[pagenum].iconname, textprompts[num]->page[metapagenum].iconname, 8);
					textprompts[num]->page[pagenum].rightside = textprompts[num]->page[metapagenum].rightside;
					textprompts[num]->page[pagenum].iconflip = textprompts[num]->page[metapagenum].iconflip;
					textprompts[num]->page[pagenum].lines = textprompts[num]->page[metapagenum].lines;
					textprompts[num]->page[pagenum].backcolor = textprompts[num]->page[metapagenum].backcolor;
					textprompts[num]->page[pagenum].align = textprompts[num]->page[metapagenum].align;
					textprompts[num]->page[pagenum].verticalalign = textprompts[num]->page[metapagenum].verticalalign;
					textprompts[num]->page[pagenum].textspeed = textprompts[num]->page[metapagenum].textspeed;
					textprompts[num]->page[pagenum].textsfx = textprompts[num]->page[metapagenum].textsfx;
					textprompts[num]->page[pagenum].hidehud = textprompts[num]->page[metapagenum].hidehud;

					// music: don't copy, else each page change may reset the music
				}
			}
			else if (fastcmp(word, "TAG"))
				strncpy(textprompts[num]->page[pagenum].tag, word2, 33);
			else if (fastcmp(word, "NEXTPROMPT"))
				textprompts[num]->page[pagenum].nextprompt = usi;
			else if (fastcmp(word, "NEXTPAGE"))
				textprompts[num]->page[pagenum].nextpage = usi;
			else if (fastcmp(word, "NEXTTAG"))
				strncpy(textprompts[num]->page[pagenum].nexttag, word2, 33);
			else if (fastcmp(word, "TIMETONEXT"))
				textprompts[num]->page[pagenum].timetonext = get_number(word2);
			else
				deh_warning("PromptPage %d: unknown word '%s'", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readtextprompt(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2;
	char *tmp;
	INT32 value;

	// Allocate memory for this prompt if we don't yet have any
	if (!textprompts[num])
		textprompts[num] = Z_Calloc(sizeof (textprompt_t), PU_STATIC, NULL);

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			word2 = strtok(NULL, " ");
			if (word2)
				value = atoi(word2);
			else
			{
				deh_warning("No value for token %s", word);
				continue;
			}

			if (fastcmp(word, "NUMPAGES"))
			{
				textprompts[num]->numpages = min(max(value, 0), MAX_PAGES);
			}
			else if (fastcmp(word, "PAGE"))
			{
				if (1 <= value && value <= MAX_PAGES)
				{
					textprompts[num]->page[value - 1].backcolor = 1; // default to gray
					textprompts[num]->page[value - 1].hidehud = 1; // hide appropriate HUD elements
					readtextpromptpage(f, num, value - 1);
				}
				else
					deh_warning("Page number %d out of range (1 - %d)", value, MAX_PAGES);

			}
			else
				deh_warning("Prompt %d: unknown word '%s', Page <num> expected.", num, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readframe(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word1;
	char *word2 = NULL;
	char *tmp;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			word1 = strtok(s, " ");
			if (word1)
				strupr(word1);
			else
				break;

			word2 = strtok(NULL, " = ");
			if (word2)
				strupr(word2);
			else
				break;
			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';

			if (fastcmp(word1, "SPRITENUMBER") || fastcmp(word1, "SPRITENAME"))
			{
				states[num].sprite = get_sprite(word2);
			}
			else if (fastcmp(word1, "SPRITESUBNUMBER") || fastcmp(word1, "SPRITEFRAME"))
			{
				states[num].frame = (INT32)get_number(word2); // So the FF_ flags get calculated
			}
			else if (fastcmp(word1, "DURATION"))
			{
				states[num].tics = (INT32)get_number(word2); // So TICRATE can be used
			}
			else if (fastcmp(word1, "NEXT"))
			{
				states[num].nextstate = get_state(word2);
			}
			else if (fastcmp(word1, "VAR1"))
			{
				states[num].var1 = (INT32)get_number(word2);
			}
			else if (fastcmp(word1, "VAR2"))
			{
				states[num].var2 = (INT32)get_number(word2);
			}
			else if (fastcmp(word1, "ACTION"))
			{
				size_t z;
				boolean found = false;
				char actiontocompare[32];

				memset(actiontocompare, 0x00, sizeof(actiontocompare));
				strlcpy(actiontocompare, word2, sizeof (actiontocompare));
				strupr(actiontocompare);

				for (z = 0; z < 32; z++)
				{
					if (actiontocompare[z] == '\n' || actiontocompare[z] == '\r')
					{
						actiontocompare[z] = '\0';
						break;
					}
				}

				for (z = 0; actionpointers[z].name; z++)
				{
					if (actionpointers[z].action.acv == states[num].action.acv)
						break;
				}

				z = 0;
				found = LUA_SetLuaAction(&states[num], actiontocompare);
				if (!found)
					while (actionpointers[z].name)
					{
						if (fastcmp(actiontocompare, actionpointers[z].name))
						{
							states[num].action = actionpointers[z].action;
							states[num].action.acv = actionpointers[z].action.acv; // assign
							states[num].action.acp1 = actionpointers[z].action.acp1;
							found = true;
							break;
						}
						z++;
					}

				if (!found)
					deh_warning("Unknown action %s", actiontocompare);
			}
			else
				deh_warning("Frame %d: unknown word '%s'", num, word1);
		}
	} while (!myfeof(f));

	Z_Free(s);
}

void readsound(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2;
	char *tmp;
	INT32 value;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Set / reset word
			word = s;

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;
			value = atoi(word2); // used for numerical settings

			if (fastcmp(word, "SINGULAR"))
			{
				S_sfx[num].singularity = value;
			}
			else if (fastcmp(word, "PRIORITY"))
			{
				S_sfx[num].priority = value;
			}
			else if (fastcmp(word, "FLAGS"))
			{
				S_sfx[num].pitch = value;
			}
			else if (fastcmp(word, "VOLUME"))
			{
				S_sfx[num].volume = value;
			}
			else if (fastcmp(word, "CAPTION") || fastcmp(word, "DESCRIPTION"))
			{
				deh_strlcpy(S_sfx[num].caption, word2,
					sizeof(S_sfx[num].caption), va("Sound effect %d: caption", num));
			}
			else
				deh_warning("Sound %d : unknown word '%s'",num,word);
		}
	} while (!myfeof(f));

	Z_Free(s);
}

/** Checks if a game data file name for a mod is good.
 * "Good" means that it contains only alphanumerics, _, and -;
 * ends in ".dat"; has at least one character before the ".dat";
 * and is not "gamedata.dat" (tested case-insensitively).
 *
 * Assumption: that gamedata.dat is the only .dat file that will
 * ever be treated specially by the game.
 *
 * Note: Check for the tail ".dat" case-insensitively since at
 * present, we get passed the filename in all uppercase.
 *
 * \param s Filename string to check.
 * \return True if the filename is good.
 * \sa readmaincfg()
 * \author Graue <graue@oceanbase.org>
 */
static boolean GoodDataFileName(const char *s)
{
	const char *p;
	const char *tail = ".dat";

	for (p = s; *p != '\0'; p++)
		if (!isalnum(*p) && *p != '_' && *p != '-' && *p != '.')
			return false;

	p = s + strlen(s) - strlen(tail);
	if (p <= s) return false; // too short
	if (!fasticmp(p, tail)) return false; // doesn't end in .dat
	if (fasticmp(s, "gamedata.dat")) return false;

	return true;
}

void reademblemdata(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word = s;
	char *word2;
	char *tmp;
	INT32 value;

	memset(&emblemlocations[num-1], 0, sizeof(emblem_t));

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;
			value = atoi(word2); // used for numerical settings

			strupr(word2);

			if (fastcmp(word, "TYPE"))
			{
				if (fastcmp(word2, "GLOBAL"))
					emblemlocations[num-1].type = ET_GLOBAL;
				else if (fastcmp(word2, "MAP"))
					emblemlocations[num-1].type = ET_MAP;
				else if (fastcmp(word2, "TIME"))
					emblemlocations[num-1].type = ET_TIME;
				else
					emblemlocations[num-1].type = (UINT8)value;
			}
			else if (fastcmp(word, "TAG"))
				emblemlocations[num-1].tag = get_number(word2);
			else if (fastcmp(word, "MAPNAME"))
			{
				emblemlocations[num-1].level = Z_StrDup(word2);
				emblemlocations[num-1].levelCache = NEXTMAP_INVALID;
			}
			else if (fastcmp(word, "SPRITE"))
			{
				if (word2[0] >= 'A' && word2[0] <= 'Z')
					value = word2[0];
				else
					value += 'A'-1;

				if (value < 'A' || value > 'Z')
					deh_warning("Emblem %d: sprite must be from A - Z (1 - 26)", num);
				else
					emblemlocations[num-1].sprite = (UINT8)value;
			}
			else if (fastcmp(word, "COLOR"))
				emblemlocations[num-1].color = get_number(word2);
			else if (fastcmp(word, "VAR"))
			{
				Z_Free(emblemlocations[num-1].stringVar);
				emblemlocations[num-1].stringVar = Z_StrDup(word2);

				emblemlocations[num-1].var = get_number(word2);
			}
			else if (fastcmp(word, "VAR2"))
			{
				Z_Free(emblemlocations[num-1].stringVar2);
				emblemlocations[num-1].stringVar2 = Z_StrDup(word2);

				emblemlocations[num-1].var2 = get_number(word2);
			}
			else if (fastcmp(word, "FLAGS"))
				emblemlocations[num-1].flags = get_number(word2);
			else
				deh_warning("Emblem %d: unknown word '%s'", num, word);
		}
	} while (!myfeof(f));

	// Default sprite and color definitions for lazy people like me
	if (!emblemlocations[num-1].sprite)
	{
		emblemlocations[num-1].sprite = 'A';
	}
	if (!emblemlocations[num-1].color)
	{
		emblemlocations[num-1].color = SKINCOLOR_GOLD;
	}

	Z_Free(s);
}

static INT16 parseunlockabletype(char *type)
{
	if (fastcmp(type, "EXTRAMEDAL"))
		return SECRET_EXTRAMEDAL;
	else if (fastcmp(type, "CUP"))
		return SECRET_CUP;
	else if (fastcmp(type, "MAP"))
		return SECRET_MAP;
	else if (fastcmp(type, "ALTMUSIC"))
		return SECRET_ALTMUSIC;
	else if (fastcmp(type, "SKIN"))
		return SECRET_SKIN;
	else if (fastcmp(type, "FOLLOWER"))
		return SECRET_FOLLOWER;
	else if (fastcmp(type, "COLOR"))
		return SECRET_COLOR;

	else if (fastcmp(type, "HARDSPEED"))
		return SECRET_HARDSPEED;
	else if (fastcmp(type, "MASTERMODE"))
		return SECRET_MASTERMODE;
	else if (fastcmp(type, "ENCORE"))
		return SECRET_ENCORE;
	else if (fastcmp(type, "TIMEATTACK"))
		return SECRET_TIMEATTACK;
	else if (fastcmp(type, "PRISONBREAK"))
		return SECRET_PRISONBREAK;
	else if (fastcmp(type, "SPECIALATTACK"))
		return SECRET_SPECIALATTACK;
	else if (fastcmp(type, "SPBATTACK"))
		return SECRET_SPBATTACK;
	else if (fastcmp(type, "ONLINE"))
		return SECRET_ONLINE;
	else if (fastcmp(type, "ADDONS"))
		return SECRET_ADDONS;
	else if (fastcmp(type, "EGGTV"))
		return SECRET_EGGTV;
	else if (fastcmp(type, "SOUNDTEST"))
		return SECRET_SOUNDTEST;
	else if (fastcmp(type, "ALTTITLE"))
		return SECRET_ALTTITLE;
	else if (fastcmp(type, "ITEMFINDER"))
		return SECRET_ITEMFINDER;

	return SECRET_NONE;
}

void readunlockable(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word = s;
	char *word2;
	char *tmp;
	INT32 i;

	memset(&unlockables[num], 0, sizeof(unlockable_t));

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;

			i = atoi(word2); // used for numerical settings

			if (fastcmp(word, "NAME"))
				deh_strlcpy(unlockables[num].name, word2,
					sizeof (unlockables[num].name), va("Unlockable %d: name", num+1));
			else
			{
				strupr(word2);

				if (fastcmp(word, "CONDITIONSET"))
					unlockables[num].conditionset = (UINT16)i;
				else if (fastcmp(word, "MAJORUNLOCK"))
					unlockables[num].majorunlock = (UINT8)(i != 0 || word2[0] == 'T' || word2[0] == 'Y');
				else if (fastcmp(word, "TYPE"))
				{
					unlockables[num].type = parseunlockabletype(word2);
					unlockables[num].stringVarCache = -1;
				}
				else if (fastcmp(word, "VAR"))
				{
					Z_Free(unlockables[num].stringVar);
					unlockables[num].stringVar = Z_StrDup(word2);
					unlockables[num].stringVarCache = -1;
					unlockables[num].variable = (INT16)i;
				}
				else if (fastcmp(word, "ICON"))
				{
					Z_Free(unlockables[num].icon);
					unlockables[num].icon = Z_StrDup(word2);
				}
				else if (fastcmp(word, "COLOR"))
				{
					unlockables[num].color = get_number(word2);
				}
				else
					deh_warning("Unlockable %d: unknown word '%s'", num+1, word);
			}
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

// Todo: Own func
static cupheader_t *SOChelper_cupbyname(char *name)
{
	cupheader_t *cup = kartcupheaders;
	UINT32 hash = quickncasehash(name, MAXCUPNAME);
	while (cup)
	{
		if (hash == cup->namehash && !strcmp(cup->name, name))
			return cup;
		cup = cup->next;
	}
	return NULL;
}

// This is a home-grown strtok(" ") equivalent so we can isolate the first chunk without destroying the rest of the line.
static void conditiongetparam(char **params, UINT8 paramid, char **spos)
{
	if (*spos == NULL || *(*spos) == '\0')
	{
		params[paramid] = NULL;
		return;
	}

	params[paramid] = *spos;
	while (*(*spos) != '\0' && *(*spos) != ' ')
	{
		*(*spos) = toupper(*(*spos));
		(*spos)++;
	}
	if (*(*spos) == ' ')
	{
		*(*spos) = '\0';
		(*spos)++;

		while (*(*spos) == ' ')
			(*spos)++;
	}
}

static void readcondition(UINT16 set, UINT32 id, char *word2)
{
	INT32 i;
#define MAXCONDITIONPARAMS 5
	char *params[MAXCONDITIONPARAMS]; // condition, requirement, extra info, extra info, stringvar
	char *spos = NULL;
	char *stringvar = NULL;

	conditiontype_t ty = UC_NONE;
	INT32 re = 0;
	INT16 x1 = 0, x2 = 0;

	INT32 offset = 0;

	// Lop the leading spaces off
	if (word2 && *word2)
	{
		spos = word2;
		while (*spos == ' ')
			spos++;
	}

	conditiongetparam(params, 0, &spos);

	if (!params[0])
	{
		deh_warning("condition line is empty for condition ID %d", id+1);
		return;
	}

	// We do free descriptions first.

	if (fastcmp(params[0], "DESCRIPTIONOVERRIDE"))
	{
		EXTENDEDPARAMCHECK(spos, 1);
		ty = UC_DESCRIPTIONOVERRIDE;

		stringvar = Z_StrDup(spos);
	}
	else if (fastcmp(params[0], "PASSWORD"))
	{
		size_t slen = strlen(spos);

		EXTENDEDPARAMCHECK(spos, 1);
		ty = UC_PASSWORD;

		if (slen > modp_b64_encode_len(M_PW_BUF_SIZE)-1)
		{
			deh_warning("Password hash is invalid");
			return;
		}

		stringvar = Z_Malloc(modp_b64_decode_len(slen), PU_STATIC, NULL);
		if (modp_b64_decode(stringvar, spos, slen) != M_PW_BUF_SIZE)
		{
			deh_warning("Password hash is invalid");
			Z_Free(stringvar);
			return;
		}
	}

	if (ty != UC_NONE)
		goto setcondition;

	// Now conditions that take one standard param and one free description.

	conditiongetparam(params, 1, &spos);

	if (fastcmp(params[0], "WETPLAYER"))
	{
		PARAMCHECK(1);
		//EXTENDEDPARAMCHECK(spos, 2);
		ty = UCRP_WETPLAYER;
		re = MFE_UNDERWATER;
		x1 = 1;

		if (fastcmp(params[1], "STRICT"))
			re |= MFE_TOUCHWATER;
		else if (fastcmp(params[1], "STANDARD"))
			;
		else
		{
			deh_warning("liquid strictness requirement \"%s\" invalid for condition ID %d", params[1], id+1);
			return;
		}

		if (spos && *spos)
			stringvar = Z_StrDup(spos);
	}
	else if (fastcmp(params[0], "MAPDESTROYOBJECTS"))
	{
		PARAMCHECK(1);
		EXTENDEDPARAMCHECK(spos, 2);
		ty = UCRP_MAPDESTROYOBJECTS;
		re = G_MapNumber(params[1]);

		stringvar = Z_StrDup(spos);
	}

	if (ty != UC_NONE)
		goto setcondition;

	// Now for all other conditions.

	for (i = 2; i < MAXCONDITIONPARAMS; i++)
	{
		conditiongetparam(params, i, &spos);
	}

	if (ty != UC_NONE)
		;
	else if (fastcmp(params[0], "PLAYTIME"))
	{
		PARAMCHECK(1);
		ty = UC_PLAYTIME;
		re = get_number(params[1]);
	}
	else if (fastcmp(params[0], "ROUNDSPLAYED"))
	{
		PARAMCHECK(1);
		ty = UC_ROUNDSPLAYED;
		re = atoi(params[1]);
		x1 = GDGT_MAX;

		if (re == 0)
		{
			deh_warning("Rounds played requirement is %d for condition ID %d", re, id+1);
			return;
		}

		if (params[2])
		{
			if (fastcmp(params[2], "RACE"))
				x1 = GDGT_RACE;
			else if (fastcmp(params[2], "BATTLE"))
				x1 = GDGT_BATTLE;
			else if (fastcmp(params[2], "PRISONS"))
				x1 = GDGT_PRISONS;
			else if (fastcmp(params[2], "SPECIAL"))
				x1 = GDGT_SPECIAL;
			else if (fastcmp(params[2], "CUSTOM"))
				x1 = GDGT_CUSTOM;
			else
			{
				deh_warning("gametype requirement \"%s\" invalid for condition ID %d", params[2], id+1);
				return;
			}
		}
	}
	else if (fastcmp(params[0], "TOTALRINGS"))
	{
		PARAMCHECK(1);
		ty = UC_TOTALRINGS;
		re = atoi(params[1]);

		if (re < 2 || re > GDMAX_RINGS)
		{
			deh_warning("Total Rings requirement %d out of range (%d - %d) for condition ID %d", re, 2, GDMAX_RINGS, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "TOTALTUMBLETIME"))
	{
		PARAMCHECK(1);
		ty = UC_TOTALTUMBLETIME;
		re = get_number(params[1]);
	}
	else if (fastcmp(params[0], "GAMECLEAR"))
	{
		ty = UC_GAMECLEAR;
		re = (params[1]) ? atoi(params[1]) : 1;
	}
	else if (fastcmp(params[0], "OVERALLTIME"))
	{
		PARAMCHECK(1);
		ty = UC_OVERALLTIME;
		re = get_number(params[1]);
	}
	else if ((offset=0) || fastcmp(params[0], "MAPVISITED")
	||        (++offset && fastcmp(params[0], "MAPBEATEN"))
	||        (++offset && fastcmp(params[0], "MAPENCORE"))
	||        (++offset && fastcmp(params[0], "MAPSPBATTACK"))
	||        (++offset && fastcmp(params[0], "MAPMYSTICMELODY")))
	{
		PARAMCHECK(1);
		ty = UC_MAPVISITED + offset;
		re = G_MapNumber(params[1]);

		if (re >= nummapheaders)
		{
			deh_warning("Invalid level %s for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "MAPTIME"))
	{
		PARAMCHECK(2);
		ty = UC_MAPTIME;
		re = get_number(params[2]);
		x1 = G_MapNumber(params[1]);

		if (x1 >= nummapheaders)
		{
			deh_warning("Invalid level %s for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "CHARACTERWINS"))
	{
		PARAMCHECK(2);
		ty = UC_CHARACTERWINS;
		stringvar = Z_StrDup(params[1]);
		re = -1;
		x1 = atoi(params[2]);
	}
	else if (fastcmp(params[0], "ALLCUPRECORDS"))
	{
		ty = UC_ALLCUPRECORDS;

		re = -1;
		x1 = 0;
		x2 = KARTSPEED_EASY;

		if (params[1])
		{
			if (!fastcmp(params[1], "ALL"))
			{
				cupheader_t *cup = SOChelper_cupbyname(params[1]);

				if (!cup)
				{
					deh_warning("Invalid cup %s for condition ID %d", params[1], id+1);
					return;
				}

				re = cup->id;
			}

			if (params[2])
			{
				if (!fastcmp(params[1], "ANY"))
				{
					if ((offset=0) || fastcmp(params[2], "GOLD")
					||    (++offset && fastcmp(params[2], "SILVER"))
					||    (++offset && fastcmp(params[2], "BRONZE")))
					{
						x1 = offset + 1;
					}
					else
					{
						deh_warning("placement requirement \"%s\" invalid for condition ID %d", params[2], id+1);
						return;
					}
				}

				if (params[3])
				{
					if (fastcmp(params[3], "NORMAL"))
						x2 = KARTSPEED_NORMAL;
					else if (fastcmp(params[3], "HARD"))
						x2 = KARTSPEED_HARD;
					else if (fastcmp(params[3], "MASTER"))
						x2 = KARTGP_MASTER;
					else
					{
						deh_warning("difficulty requirement \"%s\" invalid for condition ID %d", params[3], id+1);
						return;
					}
				}
			}
		}
	}
	else if ((offset=0) || fastcmp(params[0], "ALLCHAOS")
	||        (++offset && fastcmp(params[0], "ALLSUPER"))
	||        (++offset && fastcmp(params[0], "ALLEMERALDS")))
	{
		//PARAMCHECK(1);
		ty = UC_ALLCHAOS + offset;
		re = KARTSPEED_NORMAL;
		if (params[1])
		{
			if (fastcmp(params[1], "NORMAL"))
				;
			else if (fastcmp(params[1], "HARD"))
				re = KARTSPEED_HARD;
			else if (fastcmp(params[1], "MASTER"))
				re = KARTGP_MASTER;
			else
			{
				deh_warning("difficulty requirement \"%s\" invalid for condition ID %d", params[1], id+1);
				return;
			}
		}
	}
	else if (fastcmp(params[0], "TOTALMEDALS"))
	{
		PARAMCHECK(1);
		ty = UC_TOTALMEDALS;
		re = atoi(params[1]);
	}
	else if (fastcmp(params[0], "EMBLEM"))
	{
		PARAMCHECK(1);
		ty = UC_EMBLEM;
		re = atoi(params[1]);

		if (re <= 0 || re > MAXEMBLEMS)
		{
			deh_warning("Emblem %d out of range (1 - %d) for condition ID %d", re, MAXEMBLEMS, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "UNLOCKABLE"))
	{
		PARAMCHECK(1);
		ty = UC_UNLOCKABLE;
		re = atoi(params[1]);

		if (re <= 0 || re > MAXUNLOCKABLES)
		{
			deh_warning("Unlockable %d out of range (1 - %d) for condition ID %d", re, MAXUNLOCKABLES, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "CONDITIONSET"))
	{
		PARAMCHECK(1);
		ty = UC_CONDITIONSET;
		re = atoi(params[1]);

		if (re <= 0 || re > MAXCONDITIONSETS)
		{
			deh_warning("Condition set %d out of range (1 - %d) for condition ID %d", re, MAXCONDITIONSETS, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "UNLOCKPERCENT"))
	{
		PARAMCHECK(1);
		ty = UC_UNLOCKPERCENT;
		re = atoi(params[1]);
		x1 = SECRET_NONE;

		// Valid percentages only!
		if (re <= 0 || re > 100)
		{
			deh_warning("Condition percent %d out of range (1 - 100) for condition ID %d", re, id+1);
			return;
		}

		if (params[2] && params[2][0])
		{
			x1 = parseunlockabletype(params[2]);

			if (x1 <= SECRET_NONE || x1 >= SECRET_ONEPERBOARD)
			{
				deh_warning("Condition challenge type \"%s\" invalid for condition ID %d", params[2], id+1);
				return;
			}

			x2 = 0;
			if (params[3])
			{
				// fudge value
				x2 = atoi(params[3]);
			}
		}
		else
			x2 = 1; // guaranteed fudge for raw Unlockables count
	}
	else if ((offset=0) || fastcmp(params[0], "ADDON")
	||        (++offset && fastcmp(params[0], "CREDITS"))
	||        (++offset && fastcmp(params[0], "REPLAY"))
	||        (++offset && fastcmp(params[0], "CRASH"))
	||        (++offset && fastcmp(params[0], "TUTORIALSKIP"))
	||        (++offset && fastcmp(params[0], "TUTORIALDONE"))
	||        (++offset && fastcmp(params[0], "PLAYGROUNDROUTE")))
	{
		//PARAMCHECK(1);
		ty = UC_ADDON + offset;
	}
	else if (fastcmp(params[0], "SPRAYCAN"))
	{
		PARAMCHECK(1);
		ty = UC_SPRAYCAN;
		re = get_skincolor(params[1]);

		// Force at head of the list?
		x1 = (params[2] && (params[2][0] == 'Y' || params[2][0] == 'T')) ? 1 : 0;
	}
	else if (fastcmp(params[0], "PRISONEGGCD"))
	{
		ty = UC_PRISONEGGCD;
		re = NEXTMAP_INVALID;

		if (params[1])
		{
			re = G_MapNumber(params[1]);

			if (re >= nummapheaders)
			{
				deh_warning("Invalid level %s for condition ID %d", params[1], id+1);
				return;
			}
		}
	}
	else if ((offset=0) || fastcmp(params[0], "AND")
	||        (++offset && fastcmp(params[0], "THEN"))
	||        (++offset && fastcmp(params[0], "COMMA")))
	{
		//PARAMCHECK(1);
		ty = UC_AND + offset;
	}
	else if ((offset=0) || fastcmp(params[0], "PREFIX_GRANDPRIX")
	||        (++offset && fastcmp(params[0], "PREFIX_BONUSROUND"))
	||        (++offset && fastcmp(params[0], "PREFIX_TIMEATTACK"))
	||        (++offset && fastcmp(params[0], "PREFIX_PRISONBREAK"))
	||        (++offset && fastcmp(params[0], "PREFIX_SEALEDSTAR")))
	{
		//PARAMCHECK(1);
		ty = UCRP_PREFIX_GRANDPRIX + offset;
	}
	else if ((offset=0) || fastcmp(params[0], "PREFIX_ISMAP")
	||        (++offset && fastcmp(params[0], "ISMAP")))
	{
		PARAMCHECK(1);
		ty = UCRP_PREFIX_ISMAP + offset;
		re = G_MapNumber(params[1]);

		if (re >= nummapheaders)
		{
			deh_warning("Invalid level %s for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "ISCHARACTER"))
	{
		PARAMCHECK(1);
		ty = UCRP_ISCHARACTER;
		stringvar = Z_StrDup(params[1]);
		re = -1;
	}
	else if (fastcmp(params[0], "ISENGINECLASS"))
	{
		PARAMCHECK(1);
		ty = UCRP_ISENGINECLASS;
		if (!params[1][1]
			&& params[1][0] >= 'A' && params[1][0] <= 'J')
		{
			re = params[1][0] - 'A';
		}
		else
		{
			deh_warning("engine class requirement \"%s\" invalid for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "HASFOLLOWER"))
	{
		PARAMCHECK(1);
		ty = UCRP_HASFOLLOWER;
		stringvar = Z_StrDup(params[1]);
		re = -1;
	}
	else if (fastcmp(params[0], "ISDIFFICULTY"))
	{
		PARAMCHECK(1);
		ty = UCRP_ISDIFFICULTY;
		{
			if (fastcmp(params[1], "NORMAL"))
				re = KARTSPEED_NORMAL;
			else if (fastcmp(params[1], "HARD"))
				re = KARTSPEED_HARD;
			else if (fastcmp(params[1], "MASTER"))
				re = KARTGP_MASTER;
			else
			{
				deh_warning("difficulty requirement \"%s\" invalid for condition ID %d", params[1], id+1);
				return;
			}
		}
	}
	else if (fastcmp(params[0], "ISGEAR"))
	{
		PARAMCHECK(1);
		ty = UCRP_ISGEAR;
		re = atoi(params[1]) - 1;
		if (re < KARTSPEED_NORMAL || re > KARTSPEED_HARD)
		{
			deh_warning("gear requirement \"%s\" invalid for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "PODIUMCUP"))
	{
		PARAMCHECK(1);
		ty = UCRP_PODIUMCUP;

		re = -1;
		if (!fastcmp(params[1], "ANY"))
		{
			cupheader_t *cup = SOChelper_cupbyname(params[1]);

			if (!cup)
			{
				deh_warning("Invalid cup %s for condition ID %d", params[1], id+1);
				return;
			}

			re = cup->id;
		}

		if (params[2])
		{
			if (params[2][0] && !params[2][1])
			{
				x2 = 1;

				switch (params[2][0])
				{
					case 'E': { x1 = GRADE_E; break; }
					case 'D': { x1 = GRADE_D; break; }
					case 'C': { x1 = GRADE_C; break; }
					case 'B': { x1 = GRADE_B; break; }
					case 'A': { x1 = GRADE_A; break; }
					case 'S': { x1 = GRADE_S; break; }
					default:
						deh_warning("Invalid grade %s for condition ID %d", params[2], id+1);
						return;
				}
			}
			else if ((offset=0) || fastcmp(params[2], "GOLD")
				||    (++offset && fastcmp(params[2], "SILVER"))
				||    (++offset && fastcmp(params[2], "BRONZE")))
			{
				x1 = offset + 1;
			}
			else
			{
				deh_warning("Invalid cup result %s for condition ID %d", params[2], id+1);
				return;
			}

		}
	}
	else if ((offset=0) || fastcmp(params[0], "PODIUMEMERALD")
	||        (++offset && fastcmp(params[0], "PODIUMPRIZE"))
	||        (++offset && fastcmp(params[0], "PODIUMNOCONTINUES")))
	{
		//PARAMCHECK(1);
		ty = UCRP_PODIUMEMERALD + offset;
	}
	else if ((offset=0) || fastcmp(params[0], "FINISHCOOL")
	||        (++offset && fastcmp(params[0], "FINISHPERFECT"))
	||        (++offset && fastcmp(params[0], "FINISHALLPRISONS"))
	||        (++offset && fastcmp(params[0], "SURVIVE"))
	||        (++offset && fastcmp(params[0], "NOCONTEST"))
	||        (++offset && fastcmp(params[0], "SMASHUFO"))
	||        (++offset && fastcmp(params[0], "CHASEDBYSPB")))
	{
		//PARAMCHECK(1);
		ty = UCRP_FINISHCOOL + offset;
	}
	else if (fastcmp(params[0], "MAKERETIRE"))
	{
		PARAMCHECK(1);
		ty = UCRP_MAKERETIRE;
		stringvar = Z_StrDup(params[1]);
		re = -1;
	}
	else if ((offset=0) || fastcmp(params[0], "FINISHPLACE")
	||        (++offset && fastcmp(params[0], "FINISHPLACEEXACT")))
	{
		PARAMCHECK(1);
		ty = UCRP_FINISHPLACE + offset;
		re = atoi(params[1]);

		if (re < 1 || re > MAXPLAYERS)
		{
			deh_warning("Invalid place %s for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "FINISHGRADE"))
	{
		PARAMCHECK(1);
		ty = UCRP_FINISHGRADE;

		re = -1;
		if (!params[1][1])
		{
			switch (params[1][0])
			{
				case 'E': { re = GRADE_E; break; }
				case 'D': { re = GRADE_D; break; }
				case 'C': { re = GRADE_C; break; }
				case 'B': { re = GRADE_B; break; }
				case 'A': { re = GRADE_A; break; }
				default: { break; }
			}
		}

		if (re == -1)
		{
			deh_warning("Invalid grade %s for condition ID %d", params[1], id+1);
				return;
		}
	}
	else if ((offset=0) || fastcmp(params[0], "FINISHTIME")
	||        (++offset && fastcmp(params[0], "FINISHTIMEEXACT"))
	||        (++offset && fastcmp(params[0], "FINISHTIMELEFT")))
	{
		PARAMCHECK(1);
		ty = UCRP_FINISHTIME + offset;
		re = get_number(params[1]);

		if (re < 0)
		{
			deh_warning("Invalid time %s for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if ((offset=0) || fastcmp(params[0], "RINGS")
	||        (++offset && fastcmp(params[0], "RINGSEXACT")))
	{
		PARAMCHECK(1);
		ty = UCRP_RINGS + offset;
		re = get_number(params[1]);

		if (re < -20 || re > 20)
		{
			deh_warning("Invalid ring count %d for condition ID %d", re, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "SPEEDOMETER"))
	{
		PARAMCHECK(1);
		ty = UCRP_SPEEDOMETER;
		re = get_number(params[1]);

		if (re < 100 || re > 999)
		{
			deh_warning("Speed percent %d out of range (100 - 999) for condition ID %d", re, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "DRAFTDURATION"))
	{
		PARAMCHECK(1);
		ty = UCRP_DRAFTDURATION;
		re = get_number(params[1]);

		if (re < 5)
		{
			deh_warning("Duration %d seconds too low for condition ID %d", re, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "GROWCONSECUTIVEBEAMS"))
	{
		PARAMCHECK(1);
		ty = UCRP_GROWCONSECUTIVEBEAMS;
		re = get_number(params[1]);

		if (re < 2 || re > UINT8_MAX)
		{
			deh_warning("Touch count %d out of range (2 - %u) for condition ID %d", re, UINT8_MAX, id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "TRIGGER"))
	{
		PARAMCHECK(1);
		ty = UCRP_TRIGGER;
		re = atoi(params[1]);

		// constrained by 32 bits
		if (re < 0 || re > 31)
		{
			deh_warning("Trigger ID %d out of range (0 - 31) for condition ID %d", re, id+1);
			return;
		}
	}
	else if ((offset=0) || fastcmp(params[0], "FALLOFF")
	||        (++offset && fastcmp(params[0], "TOUCHOFFROAD"))
	||        (++offset && fastcmp(params[0], "TOUCHSNEAKERPANEL"))
	||        (++offset && fastcmp(params[0], "RINGDEBT"))
	||        (++offset && fastcmp(params[0], "FAULTED")))
	{
		PARAMCHECK(1);
		ty = UCRP_FALLOFF + offset;
		re = 1;

		if (params[1][0] == 'F' || params[1][0] == 'N' || params[1][0] == '0')
			re = 0;
	}
	else if ((offset=0) || fastcmp(params[0], "TRIPWIREHYUU")
	||        (++offset && fastcmp(params[0], "WHIPHYUU"))
	||        (++offset && fastcmp(params[0], "SPBNEUTER"))
	||        (++offset && fastcmp(params[0], "LANDMINEDUNK"))
	||        (++offset && fastcmp(params[0], "HITMIDAIR"))
	||        (++offset && fastcmp(params[0], "HITDRAFTERLOOKBACK"))
	||        (++offset && fastcmp(params[0], "GIANTRACERSHRUNKENORBI"))
	||        (++offset && fastcmp(params[0], "RETURNMARKTOSENDER"))
	||        (++offset && fastcmp(params[0], "ALLANCIENTGEARS")))
	{
		//PARAMCHECK(1);
		ty = UCRP_TRIPWIREHYUU + offset;
	}
	else if (fastcmp(params[0], "TRACKHAZARD"))
	{
		PARAMCHECK(1);
		ty = UCRP_TRACKHAZARD;
		re = 1;
		x1 = -1;

		if (params[1][0] == 'F' || params[1][0] == 'N' || params[1][0] == '0')
			re = 0;

		if (params[2])
		{
			if (fastcmp(params[2], "FINAL"))
				x1 = -2;
			else
			{
				x1 = atoi(params[2]);

				if (re < 0 || re > MAX_LAPS)
				{
					deh_warning("Lap number %d out of range (0 - %u) for condition ID %d", x1, MAX_LAPS, id+1);
					return;
				}
			}
		}
	}
	else if (fastcmp(params[0], "TARGETATTACKMETHOD"))
	{
		PARAMCHECK(1);
		ty = UCRP_TARGETATTACKMETHOD;

		// See targetdamaging_t
		if (fastcmp(params[1], "BOOST"))
			re = UFOD_BOOST;
		else if (fastcmp(params[1], "WHIP"))
			re = UFOD_WHIP;
		else if (fastcmp(params[1], "BANANA"))
			re = UFOD_BANANA;
		else if (fastcmp(params[1], "ORBINAUT"))
			re = UFOD_ORBINAUT;
		else if (fastcmp(params[1], "JAWZ"))
			re = UFOD_JAWZ;
		else if (fastcmp(params[1], "SPB"))
			re = UFOD_SPB;
		else if (fastcmp(params[1], "GACHABOM"))
			re = UFOD_GACHABOM;
		else
		{
			deh_warning("Unknown attack method %s for condition ID %d", params[1], id+1);
			return;
		}
	}
	else if (fastcmp(params[0], "GACHABOMMISER"))
	{
		//PARAMCHECK(1);
		ty = UCRP_GACHABOMMISER;
	}
	else
	{
		deh_warning("Invalid condition name %s for condition ID %d", params[0], id+1);
		return;
	}

setcondition:
	M_AddRawCondition(set, (UINT8)id, ty, re, x1, x2, stringvar);

#undef MAXCONDITIONPARAMS
}

void readconditionset(MYFILE *f, UINT16 setnum)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word = s;
	char *word2;
	char *tmp;
	UINT16 id;
	UINT8 previd = 0;

	M_ClearConditionSet(setnum);

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;
			//strupr(word2);

			if (fastncmp(word, "CONDITION", 9))
			{
				id = atoi(word + 9);
				if (id == 0)
				{
					deh_warning("Condition set %d: unknown word '%s'", setnum+1, word);
					continue;
				}
				if (previd > id)
				{
					// out of order conditions can cause problems, so enforce proper order
					deh_warning("Condition set %d: conditions are out of order, ignoring this line", setnum+1);
					continue;
				}
				if (id > UINT8_MAX)
				{
					deh_warning("Condition set %d: too many Condition# types, ignoring this line", setnum+1);
					continue;
				}
				previd = (UINT8)id;

				readcondition(setnum, (UINT8)id, word2);
			}
			else
				deh_warning("Condition set %d: unknown word '%s'", setnum, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readmaincfg(MYFILE *f, boolean mainfile)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word = s;
	char *word2;
	char *tmp;
	INT32 value;
	boolean doClearLevels = false;

#ifdef DEVELOP
	(void)mainfile;
#endif

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;
			strupr(word2);

			value = atoi(word2); // used for numerical settings

			if (fastcmp(word, "GAMEDATA"))
			{
				size_t filenamelen;

				// Check the data filename so that mods
				// can't write arbitrary files.
				if (!GoodDataFileName(word2))
					I_Error("Maincfg: bad data file name '%s'\n", word2);

				G_SaveGameData();
				strlcpy(gamedatafilename, word2, sizeof (gamedatafilename));
				strlwr(gamedatafilename);
				savemoddata = true;
				majormods = false;
				gamedata->loaded = false;

				// Also save a time attack folder
				filenamelen = strlen(gamedatafilename)-4;  // Strip off the extension
				filenamelen = min(filenamelen, sizeof (timeattackfolder));
				strncpy(timeattackfolder, gamedatafilename, filenamelen);
				timeattackfolder[min(filenamelen, sizeof (timeattackfolder) - 1)] = '\0';

				strcpy(savegamename, timeattackfolder);
				strlcat(savegamename, "%u.ssg", sizeof(savegamename));
				// can't use sprintf since there is %u in savegamename
				strcatbf(savegamename, srb2home, PATHSEP);

				strcpy(gpbackup, va("gp%s.bkp", timeattackfolder));
				strcatbf(gpbackup, srb2home, PATHSEP);

				refreshdirmenu |= REFRESHDIR_GAMEDATA;
				gamedataadded = true;
				titlechanged = true;

				clear_unlockables();
				clear_conditionsets();
				clear_emblems();
				//clear_levels();
				doClearLevels = true;

				G_ClearRecords();
				M_ClearStats();
				M_ClearSecrets();

				// Don't softlock the Stereo on if you won't be able to access it anymore!?
				if (soundtest.playing && M_SecretUnlocked(SECRET_SOUNDTEST, true) == false)
					S_SoundTestStop();
			}
#ifndef DEVELOP
			else if (!mainfile && !gamedataadded)
			{
				deh_warning("You must define a custom gamedata to use \"%s\"", word);
			}
#endif
			else if (fastcmp(word, "CLEARLEVELS"))
			{
				doClearLevels = (UINT8)(value == 0 || word2[0] == 'F' || word2[0] == 'N');
			}
			else if (fastcmp(word, "EXECCFG"))
			{
				if (strchr(word2, '.'))
					COM_BufAddText(va("exec \"%s\" -immediate\n", word2));
				else
				{
					lumpnum_t lumpnum;
					char newname[9];

					strncpy(newname, word2, 8);

					newname[8] = '\0';

					lumpnum = W_CheckNumForName(newname);

					if (lumpnum == LUMPERROR || W_LumpLength(lumpnum) == 0)
						CONS_Debug(DBG_SETUP, "SOC Error: script lump %s not found/not valid.\n", newname);
					else
						COM_BufInsertText(W_CacheLumpNum(lumpnum, PU_CACHE));
				}
			}
			else if (fastcmp(word, "INVULNTICS"))
			{
				invulntics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "SNEAKERTICS"))
			{
				sneakertics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "FLASHINGTICS"))
			{
				flashingtics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "TAILSFLYTICS"))
			{
				tailsflytics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "UNDERWATERTICS"))
			{
				underwatertics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "SPACETIMETICS"))
			{
				spacetimetics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "EXTRALIFETICS"))
			{
				extralifetics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "NIGHTSLINKTICS"))
			{
				nightslinktics = (UINT16)get_number(word2);
			}
			else if (fastcmp(word, "GAMEOVERTICS"))
			{
				gameovertics = get_number(word2);
			}
			else if (fastcmp(word, "AMMOREMOVALTICS"))
			{
				ammoremovaltics = get_number(word2);
			}
			else if (fastcmp(word, "INTROTOPLAY"))
			{
				introtoplay = (UINT8)get_number(word2);
				// range check, you morons.
				if (introtoplay > 128)
					introtoplay = 128;
				introchanged = true;
			}
			else if (fastcmp(word, "CREDITSCUTSCENE"))
			{
				g_credits_cutscene = (UINT8)get_number(word2);
				// range check, you morons.
				if (g_credits_cutscene > 128)
					g_credits_cutscene = 128;
			}
			else if (fastcmp(word, "USESEAL"))
			{
				useSeal = (UINT8)(value || word2[0] == 'T' || word2[0] == 'Y');
			}
			else if (fastcmp(word, "LOOPTITLE"))
			{
				looptitle = (value != 0 || word2[0] == 'T' || word2[0] == 'Y');
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLEMAP"))
			{
				Z_Free(titlemap);
				titlemap = Z_StrDup(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "TUTORIALPLAYGROUNDMAP"))
			{
				Z_Free(tutorialplaygroundmap);
				tutorialplaygroundmap = Z_StrDup(word2);
			}
			else if (fastcmp(word, "TUTORIALCHALLENGEMAP"))
			{
				Z_Free(tutorialchallengemap);
				tutorialchallengemap = Z_StrDup(word2);
			}
			else if (fastcmp(word, "GAMESTARTCHALLENGE"))
			{
				INT32 val = get_number(word2) - 1;
				if (val < 0 || val >= MAXUNLOCKABLES)
					gamestartchallenge = MAXUNLOCKABLES;
				else
					gamestartchallenge = (UINT16)val;
			}
			else if (fastcmp(word, "HIDETITLEPICS") || fastcmp(word, "TITLEPICSHIDE"))
			{
				hidetitlepics = (boolean)(value != 0 || word2[0] == 'T' || word2[0] == 'Y');
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLEPICSMODE"))
			{
				if (fastcmp(word2, "USER"))
					ttmode = TTMODE_USER;
				else if (fastcmp(word2, "HIDE") || fastcmp(word2, "HIDDEN") || fastcmp(word2, "NONE"))
				{
					ttmode = TTMODE_USER;
					ttname[0] = 0;
					hidetitlepics = true;
				}
				else if (fastcmp(word2, "RINGRACERS"))
					ttmode = TTMODE_RINGRACERS;
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLEPICSNAME"))
			{
				strncpy(ttname, word2, 9);
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLEPICSX"))
			{
				ttx = (INT16)get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLEPICSY"))
			{
				tty = (INT16)get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLEPICSLOOP"))
			{
				ttloop = (INT16)get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLEPICSTICS"))
			{
				tttics = (UINT16)get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLESCROLLSPEED") || fastcmp(word, "TITLESCROLLXSPEED"))
			{
				titlescrollxspeed = get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLESCROLLYSPEED"))
			{
				titlescrollyspeed = get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "TITLESMUSICSTART"))
			{
				titlemusicstart = (UINT32)get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "NUMDEMOS"))
			{
				numDemos = (UINT8)get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "DEMODELAYTIME"))
			{
				demoDelayTime = get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "DEMOIDLETIME"))
			{
				demoIdleTime = get_number(word2);
				titlechanged = true;
			}
			else if (fastcmp(word, "USE1UPSOUND"))
			{
				use1upSound = (UINT8)(value || word2[0] == 'T' || word2[0] == 'Y');
			}
			else if (fastcmp(word, "MAXXTRALIFE"))
			{
				maxXtraLife = (UINT8)get_number(word2);
			}
			else if (fastcmp(word, "RESETDATA"))
			{
				P_ResetData(value);
				titlechanged = true;
			}
			else if (fastcmp(word, "CUSTOMVERSION"))
			{
				strlcpy(customversionstring, word2, sizeof (customversionstring));
				//titlechanged = true;
			}
			else if (fastcmp(word, "BOOTMAP"))
			{
				Z_Free(bootmap);
				bootmap = Z_StrDup(word2);
				//titlechanged = true;
			}
			else if (fastcmp(word, "PODIUMMAP"))
			{
				Z_Free(podiummap);
				podiummap = Z_StrDup(word2);
			}
			else
				deh_warning("Maincfg: unknown word '%s'", word);
		}
	} while (!myfeof(f));

	if (doClearLevels)
	{
		clear_levels();
	}

	Z_Free(s);
}

void readwipes(MYFILE *f)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word = s;
	char *pword = word;
	char *word2;
	char *tmp;
	INT32 value;
	INT32 wipeoffset;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;
			value = atoi(word2); // used for numerical settings

			if (value < -1 || value > 99)
			{
				deh_warning("Wipes: bad value '%s'", word2);
				continue;
			}
			else if (value == -1)
				value = UINT8_MAX;

			// error catching
			wipeoffset = -1;

			if (fastncmp(word, "LEVEL_", 6))
			{
				pword = word + 6;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_level_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_level_final;
			}
			else if (fastncmp(word, "INTERMISSION_", 13))
			{
				pword = word + 13;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_intermission_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_intermission_final;
			}
			else if (fastncmp(word, "VOTING_", 7))
			{
				pword = word + 7;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_voting_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_voting_final;
			}
			else if (fastncmp(word, "CONTINUING_", 11))
			{
				pword = word + 11;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_continuing_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_continuing_final;
			}
			else if (fastncmp(word, "TITLESCREEN_", 12))
			{
				pword = word + 12;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_titlescreen_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_titlescreen_final;
			}
			else if (fastncmp(word, "MENU_", 11))
			{
				pword = word + 11;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_menu_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_menu_final;
			}
			else if (fastncmp(word, "CREDITS_", 8))
			{
				pword = word + 8;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_credits_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_credits_final;
				else if (fastcmp(pword, "INTERMEDIATE"))
					wipeoffset = wipe_credits_intermediate;
			}
			else if (fastncmp(word, "EVALUATION_", 11))
			{
				pword = word + 11;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_evaluation_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_evaluation_final;
			}
			else if (fastncmp(word, "CEREMONY_", 9))
			{
				pword = word + 9;
				if (fastcmp(pword, "TOBLACK"))
					wipeoffset = wipe_ceremony_toblack;
				else if (fastcmp(pword, "FINAL"))
					wipeoffset = wipe_ceremony_final;
			}
			else if (fastncmp(word, "ENCORE_", 7))
			{
				pword = word + 7;
				if (fastcmp(pword, "TOINVERT"))
					wipeoffset = wipe_encore_toinvert;
				else if (fastcmp(pword, "TOWHITE"))
					wipeoffset = wipe_encore_towhite;
			}

			if (wipeoffset < 0)
			{
				deh_warning("Wipes: unknown word '%s'", word);
				continue;
			}

			if (value == UINT8_MAX
			 && (wipeoffset <= wipe_level_toblack || wipeoffset >= wipe_encore_toinvert))
			{
				 // Cannot disable non-toblack wipes
				 // (or the level toblack wipe, or the special encore wipe)
				deh_warning("Wipes: can't disable wipe of type '%s'", word);
				continue;
			}

			wipedefs[wipeoffset] = (UINT8)value;
		}
	} while (!myfeof(f));

	Z_Free(s);
}

//
// SRB2KART
//

static void invalidateacrosscups(UINT16 map)
{
	cupheader_t *cup = kartcupheaders;
	UINT8 i;

	if (map >= nummapheaders)
		return;

	while (cup)
	{
		for (i = 0; i < CUPCACHE_MAX; i++)
		{
			if (cup->cachedlevels[i] != map)
				continue;
			cup->cachedlevels[i] = NEXTMAP_INVALID;
		}
		cup = cup->next;
	}

	mapheaderinfo[map]->cup = NULL;
}

static char *MapNameOrRemoval(char *name)
{
	if (name[0] == '\0'
	|| (name[0] == '/' && name[1] == '\0'))
		return NULL;

	return Z_StrDup(name);
}

void readcupheader(MYFILE *f, cupheader_t *cup)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2;
	char *tmp;
	INT32 i;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Set / reset word, because some things (Lua.) move it
			word = s;

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;
			i = atoi(word2); // used for numerical settings
			strupr(word2);

			if (fastcmp(word, "MONITOR"))
			{
				if (i > 0 && i < 10)
					cup->monitor = i;
				else if (!word2[0] || word2[1] != '\0' || word2[0] == '0')
					deh_warning("%s Cup: Invalid monitor type \"%s\" (should be 1-9 or A-Z)\n", cup->name, word2);
				else
					cup->monitor = (word2[0] - 'A') + 10;
			}
			else if (fastcmp(word, "REALNAME"))
			{
				deh_strlcpy(cup->realname, word2,
					sizeof(cup->realname), va("%s Cup: realname", cup->name));
			}
			else if (fastcmp(word, "ICON"))
			{
				deh_strlcpy(cup->icon, word2,
					sizeof(cup->icon), va("%s Cup: icon", cup->name));
			}
			else if (fastcmp(word, "LEVELLIST"))
			{
				while (cup->numlevels > 0)
				{
					cup->numlevels--;
					Z_Free(cup->levellist[cup->numlevels]);
					cup->levellist[cup->numlevels] = NULL;
					if (cup->cachedlevels[cup->numlevels] == NEXTMAP_INVALID)
						continue;
					invalidateacrosscups(cup->cachedlevels[cup->numlevels]);
				}

				tmp = strtok(word2,",");
				do {
					if (cup->numlevels >= MAXLEVELLIST)
					{
						deh_warning("%s Cup: reached max levellist (%d)\n", cup->name, MAXLEVELLIST);
						break;
					}

					cup->levellist[cup->numlevels] = Z_StrDup(tmp);
					cup->cachedlevels[cup->numlevels] = NEXTMAP_INVALID;
					cup->numlevels++;
				} while((tmp = strtok(NULL,",")) != NULL);
			}
			else if (fastcmp(word, "BONUSGAME"))
			{
				while (cup->numbonus > 0)
				{
					cup->numbonus--;
					Z_Free(cup->levellist[CUPCACHE_BONUS + cup->numbonus]);
					cup->levellist[CUPCACHE_BONUS + cup->numbonus] = NULL;
					if (cup->cachedlevels[CUPCACHE_BONUS + cup->numbonus] == NEXTMAP_INVALID)
						continue;
					invalidateacrosscups(cup->cachedlevels[CUPCACHE_BONUS + cup->numbonus]);
				}

				tmp = strtok(word2,",");
				do {
					if (cup->numbonus >= MAXBONUSLIST)
					{
						deh_warning("%s Cup: reached max bonus list (%d)\n", cup->name, MAXBONUSLIST);
						break;
					}

					cup->levellist[CUPCACHE_BONUS + cup->numbonus] = MapNameOrRemoval(tmp);
					cup->cachedlevels[CUPCACHE_BONUS + cup->numbonus] = NEXTMAP_INVALID;

					if (cup->levellist[CUPCACHE_BONUS + cup->numbonus] == NULL)
						break;

					cup->numbonus++;
				} while((tmp = strtok(NULL,",")) != NULL);
			}
			else if (fastcmp(word, "SPECIALSTAGE"))
			{
				invalidateacrosscups(cup->cachedlevels[CUPCACHE_SPECIAL]);
				Z_Free(cup->levellist[CUPCACHE_SPECIAL]);
				cup->levellist[CUPCACHE_SPECIAL] = MapNameOrRemoval(word2);
				cup->cachedlevels[CUPCACHE_SPECIAL] = NEXTMAP_INVALID;
			}
			else if (fastcmp(word, "ALTPODIUM"))
			{
				invalidateacrosscups(cup->cachedlevels[CUPCACHE_PODIUM]);
				Z_Free(cup->levellist[CUPCACHE_PODIUM]);
				cup->levellist[CUPCACHE_PODIUM] = MapNameOrRemoval(word2);
				cup->cachedlevels[CUPCACHE_PODIUM] = NEXTMAP_INVALID;
			}
			else if (fastcmp(word, "EMERALDNUM"))
			{
				if (!mainwads || (refreshdirmenu & REFRESHDIR_GAMEDATA))
				{
					if (i >= 0 && i <= 14)
						cup->emeraldnum = (UINT8)i;
					else
						deh_warning("%s Cup: invalid emerald number %d", cup->name, i);
				}
				else
				{
					deh_warning("You must define a custom gamedata to use \"%s\"", word);
				}
			}
			else if (fastcmp(word, "PLAYCREDITS"))
			{
				if (!mainwads || (refreshdirmenu & REFRESHDIR_GAMEDATA))
				{
					cup->playcredits = (i != 0 || word2[0] == 'T' || word2[0] == 'Y');
				}
				else
				{
					deh_warning("You must define a custom gamedata to use \"%s\"", word);
				}
			}
			else if (fastcmp(word, "HINTCONDITION"))
			{
				if (!mainwads || (refreshdirmenu & REFRESHDIR_GAMEDATA))
				{
					if (i > 0)
						cup->hintcondition = i-1;
				}
				else
				{
					deh_warning("You must define a custom gamedata to use \"%s\"", word);
				}
			}
			else
				deh_warning("%s Cup: unknown word '%s'", cup->name, word);
		}
	} while (!myfeof(f)); // finish when the line is empty

	Z_Free(s);
}

void readfollower(MYFILE *f)
{
	char *s;
	char *word, *word2;
	char *tmp;
	char testname[SKINNAMESIZE+1];

	boolean nameset;
	INT32 fallbackstate = 0;
	INT32 res;
	INT32 i;

	if (numfollowers >= MAXFOLLOWERS)
	{
		I_Error("Out of Followers\nLoad less addons to fix this.");
	}

	s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);

	// Ready the default variables for followers. We will overwrite them as we go! We won't set the name or states RIGHT HERE as this is handled down instead.
	followers[numfollowers].mode = FOLLOWERMODE_FLOAT;
	followers[numfollowers].scale = FRACUNIT;
	followers[numfollowers].bubblescale = 0; // No bubble by default
	followers[numfollowers].atangle = FixedAngle(230 * FRACUNIT);
	followers[numfollowers].dist = 32*FRACUNIT; // changed from 16 to 32 to better account for ogl models
	followers[numfollowers].height = 16*FRACUNIT;
	followers[numfollowers].zoffs = 32*FRACUNIT;
	followers[numfollowers].horzlag = 3*FRACUNIT;
	followers[numfollowers].vertlag = 6*FRACUNIT;
	followers[numfollowers].anglelag = 8*FRACUNIT;
	followers[numfollowers].bobspeed = TICRATE*2;
	followers[numfollowers].bobamp = 4*FRACUNIT;
	followers[numfollowers].hitconfirmtime = TICRATE;
	followers[numfollowers].ringtime = 10;
	followers[numfollowers].defaultcolor = FOLLOWERCOLOR_MATCH;
	followers[numfollowers].category = UINT8_MAX;
	followers[numfollowers].hornsound = sfx_horn00;
	strcpy(followers[numfollowers].icon, "MISSING");

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			word2 = strtok(NULL, " = ");

			if (!word2)
				break;

			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';

			if (fastcmp(word, "NAME"))
			{
				strlcpy(followers[numfollowers].name, word2, SKINNAMESIZE+1);
				nameset = true;
			}
			else if (fastcmp(word, "ICON"))
			{
				strlcpy(followers[numfollowers].icon, word2, 8+1);
				nameset = true;
			}
			else if (fastcmp(word, "CATEGORY"))
			{
				INT32 j;
				for (j = 0; j < numfollowercategories; j++)
				{
					if (!stricmp(followercategories[j].name, word2))
					{
						followers[numfollowers].category = j;
						break;
					}
				}

				if (j == numfollowercategories)
				{
					deh_warning("Follower %d: unknown follower category '%s'", numfollowers, word2);
				}
			}
			else if (fastcmp(word, "MODE"))
			{
				if (word2)
					strupr(word2);

				if (fastcmp(word2, "FLOAT") || fastcmp(word2, "DEFAULT"))
					followers[numfollowers].mode = FOLLOWERMODE_FLOAT;
				else if (fastcmp(word2, "GROUND"))
					followers[numfollowers].mode = FOLLOWERMODE_GROUND;
				else
					deh_warning("Follower %d: unknown follower mode '%s'", numfollowers, word2);
			}
			else if (fastcmp(word, "DEFAULTCOLOR"))
			{
				INT32 j;
				for (j = 0; j < numskincolors +2; j++)	// +2 because of Match and Opposite
				{
					if (!stricmp(Followercolor_cons_t[j].strvalue, word2))
					{
						followers[numfollowers].defaultcolor = Followercolor_cons_t[j].value;
						break;
					}
				}

				if (j == numskincolors+2)
				{
					deh_warning("Follower %d: unknown follower color '%s'", numfollowers, word2);
				}
			}
			else if (fastcmp(word, "HORNSOUND"))
			{
				followers[numfollowers].hornsound = get_number(word2);
			}
			else if (fastcmp(word, "SCALE"))
			{
				followers[numfollowers].scale = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "BUBBLESCALE"))
			{
				followers[numfollowers].bubblescale = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "ATANGLE"))
			{
				followers[numfollowers].atangle = (angle_t)(get_number(word2) * ANG1);
			}
			else if (fastcmp(word, "HORZLAG"))
			{
				followers[numfollowers].horzlag = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "VERTLAG"))
			{
				followers[numfollowers].vertlag = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "ANGLELAG"))
			{
				followers[numfollowers].anglelag = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "BOBSPEED"))
			{
				followers[numfollowers].bobspeed = (tic_t)get_number(word2);
			}
			else if (fastcmp(word, "BOBAMP"))
			{
				followers[numfollowers].bobamp = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "ZOFFSET") || (fastcmp(word, "ZOFFS")))
			{
				followers[numfollowers].zoffs = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "DISTANCE") || (fastcmp(word, "DIST")))
			{
				followers[numfollowers].dist = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "HEIGHT"))
			{
				followers[numfollowers].height = (fixed_t)get_number(word2);
			}
			else if (fastcmp(word, "IDLESTATE"))
			{
				if (word2)
					strupr(word2);
				followers[numfollowers].idlestate = get_number(word2);
				fallbackstate = followers[numfollowers].idlestate;
			}
			else if (fastcmp(word, "FOLLOWSTATE"))
			{
				if (word2)
					strupr(word2);
				followers[numfollowers].followstate = get_number(word2);
			}
			else if (fastcmp(word, "HURTSTATE"))
			{
				if (word2)
					strupr(word2);
				followers[numfollowers].hurtstate = get_number(word2);
			}
			else if (fastcmp(word, "LOSESTATE"))
			{
				if (word2)
					strupr(word2);
				followers[numfollowers].losestate = get_number(word2);
			}
			else if (fastcmp(word, "WINSTATE"))
			{
				if (word2)
					strupr(word2);
				followers[numfollowers].winstate = get_number(word2);
			}
			else if (fastcmp(word, "HITSTATE") || (fastcmp(word, "HITCONFIRMSTATE")))
			{
				if (word2)
					strupr(word2);
				followers[numfollowers].hitconfirmstate = get_number(word2);
			}
			else if (fastcmp(word, "HITTIME") || (fastcmp(word, "HITCONFIRMTIME")))
			{
				followers[numfollowers].hitconfirmtime = (tic_t)get_number(word2);
			}
			else if (fastcmp(word, "RINGSTATE"))
			{
				if (word2)
					strupr(word2);
				followers[numfollowers].ringstate = get_number(word2);
			}
			else if (fastcmp(word, "RINGTIME"))
			{
				followers[numfollowers].ringtime = (tic_t)get_number(word2);
			}
			else
			{
				deh_warning("Follower %d: unknown word '%s'", numfollowers, word);
			}
		}
	} while (!myfeof(f)); // finish when the line is empty

	if (!nameset)
	{
		// well this is problematic.
		strlcpy(followers[numfollowers].name, va("Follower%d", numfollowers), SKINNAMESIZE+1);
		strcpy(testname, followers[numfollowers].name);
	}
	else
	{
		strcpy(testname, followers[numfollowers].name);

		// now that the skin name is ready, post process the actual name to turn the underscores into spaces!
		for (i = 0; followers[numfollowers].name[i]; i++)
		{
			if (followers[numfollowers].name[i] == '_')
				followers[numfollowers].name[i] = ' ';
		}

		res = K_FollowerAvailable(followers[numfollowers].name);
		if (res > -1)	// yikes, someone else has stolen our name already
		{
			deh_warning("Follower%d: Name \"%s\" already in use!", numfollowers, testname);
			strlcpy(followers[numfollowers].name, va("Follower%d", numfollowers), SKINNAMESIZE+1);
		}
	}

	// fallbacks for variables
	// Print a warning if the variable is on a weird value and set it back to the minimum available if that's the case.

	if (followers[numfollowers].mode < FOLLOWERMODE_FLOAT || followers[numfollowers].mode >= FOLLOWERMODE__MAX)
	{
		followers[numfollowers].mode = FOLLOWERMODE_FLOAT;
		deh_warning("Follower '%s': Value for 'mode' should be between %d and %d.", testname, FOLLOWERMODE_FLOAT, FOLLOWERMODE__MAX-1);
	}

#define FALLBACK(field, field2, threshold, set) \
if ((signed)followers[numfollowers].field < threshold) \
{ \
	followers[numfollowers].field = set; \
	deh_warning("Follower '%s': Value for '%s' is too low! Minimum should be %d. Value was overwritten to %d.", testname, field2, threshold, set); \
} \

	FALLBACK(dist, "DIST", 0, 0);
	FALLBACK(height, "HEIGHT", 1, 1);
	FALLBACK(zoffs, "ZOFFS", 0, 0);
	FALLBACK(horzlag, "HORZLAG", FRACUNIT, FRACUNIT);
	FALLBACK(vertlag, "VERTLAG", FRACUNIT, FRACUNIT);
	FALLBACK(anglelag, "ANGLELAG", FRACUNIT, FRACUNIT);
	FALLBACK(bobamp, "BOBAMP", 0, 0);
	FALLBACK(bobspeed, "BOBSPEED", 0, 0);
	FALLBACK(hitconfirmtime, "HITCONFIRMTIME", 1, 1);
	FALLBACK(ringtime, "RINGTIME", 1, 1);
	FALLBACK(scale, "SCALE", 1, 1);				// No null/negative scale
	FALLBACK(bubblescale, "BUBBLESCALE", 0, 0);	// No negative scale

#undef FALLBACK

	// also check if we forgot states. If we did, we will set any missing state to the follower's idlestate.
	// Print a warning in case we don't have a fallback and set the state to S_INVISIBLE (rather than S_NULL) if unavailable.

#define NOSTATE(field, field2) \
if (!followers[numfollowers].field) \
{ \
	followers[numfollowers].field = fallbackstate ? fallbackstate : S_INVISIBLE; \
	if (!fallbackstate) \
		deh_warning("Follower '%s' is missing state definition for '%s', no idlestate fallback was found", testname, field2); \
} \

	NOSTATE(idlestate, "IDLESTATE");
	NOSTATE(followstate, "FOLLOWSTATE");
	NOSTATE(hurtstate, "HURTSTATE");
	NOSTATE(losestate, "LOSESTATE");
	NOSTATE(winstate, "WINSTATE");
	NOSTATE(hitconfirmstate, "HITCONFIRMSTATE");
	NOSTATE(ringstate, "RINGSTATE");
#undef NOSTATE

	if (!followers[numfollowers].hornsound)
	{
		followers[numfollowers].hornsound = sfx_horn00;
	}

	CONS_Printf("Added follower '%s'\n", testname);
	if (followers[numfollowers].category < numfollowercategories)
		followercategories[followers[numfollowers].category].numincategory++;
	numfollowers++; // add 1 follower
	Z_Free(s);
}

void readfollowercategory(MYFILE *f)
{
	char *s;
	char *word, *word2;
	char *tmp;

	boolean nameset;

	if (numfollowercategories == MAXFOLLOWERCATEGORIES)
	{
		I_Error("Out of Follower categories\nLoad less addons to fix this.");
	}

	s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);

	// Ready the default variables for followers. We will overwrite them as we go! We won't set the name or states RIGHT HERE as this is handled down instead.
	strcpy(followercategories[numfollowercategories].icon, "MISSING");
	followercategories[numfollowercategories].numincategory = 0;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			word = strtok(s, " ");
			if (word)
				strupr(word);
			else
				break;

			word2 = strtok(NULL, " = ");

			if (!word2)
				break;

			if (word2[strlen(word2)-1] == '\n')
				word2[strlen(word2)-1] = '\0';

			if (fastcmp(word, "NAME"))
			{
				strcpy(followercategories[numfollowercategories].name, word2);
				nameset = true;
			}
			else if (fastcmp(word, "ICON"))
			{
				strcpy(followercategories[numfollowercategories].icon, word2);
				nameset = true;
			}
		}
	} while (!myfeof(f)); // finish when the line is empty

	if (!nameset)
	{
		// well this is problematic.
		strcpy(followercategories[numfollowercategories].name, va("Followercategory%d", numfollowercategories)); // this is lazy, so what
	}

	CONS_Printf("Added follower category '%s'\n", followercategories[numfollowercategories].name);
	numfollowercategories++; // add 1 follower
	Z_Free(s);
}

void readweather(MYFILE *f, INT32 num)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char *word;
	char *word2;
	char *tmp;

	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;

			// First remove trailing newline, if there is one
			tmp = strchr(s, '\n');
			if (tmp)
				*tmp = '\0';

			tmp = strchr(s, '#');
			if (tmp)
				*tmp = '\0';
			if (s == tmp)
				continue; // Skip comment lines, but don't break.

			// Set / reset word
			word = s;

			// Get the part before the " = "
			tmp = strchr(s, '=');
			if (tmp)
				*(tmp-1) = '\0';
			else
				break;
			strupr(word);

			// Now get the part after
			word2 = tmp += 2;

			if (fastcmp(word, "TYPE"))
			{
				precipprops[num].type = get_mobjtype(word2);
			}
			else if (fastcmp(word, "EFFECTS"))
			{
				precipprops[num].effects = get_number(word2);
			}
			else
				deh_warning("Weather %d : unknown word '%s'", num, word);
		}
	} while (!myfeof(f));

	Z_Free(s);
}

//
//
//

mobjtype_t get_mobjtype(const char *word)
{ // Returns the value of MT_ enumerations
	mobjtype_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("MT_",word,3))
		word += 3; // take off the MT_
	for (i = 0; i < NUMMOBJFREESLOTS; i++) {
		if (!FREE_MOBJS[i])
			break;
		if (fastcmp(word, FREE_MOBJS[i]))
			return MT_FIRSTFREESLOT+i;
	}
	for (i = 0; i < MT_FIRSTFREESLOT; i++)
		if (fastcmp(word, MOBJTYPE_LIST[i]+3))
			return i;
	deh_warning("Couldn't find mobjtype named 'MT_%s'",word);
	return MT_NULL;
}

statenum_t get_state(const char *word)
{ // Returns the value of S_ enumerations
	statenum_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("S_",word,2))
		word += 2; // take off the S_
	for (i = 0; i < NUMSTATEFREESLOTS; i++) {
		if (!FREE_STATES[i])
			break;
		if (fastcmp(word, FREE_STATES[i]))
			return S_FIRSTFREESLOT+i;
	}
	for (i = 0; i < S_FIRSTFREESLOT; i++)
		if (fastcmp(word, STATE_LIST[i]+2))
			return i;
	deh_warning("Couldn't find state named 'S_%s'",word);
	return S_NULL;
}

skincolornum_t get_skincolor(const char *word)
{ // Returns the value of SKINCOLOR_ enumerations
	skincolornum_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("SKINCOLOR_",word,10))
		word += 10; // take off the SKINCOLOR_
	for (i = 0; i < NUMCOLORFREESLOTS; i++) {
		if (!FREE_SKINCOLORS[i])
			break;
		if (fastcmp(word, FREE_SKINCOLORS[i]))
			return SKINCOLOR_FIRSTFREESLOT+i;
	}
	for (i = 0; i < SKINCOLOR_FIRSTFREESLOT; i++)
		if (fastcmp(word, COLOR_ENUMS[i]))
			return i;
	deh_warning("Couldn't find skincolor named 'SKINCOLOR_%s'",word);
	return SKINCOLOR_GREEN;
}

spritenum_t get_sprite(const char *word)
{ // Returns the value of SPR_ enumerations
	spritenum_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("SPR_",word,4))
		word += 4; // take off the SPR_
	for (i = 0; i < NUMSPRITES; i++)
		if (!sprnames[i][4] && memcmp(word,sprnames[i],4)==0)
			return i;
	deh_warning("Couldn't find sprite named 'SPR_%s'",word);
	return SPR_NULL;
}

playersprite_t get_sprite2(const char *word)
{ // Returns the value of SPR2_ enumerations
	playersprite_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("SPR2_",word,5))
		word += 5; // take off the SPR2_
	for (i = 0; i < NUMPLAYERSPRITES; i++)
		if (!spr2names[i][4] && memcmp(word,spr2names[i],4)==0)
			return i;
	deh_warning("Couldn't find sprite named 'SPR2_%s'",word);
	return SPR2_STIN;
}

sfxenum_t get_sfx(const char *word)
{ // Returns the value of SFX_ enumerations
	sfxenum_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("SFX_",word,4))
		word += 4; // take off the SFX_
	else if (fastncmp("DS",word,2))
		word += 2; // take off the DS
	for (i = 0; i < NUMSFX; i++)
		if (S_sfx[i].name && fasticmp(word, S_sfx[i].name))
			return i;
	deh_warning("Couldn't find sfx named 'SFX_%s'",word);
	return sfx_None;
}

/*static INT16 get_gametype(const char *word)
{ // Returns the value of GT_ enumerations
	INT16 i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("GT_",word,3))
		word += 3; // take off the GT_
	for (i = 0; i < MAXGAMETYPES; i++)
		if (fastcmp(word, Gametype_ConstantNames[i]+3))
			return i;
	deh_warning("Couldn't find gametype named 'GT_%s'",word);
	return GT_COOP;
}*/

preciptype_t get_precip(const char *word)
{ // Returns the value of PRECIP_ enumerations
	preciptype_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("PRECIP_",word,4))
		word += 7; // take off the PRECIP_
	for (i = 0; i < MAXPRECIP; i++)
	{
		if (precipprops[i].name == NULL)
			break;
		if (fasticmp(word, precipprops[i].name))
			return i;
	}
	deh_warning("Couldn't find weather type named 'PRECIP_%s'",word);
	return PRECIP_NONE;
}

/// \todo Make ANY of this completely over-the-top math craziness obey the order of operations.
static fixed_t op_mul(fixed_t a, fixed_t b) { return a*b; }
static fixed_t op_div(fixed_t a, fixed_t b) { return a/b; }
static fixed_t op_add(fixed_t a, fixed_t b) { return a+b; }
static fixed_t op_sub(fixed_t a, fixed_t b) { return a-b; }
static fixed_t op_or(fixed_t a, fixed_t b) { return a|b; }
static fixed_t op_and(fixed_t a, fixed_t b) { return a&b; }
static fixed_t op_lshift(fixed_t a, fixed_t b) { return a<<b; }
static fixed_t op_rshift(fixed_t a, fixed_t b) { return a>>b; }

struct {
	const char c;
	fixed_t (*v)(fixed_t,fixed_t);
} OPERATIONS[] = {
	{'*',op_mul},
	{'/',op_div},
	{'+',op_add},
	{'-',op_sub},
	{'|',op_or},
	{'&',op_and},
	{'<',op_lshift},
	{'>',op_rshift},
	{0,NULL}
};

// Returns the full word, cut at the first symbol or whitespace
/*static char *read_word(const char *line)
{
	// Part 1: You got the start of the word, now find the end.
  const char *p;
	INT32 i;
	for (p = line+1; *p; p++) {
		if (*p == ' ' || *p == '\t')
			break;
		for (i = 0; OPERATIONS[i].c; i++)
			if (*p == OPERATIONS[i].c) {
				i = -1;
				break;
			}
		if (i == -1)
			break;
	}

	// Part 2: Make a copy of the word and return it.
	{
		size_t len = (p-line);
		char *word = malloc(len+1);
		M_Memcpy(word,line,len);
		word[len] = '\0';
		return word;
	}
}

static INT32 operation_pad(const char **word)
{ // Brings word the next operation and returns the operation number.
	INT32 i;
	for (; **word; (*word)++) {
		if (**word == ' ' || **word == '\t')
			continue;
		for (i = 0; OPERATIONS[i].c; i++)
			if (**word == OPERATIONS[i].c)
			{
				if ((**word == '<' && *(*word+1) == '<') || (**word == '>' && *(*word+1) == '>')) (*word)++; // These operations are two characters long.
				else if (**word == '<' || **word == '>') continue; // ... do not accept one character long.
				(*word)++;
				return i;
			}
		deh_warning("Unknown operation '%c'",**word);
		return -1;
	}
	return -1;
}

static void const_warning(const char *type, const char *word)
{
	deh_warning("Couldn't find %s named '%s'",type,word);
}

static fixed_t find_const(const char **rword)
{ // Finds the value of constants and returns it, bringing word to the next operation.
	INT32 i;
	fixed_t r;
	char *word = read_word(*rword);
	*rword += strlen(word);
	if ((*word >= '0' && *word <= '9') || *word == '-') { // Parse a number
		r = atoi(word);
		free(word);
		return r;
	}
	if (!*(word+1) && // Turn a single A-z symbol into numbers, like sprite frames.
	 ((*word >= 'A' && *word <= 'Z') || (*word >= 'a' && *word <= 'z'))) {
		r = R_Char2Frame(*word);
		free(word);
		return r;
	}
	if (fastncmp("MF_", word, 3)) {
		char *p = word+3;
		for (i = 0; MOBJFLAG_LIST[i]; i++)
			if (fastcmp(p, MOBJFLAG_LIST[i])) {
				free(word);
				return (1<<i);
			}

		// Not found error
		const_warning("mobj flag",word);
		free(word);
		return 0;
	}
	else if (fastncmp("MF2_", word, 4)) {
		char *p = word+4;
		for (i = 0; MOBJFLAG2_LIST[i]; i++)
			if (fastcmp(p, MOBJFLAG2_LIST[i])) {
				free(word);
				return (1<<i);
			}

		// Not found error
		const_warning("mobj flag2",word);
		free(word);
		return 0;
	}
	else if (fastncmp("MFE_", word, 4)) {
		char *p = word+4;
		for (i = 0; MOBJEFLAG_LIST[i]; i++)
			if (fastcmp(p, MOBJEFLAG_LIST[i])) {
				free(word);
				return (1<<i);
			}

		// Not found error
		const_warning("mobj eflag",word);
		free(word);
		return 0;
	}
	else if (fastncmp("PF_", word, 3)) {
		char *p = word+3;
		for (i = 0; PLAYERFLAG_LIST[i]; i++)
			if (fastcmp(p, PLAYERFLAG_LIST[i])) {
				free(word);
				return (1<<i);
			}
		if (fastcmp(p, "FULLSTASIS"))
			return PF_FULLSTASIS;

		// Not found error
		const_warning("player flag",word);
		free(word);
		return 0;
	}
	else if (fastncmp("S_",word,2)) {
		r = get_state(word);
		free(word);
		return r;
	}
	else if (fastncmp("SKINCOLOR_",word,10)) {
		r = get_skincolor(word);
		free(word);
		return r;
	}
	else if (fastncmp("MT_",word,3)) {
		r = get_mobjtype(word);
		free(word);
		return r;
	}
	else if (fastncmp("SPR_",word,4)) {
		r = get_sprite(word);
		free(word);
		return r;
	}
	else if (fastncmp("SFX_",word,4) || fastncmp("DS",word,2)) {
		r = get_sfx(word);
		free(word);
		return r;
	}
	else if (fastncmp("PW_",word,3)) {
		r = get_power(word);
		free(word);
		return r;
	}
	else if (fastncmp("MN_",word,3)) {
		r = get_menutype(word);
		free(word);
		return r;
	}
	else if (fastncmp("GT_",word,3)) {
		r = get_gametype(word);
		free(word);
		return r;
	}
	else if (fastncmp("GTR_", word, 4)) {
		char *p = word+4;
		for (i = 0; GAMETYPERULE_LIST[i]; i++)
			if (fastcmp(p, GAMETYPERULE_LIST[i])) {
				free(word);
				return (1<<i);
			}

		// Not found error
		const_warning("game type rule",word);
		free(word);
		return 0;
	}
	else if (fastncmp("TOL_", word, 4)) {
		char *p = word+4;
		for (i = 0; TYPEOFLEVEL[i].name; i++)
			if (fastcmp(p, TYPEOFLEVEL[i].name)) {
				free(word);
				return TYPEOFLEVEL[i].flag;
			}

		// Not found error
		const_warning("typeoflevel",word);
		free(word);
		return 0;
	}
	for (i = 0; INT_CONST[i].n; i++)
		if (fastcmp(word,INT_CONST[i].n)) {
			free(word);
			return INT_CONST[i].v;
		}

	// Not found error.
	const_warning("constant",word);
	free(word);
	return 0;
}*/
