#include "../v_draw.hpp"

#include "../doomdef.h"
#include "../i_time.h"
#include "../k_hud.h"
#include "../screen.h"

using srb2::Draw;

void K_drawEmeraldWin(void)
{
	constexpr float kScale = 0.25;
	constexpr int kH = 72 * kScale;
	constexpr int kYPad = 12;
	constexpr int kWidth = 34 + 4;

	if (I_GetTime() % 3)
	{
		return;
	}

	Draw row = Draw(BASEVIDWIDTH / 2, BASEVIDHEIGHT / 2).scale(kScale).flags(V_ADD);
	//Draw(0, row.y()).size(BASEVIDWIDTH, 1).fill(35);

	Draw top = row.y(-kYPad);
	Draw bot = row.xy(-kWidth / 2, kH + kYPad);

	auto put = [](Draw& row, int x, int n)
	{
		row.x(x * kWidth).colormap(static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1 + n)).patch("EMRCA0");
	};

	put(top, -1, 3);
	put(top, 0, 0);
	put(top, 1, 4);

	put(bot, -1, 5);
	put(bot, 0, 1);
	put(bot, 1, 2);
	put(bot, 2, 6);
}
