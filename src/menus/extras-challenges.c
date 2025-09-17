// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/extras-challenges.c
/// \brief Challenges board

#include "../i_time.h"
#include "../k_menu.h"
#include "../m_cond.h" // Condition Sets
#include "../m_random.h" // And just some randomness for the exits.
#include "../music.h"
#include "../z_zone.h"
#include "../r_skins.h"
#include "../s_sound.h"

#ifdef DEVELOP
extern consvar_t cv_debugchallenges;
#endif

static void M_StatisticsTicker(void)
{
	// the funny
	gamedata->totaltimestaringatstatistics++;
}

menuitem_t MISC_ChallengesStatsDummyMenu[] =
{
	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t MISC_ChallengesDef = {
	sizeof (MISC_ChallengesStatsDummyMenu)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_ChallengesStatsDummyMenu,
	BASEVIDWIDTH/2, 11,
	0, 0,
	0,
	"UNLOCK",
	98, 0,
	M_DrawChallenges,
	NULL,
	M_ChallengesTick,
	NULL,
	NULL,
	M_ChallengesInputs,
};

// This must be defined here so it can take sizeof
// MISC_ChallengesStatsDummyMenu :V
menu_t MISC_StatisticsDef = {
	sizeof (MISC_ChallengesStatsDummyMenu)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_ChallengesStatsDummyMenu,
	280, 185,
	0, 0,
	0,
	"EXSTAT",
	98, 0,
	M_DrawStatistics,
	M_DrawExtrasBack,
	M_StatisticsTicker,
	NULL,
	NULL,
	M_StatisticsInputs,
};

struct challengesmenu_s challengesmenu;

static void M_UpdateChallengeGridVisuals(void)
{
	UINT16 i;

	challengesmenu.cache_secondrowlocked = M_CupSecondRowLocked();

	challengesmenu.unlockcount[CMC_UNLOCKED] = 0;
	challengesmenu.unlockcount[CMC_TOTAL] = 0;
	challengesmenu.unlockcount[CMC_KEYED] = 0;
	challengesmenu.unlockcount[CMC_MAJORSKIPPED] = 0;

//#define MAJORDISTINCTION -- The "basic" medal is basically never seen because Major challenges are usually completed last before 101%. Correct that with this

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (!unlockables[i].conditionset)
		{
			continue;
		}

		challengesmenu.unlockcount[CMC_TOTAL]++;

		if (!gamedata->unlocked[i])
		{
			continue;
		}

		challengesmenu.unlockcount[CMC_UNLOCKED]++;

		if (M_Achieved(unlockables[i].conditionset - 1) == true)
		{
			continue;
		}

		challengesmenu.unlockcount[CMC_KEYED]++;

#ifdef MAJORDISTINCTION
		if (unlockables[i].majorunlock == false)
		{
			continue;
		}

		challengesmenu.unlockcount[CMC_MAJORSKIPPED]++;
#endif
	}

	challengesmenu.unlockcount[CMC_PERCENT] =
		(100 * challengesmenu.unlockcount[CMC_UNLOCKED])
			/challengesmenu.unlockcount[CMC_TOTAL];

	#define medalheight (19)

	challengesmenu.unlockcount[CMC_MEDALID] = 0;

	challengesmenu.unlockcount[CMC_MEDALFILLED] =
		(medalheight * (
			challengesmenu.unlockcount[CMC_UNLOCKED]
#ifdef MAJORDISTINCTION
			- challengesmenu.unlockcount[CMC_MAJORSKIPPED]
#endif
		)) / challengesmenu.unlockcount[CMC_TOTAL];

	if (challengesmenu.unlockcount[CMC_PERCENT] == 100)
	{
		if (challengesmenu.unlockcount[CMC_KEYED] == 0)
		{
			challengesmenu.unlockcount[CMC_MEDALID] = 2;
			challengesmenu.unlockcount[CMC_PERCENT]++; // 101%
		}
		else
#ifdef MAJORDISTINCTION
			if (challengesmenu.unlockcount[CMC_MAJORSKIPPED] == 0)
#endif
		{
			challengesmenu.unlockcount[CMC_MEDALID] = 1;
		}
	}
	else
	{
		if (challengesmenu.unlockcount[CMC_MEDALFILLED] == 0 && challengesmenu.unlockcount[CMC_UNLOCKED] != 0)
		{
			// Cheat to give you a sliver of pixel.
			challengesmenu.unlockcount[CMC_MEDALFILLED] = 1;
		}
	}

	challengesmenu.unlockcount[CMC_MEDALBLANK] =
		medalheight - challengesmenu.unlockcount[CMC_MEDALFILLED];
}

static void M_ChallengesAutoFocus(UINT16 unlockid, boolean fresh)
{
	UINT16 i;
	INT16 work;
	boolean posisvalid = false;

	if (unlockid >= MAXUNLOCKABLES && gamedata->pendingkeyrounds > 0
		&& (gamedata->chaokeys < GDMAX_CHAOKEYS))
		challengesmenu.chaokeyadd = true;

	if (fresh && unlockid >= MAXUNLOCKABLES)
	{
		if (challengesmenu.currentunlock < MAXUNLOCKABLES)
		{
			// Use the last selected time.
			unlockid = challengesmenu.currentunlock;
			posisvalid = true;
		}
		else
		{
			UINT16 selection[MAXUNLOCKABLES];
			UINT16 numunlocks = 0;

			boolean triedrandomlevel = 0;

tryfreshrandom:

			// Get a random available unlockable.
			for (i = 0; i < MAXUNLOCKABLES; i++)
			{
				if (!unlockables[i].conditionset)
				{
					continue;
				}

				// Otherwise we don't care, just pick any non-blank tile
				if (triedrandomlevel < 2)
				{
					// We try for any unlock second
					if (!gamedata->unlocked[i])
					{
						continue;
					}

					if (triedrandomlevel == 0)
					{
						// We try for a pending unlock first
						if (!gamedata->unlockpending[i])
						{
							continue;
						}
					}
				}

				selection[numunlocks++] = i;
			}

			if (numunlocks == 0)
			{
				if (triedrandomlevel == 2)
					return;

				triedrandomlevel++;
				goto tryfreshrandom;
			}

			unlockid = selection[M_RandomKey(numunlocks)];
		}
	}

	challengesmenu.unlockanim = (challengesmenu.pending && !challengesmenu.chaokeyadd ? 0 : MAXUNLOCKTIME);

	if (unlockid >= MAXUNLOCKABLES)
		return;

	challengesmenu.currentunlock = unlockid;
	if (challengesmenu.unlockcondition)
		Z_Free(challengesmenu.unlockcondition);
	challengesmenu.unlockcondition = M_BuildConditionSetString(challengesmenu.currentunlock);

	if (gamedata->challengegrid == NULL || challengesmenu.extradata == NULL || posisvalid)
		return;

	for (i = 0; i < (CHALLENGEGRIDHEIGHT * gamedata->challengegridwidth); i++)
	{
		if (gamedata->challengegrid[i] != unlockid)
		{
			// Not what we're looking for.
			continue;
		}

		if (challengesmenu.extradata[i].flags & CHE_CONNECTEDLEFT)
		{
			// no need to check for CHE_CONNECTEDUP in linear iteration
			continue;
		}

		// Helper calculation for non-fresh scrolling.
		work = (challengesmenu.col + challengesmenu.focusx);

		challengesmenu.col = challengesmenu.hilix = i/CHALLENGEGRIDHEIGHT;
		challengesmenu.row = challengesmenu.hiliy = i%CHALLENGEGRIDHEIGHT;

		// Begin animation
		if (challengesmenu.pending)
		{
			challengesmenu.extradata[i].flip = (TILEFLIP_MAX/2);
		}

		if (fresh)
		{
			// We're just entering the menu. Immediately jump to the desired position...
			challengesmenu.focusx = 0;
			// ...and since the menu is even-width, randomly select whether it's left or right of center.
			if (!unlockables[unlockid].majorunlock
				&& M_RandomChance(FRACUNIT/2))
					challengesmenu.focusx--;
		}
		else
		{
			// We're jumping between multiple unlocks in sequence. Get the difference (looped from -range/2 < work <= range/2).
			work -= challengesmenu.col;
			if (work <= -gamedata->challengegridwidth/2)
				work += gamedata->challengegridwidth;
			else if (work >= gamedata->challengegridwidth/2)
				work -= gamedata->challengegridwidth;

			if (work > 0)
			{
				// We only need to scroll as far as the rightward edge.
				if (unlockables[unlockid].majorunlock)
				{
					work--;
					challengesmenu.col++;
					if (challengesmenu.col >= gamedata->challengegridwidth)
						challengesmenu.col = 0;
				}

				// Offset right, scroll left?
				if (work > LEFTUNLOCKSCROLL)
				{
					work -= LEFTUNLOCKSCROLL;
					challengesmenu.focusx = LEFTUNLOCKSCROLL;
				}
				else
				{
					challengesmenu.focusx = work;
					work = 0;
				}
			}
			else if (work < 0)
			{
				// Offset left, scroll right?
				if (work < -RIGHTUNLOCKSCROLL)
				{
					challengesmenu.focusx = -RIGHTUNLOCKSCROLL;
					work += RIGHTUNLOCKSCROLL;
				}
				else
				{
					challengesmenu.focusx = work;
					work = 0;
				}
			}
			else
			{
				// We're right where we want to be.
				challengesmenu.focusx = 0;
			}

			// And put the pixel-based scrolling in play, too.
			challengesmenu.offset = -work*16;
		}

		break;
	}
}

static void M_CacheChallengeTiles(void)
{
	char name[9] = "UN_RR0xy";

	int i;

	for (i = 0; i < 10; ++i)
	{
		name[6] = '0' + i;

		name[7] = 'A';
		challengesmenu.tile_category[i][0] = W_CachePatchName(name, PU_CACHE);

		name[7] = 'B';
		challengesmenu.tile_category[i][1] = W_CachePatchName(name, PU_CACHE);
	}
}

menu_t *M_InterruptMenuWithChallenges(menu_t *desiredmenu)
{
	UINT16 newunlock;

	if (Playing() == true
	|| M_GameTrulyStarted() == false)
		return desiredmenu;

	M_UpdateUnlockablesAndExtraEmblems(false, true);

	newunlock = M_GetNextAchievedUnlock(true);

	if ((challengesmenu.pending = (newunlock != MAXUNLOCKABLES)))
	{
		Music_StopAll();
		if (desiredmenu && desiredmenu != &MISC_ChallengesDef)
		{
			MISC_ChallengesDef.prevMenu = desiredmenu;
		}
	}

	if (challengesmenu.pending || desiredmenu == &MISC_ChallengesDef)
	{
		static boolean firstopen = true;

		challengesmenu.ticker = 0;
		challengesmenu.requestflip = false;
		challengesmenu.requestnew = false;
		challengesmenu.chaokeyadd = false;
		challengesmenu.keywasadded = false;
		challengesmenu.tutorialfound = NEXTMAP_INVALID;
		challengesmenu.chaokeyhold = 0;
		challengesmenu.unlockcondition = NULL;
		challengesmenu.hornposting = 0;

		if (firstopen)
		{
			challengesmenu.currentunlock = MAXUNLOCKABLES;
			challengesmenu.nowplayingtile = UINT16_MAX;
			firstopen = false;
		}

		M_PopulateChallengeGrid();
		if (gamedata->challengegrid)
		{
			challengesmenu.extradata = Z_Calloc(
				(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT * sizeof(challengegridextradata_t)),
				PU_STATIC, NULL);
			M_UpdateChallengeGridExtraData(challengesmenu.extradata);
		}

		memset(setup_explosions, 0, sizeof(setup_explosions));
		memset(&challengesmenu.unlockcount, 0, sizeof(challengesmenu.unlockcount));

		M_UpdateChallengeGridVisuals();

		if (challengesmenu.pending)
			M_ChallengesAutoFocus(newunlock, true);
		else
		{
			if (newunlock >= MAXUNLOCKABLES && gamedata->pendingkeyrounds > 0
				&& (gamedata->chaokeys < GDMAX_CHAOKEYS))
				challengesmenu.chaokeyadd = true;

			M_ChallengesAutoFocus(UINT16_MAX, true);
		}

		M_CacheChallengeTiles();

		return &MISC_ChallengesDef;
	}

	return desiredmenu;
}

void M_Challenges(INT32 choice)
{
	(void)choice;

	M_InterruptMenuWithChallenges(&MISC_ChallengesDef);
	MISC_ChallengesDef.prevMenu = currentMenu;

	M_SetupNextMenu(&MISC_ChallengesDef, false);
}

static void M_CloseChallenges(void)
{
	Music_Stop("challenge_altmusic");
	challengesmenu.nowplayingtile = UINT16_MAX;

	Z_Free(challengesmenu.extradata);
	challengesmenu.extradata = NULL;

	Z_Free(challengesmenu.unlockcondition);
	challengesmenu.unlockcondition = NULL;
}

boolean M_CanKeyHiliTile(void)
{
	// No tile data?
	if (challengesmenu.extradata == NULL)
		return false;

	// No selected tile?
	if (challengesmenu.currentunlock >= MAXUNLOCKABLES)
		return false;

	// Already unlocked?
	if (gamedata->unlocked[challengesmenu.currentunlock] == true)
		return false;

#ifdef DEVELOP
	// Ignore game design?
	if (cv_debugchallenges.value)
		return true;
#endif

	// No keys to do it with?
	if (gamedata->chaokeys == 0)
		return false;

	UINT16 i = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;

	// Not a hinted tile.
	if (!(challengesmenu.extradata[i].flags & CHE_HINT))
		return false;

	// Marked as major?
	if (unlockables[challengesmenu.currentunlock].majorunlock == true)
	{
		if (!(challengesmenu.extradata[i].flags & CHE_ALLCLEAR))
			return false;

		if (gamedata->chaokeys < 10)
			return false;
	}

	// Fury Bike
	if (unlockables[challengesmenu.currentunlock].conditionset == CH_FURYBIKE)
		return false;

	// All good!
	return true;
}

enum {
	CCTUTORIAL_KEYGEN = 0,
	CCTUTORIAL_MAJORSKIP,
	CCTUTORIAL_TUTORIAL, // I heard you like tutorials, so I...
} cctutorial_e;

static void M_ChallengesTutorial(UINT8 option)
{
	switch (option)
	{
		case CCTUTORIAL_KEYGEN:
		{
			M_StartMessage("Challenges & Chao Keys",
				va(M_GetText(
				"You just generated a Chao Key!\n"
				"These can clear tough Challenges.\n"
				"\n"
				"Use them wisely - it'll take\n"
				"%u rounds to pick up another!\n"
				), GDCONVERT_ROUNDSTOKEY
				), NULL, MM_NOTHING, NULL, NULL);
			gamedata->chaokeytutorial = true;
			break;
		}
		case CCTUTORIAL_MAJORSKIP:
		{
			M_StartMessage("Big Challenges & Chao Keys",
				M_GetText(
				"Watch out! You need 10 Chao Keys\n"
				"to break open Big Challenge tiles.\n"
				"\n"
				"You'll also need to unlock\n"
				"any surrounding small tiles first.\n"
				), NULL, MM_NOTHING, NULL, NULL);
			gamedata->majorkeyskipattempted = true;
			break;
		}
		case CCTUTORIAL_TUTORIAL:
		{
			M_StartMessage("New Tutorial Data",
				va(M_GetText(
				"A new piece of Eggman and Tails'\n"
				"short adventure has been decrypted.\n"
				"\n"
				"You can find \"""\x87""%s""\x80""\" on\n"
				"the Tutorials menu under Extras!\n"
				"\n"
				"These may sometimes be needed for progression.\n"
				), (challengesmenu.tutorialfound < nummapheaders
				&& mapheaderinfo[challengesmenu.tutorialfound]
					? mapheaderinfo[challengesmenu.tutorialfound]->menuttl
					: "ERROR!?"
				)), NULL, MM_NOTHING, NULL, "Got it!");
			challengesmenu.tutorialfound = NEXTMAP_INVALID;
			break;
		}
		default:
		{
			M_StartMessage("M_ChallengesTutorial ERROR",
				"Invalid argument!?\n",
				NULL, MM_NOTHING, NULL, NULL);
			break;
		}
	}
}

void M_ChallengesTick(void)
{
	const UINT8 pid = 0;
	UINT16 i;
	UINT16 newunlock = MAXUNLOCKABLES;

	// Ticking
	challengesmenu.ticker++;
	challengesmenu.offset /= 2;
	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		if (setup_explosions[i].tics > 0)
			setup_explosions[i].tics--;
	}
	for (i = CMC_ANIM; i < CMC_MAX; i++)
	{
		if (challengesmenu.unlockcount[i] > 0)
			challengesmenu.unlockcount[i]--;
	}
	M_CupSelectTick();

	// Update tile flip state.
	if (challengesmenu.extradata != NULL)
	{
		UINT16 id = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;
		boolean seeeveryone = challengesmenu.requestflip;
		boolean allthewaythrough = allthewaythrough = (!seeeveryone && !challengesmenu.pending);

		UINT8 maxflip;

		if (id == challengesmenu.nowplayingtile)
		{
			// Don't permit the active song to stop spinning
			id = UINT16_MAX;
		}

		for (i = 0; i < (CHALLENGEGRIDHEIGHT * gamedata->challengegridwidth); i++)
		{
			maxflip = ((allthewaythrough && i != id) ? TILEFLIP_MAX : (TILEFLIP_MAX/2));
			if ((seeeveryone || (i == id) || (i == challengesmenu.nowplayingtile) || (challengesmenu.extradata[i].flip > 0))
				&& (challengesmenu.extradata[i].flip != maxflip))
			{
				challengesmenu.extradata[i].flip++;
				if (challengesmenu.extradata[i].flip >= TILEFLIP_MAX)
				{
					challengesmenu.extradata[i].flip = 0;
				}
			}
		}
	}

	if (challengesmenu.chaokeyhold)
	{
		if (M_MenuExtraHeld(pid) && M_CanKeyHiliTile())
		{
			// Not pressed just this frame?
			if (!M_MenuExtraPressed(pid))
			{
				challengesmenu.chaokeyhold++;

				UINT32 chaohold_duration =
					CHAOHOLD_PADDING
					+ ((unlockables[challengesmenu.currentunlock].majorunlock == true)
						? CHAOHOLD_MAJOR
						: CHAOHOLD_STANDARD
					);

#ifdef DEVELOP
				if (cv_debugchallenges.value)
					chaohold_duration = 0;
#endif

				if (challengesmenu.chaokeyhold > chaohold_duration)
				{
#ifdef DEVELOP
					if (!cv_debugchallenges.value)
#endif
						gamedata->chaokeys -= (unlockables[challengesmenu.currentunlock].majorunlock == true)
							? 10 : 1;

					challengesmenu.chaokeyhold = 0;
					challengesmenu.unlockcount[CMC_CHAOANIM]++;
					challengesmenu.keywasadded = false; // disappearify the Hand

					S_StartSound(NULL, sfx_chchng);

					challengesmenu.pending = true;
					//M_ChallengesAutoFocus(challengesmenu.currentunlock, false);
					challengesmenu.unlockanim = UNLOCKTIME-1;
				}
			}
		}
		else
		{
			challengesmenu.chaokeyhold = 0;
			challengesmenu.unlockcount[CMC_CHAONOPE] = 6;
			S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2
		}
	}

	if ((challengesmenu.pending || challengesmenu.chaokeyhold) && challengesmenu.fade < 5)
	{
		// Fade increase.
		challengesmenu.fade++;
	}
	else if (challengesmenu.chaokeyadd == true)
	{
		if (challengesmenu.ticker <= 5)
			; // recreate the slight delay the unlock fades provide
		else if (gamedata->pendingkeyrounds == 0)
		{
			gamedata->keyspending = 0;
			gamedata->pendingkeyroundoffset %= GDCONVERT_ROUNDSTOKEY;

			challengesmenu.chaokeyadd = false;
			challengesmenu.requestnew = true;
		}
		else if (gamedata->chaokeys >= GDMAX_CHAOKEYS)
		{
			// The above condition will run on the next tic because of this set
			gamedata->pendingkeyrounds = 0;
			gamedata->pendingkeyroundoffset = 0;
		}
		else
		{
			UINT32 keyexchange = gamedata->keyspending;

			if (keyexchange > gamedata->pendingkeyrounds)
			{
				keyexchange = 1;
			}
			else if (keyexchange >= GDCONVERT_ROUNDSTOKEY/2)
			{
				keyexchange = GDCONVERT_ROUNDSTOKEY/2;
			}

			keyexchange |= 1; // guarantee an odd delta for the sake of the sound

			gamedata->pendingkeyrounds -= keyexchange;
			gamedata->pendingkeyroundoffset += keyexchange;

			if (!(gamedata->pendingkeyrounds & 1))
			{
				S_StartSound(NULL, sfx_ptally);
			}

			if (gamedata->pendingkeyroundoffset >= GDCONVERT_ROUNDSTOKEY)
			{
				gamedata->pendingkeyroundoffset %= GDCONVERT_ROUNDSTOKEY;

				if (gamedata->keyspending > 0)
				{
					S_StartSound(NULL, sfx_keygen);
					gamedata->keyspending--;
					gamedata->chaokeys++;
					challengesmenu.unlockcount[CMC_CHAOANIM]++;

					if (gamedata->musicstate < GDMUSIC_KEYG)
						gamedata->musicstate = GDMUSIC_KEYG;

					challengesmenu.keywasadded = true;
				}
			}
		}
	}
	else if (challengesmenu.requestnew)
	{
		// The menu apparatus is requesting a new unlock.
		challengesmenu.requestnew = false;
		if ((newunlock = M_GetNextAchievedUnlock(false)) != MAXUNLOCKABLES)
		{
			// We got one!
			M_ChallengesAutoFocus(newunlock, false);
		}
		else if (gamedata->pendingkeyrounds > 0
			&& (gamedata->chaokeys < GDMAX_CHAOKEYS))
		{
			// Get ready to finish with pending chao key round tallying.
			challengesmenu.chaokeyadd = true;
		}
		else
		{
			// All done! Let's save the unlocks we've busted open.
			challengesmenu.pending = challengesmenu.chaokeyadd = false;
			G_SaveGameData();
		}
	}
	else if (challengesmenu.pending)
	{
		tic_t nexttime = M_MenuExtraHeld(pid) ? (UNLOCKTIME*2) : MAXUNLOCKTIME;

#ifdef DEVELOP
		if (cv_debugchallenges.value)
			nexttime = UNLOCKTIME;
#endif

		if (++challengesmenu.unlockanim >= nexttime)
		{
			challengesmenu.requestnew = true;
		}

		if (challengesmenu.currentunlock < MAXUNLOCKABLES
			&& challengesmenu.unlockanim == UNLOCKTIME)
		{
			unlockable_t *ref = &unlockables[challengesmenu.currentunlock];

			// Unlock animation... also tied directly to the actual unlock!
			gamedata->unlocked[challengesmenu.currentunlock] = true;
			M_UpdateUnlockablesAndExtraEmblems(true, true);

			if (challengesmenu.tutorialfound == NEXTMAP_INVALID
			&& ref->type == SECRET_MAP)
			{
				// Map exists...
				UINT16 mapnum = M_UnlockableMapNum(ref);
				if (mapnum < nummapheaders && mapheaderinfo[mapnum])
				{
					// is tutorial...
					INT32 guessgt = G_GuessGametypeByTOL(mapheaderinfo[mapnum]->typeoflevel);
					if (guessgt == GT_TUTORIAL)
					{
						// and isn't the playground?
						if (!tutorialplaygroundmap
						|| strcmp(tutorialplaygroundmap, mapheaderinfo[mapnum]->lumpname))
						{
							// Pop an alert up!
							challengesmenu.tutorialfound = mapnum;
						}
					}
				}
			}

			// Update shown description just in case..?
			if (challengesmenu.unlockcondition)
				Z_Free(challengesmenu.unlockcondition);
			challengesmenu.unlockcondition = M_BuildConditionSetString(challengesmenu.currentunlock);
			M_UpdateChallengeGridVisuals();

			challengesmenu.unlockcount[CMC_ANIM]++;

			if (challengesmenu.extradata)
			{
				UINT16 bombcolor;

				M_UpdateChallengeGridExtraData(challengesmenu.extradata);

				bombcolor = SKINCOLOR_NONE;

				if (ref->color != SKINCOLOR_NONE && ref->color < numskincolors)
				{
					bombcolor = ref->color;
				}
				else switch (ref->type)
				{
					case SECRET_SKIN:
					{
						INT32 skin = M_UnlockableSkinNum(ref);
						if (skin != -1)
						{
							bombcolor = skins[skin]->prefcolor;
						}
						break;
					}
					case SECRET_FOLLOWER:
					{
						INT32 fskin = M_UnlockableFollowerNum(ref);
						if (fskin != -1)
						{
							INT32 psk = R_SkinAvailableEx(cv_skin[0].string, false);
							if (psk == -1)
								psk = 0;
							bombcolor = K_GetEffectiveFollowerColor(followers[fskin].defaultcolor, &followers[fskin], cv_playercolor[0].value, skins[psk]);
						}
						break;
					}
					default:
						break;
				}

				if (bombcolor == SKINCOLOR_NONE)
				{
					bombcolor = M_GetCvPlayerColor(0);
				}

				i = (ref->majorunlock && M_RandomChance(FRACUNIT/2)) ? 1 : 0;
				M_SetupReadyExplosions(false, challengesmenu.hilix, challengesmenu.hiliy+i, bombcolor);
				if (ref->majorunlock)
				{
					M_SetupReadyExplosions(false, challengesmenu.hilix+1, challengesmenu.hiliy+(1-i), bombcolor);
				}

				S_StartSound(NULL, sfx_s3k4e);
			}
		}
	}
	else if (!challengesmenu.chaokeyhold)
	{
		if (challengesmenu.fade > 0)
		{
			// Fade decrease.
			if (--challengesmenu.fade == 0)
			{
				if (M_ConsiderSealedSwapAlert() == true)
				{
					// No keygen tutorial in this case...
					// not ideal but at least unlikely to
					// get at same time?? :V
				}
				else if (challengesmenu.tutorialfound != NEXTMAP_INVALID)
				{
					M_ChallengesTutorial(CCTUTORIAL_TUTORIAL);
					// Also no keygen, but that can come later.
				}
				else if (gamedata->chaokeytutorial == false
				&& challengesmenu.keywasadded == true)
				{
					M_ChallengesTutorial(CCTUTORIAL_KEYGEN);
				}

				// Play music the moment control returns.
				M_PlayMenuJam();
			}
		}

		if (challengesmenu.currentunlock < MAXUNLOCKABLES
		&& gamedata->unlockpending[challengesmenu.currentunlock] == true)
		{
			UINT16 id = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;
			if (challengesmenu.extradata
			&& challengesmenu.extradata[id].flip != (TILEFLIP_MAX/2))
			{
				// Only mark visited once flipped
			}
			else
			{
				gamedata->unlockpending[challengesmenu.currentunlock] = false;
			}
		}
	}
}

boolean M_ChallengesInputs(INT32 ch)
{
	const UINT8 pid = 0;
	UINT16 i;
	const boolean start = M_MenuButtonPressed(pid, MBT_START);
	const boolean move = (menucmd[pid].dpad_ud != 0 || menucmd[pid].dpad_lr != 0);
	(void) ch;

	if (challengesmenu.fade || challengesmenu.chaokeyadd || challengesmenu.chaokeyhold)
	{
		;
	}
	else if (M_MenuExtraPressed(pid))
	{
		if (gamedata->chaokeytutorial == true
			&& gamedata->majorkeyskipattempted == false
			&& challengesmenu.currentunlock < MAXUNLOCKABLES
			&& gamedata->unlocked[challengesmenu.currentunlock] == false
			&& unlockables[challengesmenu.currentunlock].majorunlock == true)
		{
			M_ChallengesTutorial(CCTUTORIAL_MAJORSKIP);
		}
		else if (M_CanKeyHiliTile() && !usedTourney)
		{
			challengesmenu.chaokeyhold = 1;
		}
		else
		{
			challengesmenu.unlockcount[CMC_CHAONOPE] = 6;
			S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2

#ifdef DEVELOP
			if (cv_debugchallenges.value && challengesmenu.currentunlock < MAXUNLOCKABLES && challengesmenu.unlockanim >= UNLOCKTIME && gamedata->unlocked[challengesmenu.currentunlock] == true)
			{
				gamedata->unlocked[challengesmenu.currentunlock] = gamedata->unlockpending[challengesmenu.currentunlock] = false;
				UINT16 set = unlockables[challengesmenu.currentunlock].conditionset;
				if (set > 0 && set <= MAXCONDITIONSETS)
				{
					gamedata->achieved[set - 1] = false;
				}

				M_UpdateChallengeGridVisuals();
			}
#endif
		}
		return true;
	}
#ifdef DEVELOP
	else if (M_MenuButtonPressed(pid, MBT_Z))
	{
		gamedata->chaokeys++;
		challengesmenu.unlockcount[CMC_CHAOANIM]++;

		if (gamedata->chaokeytutorial == false)
		{
			M_ChallengesTutorial(CCTUTORIAL_KEYGEN);
		}

		S_StartSound(NULL, sfx_dbgsal);
		return true;
	}
#endif
	else
	{
		if (M_MenuBackPressed(pid) || start)
		{
			currentMenu->prevMenu = M_SpecificMenuRestore(currentMenu->prevMenu);

			M_GoBack(0);
			M_SetMenuDelay(pid);

			M_CloseChallenges();

			return true;
		}

		if (M_MenuButtonPressed(pid, MBT_R))
		{
			challengesmenu.requestflip ^= true;

			return true;
		}

		if (challengesmenu.extradata != NULL && move)
		{
			challengesmenu.requestflip = false;

			// Determine movement around the grid
			// For right/down movement, we can pre-determine the number of steps based on extradata.
			// For left/up movement, we can't - we have to be ready to iterate twice, and break early if we don't run into a large tile.

			if (menucmd[pid].dpad_ud > 0)
			{
				i = 2;
				while (i > 0)
				{
					if (challengesmenu.row < CHALLENGEGRIDHEIGHT-1)
					{
						challengesmenu.row++;
					}
					else
					{
						challengesmenu.row = 0;
					}
					if (!(challengesmenu.extradata[
							(challengesmenu.col * CHALLENGEGRIDHEIGHT)
							+ challengesmenu.row
						].flags & CHE_CONNECTEDUP))
					{
						break;
					}
					i--;
				}
				S_StartSound(NULL, sfx_s3k5b);
				M_SetMenuDelay(pid);
			}
			else if (menucmd[pid].dpad_ud < 0)
			{
				i = (challengesmenu.extradata[
							(challengesmenu.col * CHALLENGEGRIDHEIGHT)
							+ challengesmenu.row
						].flags & CHE_CONNECTEDUP) ? 2 : 1;
				while (i > 0)
				{
					if (challengesmenu.row > 0)
					{
						challengesmenu.row--;
					}
					else
					{
						challengesmenu.row = CHALLENGEGRIDHEIGHT-1;
					}
					i--;
				}
				S_StartSound(NULL, sfx_s3k5b);
				M_SetMenuDelay(pid);
			}

			if (menucmd[pid].dpad_lr > 0)
			{
				i = 2;
				while (i > 0)
				{
					// Slide the focus counter to movement, if we can.
					if (challengesmenu.focusx > -RIGHTUNLOCKSCROLL)
					{
						challengesmenu.focusx--;
					}
					else
					{
						challengesmenu.move.dist = 1;
						challengesmenu.move.start = I_GetTime();
					}

					// Step the actual column right.
					if (challengesmenu.col < gamedata->challengegridwidth-1)
					{
						challengesmenu.col++;
					}
					else
					{
						challengesmenu.col = 0;
					}

					if (!(challengesmenu.extradata[
							(challengesmenu.col * CHALLENGEGRIDHEIGHT)
							+ challengesmenu.row
						].flags & CHE_CONNECTEDLEFT))
					{
						break;
					}

					i--;
				}
				S_StartSound(NULL, sfx_s3k5b);
				M_SetMenuDelay(pid);
			}
			else if (menucmd[pid].dpad_lr < 0)
			{
				i = (challengesmenu.extradata[
							(challengesmenu.col * CHALLENGEGRIDHEIGHT)
							+ challengesmenu.row
						].flags & CHE_CONNECTEDLEFT) ? 2 : 1;
				while (i > 0)
				{
					// Slide the focus counter to movement, if we can.
					if (challengesmenu.focusx < LEFTUNLOCKSCROLL)
					{
						challengesmenu.focusx++;
					}
					else
					{
						challengesmenu.move.dist = -1;
						challengesmenu.move.start = I_GetTime();
					}

					// Step the actual column left.
					if (challengesmenu.col > 0)
					{
						challengesmenu.col--;
					}
					else
					{
						challengesmenu.col = gamedata->challengegridwidth-1;
					}

					i--;
				}
				S_StartSound(NULL, sfx_s3k5b);
				M_SetMenuDelay(pid);
			}

			// After movement has been determined, figure out the current selection.
			i = (challengesmenu.col * CHALLENGEGRIDHEIGHT) + challengesmenu.row;
			challengesmenu.currentunlock = (gamedata->challengegrid[i]);
			if (challengesmenu.unlockcondition)
				Z_Free(challengesmenu.unlockcondition);
			challengesmenu.unlockcondition = M_BuildConditionSetString(challengesmenu.currentunlock);

			challengesmenu.hilix = challengesmenu.col;
			challengesmenu.hiliy = challengesmenu.row;

			if (challengesmenu.currentunlock < MAXUNLOCKABLES
				&& unlockables[challengesmenu.currentunlock].majorunlock)
			{
				// Adjust highlight coordinates up/to the left for large tiles.

				if (challengesmenu.hiliy > 0 && (challengesmenu.extradata[i].flags & CHE_CONNECTEDUP))
				{
					challengesmenu.hiliy--;
				}

				if ((challengesmenu.extradata[i].flags & CHE_CONNECTEDLEFT))
				{
					if (challengesmenu.hilix > 0)
					{
						challengesmenu.hilix--;
					}
					else
					{
						challengesmenu.hilix = gamedata->challengegridwidth-1;
					}
				}

				//i = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;
			}

			return true;
		}

		if (challengesmenu.currentunlock < MAXUNLOCKABLES
			&& gamedata->unlocked[challengesmenu.currentunlock])
		{
			unlockable_t *ref = &unlockables[challengesmenu.currentunlock];

			boolean forceflip = false;

			switch (unlockables[challengesmenu.currentunlock].type)
			{
				case SECRET_MAP:
				{
					// Only for 1p
					if (setup_numplayers <= 1 && M_MenuConfirmPressed(pid))
					{
						// Map exists...
						UINT16 mapnum = M_UnlockableMapNum(ref);
						if (mapnum < nummapheaders && mapheaderinfo[mapnum])
						{
							// is tutorial...
							INT32 guessgt = G_GuessGametypeByTOL(mapheaderinfo[mapnum]->typeoflevel);
							if (guessgt == GT_TUTORIAL)
							{
								M_SetMenuDelay(pid);

								multiplayer = true;

								restoreMenu = currentMenu;
								//restorelevellist = levellist; -- do NOT do this!
								// levellist is NOT valid here in the case of
								// interrupted NEXTMAP_TITLE menu restore

								// mild hack
								levellist.newgametype = guessgt;
								levellist.netgame = false;
								M_MenuToLevelPreamble(0, false);

								D_MapChange(
									mapnum+1,
									guessgt,
									false,
									true,
									1,
									false,
									false
								);

								M_CloseChallenges();
								M_ClearMenus(true);

								return false; // DO NOT
							}
						}
					}
					break;
				}
				case SECRET_ALTTITLE:
				{
					if (M_MenuConfirmPressed(pid))
					{
						extern consvar_t cv_alttitle;
						CV_AddValue(&cv_alttitle, 1);
						S_StartSound(NULL, sfx_s3kc3s);
						M_SetMenuDelay(pid);

						forceflip = true;
					}
					break;
				}
				case SECRET_SKIN:
				{
					if (setup_numplayers <= 1 && cv_lastprofile[0].value != PROFILE_GUEST && M_MenuConfirmPressed(pid))
					{
						INT32 skin = M_UnlockableSkinNum(ref);
						if (skin != -1)
						{
							profile_t *pr = PR_GetProfile(cv_lastprofile[0].value);

							if (pr && strcmp(pr->skinname, skins[skin]->name))
							{
								strcpy(pr->skinname, skins[skin]->name);
								CV_Set(&cv_skin[0], skins[skin]->name);

								S_StartSound(NULL, sfx_s3k63);
								S_StartSound(NULL, skins[skin]->soundsid[S_sfx[sfx_kattk1].skinsound]);
								M_SetMenuDelay(pid);

								forceflip = true;
							}
						}
					}
					break;
				}
				case SECRET_FOLLOWER:
				{
					if (!horngoner && M_MenuConfirmPressed(pid))
					{
						INT32 fskin = M_UnlockableFollowerNum(ref);
						if (fskin != -1)
						{
							if (setup_numplayers <= 1 && cv_lastprofile[0].value != PROFILE_GUEST)
							{
								profile_t *pr = PR_GetProfile(cv_lastprofile[0].value);

								if (pr && strcmp(pr->follower, followers[fskin].name))
								{
									strcpy(pr->follower, followers[fskin].name);
									CV_Set(&cv_follower[0], followers[fskin].name);

									challengesmenu.hornposting = 0;

									S_StartSound(NULL, sfx_s3k63);
									forceflip = true;
								}
							}

							if (!forceflip)
								challengesmenu.hornposting++;

							if (challengesmenu.hornposting > EASEOFFHORN)
							{
								challengesmenu.hornposting = 0;
								horngoner = true;
								S_StartSound(NULL, sfx_s3k72);
							}
							else
							{
								S_StartSound(NULL, followers[fskin].hornsound);
							}

							M_SetMenuDelay(pid);

							forceflip = true;
						}
					}
					break;
				}
				case SECRET_COLOR:
				{
					if (setup_numplayers <= 1 && cv_lastprofile[0].value != PROFILE_GUEST && M_MenuConfirmPressed(pid))
					{
						INT32 colorid = M_UnlockableColorNum(ref);
						if (colorid != SKINCOLOR_NONE)
						{
							profile_t *pr = PR_GetProfile(cv_lastprofile[0].value);

							if (pr && pr->color != colorid)
							{
								pr->color = colorid;
								CV_SetValue(&cv_playercolor[0], colorid);
								if (setup_numplayers)
								{
									G_SetPlayerGamepadIndicatorToPlayerColor(0);
								}

								S_StartSound(NULL, sfx_s3k63);
								M_SetMenuDelay(pid);

								forceflip = true;
							}
						}
					}
					break;
				}
				case SECRET_ALTMUSIC:
				{
					UINT8 trymus = 0, musicid = MAXMUSNAMES;

					if (M_MenuConfirmPressed(pid))
					{
						trymus = 1;
					}
					else if (M_MenuButtonPressed(pid, MBT_L))
					{
						trymus = 2;
					}

					if (trymus)
					{
						const char *trymusname = NULL;

						UINT16 map = M_UnlockableMapNum(ref);
						if (map >= nummapheaders
							|| !mapheaderinfo[map])
						{
							;
						}
						else for (musicid = 1; musicid < MAXMUSNAMES; musicid++)
						{
							if (mapheaderinfo[map]->cache_muslock[musicid - 1] == challengesmenu.currentunlock)
								break;
						}

						if (trymus == 1)
						{
							if (musicid < mapheaderinfo[map]->musname_size)
							{
								trymusname = mapheaderinfo[map]->musname[musicid];
							}
						}
						else
						{
							if (musicid < mapheaderinfo[map]->encoremusname_size)
							{
								trymusname = mapheaderinfo[map]->encoremusname[musicid];
							}
						}

						if (trymusname)
						{
							const char *tune = "challenge_altmusic";
							if (!Music_Playing(tune)
								|| strcmp(Music_Song(tune), trymusname))
							{
								Music_Remap(tune, trymusname);
								Music_Play(tune);
								challengesmenu.nowplayingtile = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;
							}
							else
							{
								Music_Stop(tune);
								challengesmenu.nowplayingtile = UINT16_MAX;
							}

							M_SetMenuDelay(pid);
						}
					}

					break;
				}
				default:
					break;
			}

			if (forceflip)
			{
				UINT16 id = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;
				// This construction helps pressing too early
				if (challengesmenu.extradata[id].flip <= TILEFLIP_MAX/2)
				{
					challengesmenu.extradata[id].flip = 1 + (TILEFLIP_MAX/2);
				}
			}

			return true;
		}
	}

	return true;
}
