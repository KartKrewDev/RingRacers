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

#define audience_mainstate(o) ((o)->cvmem)

#define audience_bobamp(o) ((o)->cusval)
#define audience_bobspeed(o) ((o)->reactiontime)

#define audience_animoffset(o) ((o)->threshold)

#define audience_focusplayer(o) ((o)->lastlook)
#define audience_focusdelay(o) ((o)->movecount)

void
Obj_RandomAudienceInit
(		mobj_t * mobj,
		mapthing_t *mthing)
{
	UINT16 *reflist = NULL;
	UINT16 tempreflist[MAXHEADERFOLLOWERS];
	UINT8 numref = 0;
	INT32 followerpick = 0;

	P_SetScale(mobj, (mobj->destscale <<= 1));

	audience_mainstate(mobj) = S_NULL;

	// Pick follower
	{
		if (mthing->stringargs[0] != NULL)
		{
			// From mapthing
			char *stringcopy = Z_StrDup(mthing->stringargs[0]);
			char *tok = strtok(stringcopy, " ,");

			numref = 0;
			while (tok && numref < MAXHEADERFOLLOWERS)
			{
				tempreflist[numref++] = K_FollowerAvailable(tok);
				tok = strtok(NULL, " ,");
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
			// This is the one thing a user should definitely be told about.
			CONS_Alert(CONS_WARNING, "Mapthing %s: Follower audience has no valid followers to pick from!\n", sizeu1(mthing-mapthings));
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
		audience_mainstate(mobj) = followers[followerpick].followstate;

		P_SetMobjState(mobj, audience_mainstate(mobj));
		if (P_MobjWasRemoved(mobj))
			return;

		if (mthing->args[2] != 0)
		{
			mobj->flags |= MF_NOGRAVITY;
		}

		if (mthing->args[3] != 0)
		{
			mobj->flags2 |= MF2_AMBUSH;
		}

		// The following is derived from the default bobamp
		if (!(mobj->flags & MF_NOGRAVITY) && followers[followerpick].bobamp < 4*FRACUNIT)
		{
			audience_bobamp(mobj) = 4*mobj->scale;
		}
		else
		{
			audience_bobamp(mobj) = FixedMul(mobj->scale, followers[followerpick].bobamp);
		}

		audience_bobspeed(mobj) = followers[followerpick].bobspeed;
		audience_focusplayer(mobj) = MAXPLAYERS;

		if (P_RandomChance(PR_RANDOMAUDIENCE, FRACUNIT/2))
		{
			audience_animoffset(mobj) = 5;
		}
	}

	// Handle colors
	{
		UINT16 colorpick = SKINCOLOR_NONE;

		if (mthing->stringargs[1] != NULL)
		{
			if (!strcmp("Random", mthing->stringargs[1]))
			{
				colorpick = FOLLOWERCOLOR_MATCH;
			}
			else
			{
				char *stringcopy = Z_StrDup(mthing->stringargs[1]);
				char *tok = strtok(stringcopy, " ");

				numref = 0;
				while (tok && numref < MAXHEADERFOLLOWERS)
				{
					tempreflist[numref++] = R_GetColorByName(tok);
					tok = strtok(NULL, " ");
				}

				Z_Free(stringcopy);

				if (numref)
				{
					colorpick = tempreflist[P_RandomKey(PR_RANDOMAUDIENCE, numref)];
				}
			}
		}

		if (colorpick == SKINCOLOR_NONE)
		{
			colorpick = followers[followerpick].defaultcolor;
		}

		if (colorpick == FOLLOWERCOLOR_MATCH
			|| colorpick == FOLLOWERCOLOR_OPPOSITE)
		{
			colorpick = P_RandomKey(PR_RANDOMAUDIENCE, numskincolors-1)+1;
		}

		mobj->color = colorpick;
	}
}

void
Obj_RandomAudienceThink
(		mobj_t * mobj)
{
	if (audience_mainstate(mobj) == S_NULL)
	{
		// Uninitialised, don't do anything funny.
		return;
	}

	if (mobj->flags2 & MF2_AMBUSH)
	{
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
			audience_focusdelay(mobj) = TICRATE + min((bestdist/FRACUNIT), (2*TICRATE)) + (bestdist % TICRATE);
		}
		else
		{
			audience_focusdelay(mobj)--;
		}

		if (audience_focusplayer(mobj) < MAXPLAYERS && audience_focusplayer(mobj) >= 0)
		{
			mobj->angle = R_PointToAngle2(
				mobj->x,
				mobj->y,
				players[audience_focusplayer(mobj)].mo->x,
				players[audience_focusplayer(mobj)].mo->y
			);
		}
	}

	if (mobj->flags & MF_NOGRAVITY)
	{
		// This horrible calculation was inherited from k_follower.c, with only newlines (and a FRACUNIT offset) added
		mobj->sprzoff = FixedMul(audience_bobamp(mobj),
			FRACUNIT + FINESINE(((
				FixedMul(4 * M_TAU_FIXED, audience_bobspeed(mobj))
				* (leveltime + audience_animoffset(mobj))
			) >> ANGLETOFINESHIFT) & FINEMASK));

		// Gravity
		if (mobj->flags2 & MF2_OBJECTFLIP)
		{
			mobj->sprzoff = -mobj->sprzoff;
		}
	}
	else if (audience_animoffset(mobj) > 0)
	{
		// Skipped frames at spawn for offset in jumping
		audience_animoffset(mobj)--;
	}
	else if (mobj->flags2 & MF2_OBJECTFLIP)
	{
		if (mobj->z + mobj->height >= mobj->ceilingz)
		{
			mobj->momz = -audience_bobamp(mobj);
			P_SetMobjState(mobj, audience_mainstate(mobj));
		}
	}
	else if (mobj->z <= mobj->floorz)
	{
		mobj->momz = audience_bobamp(mobj);
		P_SetMobjState(mobj, audience_mainstate(mobj));
	}
}
