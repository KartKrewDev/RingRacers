#include "../doomdef.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../s_sound.h"

mobj_t *
Obj_SpawnBrolyKi
(		mobj_t * source,
		tic_t duration)
{
	mobj_t *x = P_SpawnMobjFromMobj(
			source, 0, 0, 0, MT_BROLY);

	if (duration == 0)
	{
		return x;
	}

	// Shrink into center of source object.
	x->z = (source->z + source->height / 2);

	x->colorized = true;
	x->color = source->color;
	x->hitlag = 0; // do not copy source hitlag

	P_SetScale(x, 64 * mapobjectscale);
	x->scalespeed = x->scale / duration;

	// The last tic doesn't actually get rendered so in order
	// to show scale = destscale, add one buffer tic.
	x->tics = (duration + 1);
	x->destscale = 1; // 0 also doesn't work

	K_ReduceVFX(x, NULL);

	S_StartSound(x, sfx_cdfm74);

	return x;
}
