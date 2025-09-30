// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_race.c
/// \brief Race Mode specific code.

#include "k_race.h"

#include "k_kart.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "k_color.h"
#include "k_respawn.h"
#include "doomdef.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "p_slopes.h"
#include "p_setup.h"
#include "r_draw.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "m_misc.h"
#include "m_cond.h"
#include "f_finale.h"
#include "lua_hud.h"	// For Lua hud checks
#include "lua_hook.h"	// For MobjDamage and ShouldDamage
#include "m_cheat.h"	// objectplacing
#include "p_spec.h"

#include "k_waypoint.h"
#include "k_bot.h"
#include "k_hud.h"

line_t *finishBeamLine = NULL;

static mobj_t *beamPoints[2];
static UINT8 numBeamPoints = 0;

/*--------------------------------------------------
	void K_ClearFinishBeamLine(void)

		See header file for description.
--------------------------------------------------*/
void K_ClearFinishBeamLine(void)
{
	size_t i;

	finishBeamLine = NULL;

	for (i = 0; i < 2; i++)
	{
		beamPoints[i] = NULL;
	}

	numBeamPoints = 0;
}

/*--------------------------------------------------
	static void K_FreeFinishBeamLine(void)

		See header file for description.
--------------------------------------------------*/
static void K_FreeFinishBeamLine(void)
{
	if (finishBeamLine != NULL)
	{
		if (finishBeamLine->v1 != NULL)
		{
			Z_Free(finishBeamLine->v1);
		}

		if (finishBeamLine->v2 != NULL)
		{
			Z_Free(finishBeamLine->v2);
		}

		Z_Free(finishBeamLine);
	}

	K_ClearFinishBeamLine();
}

/*--------------------------------------------------
	static void K_CreateFinishLineFromPoints(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)

		See header file for description.
--------------------------------------------------*/
static void K_CreateFinishLineFromPoints(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
	I_Assert(finishBeamLine == NULL); // Needs to be NULL

	finishBeamLine = Z_Calloc(sizeof (*finishBeamLine), PU_LEVEL, NULL);

	finishBeamLine->v1 = Z_Calloc(sizeof (*finishBeamLine->v1), PU_LEVEL, NULL);
	finishBeamLine->v1->x = x1;
	finishBeamLine->v1->y = y1;

	finishBeamLine->v2 = Z_Calloc(sizeof (*finishBeamLine->v2), PU_LEVEL, NULL);
	finishBeamLine->v2->x = x2;
	finishBeamLine->v2->y = y2;

	finishBeamLine->dx = x2 - x1;
	finishBeamLine->dy = y2 - y1;

	finishBeamLine->angle = R_PointToAngle2(0, 0, finishBeamLine->dx, finishBeamLine->dy);

	finishBeamLine->flags = 0;
}

/*--------------------------------------------------
	boolean K_GenerateFinishBeamLine(void)

		See header file for description.
--------------------------------------------------*/
boolean K_GenerateFinishBeamLine(void)
{
	mapthing_t *mt;

	INT64 bounds[4];
	angle_t angle = 0;

	boolean valid = false;
	size_t i;

	// Ensure everything's freed by this time.
	K_FreeFinishBeamLine();

	// First: attempt to create from beam point objects
	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		if (numBeamPoints >= 2)
		{
			break;
		}

		if (mt->type == mobjinfo[MT_BEAMPOINT].doomednum)
		{
			beamPoints[numBeamPoints] = mt->mobj;
			numBeamPoints++;
		}
	}

	if (numBeamPoints == 2)
	{
		// Found 'em! Really easy to generate a line out of these :)

		K_CreateFinishLineFromPoints(
			beamPoints[0]->x, beamPoints[0]->y,
			beamPoints[1]->x, beamPoints[1]->y
		);

		return true;
	}

	bounds[0] = INT64_MAX; // min x
	bounds[1] = INT64_MIN; // max x
	bounds[2] = INT64_MAX; // min y
	bounds[3] = INT64_MIN; // max y

	for (i = 0; i < numlines; i++)
	{
		angle_t thisAngle;

		if (lines[i].special != 2001)
		{
			continue;
		}

		thisAngle = R_PointToAngle2(0, 0, lines[i].dx, lines[i].dy);

		bounds[0] = min(bounds[0], min(lines[i].v1->x, lines[i].v2->x)); // min x
		bounds[1] = max(bounds[1], max(lines[i].v1->x, lines[i].v2->x)); // max x
		bounds[2] = min(bounds[2], min(lines[i].v1->y, lines[i].v2->y)); // min y
		bounds[3] = max(bounds[3], max(lines[i].v1->y, lines[i].v2->y)); // max y

		if (valid == false)
		{
			angle = thisAngle;
			valid = true;
		}
		else if (angle != thisAngle)
		{
			// Do not even attempt to bother with curved finish lines.
			// Will likely just crash.
			valid = false;

			break;
		}
	}

	if (valid == true)
	{
		fixed_t span = P_AproxDistance(bounds[1] - bounds[0], bounds[3] - bounds[2]) / 2;

		fixed_t cx = (bounds[0] + bounds[1]) / 2;
		fixed_t cy = (bounds[2] + bounds[3]) / 2;

		fixed_t spanC = FixedMul(span, FINECOSINE(angle >> ANGLETOFINESHIFT));
		fixed_t spanS = FixedMul(span, FINESINE(angle >> ANGLETOFINESHIFT));

		K_CreateFinishLineFromPoints(
			cx - spanC, cy - spanS,
			cx + spanC, cy + spanS
		);

		return true;
	}

	return false;
}

/*--------------------------------------------------
	static void K_DrawFinishLineBeamForLine(fixed_t offset, angle_t aiming, line_t *line, boolean reverse)

		Draws a helix out of rainbow colored orbs along a line, unique for each display player.
		Called twice for the finish line beam effect.

	Input Arguments:-
		offset - Offset value for positioning. Changed every tick to make it animate.
		aiming - Starting vertical angle value. Changed every tick to make it animate.
		line - Linedef to draw along.
		reverse - Draw in reverse. Call twice with this toggled to make a double helix.

	Return:-
		None
--------------------------------------------------*/

static void K_DrawFinishLineBeamForLine(fixed_t offset, angle_t aiming, line_t *line, boolean reverse)
{
	const fixed_t linelength = P_AproxDistance(line->dx, line->dy);
	const fixed_t xstep = FixedDiv(line->dx, linelength);
	const fixed_t ystep = FixedDiv(line->dy, linelength);

	const boolean passable = (leveltime >= starttime || G_TimeAttackStart());

	fixed_t linex = line->v1->x;
	fixed_t liney = line->v1->y;
	angle_t lineangle = line->angle + ANGLE_90;

	UINT8 i;

	fixed_t dist[4] = {INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX};

	if (line->flags & ML_NOCLIMB)
	{
		// Line is flipped
		lineangle += ANGLE_180;
	}

	linex += FixedMul(offset, xstep);
	liney += FixedMul(offset, ystep);

	while (offset < linelength)
	{
#define COLORCYCLELEN 10
		const UINT8 colorcycle[COLORCYCLELEN] = {
			SKINCOLOR_PERIWINKLE,
			SKINCOLOR_SLATE,
			SKINCOLOR_BLOSSOM,
			SKINCOLOR_RASPBERRY,
			SKINCOLOR_ORANGE,
			SKINCOLOR_YELLOW,
			SKINCOLOR_LIME,
			SKINCOLOR_TURTLE,
			SKINCOLOR_ROBIN,
			SKINCOLOR_JAWZ
		};

		const UINT8 numframes = 5;
		const angle_t framethreshold = ANGLE_180 / (numframes-1);
		const angle_t frameaim = aiming + (framethreshold / 2);

		fixed_t x, y, z;
		UINT8 spriteframe = 0;

		x = linex + FixedMul(FixedMul(FINISHLINEBEAM_SPACING, FINECOSINE(lineangle >> ANGLETOFINESHIFT)), FINECOSINE(aiming >> ANGLETOFINESHIFT));
		y = liney + FixedMul(FixedMul(FINISHLINEBEAM_SPACING, FINESINE(lineangle >> ANGLETOFINESHIFT)), FINECOSINE(aiming >> ANGLETOFINESHIFT));
		z = FINISHLINEBEAM_SPACING + FixedMul(FINISHLINEBEAM_SPACING, FINESINE(aiming >> ANGLETOFINESHIFT));

		if (passable)
		{
			spriteframe = 4; // Weakest sprite when passable
		}
		else if (frameaim > ANGLE_180)
		{
			spriteframe = (ANGLE_MAX - frameaim) / framethreshold;
		}
		else
		{
			spriteframe = frameaim / framethreshold;
		}

		for (i = 0; i <= r_splitscreen; i++)
		{
			if (playeringame[displayplayers[i]] && players[displayplayers[i]].mo && !P_MobjWasRemoved(players[displayplayers[i]].mo))
			{
				mobj_t *beam;

				beam = P_SpawnMobj(x, y, players[displayplayers[i]].mo->z + z, MT_THOK);
				P_SetMobjState(beam, S_FINISHBEAM1 + spriteframe);

				beam->colorized = true;
				beam->renderflags = RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(&players[displayplayers[i]]);

				if (reverse)
				{
					beam->color = colorcycle[((leveltime / 4) + (COLORCYCLELEN/2)) % COLORCYCLELEN];
				}
				else
				{
					beam->color = colorcycle[(leveltime / 4) % COLORCYCLELEN];
				}

				fixed_t n =
					R_PointToDist2(
						players[displayplayers[i]].mo->x,
						players[displayplayers[i]].mo->y,
						beam->x,
						beam->y
					);
				if (dist[i] > n)
					dist[i] = n;
			}
		}

		offset += FINISHLINEBEAM_SPACING;
		linex += FixedMul(FINISHLINEBEAM_SPACING, xstep);
		liney += FixedMul(FINISHLINEBEAM_SPACING, ystep);

		if (reverse)
		{
			aiming -= ANGLE_45;
		}
		else
		{
			aiming += ANGLE_45;
		}
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (playeringame[displayplayers[i]] && players[displayplayers[i]].mo && !P_MobjWasRemoved(players[displayplayers[i]].mo))
		{
			UINT8 j;
			for (j = 0; j < 2; j++)
			{
				vertex_t *v = line->v1;
				mobj_t *end1, *end2;
				angle_t a = R_PointToAngle2(0, 0, line->dx, line->dy);
				fixed_t sx;
				fixed_t sy;

				/*
				if (line->flags & ML_NOCLIMB)
				{
					a += ANGLE_180;
				}
				*/

				sx = FixedMul(3*mapobjectscale, FINECOSINE(a >> ANGLETOFINESHIFT));
				sy = FixedMul(3*mapobjectscale, FINESINE(a >> ANGLETOFINESHIFT));

				if (j == 1)
				{
					v = line->v2;
					sx = -sx;
					sy = -sy;
				}

				end1 = P_SpawnMobj(
					v->x + sx,
					v->y + sy,
					players[displayplayers[i]].mo->z + FINISHLINEBEAM_SPACING,
					MT_THOK
				);

				P_SetMobjState(end1, S_FINISHBEAMEND1);
				end1->renderflags = RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(&players[displayplayers[i]]);
				end1->angle = lineangle;

				end2 = P_SpawnMobj(
					v->x + (8*sx),
					v->y + (8*sy),
					players[displayplayers[i]].mo->z + FINISHLINEBEAM_SPACING,
					MT_THOK
				);

				P_SetMobjState(end2, S_FINISHBEAMEND2);
				end2->renderflags = RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(&players[displayplayers[i]]);
				end2->angle = lineangle;

				P_SetTarget(&end2->tracer, end1);
				end2->flags2 |= MF2_LINKDRAW;
			}

			if (!passable && leveltime % 40 == 0)
			{
				const fixed_t maxdist = 1280 * mapobjectscale;
				const fixed_t mindist = 320 * mapobjectscale;
				fixed_t f = min(dist[i], maxdist);
				f = max(f, mindist);
				S_StartSoundAtVolume(players[i].mo, sfx_s3k73,
					FixedMul(FRACUNIT - FixedDiv(f - mindist, maxdist - mindist), 255));
			}
		}
	}
}

/*--------------------------------------------------
	void K_RunFinishLineBeam(void)

		See header file for description.
--------------------------------------------------*/

void K_RunFinishLineBeam(void)
{	
	if ((gametyperules & GTR_ROLLINGSTART) || !(leveltime < starttime || rainbowstartavailable == true) || P_LevelIsFrozen())
	{
		return;
	}

	if (finishBeamLine != NULL)
	{
		const angle_t angoffset = ANGLE_45;
		const angle_t angadd = ANGLE_11hh;
		const fixed_t speed = 6 * mapobjectscale;

		fixed_t offseta = (leveltime * speed) % FINISHLINEBEAM_SPACING;
		angle_t aiminga = (angoffset * -((leveltime * speed) / FINISHLINEBEAM_SPACING)) + (angadd * leveltime);

		fixed_t offsetb = FINISHLINEBEAM_SPACING - offseta;
		angle_t aimingb = (angoffset * -((leveltime * speed) / FINISHLINEBEAM_SPACING)) - (angadd * leveltime);

		K_DrawFinishLineBeamForLine(offseta, aiminga, finishBeamLine, false);
		K_DrawFinishLineBeamForLine(offsetb, aimingb, finishBeamLine, true);
	}
}

/*--------------------------------------------------
	UINT8 K_RaceLapCount(INT16 mapNum);

		See header file for description.
--------------------------------------------------*/

UINT8 K_RaceLapCount(INT16 mapNum)
{
	if (!(gametyperules & GTR_CIRCUIT))
	{
		// Not in Race mode
		return 0;
	}

	if (cv_numlaps.value == -1)
	{
		// Use map default, except on Relaxed

		UINT8 laps = mapheaderinfo[mapNum]->numlaps;

		if (gamespeed == KARTSPEED_EASY && laps > 2)
			laps = (3*laps + 4 - 1) / 4; // 3/4th laps, rounded

		return laps;
	}

	return cv_numlaps.value;
}

void K_SpawnFinishEXP(player_t *player, UINT16 exp)
{
	if (finishBeamLine != NULL)
	{
		mobj_t *p1 = P_SpawnMobj(finishBeamLine->v1->x, finishBeamLine->v1->y, player->mo->z, MT_THOK);
		mobj_t *p2 = P_SpawnMobj(finishBeamLine->v2->x, finishBeamLine->v2->y, player->mo->z, MT_THOK);
		K_SpawnEXP(player, exp, p1);
		K_SpawnEXP(player, exp, p2);
	}
	else
	{
		K_SpawnEXP(player, exp*2, player->mo);
	}
}
