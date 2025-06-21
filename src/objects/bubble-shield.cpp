// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>

#include "objects.hpp"

#include "../m_easing.h"
#include "../m_fixed.h"
#include "../tables.h"
#include "../k_hud.h" // transflag

using namespace srb2::objects;

namespace
{

struct Bubble : Mobj
{
	static constexpr fixed_t kBaseScale = 5*FRACUNIT/4;
	static constexpr fixed_t kMaxScale = 5*FRACUNIT;
	static constexpr fixed_t kScaleRange = kMaxScale - kBaseScale;

	void target() = delete;
	Mobj* follow() const { return Mobj::target(); }
	void follow(Mobj* n) { Mobj::target(n); }

	player_t* player() const { return follow()->player; }

	bool valid() const { return Mobj::valid(follow()) && player(); }
};

struct Visual : Mobj
{
	void target() = delete;
	Bubble* bubble() const { return Mobj::target<Bubble>(); }
	void bubble(Bubble* n) { Mobj::target(n); }

	void extravalue1() = delete;
	Fixed prev_scale() const { return Mobj::extravalue1; }
	void prev_scale(Fixed n) { Mobj::extravalue1 = n; }

	bool valid() const { return Mobj::valid(bubble()) && bubble()->valid(); }

	static void spawn
	(	Bubble* bubble,
		statenum_t state,
		int flicker,
		int offset)
	{
		if (!bubble->valid())
			return;

		Visual* x = Mobj::spawn<Visual>(bubble->pos(), MT_BUBBLESHIELD_VISUAL);
		//x->scale(5 * x->scale() / 4);
		x->state(state);
		x->spriteyoffset(22*FRACUNIT);
		x->bubble(bubble);
		x->linkdraw(bubble->follow(), offset);

		if (flicker)
			x->renderflags |= RF_DONTDRAW;
	}

	bool tick()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		move_origin(bubble()->pos());

		renderflags = ((renderflags ^ RF_DONTDRAW) & RF_DONTDRAW);

		// ATTENTION: this object relies on the original MT_BUBBLESHIELD object for scale
		fixed_t f = Fixed {bubble()->scale() / bubble()->follow()->scale() - Bubble::kBaseScale} / Fixed {Bubble::kScaleRange};

		scale(Easing_Linear(f,
				bubble()->follow()->scale() * Fixed {Bubble::kBaseScale},
				bubble()->follow()->scale() * 4));

		if (sprite != SPR_BUBB &&
			sprite != SPR_BUBC &&
			sprite != SPR_BUBG &&
			bubble()->player()->bubblecool &&
			f == 0) // base size
			renderflags |= RF_DONTDRAW;

		if (scale() > prev_scale())
			spritescale({ 3*FRACUNIT/2, 3*FRACUNIT/4 });
		else if (scale() < prev_scale())
			spritescale({ 3*FRACUNIT/4, 3*FRACUNIT/2 });
		else
		{
			if (f == FRACUNIT) // max size
			{
				if (leveltime & 1)
					spritescale({ 3*FRACUNIT/4, 3*FRACUNIT/2 });
				else
				{
					spritescale({ FRACUNIT, FRACUNIT });
					renderflags |= RF_ADD;
				}
			}
			else
				spritescale({ FRACUNIT, FRACUNIT });
		}

		if (sprite == SPR_BUBG)
		{
			renderflags &= ~(RF_TRANSMASK|RF_DONTDRAW);
			renderflags |= RF_ADD;

			fixed_t transpercent = K_PlayerScamPercentage(bubble()->follow()->player, BUBBLESCAM);
			UINT8 transfactor = (transpercent * NUMTRANSMAPS) / FRACUNIT;

			if (transfactor < 10)
				renderflags |= ((10-transfactor) << RF_TRANSSHIFT);
			// CONS_Printf("tp %d rf %d\n", transpercent, renderflags);
		}

		prev_scale(scale());

		return true;
	}
};

}; // namespace

void Obj_SpawnBubbleShieldVisuals(mobj_t *bubble)
{
	Visual::spawn(static_cast<Bubble*>(bubble), S_BUBA1, 1, 3); //Top shine/outline
	Visual::spawn(static_cast<Bubble*>(bubble), S_BUBB1, 0, 2); //Top wave
	Visual::spawn(static_cast<Bubble*>(bubble), S_BUBG1, 0, 1); //Fog mechanic
	Visual::spawn(static_cast<Bubble*>(bubble), S_BUBC1, 1, -1); //Back Wave
	Visual::spawn(static_cast<Bubble*>(bubble), S_BUBD1, 0, -2); //Bottom Reflection
	Visual::spawn(static_cast<Bubble*>(bubble), S_BUBE1, 1, -3); //Backlit outline
}

boolean Obj_TickBubbleShieldVisual(mobj_t *mobj)
{
	return static_cast<Visual*>(mobj)->tick();
}
