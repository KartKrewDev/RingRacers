// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  objects/audience.c
/// \brief Follower Audience

#include "../doomdef.h"
#include "../info.h"
#include "../g_game.h"
#include "../k_objects.h"
#include "../k_follower.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../p_setup.h"
#include "../r_draw.h" // R_GetColorByName
#include "../r_main.h" // R_PointToAngle2, R_PointToDist2
#include "../z_zone.h" // Z_StrDup/Z_Free

// The following cannot be used due to conflicts with MT_EMBLEM.
//#define audience_emblem_reserved_1(o) ((o)->reactiontime)

#define audience_mainstate(o) ((o)->cvmem)

#define audience_bobamp(o) ((o)->movefactor)
#define audience_bobspeed(o) ((o)->cusval)

#define audience_animoffset(o) ((o)->threshold)

#define audience_focusplayer(o) ((o)->lastlook)
#define audience_focusdelay(o) ((o)->movecount)

void
Obj_AudienceInit
(		mobj_t * mobj,
		mapthing_t *mthing,
		INT32 followerpick)
{
	const boolean ourchoiceofvisuals = (followerpick < 0 || followerpick > numfollowers);
	INT16 *reflist = NULL;
	INT16 tempreflist[MAXHEADERFOLLOWERS];
	UINT8 numref = 0;

	audience_mainstate(mobj) = S_NULL;

	// Pick follower
	if (ourchoiceofvisuals == true)
	{
		if (mthing != NULL && mthing->thing_stringargs[0] != NULL)
		{
			// From mapthing
			char *stringcopy = Z_StrDup(mthing->thing_stringargs[0]);
			char *tok = strtok(stringcopy, " ,");
			char *c; // for erasing underscores

			numref = 0;
			while (tok && numref < MAXHEADERFOLLOWERS)
			{
				// Match follower name conversion
				for (c = tok; *c; c++)
				{
					if (*c != '_')
						continue;
					*c = ' ';
				}

				if ((tempreflist[numref] = K_FollowerAvailable(tok)) == -1)
				{
					CONS_Alert(CONS_WARNING, "Mapthing %s: Follower \"%s\" is invalid!\n", sizeu1(mthing-mapthings), tok);
				}
				else
					numref++;

				tok = strtok(NULL, " ,");
			}

			if (!numref)
			{
				// This is the one thing a user should definitely be told about.
				CONS_Alert(CONS_WARNING, "Mapthing %s: Follower audience has no valid followers to pick from!\n", sizeu1(mthing-mapthings));
				// DO NOT RETURN HERE
			}

			Z_Free(stringcopy);

			reflist = tempreflist;
		}
		else
		{
			// From mapheader

			if (!mapheaderinfo[gamemap-1])
			{
				// No mapheader, no shoes, no service.
				return;
			}

			numref = mapheaderinfo[gamemap-1]->numFollowers;
			reflist = mapheaderinfo[gamemap-1]->followers;
		}

		if (!numref || !reflist)
		{
			// Clean up after ourselves.
			P_RemoveMobj(mobj);
			return;
		}

		followerpick = reflist[P_RandomKey(PR_RANDOMAUDIENCE, numref)];

		if (followerpick < 0 || followerpick >= numfollowers)
		{
			// Is this user error or user choice..?
			P_RemoveMobj(mobj);
			return;
		}
	}

	// Handle storing follower properties on the object
	{
		mobj->destscale = FixedMul(3*mobj->destscale, followers[followerpick].scale);
		P_SetScale(mobj, mobj->destscale);

		if (mobj->flags2 & MF2_BOSSNOTRAP)
		{
			audience_bobamp(mobj) = 0;
		}
		else
		{
			fixed_t bobscale = mapobjectscale * 2;
			// The following is derived from the default bobamp
			if (mobj->type != MT_EMBLEM && !(mobj->flags & MF_NOGRAVITY) && followers[followerpick].bobamp < 4*FRACUNIT)
			{
				audience_bobamp(mobj) = 4*bobscale;
			}
			else
			{
				audience_bobamp(mobj) = FixedMul(bobscale, followers[followerpick].bobamp);
			}
		}

		audience_bobspeed(mobj) = followers[followerpick].bobspeed;
		audience_focusplayer(mobj) = MAXPLAYERS;

		audience_mainstate(mobj) =
			audience_bobamp(mobj) != 0
				? followers[followerpick].followstate
				: followers[followerpick].idlestate;

		P_SetMobjState(mobj, audience_mainstate(mobj));
		if (P_MobjWasRemoved(mobj))
			return;

		if (P_RandomChance(PR_RANDOMAUDIENCE, FRACUNIT/2))
		{
			audience_animoffset(mobj) = 5;
		}
	}

	// Handle colors
	if (ourchoiceofvisuals == true)
	{
		UINT16 colorpick = SKINCOLOR_NONE;

		if (mthing != NULL && mthing->thing_stringargs[1] != NULL)
		{
			if (!stricmp("Random", mthing->thing_stringargs[1]))
			{
				colorpick = FOLLOWERCOLOR_MATCH;
			}
			else
			{
				char *stringcopy = Z_StrDup(mthing->thing_stringargs[1]);
				char *tok = strtok(stringcopy, " ");

				numref = 0;
				while (tok && numref < MAXHEADERFOLLOWERS)
				{
					if ((tempreflist[numref++] = R_GetColorByName(tok)) == SKINCOLOR_NONE)
						CONS_Alert(CONS_WARNING, "Mapthing %s: Follower color \"%s\" is invalid!\n", sizeu1(mthing-mapthings), tok);
					tok = strtok(NULL, " ");
				}

				Z_Free(stringcopy);

				if (numref)
				{
					colorpick = tempreflist[P_RandomKey(PR_RANDOMAUDIENCE, numref)];
				}
			}
		}

		if (colorpick == SKINCOLOR_NONE
			|| (colorpick >= numskincolors
				&& colorpick != FOLLOWERCOLOR_MATCH
				&& colorpick != FOLLOWERCOLOR_OPPOSITE))
		{
			colorpick = followers[followerpick].defaultcolor;
		}

		if (colorpick >= numskincolors)
		{
			colorpick = P_RandomKey(PR_RANDOMAUDIENCE, numskincolors-1)+1;
		}

		mobj->color = colorpick;
	}
}

void
Obj_AudienceThink
(		mobj_t * mobj,
		boolean focusonplayer,
		boolean checkdeathpit)
{
	boolean landed = false;

	if (mobj->fuse && mobj->fuse < (TICRATE/2))
	{
		mobj->renderflags ^= RF_DONTDRAW;
		return; // no jumping when you hit the floor, your gravity is weird
	}

	if (audience_mainstate(mobj) == S_NULL)
	{
		// Uninitialised, don't do anything funny.
		return;
	}

	if (focusonplayer == true)
	{
		if (audience_focusplayer(mobj) < MAXPLAYERS && audience_focusplayer(mobj) >= 0)
		{
			if (playeringame[audience_focusplayer(mobj)] == false
			|| players[audience_focusplayer(mobj)].spectator == true
			|| P_MobjWasRemoved(players[audience_focusplayer(mobj)].mo))
			{
				// Reset the timer, search for a player again
				audience_focusdelay(mobj) = 0;
			}
		}

		if (audience_focusdelay(mobj) == 0)
		{
			fixed_t bestdist = INT32_MAX, dist;
			UINT8 i;

			audience_focusplayer(mobj) = MAXPLAYERS;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] == false
				|| players[i].spectator == true
				|| P_MobjWasRemoved(players[i].mo))
					continue;

				dist = R_PointToDist2(
					mobj->x,
					mobj->y,
					players[i].mo->x,
					players[i].mo->y
				);

				if (dist >= bestdist)
					continue;

				dist = R_PointToDist2(
					mobj->z,
					0,
					players[i].mo->z,
					dist
				);

				if (dist >= bestdist)
					continue;

				bestdist = dist;
				audience_focusplayer(mobj) = i;
			}

			// Try to add some spacing out so the object isn't constantly looking for players
			audience_focusdelay(mobj) = TICRATE + min((bestdist/FRACUNIT), TICRATE) + (bestdist % TICRATE);
		}
		else
		{
			audience_focusdelay(mobj)--;
		}

		if (audience_focusplayer(mobj) < MAXPLAYERS && audience_focusplayer(mobj) >= 0)
		{
			angle_t diff = R_PointToAngle2(
				mobj->x,
				mobj->y,
				players[audience_focusplayer(mobj)].mo->x,
				players[audience_focusplayer(mobj)].mo->y
			) - mobj->angle;

			boolean reverse = (diff >= ANGLE_180);

			if (reverse)
				diff = InvAngle(diff);

			if (diff > (ANG1*5))
				diff /= 5;

			if (reverse)
				diff = InvAngle(diff);

			mobj->angle += diff;
		}
	}

	if (mobj->flags & MF_NOGRAVITY)
	{
		// This horrible calculation was inherited from k_follower.c
		mobj->sprzoff = FixedMul(audience_bobamp(mobj),
			FINESINE(((
				FixedMul(4 * M_TAU_FIXED, audience_bobspeed(mobj))
				* (leveltime + audience_animoffset(mobj))
			) >> ANGLETOFINESHIFT) & FINEMASK));

		// Offset to not go through floor...
		if (mobj->type == MT_EMBLEM)
		{
			; // ...unless it's important to keep a centered hitbox
		}
		else if (mobj->flags2 & MF2_OBJECTFLIP)
		{
			mobj->sprzoff -= audience_bobamp(mobj);
		}
		else
		{
			mobj->sprzoff += audience_bobamp(mobj);
		}
	}
	else if (audience_animoffset(mobj) > 0)
	{
		// Skipped frames at spawn for offset in jumping
		audience_animoffset(mobj)--;
	}
	else if (audience_bobamp(mobj) == 0)
	{
		// Just sit there
		;
	}
	else if (mobj->flags2 & MF2_OBJECTFLIP)
	{
		landed = (mobj->z + mobj->height >= mobj->ceilingz);
	}
	else
	{
		landed = (mobj->z <= mobj->floorz);
	}

	if (landed == true)
	{
		if (checkdeathpit && P_CheckDeathPitCollide(mobj))
		{
			P_RemoveMobj(mobj);
			return;
		}

		mobj->momx = mobj->momy = 0;
		mobj->momz = P_MobjFlip(mobj)*audience_bobamp(mobj);
		P_SetMobjState(mobj, audience_mainstate(mobj));
	}
}
