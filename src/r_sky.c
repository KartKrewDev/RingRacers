// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_sky.c
/// \brief Sky rendering
///        The SRB2 sky is a texture map like any
///        wall, wrapping around. A 1024 columns equal 360 degrees.
///        The default sky map is 256 columns and repeats 4 times
///        on a 320 screen.

#include "doomdef.h"
#include "doomstat.h"
#include "r_sky.h"
#include "r_local.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_maputl.h" // P_PointOnLineSide

//
// sky mapping
//

/**	\brief Needed to store the number of the dummy sky flat.
	Used for rendering, as well as tracking projectiles etc.
*/
INT32 skyflatnum;

/**	\brief the lump number of the sky texture
*/
INT32 skytexture;

/**	\brief the horizon line of the sky texture
*/
INT32 skytexturemid;

/**	\brief the x offset of the sky texture
*/
INT32 skytextureoffset;

/**	\brief the scale of the sky
*/
fixed_t skyscale[MAXSPLITSCREENPLAYERS];

/** \brief used for keeping track of the current sky
*/
char levelskytexture[9];
char globallevelskytexture[9];

/**	\brief	The R_SetupSkyDraw function

	Called at loadlevel after skytexture is set, or when sky texture changes.

	\warning wallcolfunc should be set at R_ExecuteSetViewSize()
	I don't bother because we don't use low detail anymore

	\return	void
*/
void R_SetupSkyDraw(void)
{
	// the horizon line in the sky texture
	skytexturemid = (textures[skytexture]->height / 2) << FRACBITS;
	skytextureoffset = 0;

	if (textures[skytexture]->type == TEXTURETYPE_SINGLEPATCH)
	{
		// Sal: Allow for sky offsets
		texpatch_t *const tex_patch = &textures[skytexture]->patches[0];
		patch_t *patch = W_CachePatchNumPwad(tex_patch->wad, tex_patch->lump, PU_CACHE);

		skytexturemid += (patch->topoffset << FRACBITS);
		skytextureoffset += (patch->leftoffset << FRACBITS);
	}

	R_SetSkyScale();
}

/**	\brief	The R_SetSkyScale function

	set the correct scale for the sky at setviewsize

	\return void
*/
void R_SetSkyScale(void)
{
	fixed_t difference = vid.fdupx-(vid.dupx<<FRACBITS);
	int i;
	for (i = 0; i <= r_splitscreen; ++i)
	{
		skyscale[i] = FixedDiv(fovtan[i], vid.fdupx+difference);
	}
}
