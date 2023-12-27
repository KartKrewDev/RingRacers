/// \file  menus/options-profiles-edit-accessibility.c
/// \brief Profile Accessibibility Options

#include "../v_draw.hpp"

#include "../command.h"
#include "../k_menu.h"

extern "C" consvar_t cv_mindelay;

using srb2::Draw;

namespace
{

void draw_routine()
{
	Draw row = Draw(0, currentMenu->y).font(Draw::Font::kConsole);

	M_DrawEditProfileTooltips();

	for (int i = 0; i < currentMenu->numitems; ++i)
	{
		const menuitem_t& it = currentMenu->menuitems[i];

		bool selected = i == itemOn;
		Draw h = row.flags(selected ? highlightflags : 0);

		if ((it.status & IT_TYPE) == IT_CVAR)
		{
			auto val = Draw::TextElement(it.itemaction.cvar->string).font(Draw::Font::kConsole);

			h.x(currentMenu->x).text(it.text);

			h = h.x(BASEVIDWIDTH - 16);
			h.align(Draw::Align::kRight).text(val);

			if (selected)
			{
				int ofs = skullAnimCounter / 5;
				h.x(-val.width() - 10 - ofs).text("\x1C");
				h.x(2 + ofs).text("\x1D");
			}
		}

		row = row.y(12);
	}

	// Finally, draw the card ontop
	if (optionsmenu.profile != NULL)
	{
		M_DrawProfileCard(optionsmenu.optx, optionsmenu.opty, false, optionsmenu.profile);
	}
}

}; // namespace

menuitem_t OPTIONS_ProfileAccessibility[] = {

	{IT_STRING | IT_CVAR, "Rumble", "For gamepad users - should your device rumble?",
		NULL, {.cvar = &cv_dummyprofilerumble}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Auto Roulette", "Item roulette auto-stops on a random result.",
		NULL, {.cvar = &cv_dummyprofileautoroulette}, 0, 0},

	{IT_STRING | IT_CVAR, "Kickstart Accel", "Hold A to auto-accel. Tap it to cancel.",
		NULL, {.cvar = &cv_dummyprofilekickstart}, 0, 0},

	{IT_STRING | IT_CVAR, "Lite Steer", "Hold DOWN on d-pad/keyboard for shallow turns.",
		NULL, {.cvar = &cv_dummyprofilelitesteer}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Minimum Input Delay", "Practice for online play! 0 = instant response.",
		NULL, {.cvar = &cv_mindelay}, 0, 0},
};

menu_t OPTIONS_ProfileAccessibilityDef = {
	sizeof (OPTIONS_ProfileAccessibility) / sizeof (menuitem_t),
	&OPTIONS_EditProfileDef,
	0,
	OPTIONS_ProfileAccessibility,
	145, 72,
	SKINCOLOR_ULTRAMARINE, 0,
	MBF_DRAWBGWHILEPLAYING,
	"FILE",
	2, 5,
	draw_routine,
	M_DrawOptionsCogs,
	M_OptionsTick, // animate cogs
	NULL,
	NULL,
	NULL,
};
