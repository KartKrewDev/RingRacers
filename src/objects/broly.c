#include "../doomdef.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_easing.h"
#include "../p_local.h"
#include "../s_sound.h"

// TODO: generic function
static void P_InstaScale(mobj_t *thing, fixed_t scale)
{
	P_SetScale(thing, scale);
	thing->destscale = scale;
}

/* An object may not be visible on the same tic:
   1) that it spawned
   2) that it cycles to the next state */
#define BUFFER_TICS (2)

#define broly_duration(o) ((o)->extravalue1)
#define broly_maxscale(o) ((o)->extravalue2)

static inline fixed_t
get_unit_linear (const mobj_t *x)
{
	const tic_t t = (x->tics - BUFFER_TICS);

	return t * FRACUNIT / broly_duration(x);
}

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

	broly_maxscale(x) = 64 * mapobjectscale;
	broly_duration(x) = duration;

	x->tics = (duration + BUFFER_TICS);

	K_ReduceVFX(x, NULL);

	S_StartSound(x, sfx_cdfm74);

	return x;
}

void
Obj_BrolyKiThink (mobj_t *x)
{
	const fixed_t
		t = get_unit_linear(x),
		n = Easing_OutSine(t, 0, broly_maxscale(x));

	P_InstaScale(x, n);
}
