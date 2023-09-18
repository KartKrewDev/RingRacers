#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../p_mobj.h"
#include "../p_tick.h"
#include "../typedef.h"

#define shadow_follow(o) ((o)->tracer)

struct Shadow : mobj_t
{
	mobj_t* follow() const { return shadow_follow(this); }
	void follow(mobj_t* n) { P_SetTarget(&shadow_follow(this), n); }

	static Shadow* spawn(mobj_t* from)
	{
		return static_cast<Shadow*>(P_SpawnMobjFromMobj(from, 0, 0, 0, MT_SHADOW))->init(from);
	}

	bool valid() const { return !P_MobjWasRemoved(this) && !P_MobjWasRemoved(follow()); }

	void destroy() { P_RemoveMobj(this); }

	void move()
	{
		whiteshadow = follow()->whiteshadow;
		shadowcolor = follow()->shadowcolor;

		P_MoveOrigin(this, follow()->x, follow()->y, P_GetMobjFeet(follow()));
	}

private:
	Shadow* init(mobj_t* from)
	{
		shadowscale = from->shadowscale;
		from->shadowscale = 0;

		follow(from);

		return this;
	}
};

mobj_t* Obj_SpawnFakeShadow(mobj_t* from)
{
	return Shadow::spawn(from);
}

void Obj_FakeShadowThink(mobj_t* shadow)
{
	auto x = static_cast<Shadow*>(shadow);

	if (!x->valid())
	{
		x->destroy();
		return;
	}

	x->move();
}
