
#include "k_follower.h"

#include "k_kart.h"

#include "doomtype.h"
#include "doomdef.h"
#include "g_game.h"
#include "g_demo.h"
#include "r_skins.h"
#include "p_local.h"
#include "p_mobj.h"

INT32 numfollowers = 0;
follower_t followers[MAXSKINS];

CV_PossibleValue_t Followercolor_cons_t[MAXSKINCOLORS+3];	// +3 to account for "Match", "Opposite" & NULL

/*--------------------------------------------------
	INT32 K_FollowerAvailable(const char *name)

		See header file for description.
--------------------------------------------------*/
INT32 K_FollowerAvailable(const char *name)
{
	INT32 i;

	for (i = 0; i < numfollowers; i++)
	{
		if (stricmp(followers[i].skinname, name) == 0)
			return i;
	}

	return -1;
}

/*--------------------------------------------------
	boolean K_SetFollowerByName(INT32 playernum, const char *skinname)

		See header file for description.
--------------------------------------------------*/
boolean K_SetFollowerByName(INT32 playernum, const char *skinname)
{
	INT32 i;
	player_t *player = &players[playernum];

	if (stricmp("None", skinname) == 0)
	{
		K_SetFollowerByNum(playernum, -1); // reminder that -1 is nothing
		return true;
	}

	for (i = 0; i < numfollowers; i++)
	{
		// search in the skin list
		if (stricmp(followers[i].skinname, skinname) == 0)
		{
			K_SetFollowerByNum(playernum, i);
			return true;
		}
	}

	if (P_IsLocalPlayer(player))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Follower '%s' not found.\n"), skinname);
	}
	else if (server || IsPlayerAdmin(consoleplayer))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Player %d (%s) follower '%s' not found\n"), playernum, player_names[playernum], skinname);
	}

	K_SetFollowerByNum(playernum, -1); // reminder that -1 is nothing
	return false;
}

/*--------------------------------------------------
	void K_SetFollowerByNum(INT32 playernum, INT32 skinnum)

		See header file for description.
--------------------------------------------------*/
void K_SetFollowerByNum(INT32 playernum, INT32 skinnum)
{
	player_t *player = &players[playernum];
	mobj_t *bub;
	mobj_t *tmp;

	player->followerready = true; // we are ready to perform follower related actions in the player thinker, now.

	if (skinnum >= -1 && skinnum <= numfollowers) // Make sure it exists!
	{
		/*
			We don't spawn the follower here since it'll be easier to handle all of it in the Player thinker itself.
			However, we will despawn it right here if there's any to make it easy for the player thinker to replace it or delete it.
		*/

		if (player->follower && skinnum != player->followerskin) // this is also called when we change colour so don't respawn the follower unless we changed skins
		{
			// Remove follower's possible hnext list (bubble)
			bub = player->follower->hnext;

			while (bub && !P_MobjWasRemoved(bub))
			{
				tmp = bub->hnext;
				P_RemoveMobj(bub);
				bub = tmp;
			}

			P_RemoveMobj(player->follower);
			P_SetTarget(&player->follower, NULL);
		}

		player->followerskin = skinnum;

		// for replays: We have changed our follower mid-game; let the game know so it can do the same in the replay!
		demo_extradata[playernum] |= DXD_FOLLOWER;
		return;
	}

	if (P_IsLocalPlayer(player))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Follower %d not found\n"), skinnum);
	}
	else if (server || IsPlayerAdmin(consoleplayer))
	{
		CONS_Alert(CONS_WARNING, "Player %d (%s) follower %d not found\n", playernum, player_names[playernum], skinnum);
	}

	K_SetFollowerByNum(playernum, -1); // Not found, then set -1 (nothing) as our follower.
}


/*--------------------------------------------------
	static void K_SetFollowerState(mobj_t *f, statenum_t state)

		Sets a follower object's state.
		This is done as a separate function to prevent running follower actions.

	Input Arguments:-
		f - The follower's mobj_t.
		state - The state to set.

	Return:-
		None
--------------------------------------------------*/
static void K_SetFollowerState(mobj_t *f, statenum_t state)
{
	if (f == NULL || P_MobjWasRemoved(f) == true)
	{
		// safety net
		return;
	}

	// No, do NOT set the follower to S_NULL. Set it to S_INVISIBLE.
	if (state == S_NULL)
	{
		state = S_INVISIBLE;
		f->threshold = 1; // Threshold = 1 means stop doing anything related to setting states, so that we don't get out of S_INVISIBLE
	}

	// extravalue2 stores the last "first state" we used.
	// because states default to idlestates, if we use an animation that uses an "ongoing" state line, don't reset it!
	// this prevents it from looking very dumb
	if (state == (statenum_t)f->extravalue2)
	{
		return;
	}

	// we will save the state into extravalue2.
	f->extravalue2 = state;

	P_SetMobjStateNF(f, state);
	if (f->state->tics > 0)
	{
		f->tics++;
	}
}

/*--------------------------------------------------
	void K_HandleFollower(player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_HandleFollower(player_t *player)
{
	follower_t fl;
	angle_t an;
	fixed_t zoffs;
	fixed_t sx, sy, sz, deltaz;
	UINT16 color;

	fixed_t bubble; // bubble scale (0 if no bubble)
	mobj_t *bmobj; // temp bubble mobj

	if (player->followerready == false)
	{
		// we aren't ready to perform anything follower related yet.
		return;
	}

	// How about making sure our follower exists and is added before trying to spawn it n' all?
	if (player->followerskin > numfollowers-1 || player->followerskin < -1)
	{
		//CONS_Printf("Follower skin invlaid. Setting to -1.\n");
		player->followerskin = -1;
		return;
	}

	// don't do anything if we can't have a follower to begin with.
	// (It gets removed under those conditions)
	if (player->spectator)
	{
		return;
	}

	if (player->followerskin < 0)
	{
		return;
	}

	// Before we do anything, let's be sure of where we're supposed to be
	fl = followers[player->followerskin];

	an = player->mo->angle + fl.atangle;
	zoffs = fl.zoffs;
	bubble = fl.bubblescale; // 0 if no bubble to spawn.

	// do you like angle maths? I certainly don't...
	sx = player->mo->x + FixedMul(FixedMul(player->mo->scale, fl.dist), FINECOSINE((an) >> ANGLETOFINESHIFT));
	sy = player->mo->y + FixedMul(FixedMul(player->mo->scale, fl.dist), FINESINE((an) >> ANGLETOFINESHIFT));

	// interp info helps with stretchy fix
	deltaz = (player->mo->z - player->mo->old_z);

	// for the z coordinate, don't be a doof like Steel and forget that MFE_VERTICALFLIP exists :P
	sz = player->mo->z + FixedMul(player->mo->scale, zoffs) * P_MobjFlip(player->mo);
	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		sz += FixedMul(fl.height, player->mo->scale);
	}

	// finally, add a cool floating effect to the z height.
	// not stolen from k_kart I swear!!
	{
		const fixed_t pi = (22<<FRACBITS) / 7; // loose approximation, this doesn't need to be incredibly precise
		fixed_t sine = FixedMul(fl.bobamp, FINESINE((((8 * pi * fl.bobspeed) * leveltime) >> ANGLETOFINESHIFT) & FINEMASK));
		sz += FixedMul(player->mo->scale, sine) * P_MobjFlip(player->mo);
	}

	// Set follower colour
	switch (player->followercolor)
	{
		case FOLLOWERCOLOR_MATCH: // "Match"
			color = player->skincolor;
			break;

		case FOLLOWERCOLOR_OPPOSITE: // "Opposite"
			color = skincolors[player->skincolor].invcolor;
			break;

		default:
			color = player->followercolor;
			if (color == 0 || color > MAXSKINCOLORS+2) // Make sure this isn't garbage
			{
				color = player->skincolor; // "Match" as fallback.
			}
			break;
	}

	if (player->follower == NULL) // follower doesn't exist / isn't valid
	{
		//CONS_Printf("Spawning follower...\n");

		// so let's spawn one!
		P_SetTarget(&player->follower, P_SpawnMobj(sx, sy, sz, MT_FOLLOWER));
		K_SetFollowerState(player->follower, fl.idlestate);
		P_SetTarget(&player->follower->target, player->mo);	// we need that to know when we need to disappear
		P_InitAngle(player->follower, player->mo->angle);

		// This is safe to only spawn it here, the follower is removed then respawned when switched.
		if (bubble)
		{
			bmobj = P_SpawnMobj(player->follower->x, player->follower->y, player->follower->z, MT_FOLLOWERBUBBLE_FRONT);
			P_SetTarget(&player->follower->hnext, bmobj);
			P_SetTarget(&bmobj->target, player->follower); // Used to know if we have to despawn at some point.

			bmobj = P_SpawnMobj(player->follower->x, player->follower->y, player->follower->z, MT_FOLLOWERBUBBLE_BACK);
			P_SetTarget(&player->follower->hnext->hnext, bmobj); // this seems absolutely stupid, I know, but this will make updating the momentums/flags of these a bit easier.
			P_SetTarget(&bmobj->target, player->follower); // Ditto
		}

		player->follower->extravalue1 = 0; // extravalue1 is used to know what "state set" to use.
		/*
			0 = idle
			1 = forwards
			2 = hurt
			3 = win
			4 = lose
			5 = hitconfirm (< this one uses ->movecount as timer to know when to end, and goes back to normal states afterwards, unless hurt)
		*/
	}
	else // follower exists, woo!
	{
		// Safety net (2)

		if (P_MobjWasRemoved(player->follower))
		{
			P_SetTarget(&player->follower, NULL); // Remove this and respawn one, don't crash the game if Lua decides to P_RemoveMobj this thing.
			return;
		}

		// first of all, handle states following the same model as above:
		if (player->follower->tics == 1)
		{
			K_SetFollowerState(player->follower, player->follower->state->nextstate);
		}

		// move the follower next to us (yes, this is really basic maths but it looks pretty damn clean in practice)!
		// 02/09/2021: cast lag to int32 otherwise funny things happen since it was changed to uint32 in the struct
		player->follower->momx = FixedDiv(sx - player->follower->x, fl.horzlag);
		player->follower->momy = FixedDiv(sy - player->follower->y, fl.horzlag);
		player->follower->z += FixedDiv(deltaz, fl.vertlag);
		player->follower->momz = FixedDiv(sz - player->follower->z, fl.vertlag);
		player->follower->angle = player->mo->angle;

		if (player->mo->colorized)
		{
			player->follower->color = player->mo->color;
		}
		else
		{
			player->follower->color = color;
		}

		player->follower->colorized = player->mo->colorized;

		P_SetScale(player->follower, FixedMul(fl.scale, player->mo->scale));
		K_GenericExtraFlagsNoZAdjust(player->follower, player->mo); // Not K_MatchGenericExtraFlag because the Z adjust it has only works properly if master & mo have the same Z height.

		// Match how the player is being drawn
		player->follower->renderflags = player->mo->renderflags;

		// Make the follower invisible if we no contest'd rather than removing it. No one will notice the diff seriously.
		if (player->pflags & PF_NOCONTEST)
		{
			player->follower->renderflags |= RF_DONTDRAW;
		}

		// if we're moving let's make the angle the direction we're moving towards. This is to avoid drifting / reverse looking awkward.
		player->follower->angle = K_MomentumAngle(player->follower);

		// Finally, if the follower has bubbles, move them, set their scale, etc....
		// This is what I meant earlier by it being easier, now we can just use this weird lil loop to get the job done!

		bmobj = player->follower->hnext; // will be NULL if there's no bubble

		while (bmobj != NULL && P_MobjWasRemoved(bmobj) == false)
		{
			// match follower's momentums and (e)flags(2).
			bmobj->momx = player->follower->momx;
			bmobj->momy = player->follower->momy;
			bmobj->z += FixedDiv(deltaz, fl.vertlag);
			bmobj->momz = player->follower->momz;

			P_SetScale(bmobj, FixedMul(bubble, player->mo->scale));
			K_GenericExtraFlagsNoZAdjust(bmobj, player->follower);
			bmobj->renderflags = player->mo->renderflags;

			if (player->follower->threshold)
			{
				// threshold means the follower was "despawned" with S_NULL (is actually just set to S_INVISIBLE)
				P_SetMobjState(bmobj, S_INVISIBLE); // sooooo... let's do the same!
			}

			// switch to other bubble layer or exit
			bmobj = bmobj->hnext;
		}

		if (player->follower->threshold)
		{
			// Threshold means the follower was "despanwed" with S_NULL.
			return;
		}

		// However with how the code is factored, this is just a special case of S_INVISBLE to avoid having to add other player variables.

		// handle follower animations. Could probably be better...
		// hurt or dead
		if (P_PlayerInPain(player) == true || player->mo->state == &states[S_KART_SPINOUT] || player->mo->health <= 0)
		{
			// cancel hit confirm.
			player->follower->movecount = 0;

			// spin out
			player->follower->angle = player->drawangle;

			if (player->follower->extravalue1 != 2)
			{
				player->follower->extravalue1 = 2;
				K_SetFollowerState(player->follower, fl.hurtstate);
			}

			if (player->mo->health <= 0)
			{
				// if dead, follow the player's z momentum exactly so they both look like they die at the same speed.
				player->follower->momz = player->mo->momz;
			}
		}
		else if (player->follower->movecount)
		{
			if (player->follower->extravalue1 != 5)
			{
				player->follower->extravalue1 = 5;
				K_SetFollowerState(player->follower, fl.hitconfirmstate);
			}

			player->follower->movecount--;
		}
		else if (player->speed > 10*player->mo->scale)	// animation for moving fast enough
		{
			if (player->follower->extravalue1 != 1)
			{
				player->follower->extravalue1 = 1;
				K_SetFollowerState(player->follower, fl.followstate);
			}
		}
		else
		{
			// animations when nearly still. This includes winning and losing.
			if (player->follower->extravalue1 != 0)
			{
				if (player->exiting)
				{
					// win/ loss animations
					if (K_IsPlayerLosing(player))
					{
						// L
						if (player->follower->extravalue1 != 4)
						{
							player->follower->extravalue1 = 4;
							K_SetFollowerState(player->follower, fl.losestate);
						}
					}
					else
					{
						// W
						if (player->follower->extravalue1 != 3)
						{
							player->follower->extravalue1 = 3;
							K_SetFollowerState(player->follower, fl.winstate);
						}
					}
				}
				else
				{
					// normal standstill
					player->follower->extravalue1 = 0;
					K_SetFollowerState(player->follower, fl.idlestate);
				}
			}
		}
	}
}
