/// \file  menus/extras-challenges.c
/// \brief Challenges.

#include "../k_menu.h"
#include "../m_cond.h" // Condition Sets
#include "../m_random.h" // And just some randomness for the exits.
#include "../z_zone.h"
#include "../r_skins.h"
#include "../s_sound.h"

menuitem_t MISC_ChallengesStatsDummyMenu[] =
{
	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t MISC_ChallengesDef = {
	sizeof (MISC_ChallengesStatsDummyMenu)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_ChallengesStatsDummyMenu,
	BASEVIDWIDTH/2, 30,
	0, 0,
	0,
	"EXTRAS",
	98, 0,
	M_DrawChallenges,
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
	"EXTRAS",
	98, 0,
	M_DrawStatistics,
	NULL,
	NULL,
	NULL,
	M_StatisticsInputs,
};

struct challengesmenu_s challengesmenu;

static void M_ChallengesAutoFocus(UINT16 unlockid, boolean fresh)
{
	UINT8 i;
	SINT8 work;

	if (unlockid >= MAXUNLOCKABLES && gamedata->pendingkeyrounds > 0
		&& (gamedata->chaokeys < GDMAX_CHAOKEYS))
		challengesmenu.chaokeyadd = true;

	if (fresh && unlockid >= MAXUNLOCKABLES)
	{
		UINT8 selection[MAXUNLOCKABLES];
		UINT8 numunlocks = 0;

		// Get a random available unlockable.
		for (i = 0; i < MAXUNLOCKABLES; i++)
		{
			if (!unlockables[i].conditionset)
			{
				continue;
			}

			if (!gamedata->unlocked[i])
			{
				continue;
			}

			selection[numunlocks++] = i;
		}

		if (!numunlocks)
		{
			// ...OK, get a random unlockable.
			for (i = 0; i < MAXUNLOCKABLES; i++)
			{
				if (!unlockables[i].conditionset)
				{
					continue;
				}

				selection[numunlocks++] = i;
			}
		}

		unlockid = selection[M_RandomKey(numunlocks)];
	}

	if (unlockid >= MAXUNLOCKABLES)
		return;

	challengesmenu.currentunlock = unlockid;
	challengesmenu.unlockcondition = M_BuildConditionSetString(challengesmenu.currentunlock);
	challengesmenu.unlockanim = (challengesmenu.pending && !challengesmenu.chaokeyadd ? 0 : MAXUNLOCKTIME);

	if (gamedata->challengegrid == NULL || challengesmenu.extradata == NULL)
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

menu_t *M_InterruptMenuWithChallenges(menu_t *desiredmenu)
{
	UINT16 i, newunlock;

	if (Playing())
		return desiredmenu;

	M_UpdateUnlockablesAndExtraEmblems(false, true);

	newunlock = M_GetNextAchievedUnlock();

	if ((challengesmenu.pending = (newunlock != MAXUNLOCKABLES)))
	{
		S_StopMusic();
		MISC_ChallengesDef.prevMenu = desiredmenu;
	}

	if (challengesmenu.pending || desiredmenu == NULL)
	{
		challengesmenu.ticker = 0;
		challengesmenu.requestflip = false;
		challengesmenu.requestnew = false;
		challengesmenu.chaokeyadd = false;
		challengesmenu.currentunlock = MAXUNLOCKABLES;
		challengesmenu.unlockcondition = NULL;

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
		for (i = 0; i < MAXUNLOCKABLES; i++)
		{
			if (!unlockables[i].conditionset)
			{
				continue;
			}

			challengesmenu.unlockcount[CC_TOTAL]++;

			if (!gamedata->unlocked[i])
			{
				continue;
			}

			challengesmenu.unlockcount[CC_UNLOCKED]++;
		}

		if (challengesmenu.pending)
			M_ChallengesAutoFocus(newunlock, true);
		else if (newunlock >= MAXUNLOCKABLES && gamedata->pendingkeyrounds > 0
			&& (gamedata->chaokeys < GDMAX_CHAOKEYS))
			challengesmenu.chaokeyadd = true;

		return &MISC_ChallengesDef;
	}

	return desiredmenu;
}

void M_Challenges(INT32 choice)
{
	(void)choice;

	M_InterruptMenuWithChallenges(NULL);
	MISC_ChallengesDef.prevMenu = currentMenu;

	if (gamedata->challengegrid != NULL && !challengesmenu.pending)
	{
		M_ChallengesAutoFocus(UINT16_MAX, true);
	}

	M_SetupNextMenu(&MISC_ChallengesDef, false);
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
	for (i = CC_ANIM; i < CC_MAX; i++)
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
		boolean allthewaythrough;
		UINT8 maxflip;
		for (i = 0; i < (CHALLENGEGRIDHEIGHT * gamedata->challengegridwidth); i++)
		{
			allthewaythrough = (!seeeveryone && !challengesmenu.pending && i != id);
			maxflip = ((seeeveryone || !allthewaythrough) ? (TILEFLIP_MAX/2) : TILEFLIP_MAX);
			if ((seeeveryone || (i == id) || (challengesmenu.extradata[i].flip > 0))
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

	if (challengesmenu.pending && challengesmenu.fade < 5)
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
					S_StartSound(NULL, sfx_achiev);
					gamedata->keyspending--;
					gamedata->chaokeys++;
					challengesmenu.unlockcount[CC_CHAOANIM]++;
				}
			}
		}
	}
	else if (challengesmenu.requestnew)
	{
		// The menu apparatus is requesting a new unlock.
		challengesmenu.requestnew = false;
		if ((newunlock = M_GetNextAchievedUnlock()) != MAXUNLOCKABLES)
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

		if (++challengesmenu.unlockanim >= nexttime)
		{
			challengesmenu.requestnew = true;
		}

		if (challengesmenu.currentunlock < MAXUNLOCKABLES
			&& challengesmenu.unlockanim == UNLOCKTIME)
		{
			// Unlock animation... also tied directly to the actual unlock!
			gamedata->unlocked[challengesmenu.currentunlock] = true;
			M_UpdateUnlockablesAndExtraEmblems(true, true);

			// Update shown description just in case..?
			challengesmenu.unlockcondition = M_BuildConditionSetString(challengesmenu.currentunlock);

			challengesmenu.unlockcount[CC_TALLY]++;
			challengesmenu.unlockcount[CC_ANIM]++;

			if (challengesmenu.extradata)
			{
				unlockable_t *ref;
				UINT16 bombcolor;

				M_UpdateChallengeGridExtraData(challengesmenu.extradata);

				ref = &unlockables[challengesmenu.currentunlock];
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
							bombcolor = skins[skin].prefcolor;
						}
						break;
					}
					case SECRET_FOLLOWER:
					{
						INT32 fskin = M_UnlockableFollowerNum(ref);
						if (fskin != -1)
						{
							INT32 psk = R_SkinAvailable(cv_skin[0].string);
							if (psk == -1)
								psk = 0;
							bombcolor = K_GetEffectiveFollowerColor(followers[fskin].defaultcolor, &followers[fskin], cv_playercolor[0].value, &skins[psk]);
						}
						break;
					}
					default:
						break;
				}

				if (bombcolor == SKINCOLOR_NONE)
				{
					bombcolor = cv_playercolor[0].value;
					if (bombcolor == SKINCOLOR_NONE)
					{
						INT32 psk = R_SkinAvailable(cv_skin[0].string);
						if (psk == -1)
							psk = 0;
						bombcolor = skins[psk].prefcolor;
					}
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
	else
	{

		// Tick down the tally. (currently not visible)
		/*if ((challengesmenu.ticker & 1)
			&& challengesmenu.unlockcount[CC_TALLY] > 0)
		{
			challengesmenu.unlockcount[CC_TALLY]--;
			challengesmenu.unlockcount[CC_UNLOCKED]++;
		}*/

		if (challengesmenu.fade > 0)
		{
			// Fade decrease.
			if (--challengesmenu.fade == 0)
			{
				// Play music the moment control returns.
				M_PlayMenuJam();
			}
		}
	}
}

boolean M_ChallengesInputs(INT32 ch)
{
	const UINT8 pid = 0;
	UINT8 i;
	const boolean start = M_MenuButtonPressed(pid, MBT_START);
	const boolean move = (menucmd[pid].dpad_ud != 0 || menucmd[pid].dpad_lr != 0);
	(void) ch;

	if (challengesmenu.fade || challengesmenu.chaokeyadd)
	{
		;
	}
	else if (M_MenuExtraPressed(pid)
		&& challengesmenu.extradata)
	{
		i = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;

		if (challengesmenu.currentunlock < MAXUNLOCKABLES
			&& !gamedata->unlocked[challengesmenu.currentunlock]
			&& !unlockables[challengesmenu.currentunlock].majorunlock
			&& ((challengesmenu.extradata[i].flags & CHE_HINT)
				|| (challengesmenu.unlockcount[CC_UNLOCKED] + challengesmenu.unlockcount[CC_TALLY] == 0))
			&& gamedata->chaokeys > 0)
		{
			gamedata->chaokeys--;
			challengesmenu.unlockcount[CC_CHAOANIM]++;

			S_StartSound(NULL, sfx_chchng);

			challengesmenu.pending = true;
			M_ChallengesAutoFocus(challengesmenu.currentunlock, false);
		}
		else
		{
			challengesmenu.unlockcount[CC_CHAONOPE] = 6;
			S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2
#if 0 // debugging
			if (challengesmenu.currentunlock < MAXUNLOCKABLES)
			{
				if (gamedata->unlocked[challengesmenu.currentunlock] && challengesmenu.unlockanim >= UNLOCKTIME)
				{
					if (challengesmenu.unlockcount[CC_TALLY] > 0)
						challengesmenu.unlockcount[CC_TALLY]--;
					else
						challengesmenu.unlockcount[CC_UNLOCKED]--;
				}

				Z_Free(gamedata->challengegrid);
				gamedata->challengegrid = NULL;
				gamedata->challengegridwidth = 0;
				M_PopulateChallengeGrid();
				M_UpdateChallengeGridExtraData(challengesmenu.extradata);
				challengesmenu.pending = true;
				M_ChallengesAutoFocus(challengesmenu.currentunlock, true);
			}
#endif
		}
		return true;
	}
	else
	{
		if (M_MenuBackPressed(pid) || start)
		{
			currentMenu->prevMenu = M_SpecificMenuRestore(currentMenu->prevMenu);

			M_GoBack(0);
			M_SetMenuDelay(pid);

			Z_Free(challengesmenu.extradata);
			challengesmenu.extradata = NULL;

			challengesmenu.unlockcondition = NULL;

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

		if (M_MenuConfirmPressed(pid)
			&& challengesmenu.currentunlock < MAXUNLOCKABLES
			&& gamedata->unlocked[challengesmenu.currentunlock])
		{
			switch (unlockables[challengesmenu.currentunlock].type)
			{
				case SECRET_ALTTITLE:
					CV_AddValue(&cv_alttitle, 1);
					S_StartSound(NULL, sfx_s3kc3s);
					M_SetMenuDelay(pid);
					break;
				default:
					break;
			}

			return true;
		}
	}

	return true;
}
