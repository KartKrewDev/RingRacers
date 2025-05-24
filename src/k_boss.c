// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_boss.c
/// \brief Boss battle game logic

#include "k_boss.h"
#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "p_local.h"
#include "k_kart.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "z_zone.h"

struct bossinfo bossinfo;

/*--------------------------------------------------
	void K_ResetBossInfo(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetBossInfo(void)
{
	Z_Free(bossinfo.enemyname);
	Z_Free(bossinfo.subtitle);
	memset(&bossinfo, 0, sizeof(struct bossinfo));
	bossinfo.barlen = BOSSHEALTHBARLEN;
	bossinfo.titlesound = sfx_typri1;
}

/*--------------------------------------------------
	void K_BossInfoTicker(void)

		See header file for description.
--------------------------------------------------*/
void K_BossInfoTicker(void)
{
	UINT8 i;

	if (bossinfo.valid == false)
		return;

	// Update healthbar data. (only if the hud is visible)
	if (leveltime > (TICRATE/2)+3)
	{
		// Greater than the actual health?
		if (bossinfo.visualbar > bossinfo.healthbar)
		{
			bossinfo.visualbar--;
			// If the boss is dying, start shrinking the healthbar.
			if (bossinfo.visualbar == 0)
				bossinfo.barlen -= 2;
		}
		// Less than the actual health?
		else if (bossinfo.visualbar < bossinfo.healthbar)
			bossinfo.visualbar++;
		// Continue to shrink the healthbar.
		else if (bossinfo.barlen > 1  && bossinfo.barlen < BOSSHEALTHBARLEN)
			bossinfo.barlen -= 2;

		// Jitter timer.
		if (bossinfo.visualbarimpact)
			bossinfo.visualbarimpact--;
	}

	if (bossinfo.doweakspotsound != SPOT_NONE)
	{
		S_StartSound(NULL, sfx_mbs55); // may change for bump option
		bossinfo.doweakspotsound = SPOT_NONE;
	}

	// Update weakspot data.
	for (i = 0; i < NUMWEAKSPOTS; i++)
	{
		// Clean out invalid references.
		if ((bossinfo.weakspots[i].spot && P_MobjWasRemoved(bossinfo.weakspots[i].spot)))
			P_SetTarget(&bossinfo.weakspots[i].spot, NULL);

		if (bossinfo.weakspots[i].spot == NULL)
			continue;

		// Damaged quickly? Make it disappear immediately (making sure to match the flashing).
		if ((bossinfo.weakspots[i].spot->hitlag > 0) && (bossinfo.weakspots[i].time > TICRATE/2))
			bossinfo.weakspots[i].time = (TICRATE/2) & ~(bossinfo.weakspots[i].time & 1);

		// Handle counter.
		if (!bossinfo.weakspots[i].time)
			continue;
		bossinfo.weakspots[i].time--;

		// Get rid of concluded spots.
		if (!bossinfo.weakspots[i].time && !bossinfo.weakspots[i].minimap)
			P_SetTarget(&bossinfo.weakspots[i].spot, NULL);
	}
}

/*--------------------------------------------------
	void K_InitBossHealthBar(const char *enemyname, const char *subtitle, sfxenum_t titlesound, fixed_t pinchmagnitude, UINT8 divisions)

		See header file for description.
--------------------------------------------------*/

void K_InitBossHealthBar(const char *enemyname, const char *subtitle, sfxenum_t titlesound, fixed_t pinchmagnitude, UINT8 divisions)
{
	if (!(gametyperules & GTR_BOSS))
	{
		return;
	}

	bossinfo.valid = true;

	if (!leveltime)
	{
		bossinfo.coolintro = true;
	}

	if (enemyname && enemyname[0])
	{
		Z_Free(bossinfo.enemyname);
		bossinfo.enemyname = Z_StrDup(enemyname);
	}

	if (subtitle && subtitle[0])
	{
		Z_Free(bossinfo.subtitle);
		bossinfo.subtitle = Z_StrDup(subtitle);
	}

	if (titlesound)
	{
		bossinfo.titlesound = titlesound;
	}

	bossinfo.barlen = BOSSHEALTHBARLEN;
	K_UpdateBossHealthBar(FRACUNIT, 0);

	if (pinchmagnitude > FRACUNIT)
		pinchmagnitude = FRACUNIT;
	else if (pinchmagnitude < 0)
		pinchmagnitude = 0;

	bossinfo.healthbarpinch = FixedMul(pinchmagnitude, BOSSHEALTHBARLEN*FRACUNIT)>>FRACBITS;

	// we do this here so we can fudge our working a bit
	if (divisions > 0)
	{
		if (divisions > (BOSSHEALTHBARLEN/2)) // megamanification
		{
			divisions = (BOSSHEALTHBARLEN/2);
		}
		bossinfo.visualdiv = FixedDiv(BOSSHEALTHBARLEN*FRACUNIT, divisions*FRACUNIT);
	}
	else
	{
		bossinfo.visualdiv = 0;
	}
}

/*--------------------------------------------------
	void K_UpdateBossHealthBar(fixed_t magnitude, tic_t jitterlen)

		See header file for description.
--------------------------------------------------*/

void K_UpdateBossHealthBar(fixed_t magnitude, tic_t jitterlen)
{
	if (bossinfo.valid == false)
		return;

	if (magnitude > FRACUNIT)
		magnitude = FRACUNIT;
	else if (magnitude < 0)
		magnitude = 0;

	if (jitterlen > bossinfo.visualbarimpact)
		bossinfo.visualbarimpact = jitterlen;
	bossinfo.healthbar = FixedMul(magnitude, BOSSHEALTHBARLEN);
}

/*--------------------------------------------------
	void K_DeclareWeakspot(mobj_t *weakspot, spottype_t spottype, boolean minimap)

		See header file for description.
--------------------------------------------------*/
void K_DeclareWeakspot(mobj_t *spot, spottype_t spottype, UINT16 color, boolean minimap)
{
	UINT8 i;

	if (bossinfo.valid == false)
		return;

	// First check whether the spot is already in the list and simply redeclaring weakness (for example, a vulnerable moment in the pattern).
	for (i = 0; i < NUMWEAKSPOTS; i++)
		if (bossinfo.weakspots[i].spot == spot)
			break;

	// If it's not redeclaring, check for an empty spot.
	if (i == NUMWEAKSPOTS)
	{
		for (i = 0; i < NUMWEAKSPOTS; i++)
			if (!bossinfo.weakspots[i].spot || P_MobjWasRemoved(bossinfo.weakspots[i].spot))
				break;
	}

	// If you've crammed up the list with minimap entries (the only ones that will persist), give up.
	if (i == NUMWEAKSPOTS)
		return;

	// OK, now we can slot in the weakspot data...
	P_SetTarget(&bossinfo.weakspots[i].spot, spot);
	bossinfo.weakspots[i].type = spottype;
	bossinfo.weakspots[i].time = WEAKSPOTANIMTIME;
	bossinfo.weakspots[i].color = color;
	bossinfo.weakspots[i].minimap = minimap;

	if (spottype && bossinfo.doweakspotsound != SPOT_WEAK)
	{
		bossinfo.doweakspotsound = spottype;
	}
}

/*--------------------------------------------------
	boolean K_CheckBossIntro(void);

		See header file for description.
--------------------------------------------------*/

boolean K_CheckBossIntro(void)
{
	if (bossinfo.valid == false)
		return false;
	return bossinfo.coolintro;
}
