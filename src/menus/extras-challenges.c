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
	BASEVIDWIDTH/2, 32,
	0, 0,
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
	98, 0,
	M_DrawStatistics,
	NULL,
	NULL,
	NULL,
	M_StatisticsInputs,
};

struct challengesmenu_s challengesmenu;

menu_t *M_InterruptMenuWithChallenges(menu_t *desiredmenu)
{
	UINT8 i;

	M_UpdateUnlockablesAndExtraEmblems(false);

	if ((challengesmenu.pending = challengesmenu.requestnew = (M_GetNextAchievedUnlock() < MAXUNLOCKABLES)))
	{
		MISC_ChallengesDef.prevMenu = desiredmenu;
	}

	if (challengesmenu.pending || desiredmenu == NULL)
	{
		challengesmenu.currentunlock = MAXUNLOCKABLES;
		challengesmenu.unlockcondition = NULL;

		M_PopulateChallengeGrid();
		if (gamedata->challengegrid)
			challengesmenu.extradata = M_ChallengeGridExtraData();

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

		return &MISC_ChallengesDef;
	}

	return desiredmenu;
}

static void M_ChallengesAutoFocus(UINT8 unlockid, boolean fresh)
{
	UINT8 i;
	SINT8 work;

	if (unlockid >= MAXUNLOCKABLES)
		return;

	challengesmenu.currentunlock = unlockid;
	challengesmenu.unlockcondition = M_BuildConditionSetString(challengesmenu.currentunlock);
	challengesmenu.unlockanim = 0;

	if (gamedata->challengegrid == NULL || challengesmenu.extradata == NULL)
		return;

	for (i = 0; i < (CHALLENGEGRIDHEIGHT * gamedata->challengegridwidth); i++)
	{
		if (gamedata->challengegrid[i] != unlockid)
		{
			// Not what we're looking for.
			continue;
		}

		if (challengesmenu.extradata[i] & CHE_CONNECTEDLEFT)
		{
			// no need to check for CHE_CONNECTEDUP in linear iteration
			continue;
		}

		// Helper calculation for non-fresh scrolling.
		work = (challengesmenu.col + challengesmenu.focusx);

		challengesmenu.col = challengesmenu.hilix = i/CHALLENGEGRIDHEIGHT;
		challengesmenu.row = challengesmenu.hiliy = i%CHALLENGEGRIDHEIGHT;

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

void M_Challenges(INT32 choice)
{
	UINT8 i;
	(void)choice;

	M_InterruptMenuWithChallenges(NULL);
	MISC_ChallengesDef.prevMenu = currentMenu;

	if (gamedata->challengegrid != NULL && !challengesmenu.pending)
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

		M_ChallengesAutoFocus(selection[M_RandomKey(numunlocks)], true);
	}

	M_SetupNextMenu(&MISC_ChallengesDef, false);
}

void M_ChallengesTick(void)
{
	const UINT8 pid = 0;
	UINT8 i, newunlock = MAXUNLOCKABLES;
	boolean fresh = (challengesmenu.currentunlock >= MAXUNLOCKABLES);

	// Ticking
	challengesmenu.ticker++;
	challengesmenu.offset /= 2;
	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		if (setup_explosions[i].tics > 0)
			setup_explosions[i].tics--;
	}
	if (challengesmenu.unlockcount[CC_ANIM] > 0)
		challengesmenu.unlockcount[CC_ANIM]--;
	M_CupSelectTick();

	if (challengesmenu.pending)
	{
		// Pending mode.

		if (challengesmenu.requestnew)
		{
			// The menu apparatus is requesting a new unlock.
			challengesmenu.requestnew = false;
			if ((newunlock = M_GetNextAchievedUnlock()) < MAXUNLOCKABLES)
			{
				// We got one!
				M_ChallengesAutoFocus(newunlock, fresh);
			}
			else
			{
				// All done! Let's save the unlocks we've busted open.
				challengesmenu.pending = false;
				G_SaveGameData();
			}
		}
		else if (challengesmenu.fade < 5)
		{
			// Fade increase.
			challengesmenu.fade++;
		}
		else
		{
			// Unlock sequence.
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
				M_UpdateUnlockablesAndExtraEmblems(true);

				// Update shown description just in case..?
				challengesmenu.unlockcondition = M_BuildConditionSetString(challengesmenu.currentunlock);

				challengesmenu.unlockcount[CC_TALLY]++;
				challengesmenu.unlockcount[CC_ANIM]++;

				Z_Free(challengesmenu.extradata);
				if ((challengesmenu.extradata = M_ChallengeGridExtraData()))
				{
					unlockable_t *ref = &unlockables[challengesmenu.currentunlock];
					UINT16 bombcolor = SKINCOLOR_NONE;

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
							INT32 skin = M_UnlockableFollowerNum(ref);
							if (skin != -1)
							{
								bombcolor = K_GetEffectiveFollowerColor(followers[skin].defaultcolor, cv_playercolor[0].value);
							}
							break;
						}
						default:
							break;
					}

					if (bombcolor == SKINCOLOR_NONE)
					{
						bombcolor = cv_playercolor[0].value;
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
			challengesmenu.fade--;
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

	if (challengesmenu.fade)
	{
		;
	}
#ifdef DEVELOP
	else if (M_MenuExtraPressed(pid) && challengesmenu.extradata) // debugging
	{
		if (challengesmenu.currentunlock < MAXUNLOCKABLES)
		{
			Z_Free(gamedata->challengegrid);
			gamedata->challengegrid = NULL;
			gamedata->challengegridwidth = 0;
			M_PopulateChallengeGrid();
			Z_Free(challengesmenu.extradata);
			challengesmenu.extradata = M_ChallengeGridExtraData();

			M_ChallengesAutoFocus(challengesmenu.currentunlock, true);

			challengesmenu.pending = true;
		}
		return true;
	}
#endif
	else
	{
		if (M_MenuBackPressed(pid) || start)
		{
			M_GoBack(0);
			M_SetMenuDelay(pid);

			Z_Free(challengesmenu.extradata);
			challengesmenu.extradata = NULL;

			challengesmenu.unlockcondition = NULL;

			return true;
		}

		if (challengesmenu.extradata != NULL && move)
		{
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
							+ challengesmenu.row]
						& CHE_CONNECTEDUP))
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
							+ challengesmenu.row]
						& CHE_CONNECTEDUP) ? 2 : 1;
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
							+ challengesmenu.row]
						& CHE_CONNECTEDLEFT))
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
							+ challengesmenu.row]
						& CHE_CONNECTEDLEFT) ? 2 : 1;
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

				if (challengesmenu.hiliy > 0 && (challengesmenu.extradata[i] & CHE_CONNECTEDUP))
				{
					challengesmenu.hiliy--;
				}

				if ((challengesmenu.extradata[i] & CHE_CONNECTEDLEFT))
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
