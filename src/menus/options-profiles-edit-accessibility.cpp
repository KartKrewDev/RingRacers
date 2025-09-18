// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-profiles-edit-accessibility.c
/// \brief Profile Accessibibility Options

#include "../v_draw.hpp"

#include "../command.h"
#include "../k_menu.h"
#include "../m_easing.h"
#include "../p_local.h" // cv_tilting

extern "C" consvar_t cv_mindelay, cv_drawinput;

using srb2::Draw;

namespace
{

void draw_routine()
{
	Draw row = Draw(M_EaseWithTransition(Easing_InSine, 5 * 48), currentMenu->y).font(Draw::Font::kMenu);

	M_DrawEditProfileTooltips();

	if (optionsmenu.profile != NULL)
	{
		M_DrawProfileCard(optionsmenu.optx, optionsmenu.opty, false, optionsmenu.profile);
	}

	for (int i = 0; i < currentMenu->numitems; ++i)
	{
		const menuitem_t& it = currentMenu->menuitems[i];

		if (it.status & IT_DISPLAY)
		{
			bool selected = i == itemOn;

			Draw h = row.x(currentMenu->x);

			if (selected)
			{
				M_DrawUnderline(h.x(), BASEVIDWIDTH - 18, h.y());
				M_DrawCursorHand(h.x(), h.y());
			}

			if ((it.status & IT_HEADERTEXT) == IT_HEADERTEXT)
			{
				h
					.x(-4)
					.flags(V_GRAYMAP)
					.text(it.text);
			}
			else
			{
				h
					.flags(selected ? highlightflags : 0)
					.text(it.text);
			}

			if ((it.status & IT_TYPE) == IT_CVAR)
			{
				bool isDefault = CV_IsSetToDefault(it.itemaction.cvar);
				auto val = Draw::TextElement(it.itemaction.cvar->string).font(Draw::Font::kMenu);

				h = row.x(BASEVIDWIDTH - 18);
				h.align(Draw::Align::kRight).flags(isDefault ? highlightflags : warningflags).text(val);

				if (selected)
				{
					Draw ar = h.flags(highlightflags);
					int ofs = skullAnimCounter / 5;
					ar.x(-val.width() - 10 - ofs).text("\x1C");
					ar.x(2 + ofs).text("\x1D");
				}

				if (!isDefault)
				{
					h.x(selected ? 12 : 5).y(-1).flags(warningflags).text(".");
				}
			}
		}

		row = row.y(11);
	}
}

}; // namespace

menuitem_t OPTIONS_ProfileAccessibility[] = {

	{IT_HEADER, "This Profile only:", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Rumble", "For gamepad users - should your device rumble?",
		NULL, srb2::itemaction(&cv_dummyprofilerumble), 0, 0},

	{IT_STRING | IT_CVAR, "Auto Roulette", "Item roulette auto-stops on a random result.",
		NULL, srb2::itemaction(&cv_dummyprofileautoroulette), 0, 0},

	{IT_STRING | IT_CVAR, "Auto Ring", "Auto-use rings to maintain momentum.",
		NULL, srb2::itemaction(&cv_dummyprofileautoring), 0, 0},

	{IT_STRING | IT_CVAR, "Kickstart Accel", "Hold A to auto-accel. Tap it to cancel.",
		NULL, srb2::itemaction(&cv_dummyprofilekickstart), 0, 0},

	{IT_STRING | IT_CVAR, "Lite Steer", "Hold DOWN on d-pad/keyboard for shallow turns.",
		NULL, srb2::itemaction(&cv_dummyprofilelitesteer), 0, 0},

	{IT_STRING | IT_CVAR, "Strict Fastfall", "Fastfall only with the Spindash button.",
		NULL, srb2::itemaction(&cv_dummyprofilestrictfastfall), 0, 0},

	{IT_STRING | IT_CVAR, "Field of View", "Higher FOV lets you see more.",
		NULL, srb2::itemaction(&cv_dummyprofilefov), 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "For all Profiles:", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Minimum Input Delay", "Practice for online play! 0 = instant response.",
		NULL, srb2::itemaction(&cv_mindelay), 0, 0},

	{IT_STRING | IT_CVAR, "Screen Tilting", "View rotation on inclines.",
		NULL, srb2::itemaction(&cv_tilting), 0, 0},

	{IT_STRING | IT_CVAR, "Reduce Effects", "If overwhelmed, hide less-important particle cues.",
		NULL, srb2::itemaction(&cv_reducevfx), 0, 0},

	{IT_STRING | IT_CVAR, "Screenshake", "Adjust shake intensity from hazards and offroad.",
		NULL, srb2::itemaction(&cv_screenshake), 0, 0},

	{IT_STRING | IT_CVAR, "Input Display", "Show virtual controller on the HUD.",
		NULL, srb2::itemaction(&cv_drawinput), 0, 0},
};

menu_t OPTIONS_ProfileAccessibilityDef = {
	sizeof (OPTIONS_ProfileAccessibility) / sizeof (menuitem_t),
	&OPTIONS_EditProfileDef,
	0,
	OPTIONS_ProfileAccessibility,
	145, 31,
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
