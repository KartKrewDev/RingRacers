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
/// \file  dehacked.c
/// \brief Load dehacked file and change tables and text

#include "doomdef.h"

#include "m_cond.h"
#include "deh_soc.h"
#include "deh_tables.h"

boolean deh_loaded = false;

boolean gamedataadded = false;
boolean titlechanged = false;
boolean introchanged = false;

static int dbg_line;
static INT32 deh_num_warning = 0;

FUNCPRINTF void deh_warning(const char *first, ...)
{
	va_list argptr;
	char *buf = Z_Malloc(1000, PU_STATIC, NULL);

	va_start(argptr, first);
	vsnprintf(buf, 1000, first, argptr); // sizeof only returned 4 here. it didn't like that pointer.
	va_end(argptr);

	if(dbg_line == -1) // Not in a SOC, line number unknown.
		CONS_Alert(CONS_WARNING, "%s\n", buf);
	else
		CONS_Alert(CONS_WARNING, "Line %u: %s\n", dbg_line, buf);

	deh_num_warning++;

	Z_Free(buf);
}

void deh_strlcpy(char *dst, const char *src, size_t size, const char *warntext)
{
	size_t len = strlen(src)+1; // Used to determine if truncation has been done
	if (len > size)
		deh_warning("%s exceeds max length of %s", warntext, sizeu1(size-1));
	strlcpy(dst, src, size);
}

int freeslotusage[2][2] = {{0, 0}, {0, 0}}; // [S_, MT_][max, previous .wad's max]
void DEH_UpdateMaxFreeslots(void)
{
	freeslotusage[0][1] = freeslotusage[0][0];
	freeslotusage[1][1] = freeslotusage[1][0];
}

ATTRINLINE static FUNCINLINE unsigned char myfget_color(MYFILE *f)
{
	char c = *f->curpos++;
	if (c == '^') // oh, nevermind then.
		return '^';

	if (c >= '0' && c <= '9')
		return 0x80+(c-'0');

	c = tolower(c);

	if (c >= 'a' && c <= 'f')
		return 0x80+10+(c-'a');

	return 0x80; // Unhandled -- default to no color
}

ATTRINLINE static FUNCINLINE char myfget_hex(MYFILE *f)
{
	char c = *f->curpos++, endchr = 0;
	if (c == '\\') // oh, nevermind then.
		return '\\';

	if (c >= '0' && c <= '9')
		endchr += (c-'0') << 4;
	else if (c >= 'A' && c <= 'F')
		endchr += ((c-'A') + 10) << 4;
	else if (c >= 'a' && c <= 'f')
		endchr += ((c-'a') + 10) << 4;
	else // invalid. stop and return a question mark.
		return '?';

	c = *f->curpos++;
	if (c >= '0' && c <= '9')
		endchr += (c-'0');
	else if (c >= 'A' && c <= 'F')
		endchr += ((c-'A') + 10);
	else if (c >= 'a' && c <= 'f')
		endchr += ((c-'a') + 10);
	else // invalid. stop and return a question mark.
		return '?';

	return endchr;
}

char *myfgets(char *buf, size_t bufsize, MYFILE *f)
{
	size_t i = 0;
	if (myfeof(f))
		return NULL;
	// we need one byte for a null terminated string
	bufsize--;
	while (i < bufsize && !myfeof(f))
	{
		char c = *f->curpos++;
		if (c == '^')
			buf[i++] = myfget_color(f);
		else if (c == '\\')
			buf[i++] = myfget_hex(f);
		else if (c != '\r')
			buf[i++] = c;
		if (c == '\n')
			break;
	}
	buf[i] = '\0';

	dbg_line++;
	return buf;
}

char *myhashfgets(char *buf, size_t bufsize, MYFILE *f)
{
	size_t i = 0;
	if (myfeof(f))
		return NULL;
	// we need one byte for a null terminated string
	bufsize--;
	while (i < bufsize && !myfeof(f))
	{
		char c = *f->curpos++;
		if (c == '^')
			buf[i++] = myfget_color(f);
		else if (c == '\\')
			buf[i++] = myfget_hex(f);
		else if (c != '\r')
			buf[i++] = c;
		if (c == '\n') // Ensure debug line is right...
			dbg_line++;
		if (c == '#')
		{
			if (i > 0) // don't let i wrap past 0
				i--; // don't include hash char in string
			break;
		}
	}
	if (buf[i] != '#') // don't include hash char in string
		i++;
	buf[i] = '\0';

	return buf;
}

// Used when you do something invalid like read a bad item number
// to prevent extra unnecessary errors
static void ignorelines(MYFILE *f)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;
		}
	} while (!myfeof(f));
	Z_Free(s);
}

static void DEH_LoadDehackedFile(MYFILE *f, boolean mainfile)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char textline[MAXLINELEN];
	char *word;
	char *word2;
	INT32 i;

	if (!deh_loaded)
		initfreeslots();

	deh_num_warning = 0;

	gamedataadded = titlechanged = introchanged = false;

	// it doesn't test the version of SRB2 and version of dehacked file
	dbg_line = -1; // start at -1 so the first line is 0.
	while (!myfeof(f))
	{
		myfgets(s, MAXLINELEN, f);
		memcpy(textline, s, MAXLINELEN);
		if (s[0] == '\n' || s[0] == '#')
			continue;

		if (NULL != (word = strtok(s, " "))) {
			strupr(word);
			if (word[strlen(word)-1] == '\n')
				word[strlen(word)-1] = '\0';
		}
		if (word)
		{
			if (fastcmp(word, "FREESLOT"))
			{
				// This is not a major mod.
				readfreeslots(f);
				continue;
			}
			else if (fastcmp(word, "MAINCFG"))
			{
				G_SetGameModified(multiplayer, true);
				readmaincfg(f, mainfile);
				continue;
			}
			else if (fastcmp(word, "WIPES"))
			{
				// This is not a major mod.
				readwipes(f);
				continue;
			}
			//
			// SRB2KART
			//
			else if (fastcmp(word, "FOLLOWER"))
			{
				// This is not a major mod.
				readfollower(f);
				continue;
			}
			else if (fastcmp(word, "FOLLOWERCATEGORY"))
			{
				// This is not a major mod.
				readfollowercategory(f);
				continue;
			}

			word2 = strtok(NULL, " ");
			if (word2) {
				strupr(word2);
				if (word2[strlen(word2) - 1] == '\n')
					word2[strlen(word2) - 1] = '\0';
				i = atoi(word2);
			}
			else
				i = 0;

			if (word2)
			{
				if (fastcmp(word, "THING") || fastcmp(word, "MOBJ") || fastcmp(word, "OBJECT"))
				{
					if (i == 0 && word2[0] != '0') // If word2 isn't a number
						i = get_mobjtype(word2); // find a thing by name
					if (i < NUMMOBJTYPES && i > 0)
					{
						if (i < (MT_FIRSTFREESLOT+freeslotusage[1][1]))
						{
							G_SetGameModified(multiplayer, true); // Only a major mod if editing stuff that isn't your own!
						}

						readthing(f, i);
					}
					else
					{
						deh_warning("Thing %d out of range (1 - %d)", i, NUMMOBJTYPES-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SKINCOLOR") || fastcmp(word, "COLOR"))
				{
					if (i == 0 && word2[0] != '0') // If word2 isn't a number
						i = get_skincolor(word2); // find a skincolor by name
					if (i && i < numskincolors)
						readskincolor(f, i);
					else
					{
						deh_warning("Skincolor %d out of range (1 - %d)", i, numskincolors-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SPRITE2"))
				{
					if (i == 0 && word2[0] != '0') // If word2 isn't a number
						i = get_sprite2(word2); // find a sprite by name
					if (i < (INT32)free_spr2 && i >= (INT32)SPR2_FIRSTFREESLOT)
						readsprite2(f, i);
					else
					{
						deh_warning("Sprite2 number %d out of range (%d - %d)", i, SPR2_FIRSTFREESLOT, free_spr2-1);
						ignorelines(f);
					}
				}
#ifdef HWRENDER
				else if (fastcmp(word, "LIGHT"))
				{
					// TODO: Read lights by name
					if (i > 0 && i < NUMLIGHTS)
						readlight(f, i);
					else
					{
						deh_warning("Light number %d out of range (1 - %d)", i, NUMLIGHTS-1);
						ignorelines(f);
					}
				}
#endif
				else if (fastcmp(word, "LEVEL"))
				{
					size_t len = strlen(word2);
					if (len <= MAXMAPLUMPNAME-1)
					{
						readlevelheader(f, word2);
					}
					else
					{
						deh_warning("Map header's lumpname %s is too long (%s characters VS %d max)", word2, sizeu1(len), (MAXMAPLUMPNAME-1));
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "GAMETYPE"))
				{
					// Get the gametype name from textline
					// instead of word2, so that gametype names
					// aren't allcaps
					INT32 c;
					for (c = 0; c < MAXLINELEN; c++)
					{
						if (textline[c] == '\0')
							break;
						if (textline[c] == ' ')
						{
							char *gtname = (textline+c+1);
							if (gtname)
							{
								// remove funny characters
								INT32 j;
								for (j = 0; j < (MAXLINELEN - c); j++)
								{
									if (gtname[j] == '\0')
										break;
									if (gtname[j] < 32)
										gtname[j] = '\0';
								}
								readgametype(f, gtname);
							}
							break;
						}
					}
				}
				else if (fastcmp(word, "CUTSCENE"))
				{
					if (i > 0 && i < 129)
						readcutscene(f, i - 1);
					else
					{
						deh_warning("Cutscene number %d out of range (1 - 128)", i);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "PROMPT"))
				{
					if (i > 0 && i < MAX_PROMPTS)
						readtextprompt(f, i - 1);
					else
					{
						deh_warning("Prompt number %d out of range (1 - %d)", i, MAX_PROMPTS);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "FRAME") || fastcmp(word, "STATE"))
				{
					if (i == 0 && word2[0] != '0') // If word2 isn't a number
						i = get_state(word2); // find a state by name
					if (i < NUMSTATES && i > 0)
					{
						if (i < (S_FIRSTFREESLOT+freeslotusage[0][1]))
						{
							G_SetGameModified(multiplayer, true); // Only a major mod if editing stuff that isn't your own!
						}

						readframe(f, i);
					}
					else
					{
						deh_warning("Frame %d out of range (1 - %d)", i, NUMSTATES-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SOUND"))
				{
					if (i == 0 && word2[0] != '0') // If word2 isn't a number
						i = get_sfx(word2); // find a sound by name
					if (i < NUMSFX && i > 0)
						readsound(f, i);
					else
					{
						deh_warning("Sound %d out of range (1 - %d)", i, NUMSFX-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "EMBLEM"))
				{
					if (!mainfile && !gamedataadded)
					{
						deh_warning("You must define a custom gamedata to use \"%s\"", word);
						ignorelines(f);
					}
					else if (i > 0 && i <= MAXEMBLEMS)
					{
						if (numemblems < i)
						{
							// This is no longer strictly necessary... but I've left it in as an optimisation, because the datatype is now immensohuge, heh.
							numemblems = i;
						}
						reademblemdata(f, i);
					}
					else
					{
						deh_warning("Emblem number %d out of range (1 - %d)", i, MAXEMBLEMS);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "UNLOCKABLE"))
				{
					if (!mainfile && !gamedataadded)
					{
						deh_warning("You must define a custom gamedata to use \"%s\"", word);
						ignorelines(f);
					}
					else if (i > 0 && i <= MAXUNLOCKABLES)
						readunlockable(f, i - 1);
					else
					{
						deh_warning("Unlockable number %d out of range (1 - %d)", i, MAXUNLOCKABLES);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "CONDITIONSET"))
				{
					if (!mainfile && !gamedataadded)
					{
						deh_warning("You must define a custom gamedata to use \"%s\"", word);
						ignorelines(f);
					}
					else if (i > 0 && i <= MAXCONDITIONSETS)
						readconditionset(f, (UINT16)(i-1));
					else
					{
						deh_warning("Condition set number %d out of range (1 - %d)", i, MAXCONDITIONSETS);
						ignorelines(f);
					}
				}
				//
				// SRB2KART
				//
				else if (fastcmp(word, "CUP"))
				{
					size_t len = strlen(word2);
					if (len <= MAXCUPNAME-1)
					{
						cupheader_t *cup = kartcupheaders;
						cupheader_t *prev = NULL;
						UINT32 hash = quickncasehash(word2, MAXCUPNAME);

						while (cup)
						{
							if (hash == cup->namehash && fastcmp(cup->name, word2))
							{
								// Only a major mod if editing stuff that isn't your own!
								G_SetGameModified(multiplayer, true);
								break;
							}

							prev = cup;
							cup = cup->next;
						}

						// Nothing found, add to the end.
						if (!cup)
						{
							cup = Z_Calloc(sizeof (cupheader_t), PU_STATIC, NULL);

							deh_strlcpy(cup->name, word2,
								sizeof(cup->name), va("Cup header %s: name", word2));
							cup->namehash = hash;

							// Handle some variable init.
							cup->monitor = 1;
							cup->id = numkartcupheaders;
							cup->cache_cuplock = MAXUNLOCKABLES;
							cup->hintcondition = MAXCONDITIONSETS;
							for (i = 0; i < CUPCACHE_MAX; i++)
								cup->cachedlevels[i] = NEXTMAP_INVALID;

							char *start = strchr(word2, '_');
							if (start)
								start++;
							else
								start = word2;

							deh_strlcpy(cup->realname, start,
								sizeof(cup->realname), va("%s Cup: realname (default)", cup->name));

							// Check to see if we have any custom cup record data that we could substitute in.
							unloaded_cupheader_t *unloadedcup, *unloadedprev = NULL;
							for (unloadedcup = unloadedcupheaders; unloadedcup; unloadedprev = unloadedcup, unloadedcup = unloadedcup->next)
							{
								if (unloadedcup->namehash != hash)
									continue;

								if (strcasecmp(word2, unloadedcup->name) != 0)
									continue;

								// Copy in standings, etc.
								M_Memcpy(&cup->windata, &unloadedcup->windata, sizeof(cup->windata));

								// Remove this entry from the chain.
								if (unloadedprev)
								{
									unloadedprev->next = unloadedcup->next;
								}
								else
								{
									unloadedcupheaders = unloadedcup->next;
								}

								// Finally, free.
								Z_Free(unloadedcup);

								break;
							}

							if (prev != NULL)
								prev->next = cup;
							if (kartcupheaders == NULL)
								kartcupheaders = cup;
							numkartcupheaders++;
							CONS_Printf("Added cup %d ('%s')\n", cup->id, cup->name);
						}

						readcupheader(f, cup);

					}
					else
					{
						deh_warning("Cup header's name %s is too long (%s characters VS %d max)", word2, sizeu1(len), (MAXCUPNAME-1));
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "WEATHER") || fastcmp(word, "PRECIP") || fastcmp(word, "PRECIPITATION"))
				{
					if (i == 0 && word2[0] != '0') // If word2 isn't a number
						i = get_precip(word2); // find a weather type by name
					if (i < MAXPRECIP && i > 0)
						readweather(f, i);
					else
					{
						deh_warning("Weather number %d out of range (1 - %d)", i, MAXPRECIP-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "RINGRACERS"))
				{
					if (isdigit(word2[0]))
					{
						i = atoi(word2);
						if (i != PATCHVERSION)
						{
							deh_warning(
									"Patch is for Ring Racers version %d, "
									"only version %d is supported",
									i,
									PATCHVERSION
							);
						}
					}
					else
					{
						deh_warning(
								"Ring Racers version definition has incorrect format, "
								"use \"RINGRACERS %d\"",
								PATCHVERSION
						);
					}
				}
				else if (fastcmp(word, "SRB2KART"))
				{
					deh_warning("Patch is only compatible with SRB2Kart.");
				}
				else if (fastcmp(word, "SRB2"))
				{
					deh_warning("Patch is only compatible with base SRB2.");
				}
				else
					deh_warning("Unknown word: %s", word);
			}
			else
				deh_warning("missing argument for '%s'", word);
		}
		else
			deh_warning("No word in this line: %s", s);
	} // end while

	if (gamedataadded)
	{
		basenummapheaders = nummapheaders;
		basenumkartcupheaders = numkartcupheaders;
		G_LoadGameData();
	}

	if (gamestate == GS_MENU || gamestate == GS_TITLESCREEN)
	{
		if (introchanged)
		{
			menuactive = false;
			I_UpdateMouseGrab();
			COM_BufAddText("playintro");
		}
		else if (titlechanged)
		{
			menuactive = false;
			I_UpdateMouseGrab();
			COM_BufAddText("exitgame"); // Command_ExitGame_f() but delayed
		}
	}

	dbg_line = -1;
	if (deh_num_warning)
	{
		CONS_Printf(M_GetText("%d warning%s in the SOC lump\n"), deh_num_warning, deh_num_warning == 1 ? "" : "s");
		if (devparm) {
			I_Error("%s%s",va(M_GetText("%d warning%s in the SOC lump\n"), deh_num_warning, deh_num_warning == 1 ? "" : "s"), M_GetText("See log.txt for details.\n"));
			//while (!I_GetKey())
				//I_OsPolling();
		}
	}

	deh_loaded = true;
	Z_Free(s);
}

// read dehacked lump in a wad (there is special trick for for deh
// file that are converted to wad in w_wad.c)
void DEH_LoadDehackedLumpPwad(UINT16 wad, UINT16 lump, boolean mainfile)
{
	MYFILE f;
	f.wad = wad;
	f.size = W_LumpLengthPwad(wad, lump);
	f.data = Z_Malloc(f.size + 1, PU_STATIC, NULL);
	W_ReadLumpPwad(wad, lump, f.data);
	f.curpos = f.data;
	f.data[f.size] = 0;
	DEH_LoadDehackedFile(&f, mainfile);
	Z_Free(f.data);
}

void DEH_LoadDehackedLump(lumpnum_t lumpnum)
{
	DEH_LoadDehackedLumpPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum), false);
}
