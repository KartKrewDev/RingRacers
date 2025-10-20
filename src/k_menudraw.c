// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_menudraw.c
/// \brief SRB2Kart's menu drawer functions

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "k_menu.h"

#include "doomdef.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "console.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"
#include "m_argv.h"

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"

// Addfile
#include "filesrch.h"

#include "v_video.h"
#include "i_video.h"
#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"
#include "p_setup.h"
#include "f_finale.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "d_net.h"
#include "mserv.h"
#include "m_misc.h"
#include "m_anigif.h"
#include "byteptr.h"
#include "st_stuff.h"
#include "i_sound.h"
#include "k_kart.h"
#include "k_hud.h"
#include "k_follower.h"
#include "d_player.h" // KITEM_ constants
#include "doomstat.h" // MAXSPLITSCREENPLAYERS
#include "k_grandprix.h" // K_CanChangeRules
#include "k_rank.h" // K_GetGradeColor
#include "k_zvote.h" // K_GetMidVoteLabel
#include "k_boss.h"

#include "y_inter.h" // Y_RoundQueueDrawer

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// Sound Test
#include "music.h"

// And just some randomness for the exits.
#include "m_random.h"

#include "i_time.h"
#include "m_easing.h"
#include "sanitize.h"

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif

fixed_t M_TimeFrac(tic_t tics, tic_t duration)
{
	return tics < duration ? (tics * FRACUNIT + rendertimefrac_unpaused) / duration : FRACUNIT;
}

fixed_t M_ReverseTimeFrac(tic_t tics, tic_t duration)
{
	return FRACUNIT - M_TimeFrac(duration - tics, duration);
}

fixed_t M_DueFrac(tic_t start, tic_t duration)
{
	tic_t t = I_GetTime();
	tic_t n = t - start;
	return M_TimeFrac(min(n, duration), duration);
}

#define SKULLXOFF -32
#define LINEHEIGHT 13
#define STRINGHEIGHT 9
#define FONTBHEIGHT 20
#define SMALLLINEHEIGHT 9
#define SLIDER_RANGE 10
#define SLIDER_WIDTH (8*SLIDER_RANGE+6)
#define SERVERS_PER_PAGE 11


// horizontally centered text
static void M_CentreText(INT32 xoffs, INT32 y, const char *string)
{
	INT32 x;
	//added : 02-02-98 : centre on 320, because V_DrawString centers on vid.width...
	x = ((BASEVIDWIDTH - V_MenuStringWidth(string, 0))>>1) + xoffs;
	V_DrawMenuString(x,y,0,string);
}

static INT32 M_SliderX(INT32 range)
{
	if (range < 0)
		range = 0;
	if (range > 100)
		range = 100;

	return -4 + (((SLIDER_RANGE)*8 + 4)*range)/100;
}

//  A smaller 'Thermo', with range given as percents (0-100)
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv, boolean ontop)
{
	x = BASEVIDWIDTH - x - SLIDER_WIDTH;
	V_DrawFill(x - 5, y + 3, SLIDER_WIDTH + 3, 5, 31);
	V_DrawFill(x - 4, y + 4, SLIDER_WIDTH, 2, orangemap[0]);

	if (ontop)
	{
		V_DrawMenuString(x - 16 - (skullAnimCounter/5), y,
			highlightflags, "\x1C"); // left arrow
		V_DrawMenuString(x+(SLIDER_RANGE*8) + 8 + (skullAnimCounter/5), y,
			highlightflags, "\x1D"); // right arrow
	}

	INT32 range = cv->PossibleValue[1].value - cv->PossibleValue[0].value;
	INT32 val = atoi(cv->defaultvalue);

	val = (val - cv->PossibleValue[0].value) * 100 / range;
	// draw the default tick
	V_DrawFill(x + M_SliderX(val), y + 2, 3, 4, 31);

	val = (cv->value - cv->PossibleValue[0].value) * 100 / range;
	INT32 px = x + M_SliderX(val);

	// draw the slider cursor
	V_DrawFill(px - 1, y - 1, 5, 11, 31);
	V_DrawFill(px, y, 2, 8, aquamap[0]);
}

void M_DrawCursorHand(INT32 x, INT32 y)
{
	V_DrawScaledPatch(x - 24 - (I_GetTime() % 16 < 8), y, 0, W_CachePatchName("M_CURSOR", PU_CACHE));
}

void M_DrawUnderline(INT32 left, INT32 right, INT32 y)
{
	if (menutransition.tics == menutransition.dest)
		V_DrawFill(left - 1, y + 5, (right - left) + 11, 2, 31);
}

static patch_t *addonsp[NUM_EXT+5];

static INT16 bgMapID = NEXTMAP_INVALID;
void M_PickMenuBGMap(void)
{
	UINT16 *allowedMaps;
	size_t allowedMapsCount = 0;
	UINT16 ret = 0;
	INT32 i;

	allowedMaps = Z_Malloc(nummapheaders * sizeof(UINT16), PU_STATIC, NULL);

	for (i = 0; i < nummapheaders; i++)
	{
		if (mapheaderinfo[i] == NULL || mapheaderinfo[i]->lumpnum == LUMPERROR)
		{
			// Doesn't exist?
			continue;
		}

		if (mapheaderinfo[i]->thumbnailPic == NULL)
		{
			// No image...
			continue;
		}

		if ((mapheaderinfo[i]->typeoflevel & (TOL_SPECIAL|TOL_VERSUS)) != 0)
		{
			// Don't spoil Special or Versus.
			continue;
		}

		if ((mapheaderinfo[i]->menuflags & LF2_HIDEINMENU) == LF2_HIDEINMENU)
		{
			// "Hide in menu"... geddit?!
			continue;
		}

		if (!(mapheaderinfo[i]->menuflags & LF2_NOVISITNEEDED)
		&& !(mapheaderinfo[i]->records.mapvisited & MV_VISITED)
		&& !(
			mapheaderinfo[i]->cup
			&& mapheaderinfo[i]->cup->cachedlevels[0] == i
		))
		{
			// Not visited OR head of cup
			continue;
		}

		if (M_MapLocked(i + 1) == true)
		{
			// We haven't earned this one.
			continue;
		}

		// Got past the gauntlet, so we can allow this one.
		allowedMaps[ allowedMapsCount++ ] = i;
	}

	if (allowedMapsCount > 0)
	{
		ret = allowedMaps[ M_RandomKey(allowedMapsCount) ];
	}
	Z_Free(allowedMaps);

	bgMapID = ret;
}

static fixed_t bgText1Scroll = 0;
static fixed_t bgText2Scroll = 0;
static fixed_t bgImageScroll = 0;
static char bgImageName[9];

#define MENUBG_TEXTSCROLL 6
#define MENUBG_IMAGESCROLL 36

void M_UpdateMenuBGImage(boolean forceReset)
{
	char oldName[9];

	memcpy(oldName, bgImageName, 9);

	if (currentMenu->menuitems[itemOn].patch)
	{
		sprintf(bgImageName, "%s", currentMenu->menuitems[itemOn].patch);
	}
	else
	{
		sprintf(bgImageName, "MENUI000");
	}

	if (forceReset == false && strcmp(bgImageName, oldName))
	{
		bgImageScroll = (3 * BASEVIDWIDTH) * (FRACUNIT / 4);
	}

	if (forceReset == true)
	{
		M_PickMenuBGMap();
	}
}

void M_DrawMenuBackground(void)
{
	patch_t *text1 = W_CachePatchName("MENUBGT1", PU_CACHE);
	patch_t *text2 = W_CachePatchName("MENUBGT2", PU_CACHE);

	fixed_t text1loop = SHORT(text1->height)*FRACUNIT;
	fixed_t text2loop = SHORT(text2->width)*FRACUNIT;

	if (bgMapID >= nummapheaders)
	{
		M_PickMenuBGMap();
	}

	patch_t *bgMapImage = mapheaderinfo[bgMapID]->thumbnailPic;
	if (bgMapImage == NULL)
	{
		bgMapImage = W_CachePatchName("MENUBG4", PU_CACHE);
	}

	V_DrawFixedPatch(0, 0, FRACUNIT, 0, bgMapImage, R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_SLATE, GTC_MENUCACHE));
	V_DrawFixedPatch(0, 0, FRACUNIT, V_ADD, W_CachePatchName("MENUCUTD", PU_CACHE), NULL);
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUCUT", PU_CACHE), NULL);

	V_DrawFixedPatch(-bgImageScroll, 0, FRACUNIT, 0, W_CachePatchName("MENUBG1", PU_CACHE), NULL);
	V_DrawFixedPatch(-bgImageScroll, 0, FRACUNIT, 0, W_CachePatchName(bgImageName, PU_CACHE), NULL);

	V_DrawFixedPatch(0, (BASEVIDHEIGHT + 16) * FRACUNIT, FRACUNIT, V_SUBTRACT, W_CachePatchName("MENUBG2", PU_CACHE), NULL);

	V_DrawFixedPatch(8 * FRACUNIT, -bgText1Scroll,
		FRACUNIT, V_SUBTRACT, text1, NULL);
	V_DrawFixedPatch(8 * FRACUNIT, -bgText1Scroll + text1loop,
		FRACUNIT, V_SUBTRACT, text1, NULL);

	V_DrawFixedPatch(-bgText2Scroll, (BASEVIDHEIGHT-8) * FRACUNIT,
		FRACUNIT, V_ADD, text2, NULL);
	V_DrawFixedPatch(-bgText2Scroll + text2loop, (BASEVIDHEIGHT-8) * FRACUNIT,
		FRACUNIT, V_ADD, text2, NULL);

	if (renderdeltatics > 2*FRACUNIT)
		return; // wipe hitch...

	bgText1Scroll += (MENUBG_TEXTSCROLL*renderdeltatics);
	while (bgText1Scroll > text1loop)
		bgText1Scroll -= text1loop;

	bgText2Scroll += (MENUBG_TEXTSCROLL*renderdeltatics);
	while (bgText2Scroll > text2loop)
		bgText2Scroll -= text2loop;

	if (bgImageScroll > 0)
	{
		bgImageScroll -= (MENUBG_IMAGESCROLL*renderdeltatics);
		if (bgImageScroll < 0)
		{
			bgImageScroll = 0;
		}
	}
}

void M_DrawExtrasBack(void)
{
	patch_t *bg = W_CachePatchName("M_XTRABG", PU_CACHE);
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, bg, NULL);
}

UINT16 M_GetCvPlayerColor(UINT8 pnum)
{
	if (pnum >= MAXSPLITSCREENPLAYERS)
		return SKINCOLOR_NONE;

	UINT16 color = cv_playercolor[pnum].value;
	if (color != SKINCOLOR_NONE)
		return color;

	INT32 skin = R_SkinAvailableEx(cv_skin[pnum].string, false);
	if (skin == -1)
		return SKINCOLOR_NONE;

	return skins[skin]->prefcolor;
}

static void M_DrawMenuParty(void)
{
	const INT32 PLATTER_WIDTH = 19;
	const INT32 PLATTER_STAGGER = 6;
	const INT32 PLATTER_OFFSET = (PLATTER_WIDTH - PLATTER_STAGGER);

	patch_t *small = W_CachePatchName("MENUPLRA", PU_CACHE);
	patch_t *large = W_CachePatchName("MENUPLRB", PU_CACHE);

	INT32 x, y;
	INT32 skin;
	UINT16 color;
	UINT8 *colormap;

	if (setup_numplayers == 0 || currentMenu == &PLAY_CharSelectDef || currentMenu == &OPTIONS_GameplayItemsDef || currentMenu == &MISC_ChallengesDef)
	{
		return;
	}

	x = 2;
	y = BASEVIDHEIGHT - small->height - 2;

	// Despite the work put into it, can't use M_GetCvPlayerColor directly - we need to reference skin always.
	#define grab_skin_and_colormap(pnum) \
	{ \
		skin = R_SkinAvailableEx(cv_skin[pnum].string, false); \
		color = cv_playercolor[pnum].value; \
		if (skin == -1) \
			skin = 0; \
		if (color == SKINCOLOR_NONE) \
			color = skins[skin]->prefcolor; \
		colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE); \
	}

	switch (setup_numplayers)
	{
		case 1:
		{
			x -= 8;
			V_DrawScaledPatch(x, y, 0, small);

			grab_skin_and_colormap(0);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		case 2:
		{
			x -= 8;
			V_DrawScaledPatch(x, y, 0, small);
			V_DrawScaledPatch(x + PLATTER_OFFSET, y - PLATTER_STAGGER, 0, small);

			grab_skin_and_colormap(1);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 22, y - PLATTER_STAGGER + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			grab_skin_and_colormap(0);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		case 3:
		{
			V_DrawScaledPatch(x, y, 0, large);
			V_DrawScaledPatch(x + PLATTER_OFFSET, y - PLATTER_STAGGER, 0, small);

			grab_skin_and_colormap(1);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 22, y - PLATTER_STAGGER + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			grab_skin_and_colormap(0);

			V_DrawMappedPatch(x + 12, y - 2, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			grab_skin_and_colormap(2);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		case 4:
		{
			V_DrawScaledPatch(x, y, 0, large);
			V_DrawScaledPatch(x + PLATTER_OFFSET, y - PLATTER_STAGGER, 0, large);

			grab_skin_and_colormap(1);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 12, y - PLATTER_STAGGER - 2, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			grab_skin_and_colormap(0);

			V_DrawMappedPatch(x + 12, y - 2, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			grab_skin_and_colormap(3);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 22, y - PLATTER_STAGGER + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			grab_skin_and_colormap(2);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		default:
		{
			return;
		}
	}

	#undef grab_skin_and_color

	x += PLATTER_WIDTH;
	y += small->height;
	V_DrawScaledPatch(x + 16, y - 12, 0, W_CachePatchName(va("OPPRNK0%d", setup_numplayers % 10), PU_CACHE));
}

void M_DrawMenuForeground(void)
{
	if (gamestate == GS_MENU)
	{
		M_DrawMenuParty();
	}

	// draw non-green resolution border
	if ((!menuactive || currentMenu != &PAUSE_PlaybackMenuDef) && // this obscures replay menu and I want to put in minimal effort to fix that
		((vid.width % BASEVIDWIDTH != 0) || (vid.height % BASEVIDHEIGHT != 0)))
	{
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("WEIRDRES", PU_CACHE), NULL);
	}
}

//
// M_DrawMenuTooltips
//
// Draw a banner across the top of the screen, with a description of the current option displayed
//
static void M_DrawMenuTooltips(void)
{
	if (currentMenu->menuitems[itemOn].tooltip != NULL)
	{
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 12, 0, currentMenu->menuitems[itemOn].tooltip);
	}
}

static const char *M_MenuTypingCroppedString(void)
{
	static char buf[36];
	const char *p = menutyping.cache;
	size_t n = strlen(p);
	if (n > sizeof buf)
	{
		p += n - sizeof buf;
		n = sizeof buf;
	}
	memcpy(buf, p, n);
	buf[n] = '\0';
	return buf;
}

// Draws the typing submenu
static void M_DrawMenuTyping(void)
{
	const UINT8 pid = 0;

	INT32 i, j;

	INT32 x, y;

	char buf[8];	// We write there to use drawstring for convenience.

	V_DrawFadeScreen(31, (menutyping.menutypingfade+1)/2);

	// Draw the string we're editing at the top.

	const INT32 boxwidth = (8*(MAXSTRINGLENGTH + 1)) + 7;
	x = (BASEVIDWIDTH - boxwidth)/2;
	y = 80;
	if (menutyping.menutypingfade < 9)
		y += floor(pow(2, (double)(9 - menutyping.menutypingfade)));
	else
		y += (9-menutyping.menutypingfade);

	if (currentMenu->menuitems[itemOn].text)
	{
		V_DrawThinString(x + 5, y - 2, highlightflags, currentMenu->menuitems[itemOn].text);
	}

	M_DrawMenuTooltips();

	//M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
	V_DrawFill(x + 5, y + 4 + 5, boxwidth - 8, 8+6, 159);

	V_DrawFill(x + 4, y + 4 + 4, boxwidth - 6, 1, 121);
	V_DrawFill(x + 4, y + 4 + 5 + 8 + 6, boxwidth - 6, 1, 121);

	V_DrawFill(x + 4, y + 4 + 5, 1, 8+6, 121);
	V_DrawFill(x + 5 + boxwidth - 8, y + 4 + 5, 1, 8+6, 121);

	INT32 textwidth = M_DrawCaretString(x + 8, y + 12, M_MenuTypingCroppedString(), true);
	if (skullAnimCounter < 4
		&& menutyping.menutypingclose == false
		&& menutyping.menutypingfade == (menutyping.keyboardtyping ? 9 : 18))
	{
		V_DrawCharacter(x + 8 + textwidth, y + 12 + 1, '_', false);
	}

	const INT32 buttonwidth = ((boxwidth + 1)/NUMVIRTUALKEYSINROW);
#define BUTTONHEIGHT (11)

	// Now the keyboard itself
	x += 5;
	INT32 returnx = x;

	if (menutyping.menutypingfade > 9)
	{
		y += 26;

		if (menutyping.menutypingfade < 18)
		{
			y += floor(pow(2, (double)(18 - menutyping.menutypingfade))); // double yoffs for animation
		}

		INT32 tempkeyboardx = menutyping.keyboardx;

		while (virtualKeyboard[menutyping.keyboardy][tempkeyboardx] == 1
		&& tempkeyboardx > 0)
			tempkeyboardx--;

		for (i = 0; i < 5; i++)
		{
			j = 0;
			while (j < NUMVIRTUALKEYSINROW)
			{
				INT32 mflag = 0;
				INT16 c = virtualKeyboard[i][j];

				INT32 buttonspacing = 1;

				UINT8 col = 27;

				INT32 arrowoffset = 0;

				while (j + buttonspacing < NUMVIRTUALKEYSINROW
				&& virtualKeyboard[i][j + buttonspacing] == 1)
				{
					buttonspacing++;
				}

				if (menutyping.keyboardshift ^ menutyping.keyboardcapslock)
					c = shift_virtualKeyboard[i][j];

				if (i < 4 && j < NUMVIRTUALKEYSINROW-2)
				{
					col = 25;
				}

				boolean canmodifycol = (menutyping.menutypingfade == 18);

				if (c == KEY_BACKSPACE)
				{
					arrowoffset = 1;
					buf[0] = '\x1C'; // left arrow
					buf[1] = '\0';

					if (canmodifycol && M_MenuBackHeld(pid))
					{
						col -= 4;
						canmodifycol = false;
					}
				}
				else if (c == KEY_RSHIFT)
				{
					arrowoffset = 2;
					buf[0] = '\x1A'; // up arrow
					buf[1] = '\0';

					if (menutyping.keyboardcapslock || menutyping.keyboardshift)
					{
						col = 22;
					}

					if (canmodifycol && M_MenuExtraHeld(pid))
					{
						col -= 4;
						canmodifycol = false;
					}
				}
				else if (c == KEY_ENTER)
				{
					strcpy(buf, "OK");

					if (menutyping.menutypingclose)
					{
						col -= 4;
						canmodifycol = false;
					}
				}
				else if (c == KEY_SPACE)
				{
					strcpy(buf, "Space");
				}
				else
				{
					buf[0] = c;
					buf[1] = '\0';
				}

				INT32 width = (buttonwidth * buttonspacing) - 1;

				// highlight:
				/*if (menutyping.keyboardtyping)
				{
					mflag |= V_TRANSLUCENT;	// grey it out if we can't use it.
				}
				else*/
				{
					if (tempkeyboardx == j && menutyping.keyboardy == i)
					{
						if (canmodifycol && M_MenuConfirmHeld(pid))
						{
							col -= 4;
							canmodifycol = false;
						}

						V_DrawFill(x + 1, y + 1, width - 2, BUTTONHEIGHT - 2, col - 3);

						V_DrawFill(x, y,                    width, 1, 121);
						V_DrawFill(x, y + BUTTONHEIGHT - 1, width, 1, 121);

						V_DrawFill(x,             y + 1, 1, BUTTONHEIGHT - 2, 121);
						V_DrawFill(x + width - 1, y + 1, 1, BUTTONHEIGHT - 2, 121);

						mflag |= highlightflags;
					}
					else
					{
						V_DrawFill(x, y, width, BUTTONHEIGHT, col);
					}
				}

				if (arrowoffset != 0)
				{
					if (c == KEY_RSHIFT)
					{
						V_DrawFill(x + width - 5, y + 1, 4, 4, 31);

						if (menutyping.keyboardcapslock)
						{
							V_DrawFill(x + width - 4, y + 2, 2, 2, 121);
						}
					}

					V_DrawCenteredString(x + (width/2), y + 1 + arrowoffset, mflag, buf);
				}
				else
				{
					V_DrawCenteredThinString(x + (width/2), y + 1, mflag, buf);
				}

				x += width + 1;
				j += buttonspacing;
			}
			x = returnx;
			y += BUTTONHEIGHT + 1;
		}
	}

#undef BUTTONHEIGHT

	y = 187;

	if (menutyping.menutypingfade < 9)
	{
		y += 3 * (9 - menutyping.menutypingfade);
	}

	// Some contextual stuff
	if (menutyping.keyboardtyping)
	{
		V_DrawThinString(returnx, y, V_GRAYMAP,
			"Type using your keyboard. Press Enter to confirm & exit."
			//"\nPress any button on your controller to use the Virtual Keyboard."
		);
	}
	else
	{
		V_DrawThinString(returnx, y, V_GRAYMAP,
			"Type using the Virtual Keyboard. Use the \'OK\' button to confirm & exit."
			//"\nPress any keyboard key to type normally."
		);
	}

}

static void M_DrawPauseRoundQueue(INT16 offset, boolean canqueue)
{
	y_data_t standings;
	memset(&standings, 0, sizeof (standings));

	if (gamestate == GS_MENU)
	{
		standings.mainplayer = MAXPLAYERS;
	}
	else
	{
		standings.mainplayer = (demo.playback ? displayplayers[0] : consoleplayer);
	}

	// See also G_GetNextMap, Y_CalculateMatchData
	if (
		canqueue == false
		&& grandprixinfo.gp == true
		&& netgame == false // TODO netgame Special Mode support
		&& grandprixinfo.gamespeed >= KARTSPEED_NORMAL
		&& roundqueue.size > 1
		&& roundqueue.entries[roundqueue.size - 1].rankrestricted == true
		&& (
			gamedata->everseenspecial == true
			|| roundqueue.position == roundqueue.size
		)
	)
	{
		// Additional cases in which it should always be shown.
		standings.showrank = true;
	}

	Y_RoundQueueDrawer(&standings, offset, false, false, canqueue);
}

// Draw the message popup submenu
void M_DrawMenuMessage(void)
{
	if (!menumessage.active)
		return;

	INT32 x = (BASEVIDWIDTH - menumessage.x)/2;
	INT32 y = (BASEVIDHEIGHT - menumessage.y)/2 + floor(pow(2, (double)(9 - menumessage.fadetimer)));
	size_t i, start = 0;
	char string[MAXMENUMESSAGE];
	const char *msg = menumessage.message;

	V_DrawFadeScreen(31, menumessage.fadetimer);

	V_DrawFill(0, y, BASEVIDWIDTH, menumessage.y, 159);

	if (menumessage.header != NULL)
	{
		V_DrawThinString(x, y - 10, highlightflags, menumessage.header);
	}

	if (menumessage.defaultstr)
	{
		INT32 workx = x + menumessage.x;
		INT32 worky = y + menumessage.y;

		boolean push;

		if (menumessage.closing)
			push = (menumessage.answer != MA_YES);
		else
		{
			const UINT8 anim_duration = 16;
			push = ((menumessage.timer % (anim_duration * 2)) < anim_duration);
		}

		workx -= V_ThinStringWidth(menumessage.defaultstr, 0);
		V_DrawThinString(
			workx, worky + 1,
			((push && (menumessage.closing & MENUMESSAGECLOSE))
				? highlightflags : 0),
			menumessage.defaultstr
		);

		workx -= 2;

		workx -= K_DrawGameControl(
			workx+2, worky+2,
			0, "<b_animated> <x_animated> ",
			2, MENU_FONT, 0
		);

		if (menumessage.confirmstr)
		{
			workx -= 12;

			if (menumessage.closing)
				push = !push;

			workx -= V_ThinStringWidth(menumessage.confirmstr, 0);
			V_DrawThinString(
				workx, worky + 1,
				((push && (menumessage.closing & MENUMESSAGECLOSE))
					? highlightflags : 0),
				menumessage.confirmstr
			);

			workx -= 2;
		}

		workx -= K_DrawGameControl(
			workx+2, worky+2,
			0, "<a_animated> ",
			2, MENU_FONT, 0
		);
	}

	x -= 4;
	y += 4;

	while (*(msg+start))
	{
		size_t len = strlen(msg+start);

		for (i = 0; i < len; i++)
		{
			if (*(msg+start+i) == '\n')
			{
				memset(string, 0, MAXMENUMESSAGE);
				if (i >= MAXMENUMESSAGE)
				{
					CONS_Printf("M_DrawMenuMessage: too long segment in %s\n", msg);
					return;
				}
				else
				{
					strncpy(string,msg+start, i);
					string[i] = '\0';
					start += i;
					i = (size_t)-1; //added : 07-02-98 : damned!
					start++;
				}
				break;
			}
		}

		if (i == strlen(msg+start))
		{
			if (i >= MAXMENUMESSAGE)
			{
				CONS_Printf("M_DrawMenuMessage: too long segment in %s\n", msg);
				return;
			}
			else
			{
				strcpy(string, msg + start);
				start += i;
			}
		}

		V_DrawString((BASEVIDWIDTH - V_StringWidth(string, 0))/2, y, 0, string);
		y += 8;
	}
}

// PAUSE
static void M_DrawPausedText(INT32 x)
{
	patch_t *pausebg = W_CachePatchName("M_STRIPU", PU_CACHE);
	patch_t *pausetext = W_CachePatchName("M_PAUSET", PU_CACHE);

	INT32 snapFlags = menuactive ? 0 : (V_SNAPTOLEFT|V_SNAPTOTOP);

	V_DrawFixedPatch(x, -5*FRACUNIT, FRACUNIT, snapFlags|V_ADD, pausebg,   NULL);
	V_DrawFixedPatch(x, -5*FRACUNIT, FRACUNIT, snapFlags,       pausetext, NULL);
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//

void M_Drawer(void)
{
	if (menuwipe)
		F_WipeStartScreen();

	// background layer
	if (menuactive)
	{
		boolean drawbgroutine = false;
		boolean trulystarted = M_GameTrulyStarted();

		if (gamestate == GS_MENU && trulystarted)
		{
			if (currentMenu->bgroutine)
				drawbgroutine = true;
			else
				M_DrawMenuBackground();
		}
		else
		{
			if (currentMenu->bgroutine
			&& (currentMenu->behaviourflags & MBF_DRAWBGWHILEPLAYING))
				drawbgroutine = true;

			if (!Playing() && !trulystarted)
			{
				M_DrawGonerBack();
			}
			else if (!WipeInAction && currentMenu != &PAUSE_PlaybackMenuDef && currentMenu != &OPTIONS_VideoColorProfileDef)
			{
				V_DrawFadeScreen(122, 3);
			}
		}

		if (drawbgroutine)
			currentMenu->bgroutine();
	}

	// draw pause pic
	if (paused && !demo.playback && (menuactive || R_ShowHUD()))
	{
		M_DrawPausedText(0);
	}

	// foreground layer
	if (menuactive)
	{
		if (currentMenu->drawroutine)
			currentMenu->drawroutine(); // call current menu Draw routine

		if (
			(
				currentMenu == &PLAY_LevelSelectDef
				|| currentMenu == &PLAY_CupSelectDef
			) && levellist.canqueue
		)
		{
			M_DrawPauseRoundQueue(0, true);
		}

		M_DrawMenuForeground();

		// Draw version down in corner
		// ... but only in the MAIN MENU.  I'm a picky bastard.
		if (currentMenu == &MainDef)
		{
			F_VersionDrawer();
		}

		// Draw typing overlay when needed, above all other menu elements.
		if (menutyping.active)
			M_DrawMenuTyping();

		// Draw message overlay when needed
		M_DrawMenuMessage();
	}

	if (menuwipe)
	{
		F_WipeEndScreen();
		F_RunWipe(wipe_menu_final, wipedefs[wipe_menu_final], false, "FADEMAP0", true, false);
		menuwipe = false;
	}

	if (netgame && Playing())
	{
		boolean mainpause_open = menuactive && currentMenu == &PAUSE_MainDef;

		ST_DrawServerSplash(!mainpause_open);
	}

	// focus lost notification goes on top of everything, even the former everything
	if (window_notinfocus && cv_showfocuslost.value)
	{
		M_DrawTextBox((BASEVIDWIDTH/2) - (60), (BASEVIDHEIGHT/2) - (16), 13, 2);
		V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2) - (4), highlightflags|V_FORCEUPPERCASE, "Focus Lost");
	}
}

// ==========================================================================
// GENERIC MENUS
// ==========================================================================

// Converts a string into question marks.
// Used for the secrets menu, to hide yet-to-be-unlocked stuff.
static const char *M_CreateSecretMenuOption(const char *str)
{
#if 1
	(void)str;
	return "???";
#else
	static char qbuf[64];
	int i;

	for (i = 0; i < 63; ++i)
	{
		if (!str[i])
		{
			qbuf[i] = '\0';
			return qbuf;
		}
		else if (str[i] != ' ')
			qbuf[i] = '?';
		else
			qbuf[i] = ' ';
	}

	qbuf[63] = '\0';
	return qbuf;
#endif
}

//
// M_DrawGenericMenu
//
// Default, generic text-based list menu, used for Options
//
void M_DrawGenericMenu(void)
{
	INT32 x = currentMenu->x, y = currentMenu->y, w, i, cursory = 0;

	M_DrawMenuTooltips();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					}
				}
				/* FALLTHRU */
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y = currentMenu->y + currentMenu->menuitems[i].mvar1;//+= LINEHEIGHT;
				break;
#if 0
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, currentMenu->menuitems[i].itemaction.cvar);
				y += LINEHEIGHT;
				break;
#endif
			case IT_STRING:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawMenuString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawMenuString(x, y, highlightflags, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
#if 0
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
#endif
							case IT_CV_STRING:
								{
									M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);

									INT32 xoffs = 0;
									if (itemOn == i)
									{
										xoffs += 8;
										V_DrawMenuString(x + (skullAnimCounter/5) + 6, y + 12, highlightflags, "\x1D");
									}

									V_DrawString(x + xoffs + 8, y + 12, 0, cv->string);

									y += 16;
								}
								break;
							default:
								w = V_MenuStringWidth(cv->string, 0);
								V_DrawMenuString(BASEVIDWIDTH - x - w, y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? warningflags : highlightflags), cv->string);
								if (i == itemOn)
								{
									V_DrawMenuString(BASEVIDWIDTH - x - 10 - w - (skullAnimCounter/5), y,
											highlightflags, "\x1C"); // left arrow
									V_DrawMenuString(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
											highlightflags, "\x1D"); // right arrow
								}
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawMenuString(x, y, 0, currentMenu->menuitems[i].text);
				/* FALLTHRU */
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
			case IT_TRANSTEXT:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;
				/* FALLTHRU */
			case IT_TRANSTEXT2:
				V_DrawMenuString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawMenuString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_HEADERTEXT: // draws 16 pixels to the left, in yellow text
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawMenuString(x-16, y, highlightflags, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
		}
	}

	if ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == 0)
		cursory = 300;	// Put the cursor off screen if we can't even display that option and we're on it, it makes no sense otherwise...

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(currentMenu->x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawMenuString(currentMenu->x, cursory, highlightflags, currentMenu->menuitems[itemOn].text);
	}
}

#define GM_STARTX 128
#define GM_STARTY 80
#define GM_XOFFSET 17
#define GM_YOFFSET 34
#define GM_FLIPTIME 5

static tic_t gm_flipStart;

static INT32 M_DrawRejoinIP(INT32 x, INT32 y, INT32 tx)
{
	extern consvar_t cv_dummyipselect;
	char (*ip)[MAX_LOGIP] = joinedIPlist[cv_dummyipselect.value];
	if (!*ip[0])
		return 0;

	INT16 shift = 20;
	x -= shift;

	INT16 j = 0;
	for (j=0; j <= (GM_YOFFSET + 10) / 2; j++)
	{
		// Draw rectangles that look like the current selected item starting from the top of the actual selection graphic and going up to where it's supposed to go.
		// With colour 169 (that's the index of the shade of black the plague colourization gives us. ...No I don't like using a magic number either.
		V_DrawFill((x-1) + j, y + (2*j), 226, 2, 169);
	}

	x += GM_XOFFSET + 14;
	y += GM_YOFFSET;

	const char *text = ip[0];
	INT32 w = V_ThinStringWidth(text, 0);
	INT32 f = highlightflags;
	V_DrawMenuString(x - 10 - (skullAnimCounter/5), y, f, "\x1C"); // left arrow
	V_DrawMenuString(x + w + 2+ (skullAnimCounter/5), y, f, "\x1D"); // right arrow
	V_DrawThinString(x, y, f, text);
	K_DrawGameControl(BASEVIDWIDTH + 4 + tx, y, 0, "<c> Rejoin", 2, TINY_FONT, V_ORANGEMAP);

	return shift;
}

//
// M_DrawKartGamemodeMenu
//
// Huge gamemode-selection list for main menu
//
void M_DrawKartGamemodeMenu(void)
{
	UINT8 n = 0;
	INT32 i, x, y;
	INT32 tx = M_EaseWithTransition(Easing_Linear, 5 * 48);

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (currentMenu->menuitems[i].status == IT_DISABLED)
		{
			continue;
		}

		n++;
	}

	n--;
	x = GM_STARTX - ((GM_XOFFSET / 2) * (n-1)) + tx;
	y = GM_STARTY - ((GM_YOFFSET / 2) * (n-1));

	M_DrawMenuTooltips();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		INT32 type;

		if (currentMenu->menuitems[i].status == IT_DISABLED)
		{
			continue;
		}

		if (i >= currentMenu->numitems-1)
		{
			x = GM_STARTX + (GM_XOFFSET * 5 / 2) + tx;
			y = GM_STARTY + (GM_YOFFSET * 5 / 2);

		}

		INT32 cx = x;
		boolean selected = (i == itemOn && menutransition.tics == menutransition.dest);

		if (selected)
		{
			fixed_t f = M_DueFrac(gm_flipStart, GM_FLIPTIME);
			cx -= Easing_OutSine(f, 0, (GM_XOFFSET / 2));

			// Direct Join
			if (currentMenu == &PLAY_MP_OptSelectDef && i == mp_directjoin)
			{
				INT32 shift = M_DrawRejoinIP(cx, y, cx - x);
				cx -= Easing_OutSine(f, 0, shift);
			}
		}

		type = (currentMenu->menuitems[i].status & IT_DISPLAY);

		switch (type)
		{
			case IT_STRING:
			case IT_TRANSTEXT2:
				{
					UINT8 *colormap = NULL;

					if (selected)
					{
						colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);
					}
					else
					{
						colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_MOSS, GTC_CACHE);
					}

					V_DrawFixedPatch(cx*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, W_CachePatchName("MENUPLTR", PU_CACHE), colormap);
					V_DrawGamemodeString(cx + 16, y - 3,
						(type == IT_TRANSTEXT2
							? V_TRANSLUCENT
							: 0
						),
						colormap,
						currentMenu->menuitems[i].text);
				}
				break;
		}

		x += GM_XOFFSET;
		y += GM_YOFFSET;
	}
}

void M_FlipKartGamemodeMenu(boolean slide)
{
	gm_flipStart = slide ? I_GetTime() : 0;
}

void M_DrawHorizontalMenu(void)
{
	INT32 x, y, i, final = currentMenu->extra2-1, showflags;

	const INT32 width = 80;

	y = currentMenu->y;

	V_DrawFadeFill(0, y-2, BASEVIDWIDTH, 24, 0, 31, 5);

	x = (BASEVIDWIDTH - 8*final)/2;
	for (i = 0; i < currentMenu->extra2; i++, x += 8)
	{
		if (i == itemOn)
		{
			V_DrawFill(x-2, y + 16, 4, 4, 0);
		}
		else if (i >= currentMenu->numitems)
		{
			V_DrawFill(x-1, y + 17, 2, 2, 20);
		}
		else
		{
			V_DrawFill(x-1, y + 17, 2, 2,
				(i == final && skullAnimCounter/5) ? 73 : 10
			);
		}
	}

	i = itemOn;
	x = BASEVIDWIDTH/2;

	do
	{
		if (i == 0)
			break;
		i--;
		x -= width;
	}
	while (x > -width/2);

	while (x < BASEVIDWIDTH + (width/2))
	{
		showflags = 0;
		if (i == final)
		{
			showflags |= V_STRINGDANCE;
			if (itemOn == i)
				showflags |= V_YELLOWMAP;
		}
		else if (i == itemOn)
		{
			showflags |= highlightflags;
		}

		V_DrawCenteredThinString(
			x, y,
			showflags,
			currentMenu->menuitems[i].text
		);

		if (++i == currentMenu->numitems)
			break;
		x += width;
	}

	if (itemOn != 0)
		V_DrawMenuString((BASEVIDWIDTH - width)/2 + 3 - (skullAnimCounter/5), y + 1,
			highlightflags, "\x1C"); // left arrow

	if (itemOn != currentMenu->numitems-1)
		V_DrawMenuString((BASEVIDWIDTH + width)/2 - 10 + (skullAnimCounter/5), y + 1,
			highlightflags, "\x1D"); // right arrow
}

#define MAXMSGLINELEN 256

//
//  Draw a textbox, like Quake does, because sometimes it's difficult
//  to read the text with all the stuff in the background...
//
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines)
{
	// Solid color textbox.
	V_DrawFill(x+5, y+5, width*8+6, boxlines*8+6, 159);
	//V_DrawFill(x+8, y+8, width*8, boxlines*8, 31);
}

//
// M_DrawMessageMenu
//
// Generic message prompt
//
void M_DrawMessageMenu(void)
{
	INT32 y = currentMenu->y;
	size_t i, start = 0;
	INT16 max;
	char string[MAXMENUMESSAGE];
	INT32 mlines;
	const char *msg = currentMenu->menuitems[0].text;

	mlines = currentMenu->lastOn>>8;
	max = (INT16)((UINT8)(currentMenu->lastOn & 0xFF)*8);

	M_DrawTextBox(currentMenu->x, y - 8, (max+7)>>3, mlines);

	while (*(msg+start))
	{
		size_t len = strlen(msg+start);

		for (i = 0; i < len; i++)
		{
			if (*(msg+start+i) == '\n')
			{
				memset(string, 0, MAXMENUMESSAGE);
				if (i >= MAXMENUMESSAGE)
				{
					CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
					return;
				}
				else
				{
					strncpy(string,msg+start, i);
					string[i] = '\0';
					start += i;
					i = (size_t)-1; //added : 07-02-98 : damned!
					start++;
				}
				break;
			}
		}

		if (i == strlen(msg+start))
		{
			if (i >= MAXMENUMESSAGE)
			{
				CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
				return;
			}
			else
			{
				strcpy(string, msg + start);
				start += i;
			}
		}

		V_DrawMenuString((BASEVIDWIDTH - V_MenuStringWidth(string, 0))/2,y,0,string);
		y += 8; //SHORT(hu_font[0]->height);
	}
}

// Draw an Image Def.  Aka, Help images.
// Defines what image is used in (menuitem_t)->patch.
// You can even put multiple images in one menu!
void M_DrawImageDef(void)
{
	patch_t *patch = W_CachePatchName(currentMenu->menuitems[itemOn].text, PU_CACHE);

	if (patch->width <= BASEVIDWIDTH)
	{
		V_DrawScaledPatch(0, 0, 0, patch);
	}
	else
	{
		V_DrawSmallScaledPatch(0, 0, 0, patch);
	}

	if (currentMenu->menuitems[itemOn].mvar1)
	{
		V_DrawString(2,BASEVIDHEIGHT-10, V_YELLOWMAP, va("%d", (itemOn<<1)-1)); // intentionally not highlightflags, unlike below
		V_DrawRightAlignedString(BASEVIDWIDTH-2,BASEVIDHEIGHT-10, V_YELLOWMAP, va("%d", itemOn<<1)); // ditto
	}
	else
	{
		INT32 x = BASEVIDWIDTH>>1, y = (BASEVIDHEIGHT>>1) - 4;
		x += (itemOn ? 1 : -1)*((BASEVIDWIDTH>>2) + 10);
		V_DrawCenteredString(x, y-10, highlightflags, "USE ARROW KEYS");
		V_DrawCharacter(x - 10 - (skullAnimCounter/5), y,
			'\x1C' | highlightflags, false); // left arrow
		V_DrawCharacter(x + 2 + (skullAnimCounter/5), y,
			'\x1D' | highlightflags, false); // right arrow
		V_DrawCenteredString(x, y+10, highlightflags, "TO LEAF THROUGH");
	}
}

//
// PLAY MENUS
//

static void M_DrawCharSelectCircle(setup_player_t *p, INT16 x, INT16 y)
{
	angle_t angamt = ANGLE_MAX;
	UINT16 i, numoptions = 0;
	INT16 l = 0, r = 0;
	INT16 subtractcheck;

	switch (p->mdepth)
	{
		case CSSTEP_ALTS:
			numoptions = setup_chargrid[p->gridx][p->gridy].numskins;
			break;
		case CSSTEP_COLORS:
		case CSSTEP_FOLLOWERCOLORS:
			numoptions = p->colors.listLen;
			break;
		case CSSTEP_FOLLOWERCATEGORY:
			numoptions = setup_numfollowercategories+1;
			break;
		case CSSTEP_FOLLOWER:
			numoptions = setup_followercategories[p->followercategory][0];
			break;
		default:
			return;
	}

	if (numoptions == 0)
	{
		return;
	}

	subtractcheck = 1 ^ (numoptions & 1);

	angamt /= numoptions;

	for (i = 0; i < numoptions; i++)
	{
		fixed_t cx = x << FRACBITS, cy = y << FRACBITS;
		boolean subtract = (i & 1) == subtractcheck;
		angle_t ang = ((i+1)/2) * angamt;
		patch_t *patch = NULL;
		UINT8 *colormap = NULL;
		fixed_t radius = 28<<FRACBITS;
		INT16 n = 0;

		switch (p->mdepth)
		{
			case CSSTEP_ALTS:
			{
				INT16 skin;

				n = (p->clonenum) + numoptions/2;
				if (subtract)
					n -= ((i+1)/2);
				else
					n += ((i+1)/2);
				n = (n + numoptions) % numoptions;

				skin = setup_chargrid[p->gridx][p->gridy].skinlist[n];
				patch = faceprefix[skin][FACE_RANK];
				colormap = R_GetTranslationColormap(skin, skins[skin]->prefcolor, GTC_MENUCACHE);
				radius = 24<<FRACBITS;

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				cy -= (SHORT(patch->height) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_COLORS:
			{
				INT16 diff;

				if (i == 0)
				{
					n = l = r = M_GetColorBefore(&p->colors, p->color, (numoptions/2) - (numoptions & 1));
				}
				else if (subtract)
				{
					n = l = M_GetColorBefore(&p->colors, l, 1);
				}
				else
				{
					n = r = M_GetColorAfter(&p->colors, r, 1);
				}

				colormap = R_GetTranslationColormap(TC_DEFAULT, (n == SKINCOLOR_NONE) ? skins[p->skin]->prefcolor : n, GTC_MENUCACHE);

				diff = (numoptions - i) / 2;  // only 0 when i == numoptions-1

				if (diff == 0)
					patch = W_CachePatchName("COLORSP2", PU_CACHE);
				else if (abs(diff) < 25)
					patch = W_CachePatchName("COLORSP1", PU_CACHE);
				else
					patch = W_CachePatchName("COLORSP0", PU_CACHE);

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_FOLLOWERCATEGORY:
			{
				followercategory_t *fc = NULL;

				n = (p->followercategory + 1) + numoptions/2;
				if (subtract)
					n -= ((i+1)/2);
				else
					n += ((i+1)/2);
				n = (n + numoptions) % numoptions;

				if (n == 0)
				{
					patch = W_CachePatchName("K_NOBLNS", PU_CACHE);
				}
				else
				{
					fc = &followercategories[setup_followercategories[n - 1][1]];
					patch = W_CachePatchName(fc->icon, PU_CACHE);
				}

				radius = 24<<FRACBITS;

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				cy -= (SHORT(patch->height) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_FOLLOWER:
			{
				follower_t *fl = NULL;
				INT16 startfollowern = p->followern;

				if (i == 0)
				{
					n = p->followern;
					r = (numoptions+1)/2;
					while (r)
					{
						n--;
						if (n < 0)
							n = numfollowers-1;
						if (n == startfollowern)
							break;
						if (followers[n].category == setup_followercategories[p->followercategory][1]
							&& K_FollowerUsable(n))
							r--;
					}
					l = r = n;
				}
				else if (subtract)
				{
					do
					{
						l--;
						if (l < 0)
							l = numfollowers-1;
						if (l == startfollowern)
							break;
					}
					while (followers[l].category != setup_followercategories[p->followercategory][1]
						|| !K_FollowerUsable(l));
					n = l;
				}
				else
				{
					do
					{
						r++;
						if (r >= numfollowers)
							r = 0;
						if (r == startfollowern)
							break;
					}
					while (followers[r].category != setup_followercategories[p->followercategory][1]
						|| !K_FollowerUsable(r));
					n = r;
				}

				{
					fl = &followers[n];
					patch = W_CachePatchName(fl->icon, PU_CACHE);

					colormap = R_GetTranslationColormap(TC_DEFAULT,
						K_GetEffectiveFollowerColor(fl->defaultcolor, fl, p->color, skins[p->skin]),
						GTC_MENUCACHE
					);
				}

				radius = 24<<FRACBITS;

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				cy -= (SHORT(patch->height) << FRACBITS) >> 1;
				break;
			}

			case CSSTEP_FOLLOWERCOLORS:
			{
				INT16 diff;
				UINT16 col;

				if (i == 0)
				{
					n = l = r = M_GetColorBefore(&p->colors, p->followercolor, (numoptions/2) - (numoptions & 1));
				}
				else if (subtract)
				{
					n = l = M_GetColorBefore(&p->colors, l, 1);
				}
				else
				{
					n = r = M_GetColorAfter(&p->colors, r, 1);
				}

				col = K_GetEffectiveFollowerColor(n, &followers[p->followern], p->color, skins[p->skin]);

				colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);

				diff = (numoptions - i)/2;  // only 0 when i == numoptions-1

				if (diff == 0)
					patch = W_CachePatchName("COLORSP2", PU_CACHE);
				else if (abs(diff) < 25)
					patch = W_CachePatchName("COLORSP1", PU_CACHE);
				else
					patch = W_CachePatchName("COLORSP0", PU_CACHE);

				cx -= (SHORT(patch->width) << FRACBITS) >> 1;
				break;
			}

			default:
				break;
		}

		if (subtract)
			ang = (signed)(ANGLE_90 - ang);
		else
			ang = ANGLE_90 + ang;

		if (numoptions & 1)
			ang = (signed)(ang - (angamt/2));

		if (p->rotate)
		{
			SINT8 rotate = p->rotate;
			if ((p->hitlag == true) && (setup_animcounter & 1))
				rotate = -rotate;
			ang = (signed)(ang + ((angamt / CSROTATETICS) * rotate));
		}

		cx += FixedMul(radius, FINECOSINE(ang >> ANGLETOFINESHIFT));
		cy -= FixedMul(radius, FINESINE(ang >> ANGLETOFINESHIFT)) / 3;

		V_DrawFixedPatch(cx, cy, FRACUNIT, 0, patch, colormap);
		if (p->mdepth == CSSTEP_ALTS && n != p->clonenum)
			V_DrawFixedPatch(cx, cy, FRACUNIT, V_TRANSLUCENT, W_CachePatchName("ICONDARK", PU_CACHE), NULL);
	}
}

// returns false if the character couldn't be rendered
boolean M_DrawCharacterSprite(INT16 x, INT16 y, INT16 skin, UINT8 spr2, UINT8 rotation, UINT32 frame, INT32 addflags, UINT8 *colormap)
{
	UINT8 spr;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *sprpatch;

	spr = P_GetSkinSprite2(skins[skin], spr2, NULL);
	sprdef = &skins[skin]->sprites[spr];

	if (!sprdef->numframes) // No frames ??
		return false; // Can't render!

	frame %= sprdef->numframes;

	sprframe = &sprdef->spriteframes[frame];
	sprpatch = W_CachePatchNum(sprframe->lumppat[rotation], PU_CACHE);

	if (sprframe->flip & (1<<rotation)) // Only for first sprite
	{
		addflags ^= V_FLIP; // This sprite is left/right flipped!
	}

	if (skins[skin]->highresscale != FRACUNIT)
	{
		V_DrawFixedPatch(x<<FRACBITS,
					y<<FRACBITS,
					skins[skin]->highresscale,
					addflags, sprpatch, colormap);
	}
	else
		V_DrawMappedPatch(x, y, addflags, sprpatch, colormap);

	return true;
}

// Returns false is the follower shouldn't be rendered.
// 'num' can be used to directly specify the follower number, but doing this will not animate it.
// if a setup_player_t is specified instead, its data will be used to animate the follower sprite.
static boolean M_DrawFollowerSprite(INT16 x, INT16 y, INT32 num, boolean charflip, INT32 addflags, UINT8 *colormap, setup_player_t *p)
{
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;
	INT32 followernum;
	state_t *usestate;
	UINT32 useframe;
	follower_t *fl;
	UINT8 rotation = (charflip ? 1 : 7);

	if (horngoner)
		return false;

	if (p != NULL)
		followernum = p->followern;
	else
		followernum = num;

	// Don't draw if we're outta bounds.
	if (followernum < 0 || followernum >= numfollowers)
		return false;

	fl = &followers[followernum];

	if (p != NULL)
	{
		usestate = p->follower_state;
		useframe = p->follower_frame;
	}
	else
	{
		usestate = &states[followers[followernum].followstate];
		useframe = usestate->frame & FF_FRAMEMASK;
	}

	sprdef = &sprites[usestate->sprite];

	// draw the follower

	if (useframe >= sprdef->numframes)
		useframe = 0;	// frame doesn't exist, we went beyond it... what?

	sprframe = &sprdef->spriteframes[useframe];
	patch = W_CachePatchNum(sprframe->lumppat[rotation], PU_CACHE);

	if (sprframe->flip & (1<<rotation)) // Only for first sprite
	{
		addflags ^= V_FLIP; // This sprite is left/right flipped!
	}

	fixed_t sine = 0;

	if (p != NULL)
	{
		UINT16 color = K_GetEffectiveFollowerColor(
			(p->mdepth < CSSTEP_FOLLOWERCOLORS && p->mdepth != CSSTEP_ASKCHANGES) ? fl->defaultcolor : p->followercolor,
			fl,
			p->color,
			skins[p->skin]
		);
		sine = FixedMul(fl->bobamp, FINESINE(((FixedMul(4 * M_TAU_FIXED, fl->bobspeed) * p->follower_timer)>>ANGLETOFINESHIFT) & FINEMASK));
		colormap = R_GetTranslationColormap(TC_DEFAULT, color, GTC_MENUCACHE);
	}

	V_DrawFixedPatch((x*FRACUNIT), ((y-12)*FRACUNIT) + sine, fl->scale, addflags, patch, colormap);

	return true;
}

static void M_DrawCharSelectSprite(UINT8 num, INT16 x, INT16 y, boolean charflip)
{
	setup_player_t *p = &setup_player[num];
	UINT16 color;
	UINT8 *colormap;

	if (p->skin < 0)
	{
		return;
	}

	if (p->mdepth < CSSTEP_COLORS && p->mdepth != CSSTEP_ASKCHANGES)
	{
		color = skins[p->skin]->prefcolor;
	}
	else
	{
		color = p->color;
	}

	if (color == SKINCOLOR_NONE)
	{
		color = skins[p->skin]->prefcolor;
	}

	colormap = R_GetTranslationColormap(p->skin, color, GTC_MENUCACHE);

	M_DrawCharacterSprite(x, y, p->skin, SPR2_STIN, (charflip ? 1 : 7), ((p->mdepth == CSSTEP_READY) ? setup_animcounter : 0),
		p->mdepth == CSSTEP_ASKCHANGES ? V_TRANSLUCENT : 0, colormap);
}

static void M_DrawCharSelectPreview(UINT8 num)
{
	INT16 x = 11, y = 5;
	char letter = 'A' + num;
	setup_player_t *p = &setup_player[num];
	boolean charflip = !!(num & 1);

	if (num & 1)
		x += 233;

	if (num > 1)
		y += 99;

	V_DrawScaledPatch(x, y+6, V_TRANSLUCENT, W_CachePatchName("PREVBACK", PU_CACHE));

	if (p->mdepth >= CSSTEP_CHARS || p->mdepth == CSSTEP_ASKCHANGES)
	{
		M_DrawCharSelectSprite(num, x+32, y+75, charflip);
		M_DrawCharSelectCircle(p, x+32, y+64);
	}

	if (p->showextra == false)
	{
		INT32 backx = x + ((num & 1) ? -1 : 11);
		V_DrawScaledPatch(backx, y+2, 0, W_CachePatchName("FILEBACK", PU_CACHE));

		V_DrawScaledPatch(x + ((num & 1) ? 44 : 0), y+2, 0, W_CachePatchName(va("CHARSEL%c", letter), PU_CACHE));

		profile_t *pr = NULL;
		if (p->mdepth > CSSTEP_PROFILE)
		{
			pr = PR_GetProfile(p->profilen);
		}
		V_DrawCenteredFileString(backx+26, y+2, 0, pr ? pr->profilename : "PLAYER");
	}

	if (p->mdepth >= CSSTEP_FOLLOWER || p->mdepth == CSSTEP_ASKCHANGES)
	{
		M_DrawFollowerSprite(x+32+((charflip ? 1 : -1)*16), y+75, -1, charflip, p->mdepth == CSSTEP_ASKCHANGES ? V_TRANSLUCENT : 0, NULL, p);
	}

	if ((setup_animcounter/10) & 1)
	{
		if (p->mdepth == CSSTEP_NONE && num == setup_numplayers && gamestate == GS_MENU)
		{
			V_DrawScaledPatch(x+1, y+36, 0, W_CachePatchName("4PSTART", PU_CACHE));
		}
		else if (p->mdepth >= CSSTEP_READY)
		{
			V_DrawScaledPatch(x+1, y+36, 0, W_CachePatchName("4PREADY", PU_CACHE));
		}
	}

	// Profile selection
	if (p->mdepth == CSSTEP_PROFILE)
	{
		INT16 px = x+12;
		INT16 py = y+48 - p->profilen*12 +
			Easing_OutSine(
				M_DueFrac(p->profilen_slide.start, 5),
				p->profilen_slide.dist*12,
				0
			);
		UINT8 maxp = PR_GetNumProfiles();

		UINT8 i = 0;
		UINT8 j;

		V_SetClipRect(0, (y+25)*FRACUNIT, BASEVIDWIDTH*FRACUNIT, (5*12)*FRACUNIT, 0);


		for (i = 0; i < maxp; i++)
		{
			profile_t *pr = PR_GetProfile(i);
			INT16 dist = abs(p->profilen - i);
			INT32 notSelectable = 0;
			SINT8 belongsTo = -1;

			if (i != PROFILE_GUEST)
			{
				for (j = 0; j < setup_numplayers; j++)
				{
					if (setup_player[j].mdepth > CSSTEP_PROFILE
						&& setup_player[j].profilen == i)
					{
						belongsTo = j;
						break;
					}
				}
			}

			if (belongsTo != -1 && belongsTo != num)
			{
				notSelectable |= V_TRANSLUCENT;
			}

			if (dist > 3)
			{
				py += 12;
				continue;
			}

			if (dist > 1)
			{
				V_DrawCenteredFileString(px+26, py, notSelectable, pr->version ? pr->profilename : "NEW");
				V_DrawScaledPatch(px, py, V_TRANSLUCENT, W_CachePatchName("FILEBACK", PU_CACHE));
			}
			else
			{
				V_DrawScaledPatch(px, py, 0, W_CachePatchName("FILEBACK", PU_CACHE));

				if (i != p->profilen || ((setup_animcounter/10) & 1))
				{
					const char *txt = pr->version ? pr->profilename : "NEW";

					fixed_t w = V_StringScaledWidth(
						FRACUNIT,
						FRACUNIT,
						FRACUNIT,
						notSelectable,
						FILE_FONT,
						txt
					);

					V_DrawStringScaled(
						((px+26) * FRACUNIT) - (w/2),
						py * FRACUNIT,
						FRACUNIT,
						FRACUNIT,
						FRACUNIT,
						notSelectable,
						i == p->profilen ? R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_SAPPHIRE, GTC_CACHE) : NULL,
						FILE_FONT,
						txt
					);
				}
			}
			py += 12;
		}

		V_ClearClipRect();
	}
	// "Changes?"
	else if (p->mdepth == CSSTEP_ASKCHANGES)
	{
		UINT8 i;
		char choices[2][9] = {"ALL GOOD", "CHANGE"};
		INT32 xpos = x+8;
		INT32 ypos = y+38;

		V_DrawFileString(xpos, ypos, 0, "READY?");

		for (i = 0; i < 2; i++)
		{
			UINT8 cy = ypos+16 + (i*10);

			if (p->changeselect == i)
				M_DrawCursorHand(xpos + 20, cy);

			V_DrawThinString(xpos+16, cy, (p->changeselect == i ? highlightflags : 0), choices[i]);
		}
	}

	if (p->showextra == true)
	{
		INT32 randomskin = 0;
		INT32 doping = 0;
		char variadicInfoBuffer[(MAXCOLORNAME*2) + 1 + 2 + 1];//+1 for spacing, +2 for brackets, +1 for null terminator
		UINT16 folcol;

		switch (p->mdepth)
		{
			case CSSTEP_ALTS: // Select clone
			case CSSTEP_READY:
				if (p->clonenum < setup_chargrid[p->gridx][p->gridy].numskins
					&& setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum] < numskins)
				{
					V_DrawThinString(x-3, y+12, 0,
						skins[setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum]]->name);
					randomskin = (skins[setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum]]->flags & SF_IRONMAN);
					doping = (skins[setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum]]->flags & SF_HIVOLT);
				}
				else
				{
					V_DrawThinString(x-3, y+12, 0, va("BAD CLONENUM %u", p->clonenum));
				}
				/* FALLTHRU */
			case CSSTEP_CHARS: // Character Select grid
				V_DrawThinString(x-3, y+2, 0, va("Class %c (s %c - w %c)",
					(doping
						? 'R' : ('A' + R_GetEngineClass(p->gridx+1, p->gridy+1, randomskin))),
					(randomskin
						? '?' : ('1'+p->gridx)),
					(randomskin
						? '?' : ('1'+p->gridy))
					));
				break;
			case CSSTEP_COLORS: // Select color
				if (p->color < numskincolors)
				{
					if(p->color == SKINCOLOR_NONE) //'default' handling
						sprintf(variadicInfoBuffer, "%s (%s)", skincolors[p->color].name, skincolors[skins[p->skin]->prefcolor].name);
					else
						sprintf(variadicInfoBuffer, "%s", skincolors[p->color].name);

					V_DrawThinString(x-3, y+2, 0, variadicInfoBuffer);
				}
				else
				{
					V_DrawThinString(x-3, y+2, 0, va("BAD COLOR %u", p->color));
				}
				break;
			case CSSTEP_FOLLOWERCATEGORY:
				if (p->followercategory == -1)
				{
					V_DrawThinString(x-3, y+2, 0, "None");
				}
				else
				{
					V_DrawThinString(x-3, y+2, 0,
						followercategories[setup_followercategories[p->followercategory][1]].name);
				}
				break;
			case CSSTEP_FOLLOWER:
				if (p->followern == -1)
				{
					V_DrawThinString(x-3, y+2, 0, "None");
				}
				else
				{
					V_DrawThinString(x-3, y+2, 0,
						followers[p->followern].name);
				}
				break;
			case CSSTEP_FOLLOWERCOLORS:
				folcol = K_GetEffectiveFollowerColor(p->followercolor, &followers[p->followern], p->color, skins[p->skin]);

				if (p->followercolor == FOLLOWERCOLOR_MATCH)
				{
					sprintf(variadicInfoBuffer, "Match (%s)", skincolors[folcol].name);
					V_DrawThinString(x-3, y+2, 0, variadicInfoBuffer);
				}
				else if (p->followercolor == FOLLOWERCOLOR_OPPOSITE)
				{
					sprintf(variadicInfoBuffer, "Opposite (%s)", skincolors[folcol].name);
					V_DrawThinString(x-3, y+2, 0, variadicInfoBuffer);
				}
				else if (p->followercolor < numskincolors)
				{
					if(p->followercolor == SKINCOLOR_NONE) //'default' handling
						sprintf(variadicInfoBuffer, "%s (%s)", skincolors[p->followercolor].name, skincolors[folcol].name);
					else
						sprintf(variadicInfoBuffer, "%s", skincolors[p->followercolor].name);

					V_DrawThinString(x-3, y+2, 0, variadicInfoBuffer);
				}
				else
				{
					V_DrawThinString(x-3, y+2, 0, va("BAD FOLLOWERCOLOR %u", p->followercolor));
				}
				break;
			default:
				V_DrawThinString(x-3, y+2, 0, "[extrainfo mode]");
				break;
		}
	}
}

static void M_DrawCharSelectExplosions(boolean charsel, INT16 basex, INT16 basey)
{
	UINT8 i;
	INT16 quadx = 2, quady = 2, mul = 22;

	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		UINT8 *colormap;
		UINT8 frame;

		if (setup_explosions[i].tics == 0 || setup_explosions[i].tics > 5)
			continue;

		frame = 6 - setup_explosions[i].tics;

		if (charsel)
		{
			quadx = 4 * (setup_explosions[i].x / 3);
			quady = 4 * (setup_explosions[i].y / 3);
			mul = 16;
		}

		colormap = R_GetTranslationColormap(TC_DEFAULT, setup_explosions[i].color, GTC_MENUCACHE);

		V_DrawMappedPatch(
			basex + (setup_explosions[i].x*mul) + quadx - 6,
			basey + (setup_explosions[i].y*mul) + quady - 6,
			0, W_CachePatchName(va("CHCNFRM%d", frame), PU_CACHE),
			colormap
		);
	}
}

#define IDLELEN 8
#define SELECTLEN (8 + IDLELEN + 7 + IDLELEN)

static void M_DrawCharSelectCursor(UINT8 num)
{
	static const char *idleframes[IDLELEN] = {
		"CHHOV1", "CHHOV1", "CHHOV1", "CHHOV2", "CHHOV1", "CHHOV3", "CHHOV1", "CHHOV2"
	};
	static const char *selectframesa[SELECTLEN] = {
		"CHHOV1", "CHPIKA1", "CHHOV2", "CHPIKA2", "CHHOV3", "CHPIKA3", "CHHOV2", "CHPIKA4",
		"CHHOV1", "CHHOV1", "CHHOV1", "CHHOV2", "CHHOV1", "CHHOV3", "CHHOV1", "CHHOV2",
		"CHPIKA5", "CHHOV2", "CHPIKA6", "CHHOV3", "CHPIKA7", "CHHOV2", "CHPIKA8",
		"CHHOV1", "CHHOV1", "CHHOV1", "CHHOV2", "CHHOV1", "CHHOV3", "CHHOV1", "CHHOV2"
	};
	static const char *selectframesb[SELECTLEN] = {
		NULL, "CHPIKB1", NULL, "CHPIKB2", NULL, "CHPIKB3", NULL, "CHPIKB4",
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		"CHPIKB5", NULL, "CHPIKB6", NULL, "CHPIKB7", NULL, "CHPIKB8",
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};

	setup_player_t *p = &setup_player[num];
	char letter = 'A' + num;
	UINT16 color = SKINCOLOR_NONE;
	UINT8 *colormap;
	INT16 x, y;
	INT16 quadx, quady;

	if (p->mdepth < CSSTEP_ASKCHANGES)
		return;

	quadx = 4 * (p->gridx / 3);
	quady = 4 * (p->gridy / 3);

	x = 82 + (p->gridx*16) + quadx - 13;
	y = 22 + (p->gridy*16) + quady - 12;

	// profiles skew the graphic to the right slightly
	if (optionsmenu.profile)
		x += 64;

	color = p->color;
	if (color == SKINCOLOR_NONE)
	{
		if (p->skin >= 0)
		{
			color = skins[p->skin]->prefcolor;
		}
		else
		{
			color = SKINCOLOR_GREY;
		}
	}

	colormap = R_GetTranslationColormap(TC_DEFAULT, color, GTC_MENUCACHE);

	if (p->mdepth >= CSSTEP_READY)
	{
		V_DrawMappedPatch(x, y, 0, W_CachePatchName("CHCNFRM0", PU_CACHE), colormap);
	}
	else if (p->mdepth > CSSTEP_CHARS)
	{
		if (cv_reducevfx.value)
		{
			V_DrawMappedPatch(x, y, 0, W_CachePatchName(selectframesa[0], PU_CACHE), colormap);
		}
		else
		{
			V_DrawMappedPatch(x, y, 0, W_CachePatchName(selectframesa[setup_animcounter % SELECTLEN], PU_CACHE), colormap);
			if (selectframesb[(setup_animcounter-1) % SELECTLEN] != NULL)
				V_DrawMappedPatch(x, y, V_TRANSLUCENT, W_CachePatchName(selectframesb[(setup_animcounter-1) % SELECTLEN], PU_CACHE), colormap);
		}

	}
	else
	{
		if (cv_reducevfx.value)
			V_DrawMappedPatch(x, y, 0, W_CachePatchName(idleframes[0], PU_CACHE), colormap);
		else
			V_DrawMappedPatch(x, y, 0, W_CachePatchName(idleframes[setup_animcounter % IDLELEN], PU_CACHE), colormap);
	}

	if (p->mdepth < CSSTEP_READY)
		V_DrawMappedPatch(x, y, 0, W_CachePatchName(va("CSELH%c", letter), PU_CACHE), colormap);
}

#undef IDLE
#undef IDLELEN
#undef SELECTLEN

// Draw character profile card.
// Moved here because in the case of profile edition this is drawn in the charsel menu.
void M_DrawProfileCard(INT32 x, INT32 y, boolean greyedout, profile_t *p)
{
	setup_player_t *sp = &setup_player[0];	// When editing profile character, we'll always be checking for what P1 is doing.
	patch_t *card = W_CachePatchName("PR_CARD", PU_CACHE);
	patch_t *cardbot = W_CachePatchName("PR_CARDB", PU_CACHE);
	patch_t *pwrlv = W_CachePatchName("PR_PWR", PU_CACHE);
	UINT16 truecol = SKINCOLOR_BLACK;
	UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_BLACK, GTC_CACHE);
	INT32 skinnum = -1;

	char pname[PROFILENAMELEN+1] = "NEW";

	if (p != NULL && p->version)
	{
		truecol = p->color;
		skinnum = R_SkinAvailableEx(p->skinname, false);
		strcpy(pname, p->profilename);
	}

	if (sp->mdepth >= CSSTEP_CHARS)
	{
		truecol = sp->color;
		skinnum = setup_chargrid[sp->gridx][sp->gridy].skinlist[sp->clonenum];
	}

	if (truecol == SKINCOLOR_NONE)
	{
		if (skinnum >= 0)
		{
			truecol = skins[skinnum]->prefcolor;
		}
		else
		{
			truecol = SKINCOLOR_RED;
		}
	}

	// Card
	{
		colormap = R_GetTranslationColormap(TC_DEFAULT, truecol, GTC_CACHE);
		V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, greyedout ? V_TRANSLUCENT : 0, card, colormap);
	}

	if (greyedout)
		return;	// only used for profiles we can't select.

	if (p != NULL)
	{
		V_DrawFixedPatch((x+30)*FRACUNIT, (y+84)*FRACUNIT, FRACUNIT, 0, pwrlv, colormap);
		K_DrawGameControl(x+30, y+87, 0, va("%d", p->wins), 1,
				(p->wins >= 1000 ? TINYTIMER_FONT : TIMER_FONT),
			0);
	}


	// check what setup_player is doing in priority.
	if (sp->mdepth >= CSSTEP_CHARS)
	{
		if (skinnum >= 0)
		{
			UINT8 *ccolormap = R_GetTranslationColormap(skinnum, truecol, GTC_MENUCACHE);

			if (M_DrawCharacterSprite(x-22, y+119, skinnum, SPR2_STIN, 7, 0, 0, ccolormap))
				V_DrawMappedPatch(x+14, y+66, 0, faceprefix[skinnum][FACE_RANK], ccolormap);
		}

		M_DrawCharSelectCircle(sp, x-22, y+104);

		if (sp->mdepth >= CSSTEP_FOLLOWER)
		{
			if (M_DrawFollowerSprite(x-22 - 16, y+119, 0, false, 0, 0, sp))
			{
				UINT16 col = K_GetEffectiveFollowerColor(sp->followercolor, &followers[sp->followern], sp->color, skins[sp->skin]);
				patch_t *ico = W_CachePatchName(followers[sp->followern].icon, PU_CACHE);
				UINT8 *fcolormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);
				V_DrawMappedPatch(x+14+18, y+66, 0, ico, fcolormap);
			}
		}
	}
	else if (skinnum > -1)	// otherwise, read from profile.
	{
		UINT8 *ccolormap;
		INT32 fln = K_FollowerAvailable(p->follower);

		if (R_SkinUsable(g_localplayers[0], skinnum, false))
			ccolormap = R_GetTranslationColormap(skinnum, truecol, GTC_MENUCACHE);
		else
			ccolormap = R_GetTranslationColormap(TC_BLINK, truecol, GTC_MENUCACHE);

		if (M_DrawCharacterSprite(x-22, y+119, skinnum, SPR2_STIN, 7, 0, 0, ccolormap))
		{
			V_DrawMappedPatch(x+14, y+66, 0, faceprefix[skinnum][FACE_RANK], ccolormap);
		}

		if (!horngoner && fln >= 0)
		{
			UINT16 fcol = K_GetEffectiveFollowerColor(
				p->followercolor,
				&followers[fln],
				p->color,
				skins[skinnum]
			);
			UINT8 *fcolormap = R_GetTranslationColormap(
			(K_FollowerUsable(fln) ? TC_DEFAULT : TC_BLINK),
				fcol, GTC_MENUCACHE);

			if (M_DrawFollowerSprite(x-22 - 16, y+119, fln, false, 0, fcolormap, NULL))
			{
				patch_t *ico = W_CachePatchName(followers[fln].icon, PU_CACHE);
				V_DrawMappedPatch(x+14+18, y+66, 0, ico, fcolormap);
			}
		}
	}

	V_DrawCenteredGamemodeString(x, y+24, 0, 0, pname);

	// Card bottom to overlay the skin preview
	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, cardbot, colormap);

	// Profile number & player name

	if (p != NULL)
	{
		V_DrawProfileNum(x + 37 + 10, y + 131, 0, PR_GetProfileNum(p));
		V_DrawCenteredThinString(x, y + 141, V_GRAYMAP, p->playername);
		V_DrawCenteredThinString(x, y + 151, V_GRAYMAP, GetPrettyRRID(p->public_key, true));
	}
}

void M_DrawCharacterSelect(void)
{
	const UINT8 pid = 0;

	UINT8 i, j, k;
	UINT8 priority = 0;
	INT16 quadx, quady;
	INT16 skin;
	INT32 basex = optionsmenu.profile ? (64 + M_EaseWithTransition(Easing_InSine, 5 * 48)) : 0;
	boolean forceskin = M_CharacterSelectForceInAction();

	if (setup_numplayers > 0)
	{
		priority = setup_animcounter % setup_numplayers;
	}

	{
		const int kTop = 6;

		if (!optionsmenu.profile) // Does nothing on this screen
		{
			K_DrawGameControl(BASEVIDWIDTH/2, kTop, pid, "<r_animated> Info   <c_animated> Default", 1, TINY_FONT, 0);
		}
		else
		{
			K_DrawGameControl(BASEVIDWIDTH/2+62, kTop, pid, "<a_animated> Accept  <x_animated> Back  <c_animated> Default", 1, TINY_FONT, 0);
		}
	}

	// We have to loop twice -- first time to draw the drop shadows, a second time to draw the icons.
	if (forceskin == false)
	{
		for (i = 0; i < 9; i++)
		{
			for (j = 0; j < 9; j++)
			{
				skin = setup_chargrid[i][j].skinlist[setup_page];
				quadx = 4 * (i / 3);
				quady = 4 * (j / 3);

				// Here's a quick little cheat to save on drawing time!
				// Don't draw a shadow if it'll get covered by another icon
				if ((i % 3 < 2) && (j % 3 < 2))
				{
					if ((setup_chargrid[i+1][j].skinlist[setup_page] != -1)
					&& (setup_chargrid[i][j+1].skinlist[setup_page] != -1)
					&& (setup_chargrid[i+1][j+1].skinlist[setup_page] != -1))
						continue;
				}

				if (skin != -1)
					V_DrawScaledPatch(basex+ 82 + (i*16) + quadx + 1, 22 + (j*16) + quady + 1, 0, W_CachePatchName("ICONBACK", PU_CACHE));
			}
		}
	}

	// Draw this inbetween. These drop shadows should be covered by the stat graph, but the icons shouldn't.
	V_DrawScaledPatch(basex+ 3, 2, 0, W_CachePatchName((optionsmenu.profile ? "PR_STGRPH" : "STATGRPH"), PU_CACHE));

	// Draw the icons now
	for (i = 0; i < 9; i++)
	{
		if ((forceskin == true) && (i != skins[cv_forceskin.value]->kartspeed-1))
			continue;

		for (j = 0; j < 9; j++)
		{
			if (forceskin == true)
			{
				if (j != skins[cv_forceskin.value]->kartweight-1)
					continue;
				skin = cv_forceskin.value;
			}
			else
			{
				skin = setup_chargrid[i][j].skinlist[setup_page];
			}

			for (k = 0; k < setup_numplayers; k++)
			{
				if (setup_player[k].mdepth < CSSTEP_ASKCHANGES)
					continue;
				if (setup_player[k].gridx != i || setup_player[k].gridy != j)
					continue;
				break; // k == setup_numplayers means no one has it selected
			}

			quadx = 4 * (i / 3);
			quady = 4 * (j / 3);

			if (skin != -1)
			{
				UINT8 *colormap;

				if (k == setup_numplayers)
					colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE);
				else
					colormap = R_GetTranslationColormap(skin, skins[skin]->prefcolor, GTC_MENUCACHE);

				V_DrawMappedPatch(basex + 82 + (i*16) + quadx, 22 + (j*16) + quady, 0, faceprefix[skin][FACE_RANK], colormap);

				// draw dot if there are more alts behind there!
				if (forceskin == false && setup_page+1 < setup_chargrid[i][j].numskins)
					V_DrawScaledPatch(basex + 82 + (i*16) + quadx, 22 + (j*16) + quady + 11, 0, W_CachePatchName("ALTSDOT", PU_CACHE));
			}
		}
	}

	// Explosions when you've made your final selection
	M_DrawCharSelectExplosions(true, basex + 82, 22);

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		// Draw a preview for each player
		if (optionsmenu.profile == NULL)
		{
			M_DrawCharSelectPreview(i);
		}
		else if (i == 0)
		{
			M_DrawProfileCard(optionsmenu.optx, optionsmenu.opty, false, optionsmenu.profile);
		}

		if (i >= setup_numplayers)
			continue;

		// Draw the cursors
		if (i != priority)
			M_DrawCharSelectCursor(i);
	}

	if (setup_numplayers > 0)
	{
		// Draw the priority player over the other ones
		M_DrawCharSelectCursor(priority);
	}

	if (setup_numplayers > 1)
	{
		V_DrawCenteredThinString(BASEVIDWIDTH/2, BASEVIDHEIGHT-12, V_30TRANS, "\x85""Double-input problems?\x80 Close Steam, DS4Windows, and other controller wrappers!");
	}
}

// DIFFICULTY SELECT
// This is a mix of K_DrawKartGamemodeMenu and the generic menu drawer depending on what we need.
// This is only ever used here (I hope because this is starting to pile up on hacks to look like the old m_menu.c lol...)

void M_DrawRaceDifficulty(void)
{
	patch_t *box = W_CachePatchName("M_DBOX", PU_CACHE);

	INT32 i;
	INT32 tx = M_EaseWithTransition(Easing_Linear, 5 * 48);
	INT32 x = 120 + tx;
	INT32 y = 48;

	M_DrawMenuTooltips();

	// Draw the box for difficulty...
	V_DrawFixedPatch((111 + tx)*FRACUNIT, 33*FRACUNIT, FRACUNIT, 0, box, NULL);

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i >= currentMenu->extra1)
		{
			x = GM_STARTX + (GM_XOFFSET * 5 / 2);
			y = GM_STARTY + (GM_YOFFSET * 5 / 2);

			if (i < currentMenu->numitems-1)
			{
				x -= GM_XOFFSET;
				y -= GM_YOFFSET;
			}

			x += tx;
		}

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			// This is HACKY......

			case IT_STRING2:
			{

				INT32 f = (i == itemOn) ? highlightflags : 0;

				if (currentMenu->menuitems[i].status & IT_CVAR)
				{
					// implicitely we'll only take care of normal cvars
					INT32 cx = 190 + tx;
					consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;

					if (i == itemOn)
					{

						INT32 w = V_MenuStringWidth(cv->string, 0)/2;

						M_DrawUnderline(124, 190 + w, y);

						V_DrawMenuString(cx - 10 - w - (skullAnimCounter/5), y, highlightflags, "\x1C"); // left arrow
						V_DrawMenuString(cx + w + 2 + (skullAnimCounter/5), y, highlightflags, "\x1D"); // right arrow
					}

					V_DrawCenteredMenuString(cx, y, f, cv->string);
				}

				V_DrawMenuString(124 + tx + (i == itemOn ? 1 : 0), y, f, currentMenu->menuitems[i].text);

				if (i == itemOn)
				{
					M_DrawCursorHand(124 + tx, y);
				}

				y += 14;

				break;
			}

			case IT_STRING:
			{

				INT32 cx = x;
				UINT8 *colormap = NULL;

				if (i == itemOn)
				{
					colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

					if (i >= currentMenu->extra1)
					{
						cx -= Easing_OutSine(M_DueFrac(gm_flipStart, GM_FLIPTIME), 0, GM_XOFFSET / 2);
					}
				}
				else
				{
					colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_MOSS, GTC_CACHE);
				}


				if (currentMenu->menuitems[i].status & (IT_CVAR | IT_ARROWS))
				{

					INT32 fx = (cx - tx);
					INT32 centx = fx + (320-fx)/2 + (tx);	// undo the menutransition movement to redo it here otherwise the text won't move at the same speed lole.

					const char *val = currentMenu->menuitems[i].text;
					if (currentMenu->menuitems[i].status & IT_CVAR)
					{
						consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
						val = cv->string;
					}

					V_DrawFixedPatch(cx*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, W_CachePatchName("MENUSHRT", PU_CACHE), colormap);
					V_DrawCenteredGamemodeString(centx, y - 3, 0, colormap, val);

					if (i == itemOn)
					{
						patch_t *arr_r = W_CachePatchName("GM_ARRL", PU_CACHE);
						patch_t *arr_l = W_CachePatchName("GM_ARRR", PU_CACHE);

						V_DrawFixedPatch((centx-54 - arr_r->width - (skullAnimCounter/5))*FRACUNIT, (y-3)*FRACUNIT, FRACUNIT, 0, arr_r, colormap);
						V_DrawFixedPatch((centx+54 + (skullAnimCounter/5))*FRACUNIT, (y-3)*FRACUNIT, FRACUNIT, 0, arr_l, colormap);
					}

				}
				else	// not a cvar
				{
					V_DrawFixedPatch(cx*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, W_CachePatchName("MENUPLTR", PU_CACHE), colormap);
					V_DrawGamemodeString(cx + 16, y - 3, 0, colormap, currentMenu->menuitems[i].text);
				}
				x += GM_XOFFSET;
				y += GM_YOFFSET;

				if (i < currentMenu->extra1)
				{
					y += 2; // extra spacing for Match Race options
				}

				break;
			}

			case IT_PATCH:
			{
				extern menu_anim_t g_drace_timer;
				const menuitem_t *it = &currentMenu->menuitems[i];
				boolean activated = g_drace_timer.dist == i;
				boolean flicker = activated && (I_GetTime() - g_drace_timer.start) % 2 < 1;

				INT32 cx = it->mvar1 + tx;
				INT32 cy = 79;

				const char *pat = i == drace_mritems && cv_thunderdome.value ? "RBOXTOGG" : it->patch;

				V_DrawMappedPatch(cx, cy, 0, W_CachePatchName(pat, PU_CACHE),
					flicker ? R_GetTranslationColormap(TC_HITLAG, 0, GTC_MENUCACHE) : NULL);

				if (it->itemaction.cvar && !it->itemaction.cvar->value)
				{
					V_DrawMappedPatch(cx, cy, 0, W_CachePatchName("OFF_TOGG", PU_CACHE), NULL);
				}

				switch (it->mvar2)
				{
					case MBT_Y:
						K_DrawGameControl(cx + 24, cy + 22, 0, activated ? "<y_pressed>" : "<y>", 0, MENU_FONT, 0);
						break;

					case MBT_Z:
						K_DrawGameControl(cx + 24, cy + 22, 0, activated ? "<z_pressed>" : "<z>", 0, MENU_FONT, 0);
						break;
				}
				break;
			}
		}
	}
}

// LEVEL SELECT

static void M_DrawCupPreview(INT16 y, levelsearch_t *baselevelsearch)
{
	levelsearch_t locklesslevelsearch = *baselevelsearch; // full copy
	locklesslevelsearch.checklocked = false;

	UINT8 i = 0;
	INT16 maxlevels = M_CountLevelsToShowInList(&locklesslevelsearch);
	const UINT32 ustep = 82;
	const fixed_t fracstep = (ustep * FRACUNIT);

	UINT32 unsignedportion = 0;
	fixed_t x = 0;

	INT16 map, start = M_GetFirstLevelInList(&i, &locklesslevelsearch);
	UINT8 starti = i;

	patch_t *staticpat = unvisitedlvl[cupgrid.previewanim % 4];

	if (baselevelsearch->cup && maxlevels > 0)
	{
		unsignedportion = (cupgrid.previewanim % (maxlevels * ustep));
		x = (unsignedportion * FRACUNIT) + rendertimefrac_unpaused;

		INT16 add = (x / fracstep) % maxlevels;
		map = start;
		while (add > 0)
		{
			map = M_GetNextLevelInList(map, &i, &locklesslevelsearch);

			if (map >= nummapheaders)
			{
				break;
			}

			add--;
		}

		x = -(x % fracstep);
		while (x < BASEVIDWIDTH * FRACUNIT)
		{
			if (map >= nummapheaders)
			{
				map = start;
				i = starti;
			}

			if (M_CanShowLevelInList(map, baselevelsearch))
			{
				K_DrawMapThumbnail(
					x + FRACUNIT, (y+2)<<FRACBITS,
					80<<FRACBITS,
					0,
					map,
					NULL);
			}
			else
			{
				V_DrawFixedPatch(
					x + FRACUNIT, (y+2) * FRACUNIT,
					FRACUNIT,
					0,
					staticpat,
					NULL);
			}

			x += fracstep;

			map = M_GetNextLevelInList(map, &i, &locklesslevelsearch);
		}
	}
	else
	{
		unsignedportion = (cupgrid.previewanim % ustep);
		x = (unsignedportion * FRACUNIT) + rendertimefrac_unpaused;

		x = -(x % fracstep);
		while (x < BASEVIDWIDTH * FRACUNIT)
		{
			V_DrawFixedPatch(x + FRACUNIT, (y+2) * FRACUNIT, FRACUNIT, 0, staticpat, NULL);
			x += fracstep;
		}
	}
}

static void M_DrawCupTitle(INT16 y, levelsearch_t *levelsearch)
{
	UINT8 temp = 0;

	V_DrawScaledPatch(0, y, 0, W_CachePatchName("MENUHINT", PU_CACHE));

	if (levelsearch->cup == &dummy_lostandfound)
	{
		V_DrawCenteredLSTitleLowString(BASEVIDWIDTH/2, y+6, 0, "Lost & Found");
	}
	else if (levelsearch->cup)
	{
		boolean unlocked = (M_GetFirstLevelInList(&temp, levelsearch) != NEXTMAP_INVALID);
		UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE);
		patch_t *icon = W_CachePatchName(levelsearch->cup->icon, PU_CACHE);
		const char *str = (unlocked ? va("%s Cup", levelsearch->cup->realname) : "???");
		INT16 offset = V_LSTitleLowStringWidth(str, 0) / 2;

		V_DrawLSTitleLowString(BASEVIDWIDTH/2 - offset, y+6, 0, str);

		if (unlocked)
		{
			V_DrawMappedPatch(BASEVIDWIDTH/2 - offset - 24, y+5, 0, icon, colormap);
			V_DrawMappedPatch(BASEVIDWIDTH/2 + offset + 3, y+5, 0, icon, colormap);
		}
	}
	else
	{
		if (currentMenu == &PLAY_LevelSelectDef)
		{
			UINT8 namedgt = (levellist.guessgt != MAXGAMETYPES) ? levellist.guessgt : levellist.newgametype;
			V_DrawCenteredLSTitleLowString(BASEVIDWIDTH/2, y+6, 0, va("%s Mode", gametypes[namedgt]->name));
		}
	}
}

fixed_t M_DrawCupWinData(INT32 rankx, INT32 ranky, cupheader_t *cup, UINT8 difficulty, boolean flash, boolean statsmode)
{
	INT32 rankw = 14 + 1 + 12 + 1 + 12;

	if (!cup)
		return 0;

	boolean noemerald = (!gamedata->everseenspecial || difficulty == KARTSPEED_EASY);

	if (statsmode)
	{
		rankw += 10 + 1;

		if (noemerald)
		{
			rankw -= (12 + 1);
			rankx += 7; // vibes-based maths, as tyron puts it
		}
	}

	rankx += 19 - (rankw / 2);

	cupwindata_t *windata = &(cup->windata[difficulty]);
	UINT8 emeraldnum = UINT8_MAX;

	if (!noemerald)
	{
		if (gamedata->sealedswaps[GDMAX_SEALEDSWAPS-1] != NULL // all found
		|| cup->id >= basenumkartcupheaders // custom content
		|| M_SecretUnlocked(SECRET_SPECIALATTACK, true)) // true order
		{
			// Standard order.
			if (windata->got_emerald == true)
			{
				emeraldnum = cup->emeraldnum;
			}
		}
		else
		{
			// Determine order from sealedswaps.
			UINT8 i;
			for (i = 0; (i < GDMAX_SEALEDSWAPS && gamedata->sealedswaps[i]); i++)
			{
				if (gamedata->sealedswaps[i] != cup)
					continue;

				break;
			}

			// If there's pending stars, get them from the associated cup order.
			if (i < GDMAX_SEALEDSWAPS)
			{
				cupheader_t *emeraldcup = kartcupheaders;
				while (emeraldcup)
				{
					if (emeraldcup->id >= basenumkartcupheaders)
					{
						emeraldcup = NULL;
						break;
					}

					if (emeraldcup->emeraldnum == i+1)
					{
						if (emeraldcup->windata[difficulty].got_emerald == true)
							emeraldnum = i+1;
						break;
					}

					emeraldcup = emeraldcup->next;
				}
			}
		}
	}

	if (windata->best_placement == 0)
	{
		if (statsmode)
		{
			V_DrawCharacter((10-4)/2 + rankx, ranky, '.' | V_GRAYMAP, false);
			rankx += 10 + 1;
			V_DrawCharacter((14-4)/2 + rankx, ranky, '.' | V_GRAYMAP, false);
			rankx += 14 + 1;
			V_DrawCharacter((12-4)/2 + rankx, ranky, '.' | V_GRAYMAP, false);
		}
		else
		{
			rankx += 14 + 1;
		}

		goto windataemeraldmaybe;
	}

	UINT8 *colormap = NULL;

	if (statsmode)
	{
		const char monitorletter = (cup->monitor < 10) ? ('0' + cup->monitor) : ('A' + (cup->monitor - 10));
		patch_t *monPat = W_CachePatchName(va("CUPMON%c%c", monitorletter, 'C'), PU_CACHE);
		UINT16 moncolor = SKINCOLOR_NONE;

		switch (windata->best_placement)
		{
			case 1:
				moncolor = SKINCOLOR_GOLD;
				break;
			case 2:
				moncolor = SKINCOLOR_SILVER;
				break;
			case 3:
				moncolor = SKINCOLOR_BRONZE;
				break;
			default:
				moncolor = SKINCOLOR_BEIGE;
				break;
		}

		if (moncolor != SKINCOLOR_NONE)
			colormap = R_GetTranslationColormap(TC_RAINBOW, moncolor, GTC_MENUCACHE);

		if (monPat)
			V_DrawFixedPatch((rankx)*FRACUNIT, (ranky)*FRACUNIT, FRACUNIT, 0, monPat, colormap);

		rankx += 10 + 1;

		colormap = NULL;
	}

	const gp_rank_e grade = windata->best_grade; // (cupgrid.previewanim/TICRATE) % (GRADE_S + 1); -- testing
	patch_t *gradePat = NULL;
	UINT16 gradecolor = K_GetGradeColor(grade);

	if (gradecolor != SKINCOLOR_NONE)
		colormap = R_GetTranslationColormap(TC_DEFAULT, gradecolor, GTC_MENUCACHE);

	switch (grade)
	{
		case GRADE_E:
			gradePat = W_CachePatchName("R_CUPRNE", PU_CACHE);
			break;
		case GRADE_D:
			gradePat = W_CachePatchName("R_CUPRND", PU_CACHE);
			break;
		case GRADE_C:
			gradePat = W_CachePatchName("R_CUPRNC", PU_CACHE);
			break;
		case GRADE_B:
			gradePat = W_CachePatchName("R_CUPRNB", PU_CACHE);
			break;
		case GRADE_A:
			gradePat = W_CachePatchName("R_CUPRNA", PU_CACHE);
			break;
		case GRADE_S:
			gradePat = W_CachePatchName("R_CUPRNS", PU_CACHE);
			break;
		default:
			break;
	}

	if (gradePat)
		V_DrawFixedPatch((rankx)*FRACUNIT, (ranky)*FRACUNIT, FRACUNIT, 0, gradePat, colormap);

	rankx += 14 + 1;

	patch_t *charPat = NULL;

	if ((windata->best_skin.unloaded != NULL)
		|| (windata->best_skin.id >= numskins))
	{
		colormap = NULL;

		charPat = W_CachePatchName("HUHMAP", PU_CACHE);
	}
	else
	{
		UINT16 skin = windata->best_skin.id;

		colormap = R_GetTranslationColormap(skin, skins[skin]->prefcolor, GTC_MENUCACHE);

		charPat = faceprefix[skin][FACE_MINIMAP];
	}

	if (charPat)
		V_DrawFixedPatch((rankx)*FRACUNIT, (ranky)*FRACUNIT, FRACUNIT, 0, charPat, colormap);

windataemeraldmaybe:

	rankx += 12 + 1;

	if (noemerald)
		;
	else if (emeraldnum != UINT8_MAX)
	{
		if (emeraldnum == 0)
			V_DrawCharacter(rankx+2, ranky+2, '+', false);
		else
		{
			colormap = NULL;

			if (!flash)
			{
				UINT16 col = SKINCOLOR_CHAOSEMERALD1 + (emeraldnum-1) % 7;

				colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);
			}

			const char *emname = va(
				"%sMAP%c",
				(emeraldnum > 7) ? "SUP" : "EME",
				colormap ? '\0' : 'B'
			);

			V_DrawFixedPatch((rankx)*FRACUNIT, (ranky)*FRACUNIT, FRACUNIT, 0, W_CachePatchName(emname, PU_HUDGFX), colormap);
		}
	}
	else if (statsmode)
	{
		V_DrawCharacter((12-4)/2 + rankx, ranky, '.' | V_GRAYMAP, false);
	}

	return rankw;
}

void M_DrawCup(cupheader_t *cup, fixed_t x, fixed_t y, INT32 lockedTic, boolean isTrophy, UINT8 placement)
{
	patch_t *patch = NULL;
	UINT8 *colormap = NULL;
	INT16 icony = 7;
	char status = 'A';
	char monitor = '0';

	if (isTrophy)
	{
		UINT16 col = SKINCOLOR_NONE;

		switch (placement)
		{
			case 0:
				break;
			case 1:
				col = SKINCOLOR_GOLD;
				status = 'B';
				break;
			case 2:
				col = SKINCOLOR_SILVER;
				status = 'B';
				break;
			case 3:
				col = SKINCOLOR_BRONZE;
				status = 'B';
				break;
			default:
				col = SKINCOLOR_BEIGE;
				break;
		}

		if (col != SKINCOLOR_NONE)
			colormap = R_GetTranslationColormap(TC_RAINBOW, col, GTC_MENUCACHE);
		else
			colormap = NULL;
	}

	if (cup == &dummy_lostandfound)
	{
		// No cup? Lost and found!
		monitor = '0';
	}
	else
	{
		if (cup->monitor < 10)
		{
			monitor = '0' + cup->monitor;

			if (monitor == '2')
			{
				icony = 5; // by default already 7px down, so this is really 2px further up
			}
			else if (monitor == '3')
			{
				icony = 6;
			}
		}
		else
		{
			monitor = 'A' + (cup->monitor - 10);
			if (monitor == 'X')
			{
				icony = 11;
			}
		}
	}

	patch = W_CachePatchName(va("CUPMON%c%c", monitor, status), PU_CACHE);
	V_DrawFixedPatch(x, y, FRACUNIT, 0, patch, colormap);

	if (cup == &dummy_lostandfound)
	{
		; // Only ever placed on the list if valid
	}
	else if (lockedTic != 0)
	{
		patch_t *st = W_CachePatchName(va("ICONST0%d", (lockedTic % 4) + 1), PU_CACHE);
		V_DrawFixedPatch(x + (8 * FRACUNIT), y + (icony * FRACUNIT), FRACUNIT, 0, st, NULL);
	}
	else
	{
		V_DrawFixedPatch(x + (8 * FRACUNIT), y + (icony * FRACUNIT), FRACUNIT, 0, W_CachePatchName(cup->icon, PU_CACHE), NULL);
		V_DrawFixedPatch(x + (8 * FRACUNIT), y + (icony * FRACUNIT), FRACUNIT, 0, W_CachePatchName("CUPBOX", PU_CACHE), NULL);
	}
}

void M_DrawCupSelect(void)
{
	UINT8 i, j, temp = 0;
	INT16 x, y;
	INT16 cy = M_EaseWithTransition(Easing_Linear, 5 * 30);
	cupwindata_t *windata = NULL;
	levelsearch_t templevelsearch = levellist.levelsearch; // full copy
	boolean isLocked;
	const boolean isGP = (templevelsearch.grandprix && (cv_dummygpdifficulty.value >= 0 && cv_dummygpdifficulty.value < KARTGP_MAX));
	const UINT8 numrows = (cupgrid.cache_secondrowlocked ? 1 : CUPMENU_ROWS);

	for (i = 0; i < CUPMENU_COLUMNS; i++)
	{
		x = 14 + (i*42);
		y = 20 - cy;
		if (cupgrid.cache_secondrowlocked == true)
			y += 28;

		for (j = 0; j < numrows; j++)
		{
			const size_t id = (i + (j * CUPMENU_COLUMNS)) + (cupgrid.pageno * (CUPMENU_COLUMNS * CUPMENU_ROWS));

			if (!cupgrid.builtgrid[id])
				break;

			templevelsearch.cup = cupgrid.builtgrid[id];

			if (isGP)
				windata = &templevelsearch.cup->windata[cv_dummygpdifficulty.value];

			isLocked = (M_GetFirstLevelInList(&temp, &templevelsearch) == NEXTMAP_INVALID);

			M_DrawCup(
				templevelsearch.cup,
				x * FRACUNIT, y * FRACUNIT,
				isLocked ? ((cupgrid.previewanim % 4) + 1) : 0,
				isGP,
				windata ? windata->best_placement : 0
			);

			if (!isGP || id == CUPMENU_CURSORID)
				;
			else if (isLocked)
			{
				if (templevelsearch.cup->hintcondition != MAXCONDITIONSETS
				&& M_Achieved(templevelsearch.cup->hintcondition))
				{
					// Super Cup tutorial hint.
					V_DrawScaledPatch(x + (32-10), y + (32-9), 0, W_CachePatchName("UN_HNT2A", PU_CACHE));
				}
			}
			else if (templevelsearch.cup == cupsavedata.cup
				&& id != CUPMENU_CURSORID)
			{
				// GP backup notif.
				V_DrawScaledPatch(x + 32, y + 32, 0, W_CachePatchName("CUPBKUP1", PU_CACHE));
			}

			// used to be 8 + (j*100) - (30*menutransition.tics)
			// but one-row mode means y has to be changed
			// this is the difference between y and that

			if (windata)
			{
				M_DrawCupWinData(
					x,
					y + (j ? 44 : -12),
					templevelsearch.cup,
					cv_dummygpdifficulty.value,
					(!cv_reducevfx.value && (cupgrid.previewanim & 1)),
					false
				);
			}

			y += 44;
		}
	}

	x = 14 + (cupgrid.x*42);
	y = 20 + (cupgrid.y*44) - cy;
	if (cupgrid.cache_secondrowlocked == true)
		y += 28;

	// Interpolated cursor
	{
		fixed_t tx = Easing_Linear(M_DueFrac(cupgrid.xslide.start, CUPMENU_SLIDETIME), cupgrid.xslide.dist * FRACUNIT, 0)/FRACUNIT;
		fixed_t ty = Easing_Linear(M_DueFrac(cupgrid.yslide.start, CUPMENU_SLIDETIME), cupgrid.yslide.dist * FRACUNIT, 0)/FRACUNIT;

		V_DrawScaledPatch((x - 4) - tx, (y - 1) - ty, 0, W_CachePatchName("CUPCURS", PU_CACHE));
	}

	templevelsearch.cup = cupgrid.builtgrid[CUPMENU_CURSORID];

	if (!isGP)
		;
	else if (M_GetFirstLevelInList(&temp, &templevelsearch) == NEXTMAP_INVALID)
	{
		if (templevelsearch.cup->hintcondition != MAXCONDITIONSETS
		&& M_Achieved(templevelsearch.cup->hintcondition))
		{
			// Super Cup tutorial hint.
			V_DrawScaledPatch(x + (32-10), y + (32-9), 0, W_CachePatchName("UN_HNT1A", PU_CACHE));
		}
	}
	else if (templevelsearch.cup != NULL
	&& templevelsearch.cup == cupsavedata.cup)
	{
		// GP backup hint.
		V_DrawScaledPatch(x + 32, y + 32, 0, W_CachePatchName("CUPBKUP2", PU_CACHE));
	}

	INT16 ty = M_EaseWithTransition(Easing_Linear, 5 * 24);
	y = 146 + ty;
	V_DrawFill(0, y, BASEVIDWIDTH, 54, 31);
	M_DrawCupPreview(y, &templevelsearch);

	M_DrawCupTitle(120 - ty, &templevelsearch);
	
	const char *worktext = "Undo";
	
	if (menuqueue.size)
		worktext = "Undo";
	else if (roundqueue.size)
		worktext = "Clear Queue";
	
	if (levellist.canqueue)
	{
		K_DrawGameControl(BASEVIDWIDTH/2, 6-ty, 0, va("%s Queue Cup<white>   %s %s",
			(templevelsearch.cup && templevelsearch.cup != &dummy_lostandfound && !roundqueue.size) ? "<z_animated>" : "<z_pressed><gray>",
			(roundqueue.size || menuqueue.size) ? "<c_animated>" : "<c_pressed><gray>",
		worktext), 1, TINY_FONT, 0);
	}

	if (templevelsearch.grandprix == false && templevelsearch.cup != NULL)
	{
		if (templevelsearch.cup != &dummy_lostandfound)
		{
			templevelsearch.checklocked = false;
		}

		// The following makes a HUGE assumption that we're
		// never going to have more than ~9 Race Courses
		// (with Medals) in Lost & Found. Which is almost
		// certainly true for Krew, but is very likely to
		// be violated by the long tail of modding. To those
		// finding this eventually: I'M SORRY ~toast 221024

		struct work_array_t {
			emblem_t *medal;
			UINT16 col;
			UINT16 dotcol;
		} work_array[CUPCACHE_MAX];

		boolean incj = false;

		i = j = 0;

		INT16 map = M_GetFirstLevelInList(&i, &templevelsearch);
		emblem_t *emblem = NULL;

		while (map < nummapheaders && j < CUPCACHE_MAX)
		{
			if (map < basenummapheaders)
			{
				emblem = NULL;
				incj = false;

				work_array[j].medal = NULL;
				work_array[j].col = work_array[j].dotcol = MCAN_INVALID;

				if (templevelsearch.timeattack)
				{
					emblem = M_GetLevelEmblems(map+1);

					while (emblem)
					{
						if (emblem->type == ET_TIME)
						{
							incj = true;

							if (gamedata->collected[emblem-emblemlocations])
							{
								if (!work_array[j].medal
								|| (
									(work_array[j].medal->type == ET_TIME)
									&& (work_array[j].medal->tag < emblem->tag)
									)
								)
								{
									work_array[j].medal = emblem;
								}
							}
						}
						else if ((emblem->type == ET_MAP)
							&& (emblem->flags & ME_SPBATTACK))
						{
							incj = true;

							if (gamedata->collected[emblem-emblemlocations])
							{
								work_array[j].dotcol = M_GetEmblemColor(emblem);
							}
						}

						emblem = M_GetLevelEmblems(-1);
					}
				}
				else if (mapheaderinfo[map]->typeoflevel & TOL_RACE)
				{
					incj = true;

					if (mapheaderinfo[map]->records.spraycan == MCAN_BONUS)
					{
						work_array[j].col = MCAN_BONUS;
					}
					else if (mapheaderinfo[map]->records.spraycan < gamedata->numspraycans)
					{
						work_array[j].col = gamedata->spraycans[mapheaderinfo[map]->records.spraycan].col;
					}

					if (mapheaderinfo[map]->records.mapvisited & MV_MYSTICMELODY)
					{
						work_array[j].dotcol = SKINCOLOR_TURQUOISE;
					}
				}

				if (incj)
					j++;
			}

			map = M_GetNextLevelInList(map, &i, &templevelsearch);
		}

		if (j)
		{
			x = (BASEVIDWIDTH - (j*10))/2 + 1;
			y += 2;

			V_DrawFill(x - 4, y, (j*10) + 6, 3, 31);
			for (i = 1; i <= 6; i++)
			{
				V_DrawFill(x + i - 4, y+2+i, (j*10) + 6 - (i*2), 1, 31);
			}

			y--;

			for (i = 0; i < j; i++)
			{
				if (templevelsearch.timeattack)
				{
					if (work_array[i].medal)
					{
						// Primary Medal

						V_DrawMappedPatch(x, y, 0, W_CachePatchName(M_GetEmblemPatch(work_array[i].medal, false), PU_CACHE),
							R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(work_array[i].medal), GTC_MENUCACHE));
					}
					else
					{
						// Need it!

						V_DrawScaledPatch(x, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
					}
				}
				else
				{
					if (work_array[i].col == MCAN_BONUS)
					{
						// Bonus in place of Spray Can

						V_DrawScaledPatch(x, y, 0, W_CachePatchName("GOTBON", PU_CACHE));
					}
					else if (work_array[i].col < numskincolors)
					{
						// Spray Can

						V_DrawMappedPatch(x, y, 0, W_CachePatchName("GOTCAN", PU_CACHE),
							R_GetTranslationColormap(TC_DEFAULT, work_array[i].col, GTC_MENUCACHE));
					}
					else
					{
						// Need it!

						V_DrawFill(x+3, y+3, 2, 2, 6);
					}
				}

				if (work_array[i].dotcol < numskincolors)
				{
					// Bonus (Secondary Medal or Ancient Shrine)

					V_DrawMappedPatch(x+4, y+7, 0, W_CachePatchName("COLORSP1", PU_CACHE),
						R_GetTranslationColormap(TC_DEFAULT, work_array[i].dotcol, GTC_MENUCACHE));
				}

				x += 10;
			}
		}
	}

	if (cupgrid.numpages > 1)
	{
		x = 3 - (skullAnimCounter/5);
		y = 20 + (44 - 1) - cy;

		patch_t *cuparrow = W_CachePatchName("CUPARROW", PU_CACHE);

		if (cupgrid.pageno != 0)
			V_DrawScaledPatch(x, y, 0, cuparrow);

		if (cupgrid.pageno != cupgrid.numpages-1)
			V_DrawScaledPatch(BASEVIDWIDTH-x, y, V_FLIP, cuparrow);
	}
}

static void M_DrawHighLowLevelTitle(INT16 x, INT16 y, INT16 map)
{
	char word1[22];
	char word2[22 + 2]; // actnum
	UINT8 word1len = 0;
	UINT8 word2len = 0;
	INT16 x2 = x;
	UINT8 i;

	if (!mapheaderinfo[map]
		|| (
			!mapheaderinfo[map]->menuttl[0]
			&& !mapheaderinfo[map]->lvlttl[0]
		)
	)
		return;

	if (!mapheaderinfo[map]->menuttl[0]
		&& mapheaderinfo[map]->zonttl[0])
	{
		boolean one = true;
		boolean two = true;

		for (i = 0; i < 22; i++)
		{
			if (!one && !two)
				break;

			if (mapheaderinfo[map]->lvlttl[i] && one)
			{
				word1[word1len] = mapheaderinfo[map]->lvlttl[i];
				word1len++;
			}
			else
				one = false;

			if (mapheaderinfo[map]->zonttl[i] && two)
			{
				word2[word2len] = mapheaderinfo[map]->zonttl[i];
				word2len++;
			}
			else
				two = false;
		}
	}
	else
	{
		char *ttlsource =
			mapheaderinfo[map]->menuttl[0]
			? mapheaderinfo[map]->menuttl
			: mapheaderinfo[map]->lvlttl;

		// If there are 2 or more words:
		// - Last word goes on word2
		// - Everything else on word1
		char *p = strrchr(ttlsource, ' ');
		if (p)
		{
			word2len = strlen(p + 1);
			memcpy(word2, p + 1, word2len);
		}
		else
			p = ttlsource + strlen(ttlsource);

		word1len = p - ttlsource;
		memcpy(word1, ttlsource, word1len);
	}

	if (!mapheaderinfo[map]->menuttl[0] && mapheaderinfo[map]->actnum)
	{
		word2[word2len] = ' ';
		word2len++;

		word2[word2len] = '0' + mapheaderinfo[map]->actnum;
		word2len++;
	}

	word1[word1len] = '\0';
	word2[word2len] = '\0';

	{
		char addlen[3];

		for (i = 0; i < 2; i++)
		{
			if (!word1[i])
				break;

			addlen[i] = word1[i];
		}

		addlen[i] = '\0';
		x2 += V_LSTitleLowStringWidth(addlen, 0);
	}


	if (word1len)
		V_DrawLSTitleHighString(x, y, 0, word1);
	if (word2len)
		V_DrawLSTitleLowString(x2, y+28, 0, word2);
}

static INT32 M_DrawMapMedals(INT32 mapnum, INT32 x, INT32 y, boolean allowtime, boolean allowencore, boolean allowspb, boolean allowbonus, boolean draw);

static void M_DrawLevelSelectBlock(INT16 x, INT16 y, UINT16 map, boolean redblink, boolean greyscale)
{
	UINT8 *colormap = NULL;

	if (greyscale)
		colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE);

	if (redblink)
		V_DrawScaledPatch(3+x, y, 0, W_CachePatchName("LVLSEL2", PU_CACHE));
	else
		V_DrawScaledPatch(3+x, y, 0, W_CachePatchName("LVLSEL", PU_CACHE));

	K_DrawMapThumbnail(
		(9+x)<<FRACBITS, (y+6)<<FRACBITS,
		80<<FRACBITS,
		0,
		map,
		colormap);
	M_DrawHighLowLevelTitle(98+x, y+8, map);

	if (levellist.levelsearch.tutorial && !(mapheaderinfo[map]->records.mapvisited & MV_BEATEN))
	{
		V_DrawScaledPatch(
			x + 80 + 3, y + 50, 0,
			W_CachePatchName(
				va(
					"CUPBKUP%c",
					(greyscale ? '1' : '2')
				),
				PU_CACHE
			)
		);
	}
	else if (!levellist.netgame)
	{
		x += 80+2;
		y += 50-1;

		const boolean allowencore = (
			!levellist.levelsearch.timeattack
			&& M_SecretUnlocked(SECRET_ENCORE, true)
		);
		const boolean allowspb = (
			levellist.levelsearch.timeattack
			&& M_SecretUnlocked(SECRET_SPBATTACK, true)
		);

		INT32 width = x - M_DrawMapMedals(map, x, y,
			levellist.levelsearch.timeattack,
			allowencore,
			allowspb,
			!levellist.levelsearch.timeattack,
			false
		);

		if (width > 2)
		{
			width -= 2; // minor poke

			V_DrawFill(x + 7 - width, y - 2, width, 9, 31);

			UINT8 i;
			for (i = 1; i < 7; i++)
			{
				V_DrawFill(x + 7 - width - i, y + i - 2, 1, 9 - i, 31);
			}

			M_DrawMapMedals(map, x, y,
				levellist.levelsearch.timeattack,
				allowencore,
				allowspb,
				!levellist.levelsearch.timeattack,
				true
			);
		}
	}
}

void M_DrawLevelSelect(void)
{
	INT16 i = 0;
	UINT8 j = 0;
	INT16 map = M_GetFirstLevelInList(&j, &levellist.levelsearch);
	INT16 t = M_EaseWithTransition(Easing_Linear, 5 * 64), tay = 0;
	INT16 y = 80 - levellist.y +
		Easing_OutSine(
			M_DueFrac(levellist.slide.start, 4),
			levellist.slide.dist,
			0
		);
	boolean tatransition = ((menutransition.startmenu == &PLAY_TimeAttackDef || menutransition.endmenu == &PLAY_TimeAttackDef) && menutransition.tics != menutransition.dest);

	if (tatransition)
	{
		t = -t;
		tay = t/2;
	}

	while (true)
	{
		INT16 lvlx = t, lvly = y;

		if (map >= nummapheaders)
			break;

		if (i == levellist.cursor && tatransition)
		{
			lvlx = 0;
			lvly = max(2, y+tay);
		}

		M_DrawLevelSelectBlock(lvlx, lvly, map,
			(i == levellist.cursor && (((skullAnimCounter / 4) & 1) || tatransition)),
			(i != levellist.cursor)
		);

		y += 72;
		i++;
		map = M_GetNextLevelInList(map, &j, &levellist.levelsearch);
	}

	M_DrawCupTitle(tay, &levellist.levelsearch);

	t = (abs(t)/2) + BASEVIDWIDTH - 4;
	tay += 30;

	const boolean queuewithinsize = (menuqueue.size + roundqueue.size < ROUNDQUEUE_MAX);

	const char *worktext = "Go to Course";
	if (levellist.levelsearch.timeattack)
		worktext = "Select Course";
	else if (levellist.canqueue && menuqueue.size && queuewithinsize)
		worktext = roundqueue.size ? "Add to Queue" : "Start Queue";

	if (worktext)
	{
		K_DrawGameControl(t, tay, 0, va("<a_animated> %s", worktext), 2, TINY_FONT, 0);
		tay += 13;
	}

	if (levellist.canqueue)
	{
		worktext = queuewithinsize
			? "<z_animated>" : "<z_pressed><gray>";
		K_DrawGameControl(t, tay, 0, va("%s Queue Course", worktext), 2, TINY_FONT, 0);
		tay += 13;

		worktext = NULL;
		if (menuqueue.size)
			worktext = "Undo";
		else if (roundqueue.size)
			worktext = "Clear Queue";

		if (worktext)
		{
			K_DrawGameControl(t, tay, 0, va("<c_animated> %s", worktext), 2, TINY_FONT, 0);
			tay += 13;
		}
	}
}

static boolean M_LevelSelectHasBG(menu_t *check)
{
	if (check == NULL)
		check = currentMenu;

	return (check == &PLAY_LevelSelectDef
	|| check == &PLAY_CupSelectDef);
}

static boolean M_LevelSelectToTimeAttackTransitionHelper(void)
{
	return (M_LevelSelectHasBG(menutransition.startmenu))
		!= M_LevelSelectHasBG(menutransition.endmenu);
}

void M_DrawSealedBack(void)
{
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (M_LevelSelectHasBG(currentMenu) == false)
		return;

	INT32 translucencylevel = 7;
	if (M_LevelSelectToTimeAttackTransitionHelper())
	{
		translucencylevel += menutransition.tics/3;

		if (translucencylevel >= 10)
			return;
	}

	V_DrawFixedPatch(
		0, 0,
		FRACUNIT,
		translucencylevel << V_ALPHASHIFT,
		W_CachePatchName("MENUI008", PU_CACHE),
		R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_BLACK, GTC_CACHE)
	);
}

void M_DrawTimeAttack(void)
{
	UINT16 map = levellist.choosemap;
	INT16 t = M_EaseWithTransition(Easing_Linear, 5 * 48);
	INT16 leftedge = 149+t+16;
	INT16 rightedge = 149+t+155;
	INT16 opty = 140;
	INT32 w;
	UINT8 i;

	V_DrawScaledPatch(149+t, 70, 0, W_CachePatchName("BESTTIME", PU_CACHE));

	if (!mapheaderinfo[map])
	{
		V_DrawRightAlignedMenuString(rightedge-12, opty, 0, "No map!?");
		return;
	}

	{
		patch_t *minimap = NULL;
		INT32 minimapx = 76, minimapy = 130;

		if (M_LevelSelectToTimeAttackTransitionHelper())
			minimapx -= t;

		if (levellist.newgametype == GT_SPECIAL)
		{
			// Star Within The Seal

#define SEAL_PULSELEN ((6*TICRATE)/5) // The rate of O_SSTAR3
			INT32 crossfade = (timeattackmenu.ticker % (2*SEAL_PULSELEN)) - SEAL_PULSELEN;
			if (crossfade < 0)
				crossfade = -crossfade;
			crossfade = (crossfade * 10)/SEAL_PULSELEN;
#undef SEAL_PULSELEN

			if (crossfade != 10)
			{
				minimap = W_CachePatchName(
					"K_FINB05",
					PU_CACHE
				);

				V_DrawScaledPatch(
					minimapx, minimapy,
					0, minimap
				);
			}

			if (crossfade != 0)
			{
				minimap = W_CachePatchName(
					"K_FINB04",
					PU_CACHE
				);

				V_DrawScaledPatch(
					minimapx, minimapy,
					(10-crossfade)<<V_ALPHASHIFT,
					minimap
				);
			}
		}
		else if (levellist.newgametype == GT_VERSUS)
		{
			const INT32 teaserh = 56;
			minimapy -= 1; // tiny adjustment

			V_DrawScaledPatch(
				minimapx, minimapy - teaserh,
				V_TRANSLUCENT, W_CachePatchName("K_TEASR2", PU_CACHE)
			);
			V_DrawScaledPatch(
				minimapx, minimapy + teaserh,
				V_TRANSLUCENT, W_CachePatchName("K_TEASR4", PU_CACHE)
			);
			V_DrawScaledPatch(
				minimapx, minimapy,
				0, W_CachePatchName("K_TEASR3", PU_CACHE)
			);
		}
		else if ((minimap = mapheaderinfo[map]->minimapPic))
		{
			V_DrawScaledPatch(
				minimapx - (SHORT(minimap->width)/2),
				minimapy - (SHORT(minimap->height)/2),
				0, minimap
			);
		}
	}

	if (currentMenu == &PLAY_TimeAttackDef)
	{
		recorddata_t *rcp = &mapheaderinfo[map]->records;
		recordtimes_t *record = cv_dummyspbattack.value ? &rcp->spbattack : &rcp->timeattack;
		tic_t timerec = record->time;
		tic_t laprec = record->lap;
		UINT32 timeheight = 82;

		if ((gametypes[levellist.newgametype]->rules & GTR_CIRCUIT)
			&& (mapheaderinfo[map]->numlaps != 1))
		{
			V_DrawRightAlignedMenuString(rightedge-12, timeheight, M_ALTCOLOR, "BEST LAP:");
			K_drawKartTimestamp(laprec, 162+t, timeheight+6, 0, 2);
			timeheight += 30;
		}
		else
		{
			timeheight += 15;
		}

		V_DrawRightAlignedMenuString(rightedge-12, timeheight, M_ALTCOLOR, "BEST TIME:");
		K_drawKartTimestamp(timerec, 162+t, timeheight+6, 0, 1);

		// SPB Attack control hint + menu overlay
		{
			INT32 buttonx = 162 + t;
			INT32 buttony = timeheight;

			if (M_EncoreAttackTogglePermitted())
			{
				K_DrawGameControl(buttonx + 35, buttony - 3, 0, "<r_animated>", 0, MENU_FONT, 0);
			}

			if ((timeattackmenu.spbflicker == 0 || timeattackmenu.ticker % 2) == (cv_dummyspbattack.value == 1))
			{
				V_DrawMappedPatch(buttonx + 7, buttony - 1, 0, W_CachePatchName("K_SPBATK", PU_CACHE), R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_RED, GTC_MENUCACHE));
			}
		}
	}
	else
		opty = 80;

	// Done after to overlay material
	M_DrawLevelSelectBlock(0, 2, map, true, false);

	for (i = 0; i < currentMenu->numitems; i++)
	{
		UINT32 f = (i == itemOn) ? highlightflags : 0;

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{

			case IT_HEADERTEXT:

				V_DrawMenuString(leftedge, opty, M_ALTCOLOR, currentMenu->menuitems[i].text);
				opty += 10;
				break;

			case IT_STRING:

				if (i >= currentMenu->numitems-1)
				{
					V_DrawRightAlignedMenuString(rightedge, opty, f, currentMenu->menuitems[i].text);

					if (i == itemOn)
						M_DrawCursorHand(rightedge - V_MenuStringWidth(currentMenu->menuitems[i].text, 0), opty);
				}
				else
				{
					V_DrawMenuString(leftedge, opty, f, currentMenu->menuitems[i].text);

					if (i == itemOn)
						M_DrawCursorHand(leftedge, opty);
				}
				opty += 10;

				// Cvar specific handling
				{
					const char *str = NULL;
					INT32 optflags = f;
					boolean drawarrows = (i == itemOn);

					if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_ARROWS)
					{
						// Currently assumes M_HandleStaffReplay
						if (mapheaderinfo[levellist.choosemap]->ghostCount <= 1)
							drawarrows = false;

						if (mapheaderinfo[levellist.choosemap] == NULL)
							str = "Invalid map";
						else if (cv_dummystaff.value > mapheaderinfo[levellist.choosemap]->ghostCount)
							str = va("%u - Invalid ID", cv_dummystaff.value+1);
						else if (mapheaderinfo[levellist.choosemap]->ghostBrief[cv_dummystaff.value] == NULL)
							str = va("%u - Invalid brief", cv_dummystaff.value+1);
						else
						{
							const char *th = "th";
							if (cv_dummystaff.value+1 == 1)
								th = "st";
							else if (cv_dummystaff.value+1 == 2)
								th = "nd";
							else if (cv_dummystaff.value+1 == 3)
								th = "rd";
							str = va("%u%s - %s",
								cv_dummystaff.value+1,
								th,
								mapheaderinfo[levellist.choosemap]->ghostBrief[cv_dummystaff.value]->name
							);
						}
					}
					else if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_CVAR)
					{
						str = currentMenu->menuitems[i].itemaction.cvar->string;
					}

					if (str)
					{
						w = V_MenuStringWidth(str, optflags);
						V_DrawMenuString(leftedge+12, opty, optflags, str);
						if (drawarrows)
						{
							V_DrawMenuString(leftedge+12 - 10 - (skullAnimCounter/5), opty, f, "\x1C"); // left arrow
							V_DrawMenuString(leftedge+12 + w + 2+ (skullAnimCounter/5), opty, f, "\x1D"); // right arrow
						}
						opty += 10;
					}
				}

				break;
			case IT_SPACE:
				opty += 4;
				break;
		}
	}
}

// This draws the options of a given menu in a fashion specific to the multiplayer option select screen (host game / server browser etc)
// First argument is the menu to draw the options from, the 2nd one is a table that contains which option is to be "extendded"
// Format for each option: {extended? (only used by the ticker), max extension (likewise),  # of lines to extend}

// NOTE: This is pretty rigid and only intended for use with the multiplayer options menu which has *3* choices.

void M_DrawMasterServerReminder(void)
{
	// Did you change the Server Browser address? Have a little reminder.

	INT32 mservflags = 0;
	if (CV_IsSetToDefault(&cv_masterserver))
		mservflags = highlightflags;
	else
		mservflags = warningflags;

	INT32 y = BASEVIDHEIGHT - 10;

	V_DrawFadeFill(0, y-1, BASEVIDWIDTH, 10+1, 0, 31, 5);
	V_DrawCenteredThinString(BASEVIDWIDTH/2, y,
		mservflags, va("List via \"%s\"", cv_masterserver.string));
}

static void M_MPOptDrawer(menu_t *m, INT16 extend[3][3])
{
	// This is a copypaste of the generic gamemode menu code with a few changes.
	// TODO: Allow specific options to "expand" into smaller ones.

	patch_t *buttback = W_CachePatchName("M_PLAT2", PU_CACHE);
	INT32 i, x = 132, y = 32;	// Dirty magic numbers for now but they work out.

	for (i = 0; i < m->numitems; i++)
	{

		switch (m->menuitems[i].status & IT_DISPLAY)
		{
			case IT_STRING:
				{
					UINT8 *colormap = NULL;
					INT16 j = 0;

					if ((currentMenu == m && i == itemOn) || extend[i][0])	// Selected / unfolded
					{
						colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);
					}
					else
					{
						colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_MOSS, GTC_CACHE);
					}

					if (extend[i][2])
					{
						for (j=0; j <= extend[i][2]/2; j++)
						{
							// Draw rectangles that look like the current selected item starting from the top of the actual selection graphic and going up to where it's supposed to go.
							// With colour 169 (that's the index of the shade of black the plague colourization gives us. ...No I don't like using a magic number either.
							V_DrawFill((x-1) + (extend[i][2]/2) - j - (buttback->width/2), (y + extend[i][2]) - (2*j), 226, 2, 169);
						}
					}
					V_DrawFixedPatch((x + (extend[i][2]/2)) *FRACUNIT, (y + extend[i][2])*FRACUNIT, FRACUNIT, 0, buttback, colormap);
					V_DrawCenteredGamemodeString(x, y - 3, 0, colormap, m->menuitems[i].text);
				}
				break;
		}

		x += GM_XOFFSET;
		y += GM_YOFFSET + extend[i][2];
	}
}

// Draws the EGGA CHANNEL background.
void M_DrawEggaChannelAlignable(boolean centered)
{
	patch_t *background = W_CachePatchName("M_EGGACH", PU_CACHE);

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 25);
	V_DrawFixedPatch((!centered ? 75 : 160)<<FRACBITS, 104<<FRACBITS, FRACUNIT, 0, background, NULL);
	V_DrawVhsEffect(false);	// VHS the background! (...sorry OGL my love)
}

void M_DrawEggaChannel(void)
{
	M_DrawEggaChannelAlignable(false);
}

// Multiplayer mode option select
void M_DrawMPOptSelect(void)
{
	M_DrawMenuTooltips();
	M_MPOptDrawer(&PLAY_MP_OptSelectDef, mpmenu.modewinextend);
	M_DrawMasterServerReminder();
}

// Multiplayer mode option select: HOST GAME!
// A rehash of the generic menu drawer adapted to fit into that cramped space. ...A small sacrifice for utility
void M_DrawMPHost(void)
{
	patch_t *gobutt = W_CachePatchName("M_BUTTGO", PU_CACHE);	// I'm very mature
	INT32 xp = 40, yp = 64, i = 0, w = 0;	// Starting position for the text drawing.

	M_DrawMPOptSelect();	// Draw the Multiplayer option select menu first

	// Now draw our host options...
	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == currentMenu->numitems-1)
		{
			xp = 202;
			yp = 100;

			UINT8 *colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_MOSS, GTC_CACHE);
			if (i == itemOn)
				colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

			// Ideally we'd calculate this but it's not worth it for a 1-off menu probably.....
			V_DrawFixedPatch(xp<<FRACBITS, yp<<FRACBITS, FRACUNIT, 0, gobutt, colormap);
			V_DrawCenteredGamemodeString(xp + (gobutt->width/2), yp -3, 0, colormap, currentMenu->menuitems[i].text);
		}
		else
		{
			switch (currentMenu->menuitems[i].status & IT_DISPLAY)
			{
				case IT_TRANSTEXT2:
				{
					V_DrawThinString(xp, yp, V_TRANSLUCENT, currentMenu->menuitems[i].text);
					xp += 5;
					yp += 11;
					break;
				}
				case IT_STRING:
				{
					V_DrawThinString(xp, yp, (i == itemOn ? highlightflags : 0), currentMenu->menuitems[i].text);

					// Cvar specific handling
					switch (currentMenu->menuitems[i].status & IT_TYPE)
					{
						case IT_CVAR:
						{
							consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
							switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
							{
								case IT_CV_STRING:
									{
										INT32 xoffs = 0;
										if (itemOn == i)
										{
											xoffs += 8;
											V_DrawString(xp + (skullAnimCounter/5) + 94, yp+1, highlightflags, "\x1D");
										}

										V_DrawThinString(xp + xoffs + 96, yp, 0, cv->string);
									}

									break;

								default:
									w = V_ThinStringWidth(cv->string, 0);
									V_DrawThinString(xp + 138 - w, yp, ((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? warningflags : highlightflags), cv->string);
									if (i == itemOn)
									{
										V_DrawCharacter(xp + 138 - 10 - w - (skullAnimCounter/5), yp, '\x1C' | highlightflags, false); // left arrow
										V_DrawCharacter(xp + 138 + 2 + (skullAnimCounter/5), yp, '\x1D' | highlightflags, false); // right arrow
									}
									break;
							}
							break;
						}
						case IT_ARROWS:
						{
							if (currentMenu->menuitems[i].itemaction.routine != M_HandleHostMenuGametype)
								break;

							w = V_ThinStringWidth(gametypes[menugametype]->name, 0);
							V_DrawThinString(xp + 138 - w, yp, highlightflags, gametypes[menugametype]->name);
							if (i == itemOn)
							{
								V_DrawCharacter(xp + 138 - 10 - w - (skullAnimCounter/5), yp, '\x1C' | highlightflags, false); // left arrow
								V_DrawCharacter(xp + 138 + 2 + (skullAnimCounter/5), yp, '\x1D' | highlightflags, false); // right arrow
							}
							break;
						}
					}

					xp += 5;
					yp += 11;

					break;
				}
			break;
			}

		}

	}
}

// Multiplayer mode option select: JOIN BY IP
// Once again we'll copypaste 1 bit of the generic menu handler we used for hosting but we only need it for IT_CV_STRING since that's all we got :)
// (I don't like duplicating code but I rather this than some horrible all-in-one function with too many options...)
void M_DrawMPJoinIP(void)
{
	//patch_t *minibutt = W_CachePatchName("M_SBUTT", PU_CACHE);
	// There is no such things as mini butts, only thick thighs to rest your head on.
	//patch_t *minigo = W_CachePatchName("M_SGO", PU_CACHE);
	patch_t *typebar = W_CachePatchName("M_TYPEB", PU_CACHE);

	//UINT8 *colormap = NULL;
	UINT8 *colormapc = NULL;

	INT32 xp = 73, yp = 133, i = 0;	// Starting position for the text drawing.
	M_DrawMPOptSelect();	// Draw the Multiplayer option select menu first


	// Now draw our host options...
	for (i = 0; i < currentMenu->numitems; i++)
	{

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_STRING:
			{

				char str[MAX_LOGIP];
				strcpy(str, currentMenu->menuitems[i].text);

				// The last 3 options of this menu are to be the joined IP addresses...
				if (currentMenu->numitems - i <= NUMLOGIP)
				{
					UINT8 index = NUMLOGIP - (currentMenu->numitems - i);
					if (index == 0)
					{
						xp += 8;
					}

					if (joinedIPlist[index][1][0])	// Try drawing server name
						strlcpy(str, joinedIPlist[index][1], MAX_LOGIP);
					else if (joinedIPlist[index][0][0])	// If that fails, get the address
						strlcpy(str, joinedIPlist[index][0], MAX_LOGIP);
					else
						strcpy(str, "---");		// If that fails too then there's nothing!
				}

				V_DrawThinString(xp, yp, ((i == itemOn || currentMenu->menuitems[i].status & IT_SPACE) ? highlightflags : 0), str);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
				{
					case IT_CVAR:
					{
						consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_STRING:

								//colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_MOSS, GTC_CACHE);
								colormapc = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

								V_DrawFixedPatch((xp + 12)<<FRACBITS, (yp-2)<<FRACBITS, FRACUNIT, 0, typebar, colormapc);	// Always consider that this is selected otherwise it clashes.

								{
									INT32 xoffs = 0;
									if (itemOn == i)
									{
										xoffs += 8;
										V_DrawString(xp + (skullAnimCounter/5) + 17, yp+1, highlightflags, "\x1D");
									}

									V_DrawThinString(xp + xoffs + 18, yp, 0, cv->string);
								}

								/*// On this specific menu the only time we'll ever see this is for the connect by IP typefield.
								// Draw the small GO button here (and the text which is a separate graphic)
								V_DrawFixedPatch((xp + 20 + typebar->width -4)<<FRACBITS, (yp-3)<<FRACBITS, FRACUNIT, 0, minibutt, i == itemOn ? colormapc : colormap);
								V_DrawFixedPatch((xp + 20 + typebar->width -4 + (minibutt->width/2))<<FRACBITS, (yp-3-5)<<FRACBITS, FRACUNIT, 0, minigo, i == itemOn ? colormapc : NULL);*/

								break;

							default:
								break;
						}
						break;
					}
				}

				xp += 5;
				yp += 11;

				break;
			}
		break;
		}

	}
}

// Multiplayer room select
void M_DrawMPRoomSelect(void)
{
	// Greyscale colormaps for the option that's not selected's background
	UINT8 *colormap_l = NULL;
	UINT8 *colormap_r = NULL;

	patch_t *bg_l = W_CachePatchName("BG_MPS21", PU_CACHE);
	patch_t *bg_r = W_CachePatchName("BG_MPS22", PU_CACHE);

	patch_t *split = W_CachePatchName("MPSPLIT1", PU_CACHE);

	patch_t *butt1[] = {W_CachePatchName("MP_B1", PU_CACHE), W_CachePatchName("MP_B12", PU_CACHE)};
	patch_t *butt2[] = {W_CachePatchName("MP_B2", PU_CACHE), W_CachePatchName("MP_B22", PU_CACHE)};

	patch_t *scrollp[] = {W_CachePatchName("MP_SCR1", PU_CACHE), W_CachePatchName("MP_SCR2", PU_CACHE)};
	patch_t *drawp = scrollp[mpmenu.room];

	fixed_t scrollposx[] = {(BASEVIDWIDTH/4)<<FRACBITS, (BASEVIDWIDTH/2 + BASEVIDWIDTH/4)<<FRACBITS};
	UINT8 i;
	INT32 soffy = 0;

	if (mpmenu.room)
		colormap_l = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_CACHE);
	else
		colormap_r = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_CACHE);

	// Draw the 2 sides of the background
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, bg_l, colormap_l);
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, bg_r, colormap_r);


	// Draw the black split:
	V_DrawFixedPatch(160<<FRACBITS, 0, FRACUNIT, 0, split, NULL);

	// Vertical scrolling stuff
	for (i = 0; i < 3; i++)
	{
		V_DrawFixedPatch(scrollposx[mpmenu.room], ((-((mpmenu.ticker*2) % drawp->height)) + soffy)*FRACUNIT , FRACUNIT, V_ADD, drawp, NULL);
		soffy += scrollp[mpmenu.room]->height;
	}


	// Draw buttons:

	V_DrawFixedPatch(160<<FRACBITS, 90<<FRACBITS, FRACUNIT, mpmenu.room ? (5<<V_ALPHASHIFT) : 0, butt1[(mpmenu.room) ? 1 : 0], NULL);

	V_DrawFixedPatch(160<<FRACBITS, 90<<FRACBITS, FRACUNIT, (!mpmenu.room) ? (5<<V_ALPHASHIFT) : 0, butt2[(!mpmenu.room) ? 1 : 0], NULL);

	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);

	V_DrawCenteredMenuString(BASEVIDWIDTH/2, 24, 0, "\xA3  Select a Room  \xA2");

	V_DrawCenteredThinString(BASEVIDWIDTH/2, 12, 0, (mpmenu.room) ? "Play with community maps, characters, and gametypes. (Expect additional downloads!)" : "Jump into a standard game of Ring Racers.");

	M_DrawMasterServerReminder();
}

// SERVER BROWSER
static void M_DrawServerCountAndHorizontalBar(void)
{
	const char *text;
	INT32 y = currentMenu->y+STRINGHEIGHT;

	const char throbber[4] = {'-', '\\', '|', '/'};
	UINT8 throbindex = (mpmenu.ticker/4) % 4;

	switch (M_GetWaitingMode())
	{
		case M_WAITING_VERSION:
			text = "Checking for updates...";
			break;

		case M_WAITING_SERVERS:
			text = "Loading server list...";
			break;

		default:
			if (serverlistultimatecount > serverlistcount)
			{
				text = va("%d/%d server%s found...",
						serverlistcount,
						serverlistultimatecount,
						serverlistultimatecount == 1 ? "" : "s"
				);
			}
			else
			{
				throbindex = UINT8_MAX; // No throbber!
				text = va("%d server%s found",
					serverlistcount,
					serverlistcount == 1 ? "" : "s"
				);
			}
	}

	if (throbindex == UINT8_MAX)
	{
		V_DrawRightAlignedMenuString(
			BASEVIDWIDTH - currentMenu->x,
			y,
			highlightflags,
			text
		);
	}
	else
	{
		V_DrawRightAlignedMenuString(
			BASEVIDWIDTH - currentMenu->x - 12, y,
			highlightflags,
			text
		);

		V_DrawCenteredString( // Only clean way to center the throbber without exposing character width
			BASEVIDWIDTH - currentMenu->x - 4, y,
			highlightflags,
			va("%c", throbber[throbindex])
		);
	}
}

void M_DrawMPServerBrowser(void)
{
	const char *header[3][2] = {
		{"Server Browser", "BG_MPS1"},
		{"Core Servers", "BG_MPS1"},
		{"Modded Servers", "BG_MPS2"},
	};
	int mode = M_SecretUnlocked(SECRET_ADDONS, true) ? (mpmenu.room ? 2 : 1) : 0;

	patch_t *text1 = W_CachePatchName("MENUBGT1", PU_CACHE);
	patch_t *text2 = W_CachePatchName("MENUBGT2", PU_CACHE);

	UINT8 i;

	patch_t *servpats[3];
	patch_t *gearpats[3];
	for (i = 0; i < 3; i++)
	{
		servpats[i] = W_CachePatchName(va("M_SERV%c", i + '1'), PU_CACHE);
		gearpats[i] = W_CachePatchName(va("M_SGEAR%c", i + '1'), PU_CACHE);
	}
	patch_t *voicepat;
	voicepat = W_CachePatchName("VOCRMU", PU_CACHE);

	fixed_t text1loop = SHORT(text1->height)*FRACUNIT;
	fixed_t text2loop = SHORT(text2->width)*FRACUNIT;

	const UINT8 startx = 18;
	const UINT8 basey = 56;
	const INT32 starty = basey - 18*mpmenu.scrolln + mpmenu.slide;
	INT32 ypos = 0;

	// background stuff
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName(header[mode][1], PU_CACHE), NULL);

	V_DrawFixedPatch(0, (BASEVIDHEIGHT + 16) * FRACUNIT, FRACUNIT, V_TRANSLUCENT, W_CachePatchName("MENUBG2", PU_CACHE), NULL);

	V_DrawFixedPatch(-bgText2Scroll, (BASEVIDHEIGHT-8) * FRACUNIT,
		FRACUNIT, V_TRANSLUCENT, text2, NULL);
	V_DrawFixedPatch(-bgText2Scroll + text2loop, (BASEVIDHEIGHT-8) * FRACUNIT,
		FRACUNIT, V_TRANSLUCENT, text2, NULL);

	V_DrawFixedPatch(8 * FRACUNIT, -bgText1Scroll,
		FRACUNIT, V_TRANSLUCENT, text1, NULL);
	V_DrawFixedPatch(8 * FRACUNIT, -bgText1Scroll + text1loop,
		FRACUNIT, V_TRANSLUCENT, text1, NULL);

	// bgText<x>Scroll is already handled by the menu background.

	// the actual server list.
	for (i = 0; i < serverlistcount; i++)
	{
		INT32 transflag = 0;
		INT32 basetransflag = 0;

		if (serverlist[i].info.numberofplayer >= serverlist[i].info.maxplayer)
			basetransflag = 5;

		/*if (i < mpmenu.servernum)
			// if slide > 0, then we went DOWN in the list and have to fade that server out.
			if (mpmenu.slide > 0)
				transflag = basetransflag + (18-mpmenu.slide)/2;
			else if (mpmenu.slide < 0)
				transflag = 10 - basetransflag - (18-mpmenu.slide)/2;*/

		transflag = basetransflag;

		if (transflag >= 0 && transflag < 10)
		{
			transflag = transflag << V_ALPHASHIFT;	// shift the translucency flag.

			if (serverlist[i].cachedgtcalc < 3)
			{
				patch_t *focus;
				if (itemOn == 2 && mpmenu.servernum == i)
				{
					focus = W_CachePatchName(va("M_SERH%c", serverlist[i].cachedgtcalc + '1'), PU_CACHE);
				}
				else
				{
					focus = servpats[serverlist[i].cachedgtcalc];
				}

				V_DrawFixedPatch(startx*FRACUNIT, (starty + ypos)*FRACUNIT, FRACUNIT, transflag, focus, NULL);
			}

			// Server name:
			V_DrawThinString(startx+11, starty + ypos + 6, transflag, serverlist[i].info.servername);

			// Ping:
			V_DrawThinString(startx + 191, starty + ypos + 7, transflag, va("%03d", serverlist[i].info.time));

			// Playercount
			V_DrawThinString(startx + 214, starty + ypos + 7, transflag, va("%02d/%02d", serverlist[i].info.numberofplayer, serverlist[i].info.maxplayer));

			const char *pwrtext;
			if (serverlist[i].cachedgtcalc == GTCALC_CUSTOM)
			{
				// Show custom gametype name
				// (custom PWR is not available, and this is the best place to show the name)
				pwrtext = serverlist[i].info.gametypename;
			}
			else if (serverlist[i].info.avgpwrlv != -1)
			{
				// Power Level
				pwrtext = va("%04d Pwr", serverlist[i].info.avgpwrlv);
			}
			else
			{
				// Fallback
				pwrtext = "No Pwr";
			}
			V_DrawRightAlignedThinString(startx + 276, starty + ypos, transflag, pwrtext);

			// game speed if applicable:
			if (serverlist[i].cachedgtcalc != GTCALC_BATTLE)
			{
				UINT8 speed = serverlist[i].info.kartvars & SV_SPEEDMASK;

				if (speed < 3)
				{
					V_DrawFixedPatch((startx + 251)*FRACUNIT, (starty + ypos + 9)*FRACUNIT, FRACUNIT, transflag, gearpats[speed], NULL);
				}
			}

			// voice chat enabled
			if (serverlist[i].info.kartvars & SV_VOICEENABLED)
			{
				V_DrawFixedPatch((startx - 3) * FRACUNIT, (starty + ypos + 2) * FRACUNIT, FRACUNIT, 0, voicepat, NULL);
			}
		}
		ypos += SERVERSPACE;
	}

	// Draw genericmenu ontop!
	V_DrawFill(0, 0, 320, 52, 31);
	V_DrawFill(0, 53, 320, 1, 31);
	V_DrawFill(0, 55, 320, 1, 31);

	V_DrawCenteredGamemodeString(160, 2, 0, 0, header[mode][0]);

	// normal menu options
	M_DrawGenericMenu();

	// And finally, the overlay bar!
	M_DrawServerCountAndHorizontalBar();
	M_DrawMasterServerReminder();
}

// OPTIONS MENU

// Draws the cogs and also the options background!
void M_DrawOptionsCogs(void)
{
	boolean eggahack = (
		currentMenu->prevMenu == &PLAY_MP_HostDef
		|| (
			currentMenu->prevMenu
			&& currentMenu->prevMenu->prevMenu == &PLAY_MP_HostDef
			)
		);
	boolean solidbg = M_GameTrulyStarted() && !eggahack;
	UINT32 tick = ((optionsmenu.ticker/10) % 3) + 1;

	// the background isn't drawn outside of being in the main menu state.
	if (gamestate == GS_MENU && solidbg)
	{
		patch_t *back = W_CachePatchName(va("OPT_BG%u", tick), PU_CACHE);
		INT32 tflag = 0;
		UINT8 *c;
		UINT8 *c2;	// colormap for the one we're changing

		if (optionsmenu.fade)
		{
			c2 = R_GetTranslationColormap(TC_DEFAULT, optionsmenu.lastcolour, GTC_CACHE);
			V_DrawFixedPatch(0, 0, FRACUNIT, 0, back, c2);

			// prepare fade flag:
			tflag = min(V_90TRANS, (optionsmenu.fade)<<V_ALPHASHIFT);

		}
		c = R_GetTranslationColormap(TC_DEFAULT, optionsmenu.currcolour, GTC_CACHE);
		V_DrawFixedPatch(0, 0, FRACUNIT, tflag, back, c);
	}
	else
	{
		if (eggahack)
		{
			M_DrawEggaChannelAlignable(true);
		}

		patch_t *back_pause = W_CachePatchName(va("OPT_BAK%u", tick), PU_CACHE);
		V_DrawFixedPatch(0, 0, FRACUNIT, V_MODULATE, back_pause, NULL);

		if (!solidbg)
		{
			V_DrawFixedPatch(0, 0, FRACUNIT, (V_ADD|V_70TRANS), back_pause, NULL);
		}
	}
}

// Hacking up M_DrawOptionsCogs to try and make something better suited for changing the color profile. - Freaky Mutant Man
void M_DrawOptionsColorProfile(void)
{
	boolean eggahack = (
		currentMenu->prevMenu == &PLAY_MP_HostDef
		|| (
			currentMenu->prevMenu
			&& currentMenu->prevMenu->prevMenu == &PLAY_MP_HostDef
			)
		);
	boolean solidbg = M_GameTrulyStarted() && !eggahack;
	UINT32 tick = ((optionsmenu.ticker/10) % 3) + 1;

	// the background isn't drawn outside of being in the main menu state.
	if (gamestate == GS_MENU && solidbg)
	{
		patch_t *back = W_CachePatchName(va("OPT_BG%u", tick), PU_CACHE);
		patch_t *colorp_photo = W_CachePatchName("COL_PHO", PU_CACHE);
		patch_t *colorp_bar = W_CachePatchName("COL_BAR", PU_CACHE);
		INT32 tflag = 0;
		UINT8 *c;
		UINT8 *c2;	// colormap for the one we're changing

		if (optionsmenu.fade)
		{
			c2 = R_GetTranslationColormap(TC_DEFAULT, optionsmenu.lastcolour, GTC_CACHE);
			V_DrawFixedPatch(0, 0, FRACUNIT, 0, back, c2);

			// prepare fade flag:
			tflag = min(V_90TRANS, (optionsmenu.fade)<<V_ALPHASHIFT);

		}
		c = R_GetTranslationColormap(TC_DEFAULT, optionsmenu.currcolour, GTC_CACHE);
		V_DrawFixedPatch(0, 0, FRACUNIT, tflag, back, c);
		V_DrawFixedPatch(243<<FRACBITS, 67<<FRACBITS, FRACUNIT, 0, colorp_bar, NULL);
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, colorp_photo, NULL);
		//M_DrawCharSelectSprite( //figure this out later
	}
	// Given the need for accessibility, I don't want the background to be drawn transparent here - a clear color reference is needed for proper utilization. - Freaky Mutant Man
	else
	{
		if (eggahack)
		{
			M_DrawEggaChannelAlignable(true);
		}

		patch_t *colorp_bar = W_CachePatchName("COL_BAR", PU_CACHE);
		V_DrawFixedPatch(243<<FRACBITS, 67<<FRACBITS, FRACUNIT, 0, colorp_bar, NULL);

		if (!solidbg)
		{
			V_DrawFixedPatch(243<<FRACBITS, 67<<FRACBITS, FRACUNIT, 0, colorp_bar, NULL);
		}
	}
}

void M_DrawOptionsMovingButton(void)
{
	patch_t *butt = W_CachePatchName("OPT_BUTT", PU_CACHE);
	UINT8 *c = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);
	fixed_t t = M_DueFrac(optionsmenu.topt_start, M_OPTIONS_OFSTIME);
	fixed_t z = Easing_OutSine(M_DueFrac(optionsmenu.offset.start, M_OPTIONS_OFSTIME), optionsmenu.offset.dist * FRACUNIT, 0);
	fixed_t tx = Easing_OutQuad(t, optionsmenu.optx * FRACUNIT, optionsmenu.toptx * FRACUNIT) + z;
	fixed_t ty = Easing_OutQuad(t, optionsmenu.opty * FRACUNIT, optionsmenu.topty * FRACUNIT) + z;

	V_DrawFixedPatch(tx, ty, FRACUNIT, 0, butt, c);

	const char *s = OPTIONS_MainDef.menuitems[OPTIONS_MainDef.lastOn].text;
	fixed_t w = V_StringScaledWidth(
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		0,
		GM_FONT,
		s
	);
	V_DrawStringScaled(
		tx - 3*FRACUNIT - (w/2),
		ty - 16*FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		0,
		c,
		GM_FONT,
		s
	);
}

void M_DrawOptions(void)
{
	UINT8 i;
	fixed_t t = Easing_OutSine(M_DueFrac(optionsmenu.offset.start, M_OPTIONS_OFSTIME), optionsmenu.offset.dist * FRACUNIT, 0);
	fixed_t x = (140 - (48*itemOn))*FRACUNIT + t;
	fixed_t y = 70*FRACUNIT + t;
	fixed_t tx = M_EaseWithTransition(Easing_InQuart, 5 * 64 * FRACUNIT);
	patch_t *buttback = W_CachePatchName("OPT_BUTT", PU_CACHE);

	UINT8 *c = NULL;

	for (i=0; i < currentMenu->numitems; i++)
	{
		fixed_t py = y - (itemOn*48)*FRACUNIT;
		fixed_t px = x - tx;
		INT32 tflag = 0;

		if (i == itemOn)
			c = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);
		else
			c = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_BLACK, GTC_CACHE);

		if (currentMenu->menuitems[i].status & IT_TRANSTEXT)
			tflag = V_TRANSLUCENT;

		if (!(menutransition.tics != menutransition.dest && i == itemOn))
		{
			V_DrawFixedPatch(px, py, FRACUNIT, 0, buttback, c);

			const char *s = currentMenu->menuitems[i].text;
			fixed_t w = V_StringScaledWidth(
				FRACUNIT,
				FRACUNIT,
				FRACUNIT,
				0,
				GM_FONT,
				s
			);
			V_DrawStringScaled(
				px - 3*FRACUNIT - (w/2),
				py - 16*FRACUNIT,
				FRACUNIT,
				FRACUNIT,
				FRACUNIT,
				tflag,
				(i == itemOn ? c : NULL),
				GM_FONT,
				s
			);
		}

		y += 48*FRACUNIT;
		x += 48*FRACUNIT;
	}

	M_DrawMenuTooltips();

	if (menutransition.tics != menutransition.dest)
		M_DrawOptionsMovingButton();

}

static void M_DrawOptionsBoxTerm(INT32 x, INT32 top, INT32 bottom)
{
	INT32 px = x - 20;

	V_DrawFill(px, top + 4, 2, bottom - top, orangemap[0]);
	V_DrawFill(px + 1, top + 5, 2, bottom - top, 31);

	V_DrawFill(BASEVIDWIDTH - px - 2, top + 4, 2, bottom - top, orangemap[0]);
	V_DrawFill(BASEVIDWIDTH - px, top + 5, 1, bottom - top, 31);

	V_DrawFill(px, bottom + 2, BASEVIDWIDTH - (2 * px), 2, orangemap[0]);
	V_DrawFill(px, bottom + 3, BASEVIDWIDTH - (2 * px), 2, 31);
}

static void M_DrawLinkArrow(INT32 x, INT32 y, INT32 i)
{
	UINT8 ch = currentMenu->menuitems[i].text[0];

	V_DrawMenuString(
		x + (i == itemOn ? 1 + skullAnimCounter/5 : 0),
		y - 1,
		// Use color of first character in text label
		i == itemOn ? highlightflags : (((max(ch, 0x80) - 0x80) & 15) << V_CHARCOLORSHIFT),
		"\x1D"
	);
}

void M_DrawGenericOptions(void)
{
	INT32 x = currentMenu->x - M_EaseWithTransition(Easing_Linear, 5 * 48), y = currentMenu->y, w, i, cursory = -100;
	INT32 expand = -1;
	INT32 boxy = 0;
	boolean collapse = false;
	boolean opening = false;
	fixed_t boxt = 0;

	M_DrawMenuTooltips();
	M_DrawOptionsMovingButton();

	for (i = itemOn; i >= 0; --i)
	{
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_DYBIGSPACE:
				goto box_found;

			case IT_HEADERTEXT:
				expand = i;
				goto box_found;
		}
	}
box_found:
	if (optionsmenu.box.dist != expand)
	{
		optionsmenu.box.dist = expand;
		optionsmenu.box.start = I_GetTime();
	}

	for (i = 0; i < currentMenu->numitems; i++)
	{
		boolean term = false;

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_DYBIGSPACE:
				collapse = false;
				term = (boxy != 0);
				break;

			case IT_HEADERTEXT:
				if (i != expand)
				{
					collapse = true;
					term = (boxy != 0);
				}
				else
				{
					if (collapse)
						y += 2;

					collapse = false;

					if (menutransition.tics == menutransition.dest)
					{
						INT32 px = x - 20;
						V_DrawFill(px, y + 4, BASEVIDWIDTH - (2 * px), 2, orangemap[0]);
						V_DrawFill(px + 1, y + 5, BASEVIDWIDTH - (2 * px), 2, 31);
					}

					boxy = y;

					boxt = optionsmenu.box.dist == expand ? M_DueFrac(optionsmenu.box.start, 5) : FRACUNIT;
					opening = boxt < FRACUNIT;
				}
				break;

			default:
				if (collapse)
					continue;
		}

		if (term)
		{
			if (menutransition.tics == menutransition.dest)
				M_DrawOptionsBoxTerm(x, boxy, Easing_Linear(boxt, boxy, y));

			y += SMALLLINEHEIGHT;
			boxy = 0;
			opening = false;
		}

		if (i == itemOn && !opening)
		{
			cursory = y;
			M_DrawUnderline(x, BASEVIDWIDTH - x, y);
		}

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					}
				}
				/* FALLTHRU */
			case IT_NOTHING:
				y += SMALLLINEHEIGHT;
				break;

			case IT_DYBIGSPACE:
				y += SMALLLINEHEIGHT/2;
				break;
#if 0
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, currentMenu->menuitems[i].itemaction.cvar);
				y += LINEHEIGHT;
				break;
#endif
			case IT_STRING:
			case IT_LINKTEXT: {
				boolean textBox = (currentMenu->menuitems[i].status & IT_TYPE) == IT_CVAR &&
					(currentMenu->menuitems[i].status & IT_CVARTYPE) == IT_CV_STRING;

				if (textBox)
				{
					if (opening)
						y += LINEHEIGHT;
					else
						V_DrawFill(x+5, y+5, MAXSTRINGLENGTH*7+6, 9+6, 159);
				}

				if (opening)
				{
					y += STRINGHEIGHT;
					break;
				}

				INT32 px = x + ((currentMenu->menuitems[i].status & IT_TYPE) == IT_SUBMENU
					|| (currentMenu->menuitems[i].status & IT_DISPLAY) == IT_LINKTEXT ? 8 : 0);

				if (i == itemOn)
					cursory = y;

				if (i == itemOn)
					V_DrawMenuString(px + 1, y, highlightflags, currentMenu->menuitems[i].text);
				else
					V_DrawMenuString(px, y, textBox ? V_GRAYMAP : 0, currentMenu->menuitems[i].text);

				if ((currentMenu->menuitems[i].status & IT_DISPLAY) == IT_LINKTEXT)
					M_DrawLinkArrow(x, y, i);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
				{
					case IT_SUBMENU: {
						if ((currentMenu->menuitems[i].status & IT_DISPLAY) != IT_LINKTEXT)
							M_DrawLinkArrow(x, y, i);
						break;
					}

					case IT_CVAR: {
						consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
								{
									INT32 xoffs = 6;
									if (itemOn == i)
									{
										xoffs = 8;
										V_DrawMenuString(x + (skullAnimCounter/5) + 7, y + 9, highlightflags, "\x1D");
									}

									M_DrawCaretString(x + xoffs + 8, y + 9, cv->string, false);

									y += LINEHEIGHT;
								}
								break;
							default: {
								boolean isDefault = CV_IsSetToDefault(cv);
								w = V_MenuStringWidth(cv->string, 0);
								V_DrawMenuString(BASEVIDWIDTH - x - w, y,
									(!isDefault ? warningflags : highlightflags), cv->string);
								if (i == itemOn)
								{
									V_DrawMenuString(BASEVIDWIDTH - x - 10 - w - (skullAnimCounter/5), y - 1,
											highlightflags, "\x1C"); // left arrow
									V_DrawMenuString(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y - 1,
											highlightflags, "\x1D"); // right arrow
								}
								if (!isDefault)
								{
									V_DrawMenuString(BASEVIDWIDTH - x + (i == itemOn ? 13 : 5), y - 2, warningflags, ".");
								}
								break;
							}
						}
						break;
					}
				}

				y += STRINGHEIGHT;
				break;
			}
			case IT_STRING2:
				V_DrawMenuString(x, y, 0, currentMenu->menuitems[i].text);
				/* FALLTHRU */
			case IT_DYLITLSPACE:
			case IT_SPACE:
				y += (currentMenu->menuitems[i].mvar1 ? currentMenu->menuitems[i].mvar1 : SMALLLINEHEIGHT);
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += (currentMenu->menuitems[i].mvar1 ? currentMenu->menuitems[i].mvar1 : SMALLLINEHEIGHT);
				break;
			case IT_TRANSTEXT:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;
				/* FALLTHRU */
			case IT_TRANSTEXT2:
				V_DrawMenuString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawMenuString(x + 8, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_HEADERTEXT: // draws 16 pixels to the left, in yellow text
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawMenuString(x - (collapse ? 0 : 16), y, M_ALTCOLOR, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT + 1;
				break;
		}
	}

	if (boxy && menutransition.tics == menutransition.dest)
		M_DrawOptionsBoxTerm(x, boxy, Easing_Linear(boxt, boxy, y));

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		M_DrawCursorHand(x, cursory);
	}
}

// *Heavily* simplified version of the generic options menu, cattered only towards erasing profiles.
void M_DrawProfileErase(void)
{
	INT32 x = currentMenu->x - M_EaseWithTransition(Easing_Linear, 5 * 48), y = currentMenu->y-SMALLLINEHEIGHT, i, cursory = 0;
	UINT8 np = PR_GetNumProfiles();

	M_DrawMenuTooltips();
	M_DrawOptionsMovingButton();

	for (i = 1; i < np; i++)
	{

		profile_t *pr = PR_GetProfile(i);

		if (i == optionsmenu.eraseprofilen)
		{
			cursory = y;
			M_DrawCursorHand(x, cursory);
		}

		V_DrawMenuString(x, y,
			(i == optionsmenu.eraseprofilen ? highlightflags : 0),
			va("%sPRF%03d - %s (%s)",
			(cv_currprofile.value == i) ? "[In use] " : "",
			i, pr->profilename, pr->playername));
		y += SMALLLINEHEIGHT;
	}
}

// Draws profile selection
void M_DrawProfileSelect(void)
{
	INT32 i;
	const INT32 maxp = PR_GetNumProfiles();
	INT32 x = 160 - optionsmenu.profilen*(128 + 128/8) + Easing_OutSine(M_DueFrac(optionsmenu.offset.start, M_OPTIONS_OFSTIME), optionsmenu.offset.dist, 0);
	INT32 y = 35 + M_EaseWithTransition(Easing_Linear, 5 * 32);

	M_DrawMenuTooltips();

	// This shouldn't be drawn when a profile is selected as optx/opty are used to move the card.
	if (optionsmenu.profile == NULL && menutransition.tics)
		M_DrawOptionsMovingButton();

	for (i=0; i < MAXPROFILES+1; i++)	// +1 because the default profile does not count
	{
		profile_t *p = PR_GetProfile(i);

		// don't draw the card in this specific scenario
		if (!(optionsmenu.profile != NULL && optionsmenu.profilen == i))
			M_DrawProfileCard(x, y, i > maxp, p);

		x += 128 + 128/8;
	}

	// needs to be drawn since it happens on the transition
	if (optionsmenu.profile != NULL)
	{
		fixed_t t = M_DueFrac(optionsmenu.topt_start, M_OPTIONS_OFSTIME);
		M_DrawProfileCard(
			Easing_OutQuad(t, optionsmenu.optx, optionsmenu.toptx),
			Easing_OutQuad(t, optionsmenu.opty, optionsmenu.topty),
			false,
			optionsmenu.profile
		);
	}
}

void M_DrawEditProfileTooltips(void)
{
	// Tooltip
	// The text is slightly shifted hence why we don't just use M_DrawMenuTooltips()
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);
	if (currentMenu->menuitems[itemOn].tooltip != NULL)
	{
		V_DrawCenteredThinString(224, 12, 0, currentMenu->menuitems[itemOn].tooltip);
	}
}

// Profile edition menu
void M_DrawEditProfile(void)
{

	INT32 y = 34;
	INT32 x = (145 + M_EaseWithTransition(Easing_InSine, 5 * 48));
	INT32 i;

	M_DrawEditProfileTooltips();

	// Draw the menu options...
	for (i = 0; i < currentMenu->numitems; i++)
	{

		UINT8 *colormap = NULL;
		INT32 tflag = (currentMenu->menuitems[i].status & IT_TRANSTEXT) ? V_TRANSLUCENT : 0;
		INT32 cx = x;

		y = currentMenu->menuitems[i].mvar2;

		// Background -- 169 is the plague colourization
		V_DrawFill(0, y, 400 - M_EaseWithTransition(Easing_InQuad, 5 * 128), 10, itemOn == i ? 169 : 30);

		if (i == itemOn)
		{
			colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

			cx += Easing_OutSine(M_DueFrac(optionsmenu.offset.start, 2), 0, 5);
			V_DrawMenuString(cx - 10 - (skullAnimCounter/5), y+1, highlightflags, "\x1C"); // left arrow
		}

		// Text
		//V_DrawGamemodeString(cx, y - 6, tflag, colormap, currentMenu->menuitems[i].text);
		V_DrawStringScaled(
			cx * FRACUNIT,
			(y - 3) * FRACUNIT,
			FRACUNIT,
			FRACUNIT,
			FRACUNIT,
			tflag,
			colormap,
			KART_FONT,
			currentMenu->menuitems[i].text
		);

		//y += 32 + 2;
	}

	// Finally, draw the card ontop
	if (optionsmenu.profile != NULL)
	{
		fixed_t t = M_DueFrac(optionsmenu.topt_start, M_OPTIONS_OFSTIME);
		M_DrawProfileCard(
			Easing_OutQuad(t, optionsmenu.optx, optionsmenu.toptx),
			Easing_OutQuad(t, optionsmenu.opty, optionsmenu.topty),
			false,
			optionsmenu.profile
		);
	}
}

// Controller offsets to center on each button.
INT16 controlleroffsets[][2] = {
	{0, 0},			// gc_none
	{69, 142},		// gc_up
	{69, 182},		// gc_down
	{49, 162},		// gc_left
	{89, 162},		// gc_right
	{208, 200},		// gc_a
	{237, 181},		// gc_b
	{267, 166},		// gc_c
	{191, 164},		// gc_x
	{215, 149},		// gc_y
	{242, 137},		// gc_z
	{55, 102},		// gc_l
	{253, 102},		// gc_r
	{149, 187},		// gc_start
};

static void M_DrawBindBen(INT32 x, INT32 y, INT32 scroll_remaining)
{
	// optionsmenu.bindben_swallow
	const int pose_time = 30;
	const int swallow_time = pose_time + 14;

	// Lid closed
	int state = 'A';
	int frame = 0;

	if (optionsmenu.bindben_swallow > 100)
	{
		// Quick swallow (C button)
		state = 'C';

		int t = 106 - optionsmenu.bindben_swallow;
		if (t < 3)
			frame = 0;
		else
			frame = t - 3;
	}
	else if (scroll_remaining <= 0)
	{
		// Chewing (text done scrolling)
		state = 'B';
		frame = I_GetTime() / 2 % 4;

		// When state changes from 'lid open' to 'chewing',
		// play chomp sound.
		if (!optionsmenu.bindben_swallow)
			S_StartSound(NULL, sfx_monch);

		// Ready to swallow when button is released.
		optionsmenu.bindben_swallow = swallow_time + 1;
	}
	else if (optionsmenu.bindben)
	{
		// Lid open (text scrolling)
		frame = 1;
	}
	else if (optionsmenu.bindben_swallow)
	{
		if (optionsmenu.bindben_swallow > pose_time)
		{
			// Swallow
			state = 'C';

			int t = swallow_time - optionsmenu.bindben_swallow;
			if (t < 8)
				frame = 0;
			else
				frame = 1 + (t - 8) / 2 % 3;
		}
		else
		{
			// Pose
			state = 'D';

			int t = pose_time - optionsmenu.bindben_swallow;
			if (t < 10)
				frame = 0;
			else
				frame = 1 + (t - 10) / 4 % 5;
		}
	}

	V_DrawMappedPatch(x-30, y, 0, W_CachePatchName(va("PR_BIN%c%c", state, '1' + frame), PU_CACHE), aquamap);
}

static void M_DrawBindMediumString(INT32 y, INT32 flags, const char *string)
{
	fixed_t w = V_StringScaledWidth(FRACUNIT, FRACUNIT, FRACUNIT, flags, MED_FONT, string);
	fixed_t x = BASEVIDWIDTH/2 * FRACUNIT - w/2;
	V_DrawStringScaled(
		x,
		y * FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		flags,
		NULL,
		MED_FONT,
		string
	);
}

// largely replaced by K_DrawGameControl
/*
static INT32 M_DrawProfileLegend(INT32 x, INT32 y, const char *legend, const char *mediocre_key)
{
	INT32 w = V_ThinStringWidth(legend, 0);
	V_DrawThinString(x - w, y, 0, legend);
	x -= w + 2;
	if (mediocre_key)
		M_DrawMediocreKeyboardKey(mediocre_key, &x, y, false, true);
	return x;
}
*/

// the control stuff.
// Dear god.
void M_DrawProfileControls(void)
{
	const UINT8 spacing = 34;
	INT32 y = 16 - (optionsmenu.controlscroll*spacing);
	INT32 x = 8;
	INT32 i, j, k;
	const UINT8 pid = 0;
	patch_t *hint = W_CachePatchName("MENUHINT", PU_CACHE);
	INT32 hintofs = 3;

	K_DrawInputDisplay(BASEVIDWIDTH*2/3 - optionsmenu.contx, BASEVIDHEIGHT/2 - optionsmenu.conty, 0, '_', pid, true, false);

	if (optionsmenu.trycontroller)
	{
		optionsmenu.tcontx = BASEVIDWIDTH*2/3 - 10;
		optionsmenu.tconty = BASEVIDHEIGHT/2 +70;

		V_DrawCenteredLSTitleLowString(160, 164, 0, "TRY BUTTONS");

		const char *msg = va("Press nothing for %d sec to go back", (optionsmenu.trycontroller + (TICRATE-1)) / TICRATE);
		fixed_t w = V_StringScaledWidth(FRACUNIT, FRACUNIT, FRACUNIT, highlightflags, MED_FONT, msg);
		V_DrawStringScaled(
			160*FRACUNIT - w/2,
			186*FRACUNIT,
			FRACUNIT,
			FRACUNIT,
			FRACUNIT,
			highlightflags,
			NULL,
			MED_FONT,
			msg
		);
		return;	// Don't draw the rest if we're trying the controller.
	}

	V_DrawFill(0, 0, 138, 200, 31);	// Black border

	V_SetClipRect(
		0,
		0,
		BASEVIDWIDTH * FRACUNIT,
		(BASEVIDHEIGHT - SHORT(hint->height) + hintofs) * FRACUNIT,
		0
	);

	// Draw the menu options...
	for (i = 0; i < currentMenu->numitems; i++)
	{
		char buf[256];
		char buf2[256];
		INT32 keys[MAXINPUTMAPPING];

		// cursor
		if (i == itemOn)
		{
			for (j=0; j < 24; j++)
				V_DrawFill(0, (y)+j, 128+j, 1, 73);
		}

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_HEADERTEXT:
				V_DrawFill(0, y+18, 124, 1, 0);	// underline
				V_DrawMenuString(x, y+8, 0, currentMenu->menuitems[i].text);
				y += spacing;
				break;

			case IT_STRING:
				V_DrawMenuString(x, y+2, (i == itemOn ? highlightflags : 0), currentMenu->menuitems[i].text);
				y += spacing;
				break;

			case IT_STRING2:
			{
				boolean drawnpatch = false;

				if (currentMenu->menuitems[i].patch)
				{
					V_DrawScaledPatch(x-4, y+1, 0, W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					V_DrawMenuString(x+12, y+2, (i == itemOn ? highlightflags : 0), currentMenu->menuitems[i].text);
					drawnpatch = true;
				}
				else
					V_DrawMenuString(x, y+2, (i == itemOn ? highlightflags : 0), currentMenu->menuitems[i].text);

				if (currentMenu->menuitems[i].status & IT_CVAR)	// not the proper way to check but this menu only has normal onoff cvars.
				{												// (bitch you thought - Tyron 2024-09-22)
					INT32 w;
					consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;

					w = V_MenuStringWidth(cv->string, 0);
					V_DrawMenuString(x + 12, y + 13, (!CV_IsSetToDefault(cv) ? warningflags : highlightflags), cv->string);
					if (i == itemOn)
					{
						V_DrawMenuString(x - (skullAnimCounter/5), y+12, highlightflags, "\x1C"); // left arrow
						V_DrawMenuString(x + 12 + w + 2 + (skullAnimCounter/5) , y+13, highlightflags, "\x1D"); // right arrow
					}
				}
				else if (currentMenu->menuitems[i].status & IT_CONTROL)
				{
					UINT32 vflags = V_FORCEUPPERCASE;
					INT32 gc = currentMenu->menuitems[i].mvar1;

					UINT8 available = 0, set = 0;

					if (i != itemOn)
						vflags |= V_GRAYMAP;

					// Get userbound controls...
					for (k = 0; k < MAXINPUTMAPPING; k++)
					{
						int device;
						keys[k] = optionsmenu.tempcontrols[gc][k];
						if (keys[k] == KEY_NULL)
							continue;
						set++;

						device = G_GetDeviceForPlayer(0);
						if (device == -1)
						{
							device = 0;
						}
						if (!G_KeyIsAvailable(keys[k], device))
							continue;
						available++;
					};

					buf[0] = '\0';
					buf2[0] = '\0';


					// Cool as is this, this doesn't actually help show accurate info because of how some players would set inputs with keyboard and controller at once in a volatile way...
					// @TODO: Address that mess, somehow?

					// Can't reach any of them?
					/*if (available == 0)
					{
						if (((3*optionsmenu.ticker)/(2*TICRATE)) & 1) // 1.5 seconds
						{
							vflags |= V_ORANGEMAP;
#ifdef SHOWCONTROLDEFAULT
							if (G_KeyBindIsNecessary(gc))
							{
								// Get the defaults for essential keys.
								// Went through all the trouble of making this look cool,
								// then realised defaulting should only apply to menus.
								// Too much opportunity for confusion if kept.
								for (k = 0; k < MAXINPUTMAPPING; k++)
								{
									keys[k] = gamecontroldefault[gc][k];
									if (keys[k] == KEY_NULL)
										continue;
									available++;
								}
								set = available;
							}
							else if (set)
#else
							if (!set)
							{
								if (!G_KeyBindIsNecessary(gc))
									vflags = V_REDMAP;
							}
							else
#endif
							{
								strcpy(buf, "CURRENTLY UNAVAILABLE");
							}
						}
						else
						{
							vflags |= V_REDMAP;
						}
					}*/

					char *p = buf;
					if (buf[0])
						;
					else if (!set)
					{
						vflags &= ~V_CHARCOLORMASK;
						vflags |= V_REDMAP;
						strcpy(buf, "NOT BOUND");
					}
					else
					{
						for (k = 0; k < MAXINPUTMAPPING; k++)
						{
							if (keys[k] == KEY_NULL)
								continue;

							if (k > 0)
								strcat(p," / ");

							if (k == 2)   // hacky...
								p = buf2;

							strcat(p, G_KeynumToString (keys[k]));
						}
					}

					INT32 bindx = x;
					INT32 benx = 142;
					INT32 beny = y - 8;
					if (i == itemOn)
					{
						// Extend yellow wedge down behind
						// extra line.
						if (buf2[0])
						{
							for (j=24; j < 34; j++)
								V_DrawFill(0, (y)+j, 128+j, 1, 73);
							benx += 10;
							beny += 10;
						}

						// Scroll text into Bind Ben.
						bindx += optionsmenu.bindben * 3;

						if (buf2[0])
						{
							// Bind Ben: suck characters off
							// the end of the first line onto
							// the beginning of the second
							// line.
							UINT16 n = strlen(buf);
							UINT16 t = min(optionsmenu.bindben, n);
							memmove(&buf2[t], buf2, t + 1);
							memcpy(buf2, &buf[n - t], t);
							buf[n - t] = '\0';
						}
					}

					{
						cliprect_t clip;
						V_SaveClipRect(&clip); // preserve cliprect for tooltip

						// Clip text as it scrolls into Bind Ben.
						V_SetClipRect(0, 0, (benx-14)*FRACUNIT, 200*FRACUNIT, 0);

						if (i != itemOn || !optionsmenu.bindben_swallow)
						{
							// don't shift the text if we didn't draw a patch.
							V_DrawThinString(bindx + (drawnpatch ? 13 : 1), y + 12, vflags, buf);
							V_DrawThinString(bindx + (drawnpatch ? 13 : 1), y + 22, vflags, buf2);
						}

						V_RestoreClipRect(&clip);
					}

					if (i == itemOn)
						M_DrawBindBen(benx, beny, (benx-14) - bindx);

					// controller dest coords:
					if (itemOn == i && gc > 0 && gc <= gc_start)
					{
						optionsmenu.tcontx = controlleroffsets[gc][0];
						optionsmenu.tconty = controlleroffsets[gc][1];
					}
				}


				y += spacing;
				break;
			}
		}
	}

	V_ClearClipRect();

	// Tooltip
	// Draw it at the bottom of the screen
	{
		static UINT8 blue[256];
		blue[31] = 253;
		V_DrawMappedPatch(0, BASEVIDHEIGHT + hintofs, V_VFLIP, hint, blue);
	}
	if (currentMenu->menuitems[itemOn].tooltip != NULL)
	{
		INT32 ypos = BASEVIDHEIGHT + hintofs - 9 - 12;

		if (!strcmp(currentMenu->menuitems[itemOn].tooltip, "DESCRIPTIVEINPUT-SENTINEL"))
		{
			char* help = va("Modern: Standard console controller/keyboard prompts.");
			switch (cv_dummyprofiledescriptiveinput.value)
			{
				case 0:
					help = va("\"Emulator\": Display the default (Saturn) controls.");
					break;
				case 2:
					help = va("Modern Flip: Swap A+X/B+Y. Use if Modern is wrong.");
					break;
				case 3:
					help = va("6Bt. (Auto): Tries to guess your 6-button pad's layout.");
					break;
				case 4:
					help = va("6Bt. (A): Saturn (Retro-Bit Wired DInput) - C/Z = RB/RT");
					break;
				case 5:
					help = va("6Bt. (B): Saturn (Retro-Bit Wireless DInput) - C/Z = LB/RB");
					break;
				case 6:
					help = va("6Bt. (C): Saturn (Retro-Bit XInput) - C/Z = RT/LT");
					break;
				case 7:
					help = va("6Bt. (D): Saturn (arcade / 8BitDo) - C/Z = RT/RB");
					break;
				case 8:
					help = va("6Bt. (E): Saturn (Hori/M30X) - C/Z = RT/RB, LB/LT = LS/RS");
					break;
				case 9:
					help = va("6Bt. (F): Saturn (Mayflash) - C/Z = RS/LS");
					break;
				case 10:
					help = va("6Bt. (G): Saturn (orig M30) -  C/Z = RB/LB");
					break;
			}

			V_DrawThinString(12, ypos, V_YELLOWMAP, help);
		}
		else
		{
			V_DrawThinString(12, ypos, V_YELLOWMAP, currentMenu->menuitems[itemOn].tooltip);
		}

		UINT16 oldsetting = cv_descriptiveinput->value;
		CV_StealthSetValue(cv_descriptiveinput, cv_dummyprofiledescriptiveinput.value);
		INT32 xpos = BASEVIDWIDTH - 12;
		xpos = K_DrawGameControl(xpos, ypos, 0, "<right> / <c>  Clear", 2, TINY_FONT, 0);
		CV_StealthSetValue(cv_descriptiveinput, oldsetting);
	}

	// Overlay for control binding
	if (optionsmenu.bindtimer)
	{
		INT16 reversetimer = TICRATE*5 - optionsmenu.bindtimer;
		INT32 fade = reversetimer;
		INT32 ypos;

		if (fade > 9)
			fade = 9;

		ypos = (BASEVIDHEIGHT/2) - 20 +16*(9 - fade);
		V_DrawFadeScreen(31, fade);

		M_DrawTextBox((BASEVIDWIDTH/2) - (120), ypos - 12, 30, 8);

		V_DrawCenteredMenuString(BASEVIDWIDTH/2, ypos, V_GRAYMAP, "Hold and release inputs for");
		V_DrawCenteredMenuString(BASEVIDWIDTH/2, ypos + 10, V_GRAYMAP, va("\"%s\"", currentMenu->menuitems[itemOn].text));

		if (optionsmenu.bindtimer > 0)
		{
			M_DrawBindMediumString(
				ypos + 50,
				highlightflags,
				va("(WAIT %d SEC TO SKIP)", (optionsmenu.bindtimer + (TICRATE-1)) / TICRATE)
			);
		}
		else
		{
			for (i = 0; i < MAXINPUTMAPPING && optionsmenu.bindinputs[i]; ++i)
			{
				M_DrawBindMediumString(
					ypos + (2 + i)*10,
					highlightflags | V_FORCEUPPERCASE,
					G_KeynumToString(optionsmenu.bindinputs[i])
				);
			}
		}
	}
}

// Draw the video modes list, a-la-Quake
void M_DrawVideoModes(void)
{
	INT32 i, j, row, col;
	INT32 t = M_EaseWithTransition(Easing_Linear, 5 * 64);

	M_DrawMenuTooltips();
	M_DrawOptionsMovingButton();

	V_DrawCenteredMenuString(BASEVIDWIDTH/2 + t, currentMenu->y,
		highlightflags, "Choose mode, reselect to change default");

	row = 41 + t;
	col = currentMenu->y + 14;
	for (i = 0; i < optionsmenu.vidm_nummodes; i++)
	{
		INT32 colorflag = 0;
		boolean isdefault = !strcmp(optionsmenu.modedescs[i].desc, va("%dx%d", cv_scr_width.value, cv_scr_height.value));

		if (i == optionsmenu.vidm_selected)
			colorflag = highlightflags;
		else if (isdefault)
			colorflag = V_ORANGEMAP;
		else if (optionsmenu.modedescs[i].goodratio)
			colorflag = recommendedflags; // Show multiples of 320x200 as green.

		if (isdefault)
			V_DrawScaledPatch(row + 2 + V_MenuStringWidth(optionsmenu.modedescs[i].desc, colorflag), col - 2, 0, W_CachePatchName("RHFAV", PU_CACHE));

		V_DrawMenuString(row, col, colorflag, optionsmenu.modedescs[i].desc);

		col += 9;
		if ((i % optionsmenu.vidm_column_size) == (optionsmenu.vidm_column_size-1))
		{
			row += 7*13;
			col = currentMenu->y + 14;
		}
	}

	if (optionsmenu.vidm_testingmode > 0)
	{
		INT32 testtime = (optionsmenu.vidm_testingmode/TICRATE) + 1;

		M_CentreText(t, currentMenu->y + 75,
			va("Previewing mode %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : 0x80,
				vid.width, vid.height));
		M_CentreText(t, currentMenu->y + 75+9,
			"Press ENTER again to keep this mode");
		M_CentreText(t, currentMenu->y + 75+18,
			va("Wait %d second%s", testtime, (testtime > 1) ? "s" : ""));
		M_CentreText(t, currentMenu->y + 75+27,
			"or press ESC to return");

	}
	else
	{
		M_CentreText(t, currentMenu->y + 75,
			va("Current mode is %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : 0x80,
				vid.width, vid.height));
		M_CentreText(t, currentMenu->y + 75+9,
			va("\x87" "Default mode is %dx%d",
				cv_scr_width.value, cv_scr_height.value));



		if (vid.width > 1280 || vid.height > 800)
			V_DrawCenteredMenuString(BASEVIDWIDTH/2 + t, currentMenu->y + 75+24,
				(I_GetTime() % 20 >= 10) ? V_REDMAP : V_YELLOWMAP, va("High resolutions will impact performance. Careful!"));
		else
			V_DrawCenteredMenuString(BASEVIDWIDTH/2 + t, currentMenu->y + 75+24,
				recommendedflags, "Modes marked in GREEN are recommended.");
		/*
		V_DrawCenteredString(BASEVIDWIDTH/2 + t, currentMenu->y + 75+16,
			highlightflags, "High resolutions stress your PC more, but will");
		V_DrawCenteredString(BASEVIDWIDTH/2 + t, currentMenu->y + 75+24,
			highlightflags, "look sharper. Balance visual quality and FPS!");
		*/
	}

	// Draw the cursor for the VidMode menu
	i = 41 - 10 + ((optionsmenu.vidm_selected / optionsmenu.vidm_column_size)*7*13) + t;
	j = currentMenu->y + 14 + ((optionsmenu.vidm_selected % optionsmenu.vidm_column_size)*9);

	M_DrawCursorHand(i + 14, j);
}

// Gameplay Item Tggles:
tic_t shitsfree = 0;

static void DrawMappedString(INT32 x, INT32 y, INT32 option, int font, const char *text, const UINT8 *colormap)
{
	V_DrawStringScaled(
		x * FRACUNIT,
		y * FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		option,
		colormap,
		font,
		text
	);
}

void M_DrawItemToggles(void)
{
	static UINT8 black[256];
	memset(black, 16, 256);

	const INT32 edges = 8;
	const INT32 height = 4;
	const INT32 spacing = 35;
	const INT32 column = itemOn/height;
	const INT32 row = itemOn%height;
	INT32 leftdraw, rightdraw, totaldraw;
	INT32 x, y = currentMenu->y;
	INT32 onx = 0, ony = 0;
	consvar_t *cv;
	INT32 i, drawnum;
	patch_t *pat;

	x = currentMenu->x
		+ M_EaseWithTransition(Easing_Linear, 5 * 64);

	M_DrawMenuTooltips();
	M_DrawOptionsMovingButton();

	// Find the available space around column
	leftdraw = rightdraw = column;
	totaldraw = 0;
	for (i = 0; (totaldraw < edges*2 && i < edges*4); i++)
	{
		if (rightdraw+1 < (currentMenu->numitems/height)+1)
		{
			rightdraw++;
			totaldraw++;
		}
		if (leftdraw-1 >= 0)
		{
			leftdraw--;
			totaldraw++;
		}
	}

	patch_t *isbg = W_CachePatchName("K_ISBG", PU_CACHE);
	patch_t *isbgd = W_CachePatchName("K_ISBGD", PU_CACHE);
	patch_t *ismul = W_CachePatchName("K_ISMUL", PU_CACHE);
	patch_t *isstrk = W_CachePatchName("K_ISSTRK", PU_CACHE);

	for (i = leftdraw; i <= rightdraw; i++)
	{
		INT32 j;

		for (j = 0; j < height; j++)
		{
			const INT32 thisitem = (i*height)+j;

			if (thisitem >= currentMenu->numitems)
				break;

			if (thisitem == itemOn)
			{
				onx = x;
				ony = y;
				y += spacing;
				continue;
			}

			if (currentMenu->menuitems[thisitem].mvar1 == 0)
			{
				V_DrawScaledPatch(x, y, 0, isbgd);
				y += spacing;
				continue;
			}

			cv = &cv_items[currentMenu->menuitems[thisitem].mvar1-1];

			drawnum = K_ItemResultToAmount(currentMenu->menuitems[thisitem].mvar1, NULL);

			V_DrawScaledPatch(x, y, 0, cv->value ? isbg : isbgd);

			if (drawnum > 1)
				V_DrawScaledPatch(x, y, 0, ismul);

			pat = W_CachePatchName(K_GetItemPatch(currentMenu->menuitems[thisitem].mvar1, true), PU_CACHE);

			V_DrawScaledPatch(x, y, 0, pat);

			if (!cv->value)
				V_DrawMappedPatch(x, y, V_MODULATE, pat, black);

			if (drawnum > 1)
			{
				V_DrawString(x+24, y+31, 0, va("x%d", drawnum));

				if (!cv->value)
					DrawMappedString(x+24, y+31, V_MODULATE, HU_FONT, va("x%d", drawnum), black);
			}

			if (!cv->value)
				V_DrawScaledPatch(x, y, 0, isstrk);

			y += spacing;
		}

		x += spacing;
		y = currentMenu->y;
	}

	drawnum = 0;

	{
		if (currentMenu->menuitems[itemOn].mvar1 == 0)
		{
			V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITBGD", PU_CACHE));
			if (shitsfree)
			{
				INT32 trans = V_TRANSLUCENT;
				if (shitsfree-1 > TICRATE-5)
					trans = ((10-TICRATE)+shitsfree-1)<<V_ALPHASHIFT;
				else if (shitsfree < 5)
					trans = (10-shitsfree)<<V_ALPHASHIFT;
				V_DrawScaledPatch(onx-1, ony-2, trans, W_CachePatchName("K_ITFREE", PU_CACHE));
			}
		}
		else
		{
			cv = &cv_items[currentMenu->menuitems[itemOn].mvar1-1];

			drawnum = K_ItemResultToAmount(currentMenu->menuitems[itemOn].mvar1, NULL);

			if (cv->value)
				V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITBG", PU_CACHE));
			else
				V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITBGD", PU_CACHE));

			if (drawnum > 1)
				V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITMUL", PU_CACHE));

			pat = W_CachePatchName(K_GetItemPatch(currentMenu->menuitems[itemOn].mvar1, false), PU_CACHE);

			V_DrawScaledPatch(onx-1, ony-2, 0, pat);

			if (!cv->value)
				V_DrawMappedPatch(onx-1, ony-2, V_MODULATE, pat, black);

			if (drawnum > 1)
			{
				V_DrawScaledPatch(onx+27, ony+39, 0, W_CachePatchName("K_ITX", PU_CACHE));
				V_DrawTimerString(onx+37, ony+34, 0, va("%d", drawnum));

				if (!cv->value)
				{
					V_DrawMappedPatch(onx+27, ony+39, V_MODULATE, W_CachePatchName("K_ITX", PU_CACHE), black);
					DrawMappedString(onx+37, ony+34, V_MODULATE, TIMER_FONT, va("%d", drawnum), black);
				}
			}

			if (!cv->value)
			{
				V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITSTRK", PU_CACHE));
			}
		}
	}

	// Button prompts
	x = (BASEVIDWIDTH/2) - cv_kartfrantic.value;
	y = BASEVIDHEIGHT-20;
	INT32 w = K_DrawGameControl(
		x, y, 0,
		va(
			"<c_animated> Toggle All %s<white>   <r_animated> Frantic Mode: %s",
			cv_thunderdome.value ? "<yellow>(Ring Box Mode) " : "<gold>(Item Box Mode)",
			cv_kartfrantic.value ? "<red> On" : "<gray>Off"
		),
		1, TINY_FONT,
		(((row == height-1) && (drawnum > 1)) ? V_TRANSLUCENT : 0)
	);

	if (cv_kartfrantic.value != franticitems)
	{
		V_DrawThinString(
			x + w/2, y, 0,
			(cv_kartfrantic.value ? "\x85 (next)" : "\x86 (next)")
		);
	}
}


// EXTRAS:
// Copypasted from options but separate either way in case we want it to look more unique later down the line.
void M_DrawExtrasMovingButton(void)
{
	patch_t *butt = W_CachePatchName("OPT_BUTT", PU_CACHE);
	UINT8 *c = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

	V_DrawFixedPatch((extrasmenu.extx)*FRACUNIT, (extrasmenu.exty)*FRACUNIT, FRACUNIT, 0, butt, c);
	V_DrawCenteredGamemodeString((extrasmenu.extx)-3, (extrasmenu.exty) - 16, 0, c, EXTRAS_MainDef.menuitems[EXTRAS_MainDef.lastOn].text);
}

void M_DrawExtras(void)
{
	UINT8 i;
	INT32 t = Easing_OutSine(M_DueFrac(extrasmenu.offset.start, M_EXTRAS_OFSTIME), extrasmenu.offset.dist, 0);
	INT32 x = 140 - (48*itemOn) + t;
	INT32 y = 70 + t;
	patch_t *buttback = W_CachePatchName("OPT_BUTT", PU_CACHE);

	UINT8 *c = NULL;

	for (i=0; i < currentMenu->numitems; i++)
	{
		INT32 py = y - (itemOn*48);
		INT32 px = x - menutransition.tics*64;
		INT32 tflag = 0;

		if (i == itemOn)
			c = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);
		else
			c = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_BLACK, GTC_CACHE);

		if (currentMenu->menuitems[i].status & IT_TRANSTEXT)
			tflag = V_TRANSLUCENT;

		if (!(menutransition.tics && i == itemOn))
		{
			V_DrawFixedPatch(px*FRACUNIT, py*FRACUNIT, FRACUNIT, 0, buttback, c);
			V_DrawCenteredGamemodeString(px-3, py - 16, tflag, (i == itemOn ? c : NULL), currentMenu->menuitems[i].text);
		}

		y += 48;
		x += 48;
	}

	M_DrawMenuTooltips();

	if (menutransition.tics)
		M_DrawExtrasMovingButton();

}

//
// INGAME / PAUSE MENUS
//

static char *M_GetGameplayMode(void)
{
	if (grandprixinfo.gp == true)
	{
		if (grandprixinfo.masterbots)
			return va("Master");
		if (grandprixinfo.gamespeed == KARTSPEED_HARD)
			return va("Vicious");
		if (grandprixinfo.gamespeed == KARTSPEED_NORMAL)
			return va("Intense");
		return va("Relaxed");
	}

	if (franticitems)
	{
		if (cv_4thgear.value)
			return va("4th Gear! Frantic!");
		else
			return va("Gear %d - Frantic\n", gamespeed+1);
	}

	if (cv_4thgear.value)
		return va("4th Gear!");

	return va("Gear %d\n", gamespeed+1);
}

// PAUSE MAIN MENU
void M_DrawPause(void)
{

	SINT8 i;
	SINT8 itemsdrawn = 0;
	SINT8 countdown = 0;
	INT16 ypos = -50;	// Draw 3 items from selected item (y=100 - 3 items spaced by 50 px each... you get the idea.)
	INT16 dypos;

	fixed_t mt = M_DueFrac(pausemenu.openoffset.start, 6);

	if (pausemenu.openoffset.dist)
		mt = FRACUNIT - mt;

	INT16 offset = menutransition.tics ? floor(pow(2, (double)menutransition.tics)) : Easing_OutQuad(mt, 256, 0);
	INT16 arrxpos = 150 + 2*offset;	// To draw the background arrow.

	INT16 j = 0;

	patch_t *vertbg = W_CachePatchName("M_STRIPV", PU_CACHE);
	patch_t *arrstart = W_CachePatchName("M_PTIP", PU_CACHE);
	patch_t *arrfill = W_CachePatchName("M_PFILL", PU_CACHE);

	fixed_t t = M_DueFrac(pausemenu.offset.start, 3);

	UINT8 splitspectatestate = 0;
	if (G_GametypeHasSpectators() && pausemenu.splitscreenfocusid <= splitscreen)
	{
		// Identify relevant spectator state of pausemenu.splitscreenfocusid.
		// See also M_HandleSpectatorToggle.

		const UINT8 splitspecid =
			g_localplayers[pausemenu.splitscreenfocusid];

		if (players[splitspecid].spectator)
		{
			splitspectatestate =
				(players[splitspecid].pflags & PF_WANTSTOJOIN)
					? UINT8_MAX
					: 1;
		}
	}

	//V_DrawFadeScreen(0xFF00, 16);

	{
		INT32 x = Easing_OutQuad(mt, -BASEVIDWIDTH, 0);
		INT32 y = 56;

		if (g_realsongcredit && !S_MusicDisabled())
		{
			V_DrawThinString(x + 2, y, 0, g_realsongcredit);
		}

		if (gamestate == GS_LEVEL)
		{
			const char *name = bossinfo.valid && bossinfo.enemyname ?
				bossinfo.enemyname : mapheaderinfo[gamemap-1]->menuttl;
			char *buf = NULL;

			if (!name[0])
			{
				buf = G_BuildMapTitle(gamemap);
				name = buf;
			}

			INT32 width = V_StringScaledWidth(
				FRACUNIT,
				FRACUNIT,
				FRACUNIT,
				0,
				MED_FONT,
				name
			) / FRACUNIT;

			y += 11;

			V_DrawFill(x + 1, y + 8, width + 20, 3, 31);

			V_DrawStringScaled(
				(x + 19) * FRACUNIT,
				y * FRACUNIT,
				FRACUNIT,
				FRACUNIT,
				FRACUNIT,
				V_AQUAMAP,
				NULL,
				MED_FONT,
				name
			);

			K_DrawMapThumbnail(
				(x + 1) * FRACUNIT,
				(y - 1) * FRACUNIT,
				16 * FRACUNIT,
				0,
				gamemap - 1,
				NULL
			);

			Z_Free(buf);
		}
	}

	// "PAUSED"
	if (!paused && !demo.playback && !modeattacking && !netgame) // as close to possible as P_AutoPause, but not dependent on menuactive
	{
		M_DrawPausedText(-offset*FRACUNIT);
	}

	// Vertical Strip:
	V_DrawFixedPatch((230 + offset)<<FRACBITS, 0, FRACUNIT, V_ADD, vertbg, NULL);

	// Okay that's cool but which icon do we draw first? let's roll back from itemOn!
	// At most we'll draw 7 items, 1 in the center, 3 above, 3 below.
	// Which means... let's count down from itemOn
	for (i = itemOn; countdown < 3; countdown++)
	{
		i--;
		if (i < 0)
			i = currentMenu->numitems-1;

		while (currentMenu->menuitems[i].status == IT_DISABLED)
		{
			i--;

			if (i < 0)
				i = currentMenu->numitems-1;
		}
	}

	// Aaaaand now we can start drawing!
	// Reminder that we set the patches of the options to the description since we're not using that. I'm smart, I know...

	// Draw the background arrow
	V_DrawFixedPatch(arrxpos<<FRACBITS, 100<<FRACBITS, FRACUNIT, 0, arrstart, NULL);

	while ((arrxpos - arrfill->width) < BASEVIDWIDTH)
	{
		V_DrawFixedPatch(arrxpos<<FRACBITS, 100<<FRACBITS, FRACUNIT, 0, arrfill, NULL);
		arrxpos += arrfill->width;
	}

	while (itemsdrawn < 7)
	{
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_STRING:
			{
				patch_t *pp;
				UINT8 *colormap = NULL;

				if (i == itemOn && (i == mpause_restartmap || i == mpause_tryagain))
				{
					pp = W_CachePatchName(
						va("M_ICOR2%c", ('A'+(pausemenu.ticker & 1))),
						PU_CACHE);
				}
				else if (i == mpause_spectatetoggle)
				{
					pp = W_CachePatchName(
						((splitspectatestate == 1)
							? "M_ICOENT"
							: "M_ICOSPC"
						), PU_CACHE
					);
					if (i == itemOn)
						colormap = yellowmap;
				}
				else
				{
					pp = W_CachePatchName(currentMenu->menuitems[i].tooltip, PU_CACHE);
					if (i == itemOn)
						colormap = yellowmap;
				}

				// 294 - 261 = 33
				// We need to move 33 px in 50 tics which means we move 33/50 = 0.66 px every tic = 2/3 of the offset.
				// trust me i graduated highschool!!!!

				// Multiply by -1 or 1 depending on whether we're below or above 100 px.
				// This double ternary is awful, yes.

				INT32 yofs = Easing_InQuad(t, pausemenu.offset.dist, 0);
				dypos = ypos + yofs;
				V_DrawFixedPatch( ((i == itemOn ? (294 - yofs*2/3 * (dypos > 100 ? 1 : -1)) : 261) + offset) << FRACBITS, (dypos)*FRACUNIT, FRACUNIT, 0, pp, colormap);

				ypos += 50;
				itemsdrawn++;	// We drew that!
				break;
			}
		}

		i++;	// Regardless of whether we drew or not, go to the next item in the menu.
		if (i >= currentMenu->numitems)
		{
			i = 0;
			while (!(currentMenu->menuitems[i].status & IT_DISPLAY))
				i++;
		}
	}

	// Draw the string!

	const char *maintext = NULL;
	const char *selectableheadertext = NULL;
	const char *selectabletext = NULL;
	INT32 mainflags = 0, selectableflags = 0;

	if (itemOn == mpause_changegametype)
	{
		selectableheadertext = currentMenu->menuitems[itemOn].text;
		selectabletext = gametypes[menugametype]->name;
	}
	else if (itemOn == mpause_addons)
	{
		selectableheadertext = "ADDONS";
		selectabletext = menuaddonoptions ? "LOAD..." : "SETTINGS";
	}
	else if (itemOn == mpause_callvote)
	{
		selectableheadertext = currentMenu->menuitems[itemOn].text;
		selectabletext = K_GetMidVoteLabel(menucallvote);

		if (K_MinimalCheckNewMidVote(menucallvote) == false)
		{
			if (g_midVote.active == true)
			{
				maintext = "ACTIVE...";
			}
			else if (g_midVote.delay > 0)
			{
				if (g_midVote.delay != 1)
					maintext = va("%u", ((g_midVote.delay - 1) / TICRATE) + 1);
			}
			else if (K_PlayerIDAllowedInMidVote(consoleplayer) == false)
			{
				maintext = "SPECTATING";
			}
			else
			{
				maintext = "INVALID!?";
			}

			if (maintext != NULL)
			{
				mainflags |= V_YELLOWMAP;
				selectableflags |= V_MODULATE;
			}
		}
	}
	else if (itemOn == mpause_spectatetoggle)
	{
		const char *spectatetext = NULL;
		INT32 spectateflags = 0;

		if (splitspectatestate == 0)
			spectatetext = "SPECTATE";
		else if (splitspectatestate == 1)
		{
			spectatetext = "ENTER GAME";

			if (!cv_allowteamchange.value)
			{
				spectateflags |= V_MODULATE;
			}
		}
		else
			spectatetext = "CANCEL JOIN";

		if (splitscreen)
		{
			selectableheadertext = spectatetext;
			selectabletext = va("PLAYER %c", 'A' + pausemenu.splitscreenfocusid);
			selectableflags |= spectateflags;
		}
		else
		{
			maintext = spectatetext;
			mainflags |= spectateflags;
		}
	}
	else
	{
		maintext = currentMenu->menuitems[itemOn].text;
	}

	if (selectableheadertext != NULL)
	{
		// For selections, show the full menu text on top.
		V_DrawCenteredLSTitleHighString(220 + offset*2, 75, selectableflags, selectableheadertext);
	}

	if (selectabletext != NULL)
	{
		// The selectable text is shown below.
		selectableflags |= V_YELLOWMAP;

		INT32 w = V_LSTitleLowStringWidth(selectabletext, selectableflags)/2;
		V_DrawLSTitleLowString(220-w + offset*2, 103, selectableflags, selectabletext);

		V_DrawMenuString(220-w + offset*2 - 8 - (skullAnimCounter/5), 103+6, selectableflags, "\x1C"); // left arrow
		V_DrawMenuString(220+w + offset*2 + (skullAnimCounter/5), 103+6, selectableflags, "\x1D"); // right arrow
	}

	if (maintext != NULL)
	{
		// This is a regular menu option. Try to break it onto two lines.

		char word1[MAXSTRINGLENGTH];
		INT16 word1len = 0;
		char word2[MAXSTRINGLENGTH];
		INT16 word2len = 0;
		boolean sok = false;

		while (maintext[j] && j < MAXSTRINGLENGTH)
		{
			if (maintext[j] == ' ' && !sok)
			{
				sok = true;
				j++;
				continue;	// We don't care about this :moyai:
			}

			if (sok)
			{
				word2[word2len] = maintext[j];
				word2len++;
			}
			else
			{
				word1[word1len] = maintext[j];
				word1len++;
			}

			j++;
		}

		word1[word1len] = '\0';
		word2[word2len] = '\0';

		// If there's no 2nd word, take this opportunity to center this line of text.
		if (word1len)
			V_DrawCenteredLSTitleHighString(220 + offset*2, 75 + (!word2len ? 10 : 0), mainflags, word1);

		if (word2len)
			V_DrawCenteredLSTitleLowString(220 + offset*2, 103, mainflags, word2);
	}

	const boolean rulescheck = (K_CanChangeRules(false) && (server || IsPlayerAdmin(consoleplayer)));
	boolean drawqueue = (rulescheck && (menuqueue.size > 0));

	if (gamestate != GS_INTERMISSION && roundqueue.size > 0)
	{
		if (roundqueue.position > 0 && roundqueue.position <= roundqueue.size)
		{
			patch_t *smallroundpatch = ST_getRoundPicture(true);

			if (smallroundpatch != NULL)
			{
				V_DrawMappedPatch(
					24, 145 + offset/2,
					0,
					smallroundpatch,
					NULL);
			}
		}

		V_DrawCenteredMenuString(24, 167 + offset/2, V_YELLOWMAP, M_GetGameplayMode());

		drawqueue = true;
	}
	else if (gametype == GT_TUTORIAL)
	{
		K_DrawGameControl(4, 184 - 60 + offset/2, 0, "<left> <right> <up> <down> Steering", 0, TINY_FONT, 0);
		K_DrawGameControl(4, 184 - 45 + offset/2, 0, "<a> Accelerate", 0, TINY_FONT, 0);
		K_DrawGameControl(4, 184 - 30 + offset/2, 0, "<b> Look Back", 0, TINY_FONT, 0);
		K_DrawGameControl(4, 184 - 15 + offset/2, 0, "<c> Spin Dash", 0, TINY_FONT, 0);
		K_DrawGameControl(4, 184 - 0 + offset/2, 0, "<l> Item / Rings", 0, TINY_FONT, 0);

		K_DrawGameControl(90, 184 - 45 + offset/2, 0, "<x> Brake", 0, TINY_FONT, 0);
		K_DrawGameControl(90, 184 - 30 + offset/2, 0, "<y> Bail / Burst", 0, TINY_FONT, 0);
		K_DrawGameControl(90, 184 - 15 + offset/2, 0, "<z> Dialogue / Action", 0, TINY_FONT, 0);
		K_DrawGameControl(90, 184 - 0 + offset/2, 0, "<r> Drift", 0, TINY_FONT, 0);
	}
	else
	{
		V_DrawMenuString(4, 188 + offset/2, V_YELLOWMAP, M_GetGameplayMode());
	}

	if (drawqueue)
	{
		M_DrawPauseRoundQueue(offset/2, rulescheck);
	}
}

void M_DrawKickHandler(void)
{
	// fake round queue drawer simply to make release
	INT32 x = 29 + 4, y = 70, returny = y;
	INT32 pokeamount = playerkickmenu.poke ? ((playerkickmenu.poke & 1) ? -playerkickmenu.poke/2 : playerkickmenu.poke/2) : (I_GetTime() % 16 < 8);
	INT32 x2 = x + pokeamount - 9 - 8 - 2;

	boolean datarightofcolumn = false;

	patch_t *resbar = W_CachePatchName("R_RESBAR", PU_CACHE); // Results bars for players

	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		V_DrawMappedPatch(
			x, y,
			(playeringame[i] == true)
				? ((players[i].spectator == true) ? V_TRANSLUCENT : 0)
				: V_MODULATE,
			resbar, NULL
		);

		V_DrawRightAlignedThinString(
			x+13, y-2,
				((i == playerkickmenu.player)
					? highlightflags
					: 0
				),
			va("%u", i)
		);

		if (playeringame[i] == true)
		{
			if (players[i].skincolor != SKINCOLOR_NONE)
			{
				UINT8 *charcolormap;
				if ((players[i].pflags & PF_NOCONTEST) && players[i].bot)
				{
					// RETIRED !!
					charcolormap = R_GetTranslationColormap(TC_DEFAULT, players[i].skincolor, GTC_CACHE);
					V_DrawMappedPatch(x+14, y-5, 0, W_CachePatchName("MINIDEAD", PU_CACHE), charcolormap);
				}
				else
				{
					charcolormap = R_GetTranslationColormap(players[i].skin, players[i].skincolor, GTC_CACHE);
					V_DrawMappedPatch(x+14, y-5, 0, faceprefix[players[i].skin][FACE_MINIMAP], charcolormap);
				}
			}

			V_DrawThinString(
				x+27, y-2,
				(
					P_IsMachineLocalPlayer(&players[i])
						? highlightflags
						: 0
				),
				player_names[i]
			);

			V_DrawRightAlignedThinString(
				x+118, y-2,
				0,
				(players[i].spectator) ? "SPECTATOR" : "PLAYING"
			);
		}

		if (i == playerkickmenu.player)
		{
			V_DrawScaledPatch(
				x2, y-1,
				(datarightofcolumn ? V_FLIP : 0),
				W_CachePatchName("M_CURSOR", PU_CACHE)
			);
		}

		y += 13;

		if (i == (MAXPLAYERS-1)/2)
		{
			x = 169 - 6;
			y = returny;

			datarightofcolumn = true;
			x2 = x + 118 + 9 + 8 + 8 - pokeamount;
		}
	}

	//V_DrawFill(32 + (playerkickmenu.player & 8), 32 + (playerkickmenu.player & 7)*8, 8, 8, playeringame[playerkickmenu.player] ? 0 : 16);

	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);
	K_DrawGameControl(
		BASEVIDWIDTH/2, 12, 0,
		(playerkickmenu.adminpowered)
			? "You are using <red>Admin Tools<white>.  <a> Kick  <c> Ban"
			: K_GetMidVoteLabel(menucallvote),
		1, TINY_FONT, 0
	);
}

void M_DrawPlaybackMenu(void)
{
	INT16 i;
	patch_t *icon = NULL;
	UINT8 *activemap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GOLD, GTC_MENUCACHE);
	UINT32 transmap = max(0, (INT32)(leveltime - playback_last_menu_interaction_leveltime - 4*TICRATE)) / 5;
	transmap = min(8, transmap) << V_ALPHASHIFT;

	// wip
	//M_DrawTextBox(currentMenu->x-68, currentMenu->y-7, 15, 15);
	//M_DrawCenteredMenu();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		UINT8 *inactivemap = NULL;

		if (i >= playback_view1 && i <= playback_view4)
		{
			if (modeattacking) continue;

			if (r_splitscreen >= i - playback_view1)
			{
				INT32 ply = displayplayers[i - playback_view1];

				icon = faceprefix[players[ply].skin][FACE_RANK];
				if (i != itemOn)
					inactivemap = R_GetTranslationColormap(players[ply].skin, players[ply].skincolor, GTC_MENUCACHE);
			}
			else if (currentMenu->menuitems[i].patch && W_CheckNumForName(currentMenu->menuitems[i].patch) != LUMPERROR)
				icon = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
		}
		else if (currentMenu->menuitems[i].status == IT_DISABLED)
			continue;
		else if (currentMenu->menuitems[i].patch && W_CheckNumForName(currentMenu->menuitems[i].patch) != LUMPERROR)
			icon = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);

		if ((i == playback_fastforward && cv_playbackspeed.value > 1))
			V_DrawMappedPatch(currentMenu->x + currentMenu->menuitems[i].mvar1, currentMenu->y, V_SNAPTOTOP, icon, R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_JAWZ, GTC_MENUCACHE));
		else
			V_DrawMappedPatch(currentMenu->x + currentMenu->menuitems[i].mvar1, currentMenu->y, V_SNAPTOTOP, icon, (i == itemOn) ? activemap : inactivemap);

		if (i == itemOn)
		{
			V_DrawMenuString(currentMenu->x + currentMenu->menuitems[i].mvar1 + 4, currentMenu->y + 14,
				V_SNAPTOTOP|highlightflags, "\x1A");

			V_DrawCenteredMenuString(BASEVIDWIDTH/2, currentMenu->y + 19, V_SNAPTOTOP, currentMenu->menuitems[i].text);

			if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_ARROWS)
			{
				char *str;

				if (!(i == playback_viewcount && r_splitscreen == 3))
					V_DrawMenuString(BASEVIDWIDTH/2 - 4, currentMenu->y + 28 - (skullAnimCounter/5),
						V_SNAPTOTOP|highlightflags, "\x1A"); // up arrow

				if (!(i == playback_viewcount && r_splitscreen == 0))
					V_DrawMenuString(BASEVIDWIDTH/2 - 4, currentMenu->y + 48 + (skullAnimCounter/5),
						V_SNAPTOTOP|highlightflags, "\x1B"); // down arrow

				switch (i)
				{
				case playback_viewcount:
					str = va("%d", r_splitscreen+1);
					break;

				case playback_view1:
				case playback_view2:
				case playback_view3:
				case playback_view4:
					str = player_names[displayplayers[i - playback_view1]]; // 0 to 3
					break;

				default: // shouldn't ever be reached but whatever
					continue;
				}

				V_DrawCenteredMenuString(BASEVIDWIDTH/2, currentMenu->y + 38, V_SNAPTOTOP|highlightflags, str);
			}
		}
	}
}


// Draw misc menus:

// Addons

#define lsheadingheight 16

// Just do this here instead.
static void M_CacheAddonPatches(void)
{
	addonsp[EXT_FOLDER] = W_CachePatchName("M_FFLDR", PU_STATIC);
	addonsp[EXT_UP] = W_CachePatchName("M_FBACK", PU_STATIC);
	addonsp[EXT_NORESULTS] = W_CachePatchName("M_FNOPE", PU_STATIC);
	addonsp[EXT_TXT] = W_CachePatchName("M_FTXT", PU_STATIC);
	addonsp[EXT_CFG] = W_CachePatchName("M_FCFG", PU_STATIC);
	addonsp[EXT_WAD] = W_CachePatchName("M_FWAD", PU_STATIC);
	#ifdef USE_KART
	addonsp[EXT_KART] = W_CachePatchName("M_FKART", PU_STATIC);
	#endif
	addonsp[EXT_PK3] = W_CachePatchName("M_FPK3", PU_STATIC);
	addonsp[EXT_SOC] = W_CachePatchName("M_FSOC", PU_STATIC);
	addonsp[EXT_LUA] = W_CachePatchName("M_FLUA", PU_STATIC);
	addonsp[NUM_EXT] = W_CachePatchName("M_FUNKN", PU_STATIC);
	addonsp[NUM_EXT+1] = W_CachePatchName("M_FSEL", PU_STATIC);
	addonsp[NUM_EXT+2] = W_CachePatchName("M_FLOAD", PU_STATIC);
	addonsp[NUM_EXT+3] = W_CachePatchName("M_FSRCH", PU_STATIC);
	addonsp[NUM_EXT+4] = W_CachePatchName("M_FSAVE", PU_STATIC);
}

#define addonsseperation 16

void M_DrawAddons(void)
{
	INT32 x, y;
	ptrdiff_t i, m;
	const UINT8 *flashcol = NULL;
	UINT8 hilicol;

	M_CacheAddonPatches();

	if (Playing())
		V_DrawCenteredMenuString(BASEVIDWIDTH/2, 4, warningflags, "Adding files mid-game may cause problems.");
	else
		V_DrawCenteredMenuString(BASEVIDWIDTH/2, 4, 0,
		LOCATIONSTRING1);

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y - 1;

	hilicol = V_GetStringColormap(highlightflags)[0];

	y -= 16;

	V_DrawString(x-21, y + (lsheadingheight - 12), highlightflags, M_AddonsHeaderPath());
	V_DrawFill(x-21, y + (lsheadingheight - 3), MAXSTRINGLENGTH*8+6, 1, hilicol);
	//V_DrawFill(x-21, y + (lsheadingheight - 2), MAXSTRINGLENGTH*8+6, 1, 30);

	y += 10;

	M_DrawTextBox(x - (21 + 5), y, MAXSTRINGLENGTH, 1);
	{
		const char *str = (menusearch[0] ? cv_dummyaddonsearch.string : "Search...");
		INT32 tflag = (menusearch[0] ? 0 : V_TRANSLUCENT);
		INT32 xoffs = 0;
		if (itemOn == 0)
		{
			xoffs += 8;
			V_DrawMenuString(x + (skullAnimCounter/5) - 20, y+8, highlightflags, "\x1D");
		}
		V_DrawString(x + xoffs - 18, y+8, tflag, str);
	}

	V_DrawSmallScaledPatch(x - (21 + 5 + 16), y+4, (menusearch[0] ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+3]);

	y += 21;

	m = (addonsseperation*(2*numaddonsshown + 1)) + 1 + 2*(16-addonsseperation);
	V_DrawFill(x - 21, y - 1, MAXSTRINGLENGTH*8+6, m, 159);

	// scrollbar!
	if (sizedirmenu <= (2*numaddonsshown + 1))
		i = 0;
	else
	{
		ptrdiff_t q = m;
		m = ((2*numaddonsshown + 1) * m)/sizedirmenu;
		if (dir_on[menudepthleft] <= numaddonsshown) // all the way up
			i = 0;
		else if (sizedirmenu <= (dir_on[menudepthleft] + numaddonsshown + 1)) // all the way down
			i = q-m;
		else
			i = ((dir_on[menudepthleft] - numaddonsshown) * (q-m))/(sizedirmenu - (2*numaddonsshown + 1));
	}

	V_DrawFill(x + MAXSTRINGLENGTH*8+5 - 21, (y - 1) + i, 1, m, hilicol);

	// get bottom...
	m = dir_on[menudepthleft] + numaddonsshown + 1;
	if (m > (ptrdiff_t) sizedirmenu)
		m = sizedirmenu;

	// then compute top and adjust bottom if needed!
	if (m < (2*numaddonsshown + 1))
	{
		m = min(sizedirmenu, 2*numaddonsshown + 1);
		i = 0;
	}
	else
		i = m - (2*numaddonsshown + 1);

	if (i != 0)
		V_DrawMenuString(19, y+4 - (skullAnimCounter/5), highlightflags, "\x1A");

	if (skullAnimCounter < 4)
		flashcol = V_GetStringColormap(highlightflags);

	y -= (16-addonsseperation);

	for (; i < m; i++)
	{
		UINT32 flags = 0;
		if (y > BASEVIDHEIGHT) break;
		if (dirmenu[i])
#define type (UINT8)(dirmenu[i][DIR_TYPE])
		{
			if (type & EXT_LOADED)
			{
				flags |= V_TRANSLUCENT;
				V_DrawSmallScaledPatch(x-(16+4), y, V_TRANSLUCENT, addonsp[(type & ~EXT_LOADED)]);
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[NUM_EXT+2]);
			}
			else
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[(type & ~EXT_LOADED)]);

			if (itemOn == 1 && (size_t)i == dir_on[menudepthleft])
			{
				V_DrawFixedPatch((x-(16+4))<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, 0, addonsp[NUM_EXT+1], flashcol);
				flags = highlightflags;
			}

#define charsonside 14
			if (dirmenu[i][DIR_LEN] > (charsonside*2 + 3))
				V_DrawString(x, y+4, flags, va("%.*s...%s", charsonside, dirmenu[i]+DIR_STRING, dirmenu[i]+DIR_STRING+dirmenu[i][DIR_LEN]-(charsonside+1)));
#undef charsonside
			else
				V_DrawString(x, y+4, flags, dirmenu[i]+DIR_STRING);
		}
#undef type
		y += addonsseperation;
	}

	if (m != (ptrdiff_t) sizedirmenu)
		V_DrawMenuString(19, y-12 + (skullAnimCounter/5), highlightflags, "\x1B");

	if (m < (2*numaddonsshown + 1))
	{
		y += ((2*numaddonsshown + 1)-m)*addonsseperation;
	}

	y -= 2;

	V_DrawSmallScaledPatch(x, y, ((!majormods) ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+4]);
	if (modifiedgame)
		V_DrawSmallScaledPatch(x, y, 0, addonsp[NUM_EXT+2]);

	m = numwadfiles-(mainwads+musicwads);

	V_DrawCenteredMenuString(BASEVIDWIDTH/2, y+4, (majormods ? highlightflags : V_TRANSLUCENT), va("%ld ADD-ON%s LOADED", (long)m, (m == 1) ? "" : "S")); //+2 for music, sounds, +1 for bios.pk3
}

#undef addonsseperation

// Challenges Menu

static void M_DrawChallengeTile(INT16 i, INT16 j, INT32 x, INT32 y, UINT8 *flashmap, boolean hili)
{
#ifdef DEVELOP
	extern consvar_t cv_debugchallenges;
#endif
	unlockable_t *ref = NULL;
	patch_t *pat = missingpat;
	UINT8 *colormap = NULL, *bgmap = NULL;
	INT32 tileflags = 0;
	fixed_t siz, accordion;
	UINT16 id, num;
	boolean unlockedyet;

	id = (i * CHALLENGEGRIDHEIGHT) + j;
	num = gamedata->challengegrid[id];

	// Empty spots in the grid are always unconnected.
	if (num >= MAXUNLOCKABLES)
	{
		goto drawborder;
	}

	// Okay, this is what we want to draw.
	ref = &unlockables[num];

#ifdef DEVELOP
	if (cv_debugchallenges.value > 0 &&
		cv_debugchallenges.value != num+1)
	{
		// Searching for an unlockable ID, fade out any tiles
		// that don't match.
		tileflags = V_80TRANS;
	}
#endif

	unlockedyet = !((gamedata->unlocked[num] == false)
		|| (challengesmenu.pending && num == challengesmenu.currentunlock && challengesmenu.unlockanim <= UNLOCKTIME));

	// If we aren't unlocked yet, return early.
	if (!unlockedyet)
	{
		UINT32 flags = 0;
		boolean hint = !!(challengesmenu.extradata[id].flags & CHE_HINT);

		pat = W_CachePatchName(
			va("UN_OUTL%c",
				ref->majorunlock ? 'B' : 'A'
			),
			PU_CACHE);

		V_DrawFixedPatch(
			x*FRACUNIT, y*FRACUNIT,
			FRACUNIT,
			(hint ? V_ADD : V_SUBTRACT)|V_90TRANS,
			pat,
			colormap
		);

		if (!hint)
		{
			colormap = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_CACHE);
			flags = V_SUBTRACT|V_90TRANS;
		}

		pat = W_CachePatchName(
			va("UN_HNT%c%c",
				(hili && !colormap) ? '1' : '2',
				ref->majorunlock ? 'B' : 'A'
			),
			PU_CACHE);

		V_DrawFixedPatch(
			x*FRACUNIT, y*FRACUNIT,
			FRACUNIT,
			flags, pat,
			colormap
		);

		pat = missingpat;
		colormap = NULL;

		goto drawborder;
	}

	accordion = FRACUNIT;

	boolean categoryside = (challengesmenu.extradata[id].flip == 0);

	if (!categoryside // optimised, this is not the true value with anything but instaflip
		&& challengesmenu.extradata[id].flip != (TILEFLIP_MAX/2))
	{
		fixed_t bad = challengesmenu.extradata[id].flip * FRACUNIT + rendertimefrac;
		angle_t worse = (FixedAngle(FixedMul(bad, 360*FRACUNIT/TILEFLIP_MAX)) >> ANGLETOFINESHIFT) & FINEMASK;
		accordion = FINECOSINE(worse);
		if (accordion < 0)
			accordion = -accordion;

		// NOW we set it in an interp-friendly way
		categoryside = (bad <= FRACUNIT*TILEFLIP_MAX/4
			|| bad > (3*FRACUNIT*TILEFLIP_MAX)/4);
	}

	pat = W_CachePatchName(
		(ref->majorunlock ? "UN_BORDB" : "UN_BORDA"),
		PU_CACHE);

	UINT8 iconid = 0;

	{
		UINT16 bcol = SKINCOLOR_SILVER;
		switch (ref->type)
		{
			case SECRET_SKIN:
				bcol = SKINCOLOR_NOVA;
				iconid = 1;
				break;
			case SECRET_FOLLOWER:
				if (horngoner)
				{
					bcol = SKINCOLOR_BLACK;
				}
				else
				{
					bcol = SKINCOLOR_SAPPHIRE;
					iconid = 2;
				}
				break;
			case SECRET_COLOR:
				//bcol = SKINCOLOR_SILVER;
				iconid = 3;
				break;
			case SECRET_CUP:
				bcol = SKINCOLOR_GOLD;
				iconid = 4;
				break;
			case SECRET_MAP:
				bcol = SKINCOLOR_PURPLE;
				iconid = 8;
				break;
			case SECRET_HARDSPEED:
			case SECRET_MASTERMODE:
			case SECRET_ENCORE:
				bcol = SKINCOLOR_RUBY;
				iconid = 5;
				break;
			case SECRET_ONLINE:
			case SECRET_ADDONS:
			case SECRET_EGGTV:
			case SECRET_SOUNDTEST:
			case SECRET_ALTTITLE:
				bcol = SKINCOLOR_BLUEBERRY;
				iconid = 6;
				break;
			case SECRET_TIMEATTACK:
			case SECRET_PRISONBREAK:
			case SECRET_SPECIALATTACK:
			case SECRET_SPBATTACK:
				bcol = SKINCOLOR_PERIDOT;
				iconid = 7;
				break;
			case SECRET_ALTMUSIC:
				bcol = SKINCOLOR_MAGENTA;
				iconid = 9;
				break;
		}

		bgmap = R_GetTranslationColormap(TC_DEFAULT, bcol, GTC_MENUCACHE);
	}

	V_DrawStretchyFixedPatch(
		(x*FRACUNIT) + (SHORT(pat->width)*(FRACUNIT-accordion)/2), y*FRACUNIT,
		accordion,
		FRACUNIT,
		tileflags, pat,
		bgmap
	);

	pat = missingpat;

#ifdef DEVELOP
	if (cv_debugchallenges.value)
	{
		// Show the content of every tile without needing to flip them.
		categoryside = false;
	}
#endif

	if (horngoner && ref->type == SECRET_FOLLOWER)
		goto drawborder;
	else if (categoryside)
	{
		colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_SILVER, GTC_MENUCACHE);

		// iconid is already prepopulated because we had to draw the border
		pat = challengesmenu.tile_category[iconid][ref->majorunlock ? 1 : 0];
		if (pat == missingpat)
		{
			pat = challengesmenu.tile_category[iconid][ref->majorunlock ? 0 : 1];
		}
	}
	else if (ref->icon != NULL && ref->icon[0])
	{
		pat = W_CachePatchName(ref->icon, PU_CACHE);
		if (ref->color != SKINCOLOR_NONE && ref->color < numskincolors)
		{
			colormap = R_GetTranslationColormap(TC_DEFAULT, ref->color, GTC_MENUCACHE);
		}
	}
	else
	{
		iconid = 0; // reuse
		switch (ref->type)
		{
			case SECRET_SKIN:
			{
				INT32 skin = M_UnlockableSkinNum(ref);
				if (skin != -1)
				{
					colormap = R_GetTranslationColormap(skin, skins[skin]->prefcolor, GTC_MENUCACHE);
					pat = faceprefix[skin][(ref->majorunlock) ? FACE_WANTED : FACE_RANK];
				}
				break;
			}
			case SECRET_FOLLOWER:
			{
				INT32 skin = M_UnlockableFollowerNum(ref);
				if (skin != -1)
				{
					INT32 psk = R_SkinAvailableEx(cv_skin[0].string, false);
					UINT16 col = K_GetEffectiveFollowerColor(followers[skin].defaultcolor, &followers[skin], cv_playercolor[0].value, (psk != -1) ? skins[psk] : skins[0]);
					colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);
					pat = W_CachePatchName(followers[skin].icon, PU_CACHE);
				}
				break;
			}
			case SECRET_COLOR:
			{
				INT32 colorid = M_UnlockableColorNum(ref);
				if (colorid != SKINCOLOR_NONE)
				{
					colormap = R_GetTranslationColormap(TC_DEFAULT, colorid, GTC_MENUCACHE);
				}
				iconid = 2;
				break;
			}

			case SECRET_MAP:
			{
				UINT16 mapnum = M_UnlockableMapNum(ref);
				if (mapnum < nummapheaders && mapheaderinfo[mapnum]
					&& (
					( // Check for visitation
						(mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
						|| (mapheaderinfo[mapnum]->records.mapvisited & MV_VISITED)
					) && ( // Check for completion
						!(mapheaderinfo[mapnum]->menuflags & LF2_FINISHNEEDED)
						|| (mapheaderinfo[mapnum]->records.mapvisited & MV_BEATEN)
					)
				))
				{
					if (ref->majorunlock)
					{
						K_DrawMapAsFace(
							(x + 5) + (32*(FRACUNIT-accordion))/(2*FRACUNIT), (y + 5),
							tileflags,
							mapnum,
							NULL, accordion, 2
						);
					}
					else
					{
						K_DrawMapAsFace(
							(x + 2) + (16*(FRACUNIT-accordion))/(2*FRACUNIT), (y + 2),
							tileflags,
							mapnum,
							NULL, accordion, 1
						);
					}
					pat = NULL;
				}
				iconid = 0; //14; -- This one suits a little better for "go complete this level normally"
				break;
			}
			case SECRET_ALTMUSIC:
				iconid = 16;
				break;

			case SECRET_HARDSPEED:
				iconid = 3;
				break;
			case SECRET_MASTERMODE:
				iconid = 4;
				break;
			case SECRET_ENCORE:
				iconid = 5;
				break;

			case SECRET_ONLINE:
				iconid = 10;
				break;
			case SECRET_ADDONS:
				iconid = 12;
				break;
			case SECRET_EGGTV:
				iconid = 11;
				break;
			case SECRET_SOUNDTEST:
				iconid = 1;
				break;
			case SECRET_ALTTITLE:
				iconid = 6;
				break;

			case SECRET_TIMEATTACK:
				iconid = 7;
				break;
			case SECRET_PRISONBREAK:
				iconid = 8;
				break;
			case SECRET_SPECIALATTACK:
				iconid = 9;
				break;
			case SECRET_SPBATTACK:
				iconid = 15;
				break;

			default:
			{
				if (!colormap && ref->color != SKINCOLOR_NONE && ref->color < numskincolors)
				{
					colormap = R_GetTranslationColormap(TC_RAINBOW, ref->color, GTC_MENUCACHE);
				}
				break;
			}
		}

		if (pat == missingpat)
		{
			pat = W_CachePatchName(va("UN_IC%02u%c",
				iconid,
				ref->majorunlock ? 'B' : 'A'),
				PU_CACHE);
			if (pat == missingpat)
			{
				pat = W_CachePatchName(va("UN_IC%02u%c",
					iconid,
					ref->majorunlock ? 'A' : 'B'),
					PU_CACHE);
			}
		}
	}

	if (pat)
	{
		siz = (SHORT(pat->width) << FRACBITS);

		if (!siz)
			; // prevent div/0
		else if (ref->majorunlock)
		{
			V_DrawStretchyFixedPatch(
				((x + 5)*FRACUNIT) + (32*(FRACUNIT-accordion))/2, (y + 5)*FRACUNIT,
				FixedDiv(32*accordion, siz),
				FixedDiv(32 << FRACBITS, siz),
				tileflags, pat,
				colormap
			);
		}
		else
		{
			V_DrawStretchyFixedPatch(
				((x + 2)*FRACUNIT) + (16*(FRACUNIT-accordion))/2, (y + 2)*FRACUNIT,
				FixedDiv(16*accordion, siz),
				FixedDiv(16 << FRACBITS, siz),
				tileflags, pat,
				colormap
			);
		}
	}

drawborder:

	if (num < MAXUNLOCKABLES && gamedata->unlockpending[num])
	{
		const INT32 area = (ref->majorunlock) ? 42 : 20;
		INT32 val;
		for (i = 0; i < area; i++)
		{
			val = (x + i + challengesmenu.ticker) % 40;
			if (val >= 20)
				val = 40 - val;
			val = (val + 6)/5;
			V_DrawFadeFill(x + i, y, 1, area, 0, flashmap[98 + val], 2);
		}
	}

	if (hili)
	{
		boolean maj = (ref != NULL && ref->majorunlock);
		char buffer[9];
		sprintf(buffer, "UN_RETA1");
		buffer[6] = maj ? 'B' : 'A';
		buffer[7] = (skullAnimCounter/5) ? '2' : '1';
		pat = W_CachePatchName(buffer, PU_CACHE);

		V_DrawFixedPatch(
			x*FRACUNIT, y*FRACUNIT,
			FRACUNIT,
			0, pat,
			flashmap
		);
	}

#ifdef DEVELOP
	if (
		(
			cv_debugchallenges.value == -2
			|| cv_debugchallenges.value > 0
		)
		&& num < MAXUNLOCKABLES
	)
	{
		// Display the conditionset for this tile.
		V_DrawThinString(x, y,
			num+1 == cv_debugchallenges.value ? V_AQUAMAP : V_GRAYMAP,
			va("%u", num+1));
	}
#endif
}

#define challengetransparentstrength 8

void M_DrawCharacterIconAndEngine(INT32 x, INT32 y, UINT16 skin, UINT8 *colormap, UINT16 baseskin)
{
	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT,
		FRACUNIT,
		0, faceprefix[skin][FACE_RANK],
		colormap);

	if (skin != baseskin)
	{
		V_DrawScaledPatch(x, y + 11, 0, W_CachePatchName("ALTSDOT", PU_CACHE));
	}

	V_DrawFadeFill(x+16, y, 16, 16, 0, 31, challengetransparentstrength);

	V_DrawFill(x+16+5,   y+1,    1, 14,  0);
	V_DrawFill(x+16+5+5, y+1,    1, 14,  0);
	V_DrawFill(x+16+1,   y+5,   14,  1,  0);
	V_DrawFill(x+16+1,   y+5+5, 14,  1,  0);

	INT32 s, w;

	if (skins[baseskin]->flags & SF_IRONMAN)
	{
		// this is the last thing i will do for rr pre-launhc ~toast 150424
		// quoth tyron: "stat block rave holy shit"

		UINT32 funnywomantimer = (gamedata->totalmenutime/4);
		funnywomantimer %= (8*2);
		if (funnywomantimer <= 8)
		{
			s = funnywomantimer % 3;
			w = (funnywomantimer/3) % 3;

			// snake!
			if (w == 1)
				s = 2 - s;
		}
		else
		{
			funnywomantimer -= 8;

			s = 2 - ((funnywomantimer/3) % 3);
			w = 2 - (funnywomantimer % 3);

			// snake other way!
			if (s == 1)
				w = 2 - w;
		}

	}
	else if (skins[baseskin]->flags & SF_HIVOLT)
	{
		UINT32 fucktimer = (gamedata->totalmenutime/2)%8;
		UINT8 sq[] = {0, 1, 2, 2, 2, 1, 0, 0};
		UINT8 wq[] = {0, 0, 0, 1, 2, 2, 2, 1};
		s = sq[fucktimer];
		w = wq[fucktimer];
	}
	else
	{
		// The following is a partial duplication of R_GetEngineClass

		s = (skins[skin]->kartspeed - 1)/3;
		w = (skins[skin]->kartweight - 1)/3;

		#define LOCKSTAT(stat) \
			if (stat < 0) { stat = 0; } \
			if (stat > 2) { stat = 2; }
			LOCKSTAT(s);
			LOCKSTAT(w);
		#undef LOCKSTAT
	}

	V_DrawFill(x+16 + (s*5), y + (w*5), 6, 6, 0);
}

static const char* M_DrawChallengePreview(INT32 x, INT32 y)
{
	unlockable_t *ref = NULL;
	UINT8 *colormap = NULL;
	UINT16 specialmap = NEXTMAP_INVALID;

	if (challengesmenu.currentunlock >= MAXUNLOCKABLES)
	{
		return NULL;
	}

	// Funny question mark?
	if (!gamedata->unlocked[challengesmenu.currentunlock])
	{
		spritedef_t *sprdef = &sprites[SPR_UQMK];
		spriteframe_t *sprframe;
		patch_t *patch;
		UINT32 useframe;
		UINT32 addflags = 0;

		if (!sprdef->numframes)
		{
			return NULL;
		}

		useframe = (challengesmenu.ticker / 2) % sprdef->numframes;

		sprframe = &sprdef->spriteframes[useframe];
		patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);

		if (sprframe->flip & 1) // Only for first sprite
		{
			addflags ^= V_FLIP; // This sprite is left/right flipped!
		}

		V_DrawFixedPatch(x*FRACUNIT, (y+2)*FRACUNIT, FRACUNIT, addflags, patch, NULL);
		return NULL;
	}

	// Okay, this is what we want to draw.
	ref = &unlockables[challengesmenu.currentunlock];

	const char *actiontext = NULL;

	switch (ref->type)
	{
		case SECRET_SKIN:
		{
			INT32 skin = M_UnlockableSkinNum(ref), i;
			// Draw our character!
			if (skin != -1)
			{
				colormap = R_GetTranslationColormap(skin, skins[skin]->prefcolor, GTC_MENUCACHE);
				M_DrawCharacterSprite(x, y, skin, SPR2_STIN, 7, 0, 0, colormap);

				y = (BASEVIDHEIGHT-14);

				if (setup_numplayers <= 1 && cv_lastprofile[0].value != PROFILE_GUEST)
				{
					profile_t *pr = PR_GetProfile(cv_lastprofile[0].value);

					actiontext = (pr && strcmp(pr->skinname, skins[skin]->name))
						? "<a> <sky>Set on Profile"
						: "<a_pressed> <gray>Set on Profile";

					y -= 14;
				}

				for (i = 0; i < skin; i++)
				{
					if (!R_SkinUsable(-1, i, false))
						continue;
					if (skins[i]->kartspeed != skins[skin]->kartspeed)
						continue;
					if (skins[i]->kartweight != skins[skin]->kartweight)
						continue;

					colormap = R_GetTranslationColormap(i, skins[i]->prefcolor, GTC_MENUCACHE);
					break;
				}

				M_DrawCharacterIconAndEngine(4, y-6, i, colormap, skin);
			}
			break;
		}
		case SECRET_FOLLOWER:
		{
			INT32 skin = R_SkinAvailableEx(cv_skin[0].string, false);
			INT32 fskin = M_UnlockableFollowerNum(ref);

			// Draw proximity reference for character
			if (skin == -1)
				skin = 0;
			colormap = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_MENUCACHE);
			M_DrawCharacterSprite(x, y, skin, SPR2_STIN, 7, 0, 0, colormap);

			if (horngoner)
			{
				return "<a_pressed> <gray>MISSING.";
			}

			// Draw follower next to them
			if (fskin != -1)
			{
				UINT16 col = K_GetEffectiveFollowerColor(followers[fskin].defaultcolor, &followers[fskin], cv_playercolor[0].value, skins[skin]);
				colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);
				M_DrawFollowerSprite(x - 16, y, fskin, false, 0, colormap, NULL);

				y = (BASEVIDHEIGHT-14);

				if (setup_numplayers <= 1 && cv_lastprofile[0].value != PROFILE_GUEST)
				{
					profile_t *pr = PR_GetProfile(cv_lastprofile[0].value);

					if (pr && strcmp(pr->follower, followers[fskin].name))
					{
						actiontext = (followers[fskin].hornsound == sfx_melody)
							? "<a> <aqua>Set on Profile"
							: "<a> <sky>Set on Profile";
					}
				}

				if (!actiontext)
				{
					if (followers[fskin].hornsound == sfx_melody)
					{
						actiontext = "<a_animated> <aqua>Play Ancient Melody?";
					}
					else if (challengesmenu.hornposting >= EASEOFFHORN)
						actiontext = "<a> <red>Time to die";
					else if (challengesmenu.hornposting >= (EASEOFFHORN-5))
					{
						if (challengesmenu.hornposting == EASEOFFHORN)
							actiontext = "Time to die";
						else
							actiontext = "I asked politely";
						actiontext = va("%s%s",
							(M_MenuConfirmPressed(0)
								? "<a_pressed> <yellow>"
								: "<a> <red>"
							), actiontext
						);
					}
					else if (challengesmenu.hornposting >= (EASEOFFHORN-10))
					{
						actiontext = M_MenuConfirmPressed(0)
							? "<a_pressed> <yellow>Ease off the horn"
							: "<a> <orange>Ease off the horn";
					}
					else switch (challengesmenu.hornposting % 4)
					{
						default:
							actiontext = "<a_animated> <sky>Say hello";
							break;
						case 1:
							actiontext = "<a_animated> <sky>Express your feelings";
							break;
						case 2:
							actiontext = "<a_animated> <sky>Celebrate victory";
							break;
						case 3:
							actiontext = "<a_animated> <sky>Announce you are pressing horn";
							break;
					}
				}

				y -= 14;

				if (followers[fskin].category < numfollowercategories)
				{
					V_DrawFixedPatch(4*FRACUNIT, (y - 6)*FRACUNIT,
						FRACUNIT,
						0, W_CachePatchName(followercategories[followers[fskin].category].icon, PU_CACHE),
						NULL);
				}
			}
			break;
		}
		case SECRET_COLOR:
		{
			INT32 colorid = M_UnlockableColorNum(ref);
			if (colorid == SKINCOLOR_NONE)
				break;
			INT32 skin = R_SkinAvailableEx(cv_skin[0].string, false);
			if (skin == -1)
				skin = 0;
			colormap = R_GetTranslationColormap(skin, colorid, GTC_MENUCACHE);

			// Draw reference for character bathed in coloured slime
			M_DrawCharacterSprite(x, y, skin, SPR2_STIN, 7, 0, 0, colormap);

			if (setup_numplayers <= 1 && cv_lastprofile[0].value != PROFILE_GUEST)
			{
				profile_t *pr = PR_GetProfile(cv_lastprofile[0].value);

				actiontext = (pr && pr->color != colorid)
					? "<a> <sky>Set on Profile"
					: "<a_pressed> <gray>Set on Profile";
			}

			break;
		}
		case SECRET_CUP:
		{
			levelsearch_t templevelsearch;
			UINT32 i, id, maxid, offset;
			cupheader_t *temp = M_UnlockableCup(ref);

			if (!temp)
				break;

			templevelsearch.cup = temp;
			templevelsearch.typeoflevel = 0; // doesn't matter...
			templevelsearch.grandprix = true; // this will overwrite
			templevelsearch.cupmode = true;
			templevelsearch.timeattack = false;
			templevelsearch.tutorial = false;
			templevelsearch.checklocked = true;

			M_DrawCupPreview(146, &templevelsearch);

			maxid = id = (temp->id % (CUPMENU_COLUMNS * CUPMENU_ROWS));
			offset = (temp->id - id) * 2;
			while (temp && maxid < (CUPMENU_COLUMNS * CUPMENU_ROWS))
			{
				maxid++;
				temp = temp->next;
			}

			y = (BASEVIDHEIGHT-(4+16));
			if (challengesmenu.cache_secondrowlocked == true)
				y += 8;

			V_DrawFadeFill(
				4,
				y,
				28 + offset,
				(challengesmenu.cache_secondrowlocked ? 8 : 16),
				0,
				31,
				challengetransparentstrength
			);

			for (i = 0; i < offset; i += 4)
			{
				V_DrawFill(4+1 + i, y+3,   2, 2, 15);

				if (challengesmenu.cache_secondrowlocked == false)
					V_DrawFill(4+1 + i, y+8+3, 2, 2, 15);
			}

			for (i = 0; i < CUPMENU_COLUMNS; i++)
			{
				if (templevelsearch.cup && id == i)
				{
					V_DrawFill(offset + 4   + (i*4), y,     4, 8, 0);
				}
				else if (i < maxid)
				{
					V_DrawFill(offset + 4+1 + (i*4), y+3, 2, 2, 0);
				}

				if (templevelsearch.cup && id == i+CUPMENU_COLUMNS)
				{
					V_DrawFill(offset + 4 + (i*4), y+8, 4, 8, 0);
				}
				else if (challengesmenu.cache_secondrowlocked == true)
					;
				else if (i+CUPMENU_COLUMNS < maxid)
				{
					V_DrawFill(offset + 4+1 + (i*4), y+8+3, 2, 2, 0);
				}
			}

			break;
		}
		case SECRET_MAP:
		{
			boolean validdraw = false;
			const char *gtname = "Find your prize...";
			UINT16 mapnum = M_UnlockableMapNum(ref);

			y = (BASEVIDHEIGHT-14);

			if (mapnum >= nummapheaders
				|| mapheaderinfo[mapnum] == NULL
				|| mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU)
			{
				gtname = "INVALID HEADER";
			}
			else if (
				( // Check for visitation
					(mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
					|| (mapheaderinfo[mapnum]->records.mapvisited & MV_VISITED)
				) && ( // Check for completion
					!(mapheaderinfo[mapnum]->menuflags & LF2_FINISHNEEDED)
					|| (mapheaderinfo[mapnum]->records.mapvisited & MV_BEATEN)
				)
			)
			{
				validdraw = true;
			}

			if (validdraw)
			{
				K_DrawMapThumbnail(
					(x-50)<<FRACBITS, (146+2)<<FRACBITS,
					80<<FRACBITS,
					0,
					mapnum,
					NULL);

				INT32 guessgt = G_GuessGametypeByTOL(mapheaderinfo[mapnum]->typeoflevel);

				if (guessgt == -1)
				{
					// No Time Attack support, so specify...
					gtname = "Match Race/Online";
				}
				else
				{
					if (guessgt == GT_VERSUS)
					{
						// Fudge since there's no Versus-specific menu right now...
						guessgt = GT_SPECIAL;
					}

					if (setup_numplayers <= 1 && guessgt == GT_TUTORIAL)
					{
						// Only for 1p
						actiontext = "<a_animated> <orange>Play Tutorial";
						gtname = NULL;
					}
					else if (guessgt == GT_SPECIAL && !M_SecretUnlocked(SECRET_SPECIALATTACK, true))
					{
						gtname = "???";
					}
					else
					{
						gtname = gametypes[guessgt]->name;
					}
				}
			}
			else
			{
				V_DrawFixedPatch(
					(x-50)<<FRACBITS, (146+2)<<FRACBITS,
					FRACUNIT,
					0,
					unvisitedlvl[challengesmenu.ticker % 4],
					NULL);
			}

			if (gtname)
			{
				V_DrawThinString(4, y, 0, gtname);
			}

			break;
		}
		case SECRET_ENCORE:
		{
			static UINT16 encoremapcache = NEXTMAP_INVALID;
			if (encoremapcache > nummapheaders)
			{
				encoremapcache = G_RandMap(G_TOLFlag(GT_RACE), UINT16_MAX, true, false, NULL);
			}
			specialmap = encoremapcache;
			break;
		}
		case SECRET_TIMEATTACK:
		{
			static UINT16 tamapcache = NEXTMAP_INVALID;
			if (tamapcache > nummapheaders)
			{
				tamapcache = G_RandMap(G_TOLFlag(GT_RACE), UINT16_MAX, true, false, NULL);
			}
			specialmap = tamapcache;
			break;
		}
		case SECRET_PRISONBREAK:
		{
			static UINT16 btcmapcache = NEXTMAP_INVALID;
			if (btcmapcache > nummapheaders)
			{
				btcmapcache = G_RandMap(G_TOLFlag(GT_BATTLE), UINT16_MAX, true, false, NULL);
			}
			specialmap = btcmapcache;
			break;
		}
		case SECRET_SPECIALATTACK:
		{
			static UINT16 sscmapcache = NEXTMAP_INVALID;
			if (sscmapcache > nummapheaders)
			{
				sscmapcache = G_RandMap(G_TOLFlag(GT_SPECIAL), UINT16_MAX, true, false, NULL);
			}
			specialmap = sscmapcache;
			break;
		}
		case SECRET_SPBATTACK:
		{
			static UINT16 spbmapcache = NEXTMAP_INVALID;
			if (spbmapcache > nummapheaders)
			{
				spbmapcache = G_RandMap(G_TOLFlag(GT_RACE), UINT16_MAX, true, false, NULL);
			}
			specialmap = spbmapcache;
			break;
		}
		case SECRET_HARDSPEED:
		{
			static UINT16 hardmapcache = NEXTMAP_INVALID;
			if (hardmapcache > nummapheaders)
			{
				hardmapcache = G_RandMap(G_TOLFlag(GT_RACE), UINT16_MAX, true, false, NULL);
			}
			specialmap = hardmapcache;
			break;
		}
		case SECRET_MASTERMODE:
		{
			static UINT16 mastermapcache = NEXTMAP_INVALID;
			if (mastermapcache > nummapheaders)
			{
				mastermapcache = G_RandMap(G_TOLFlag(GT_RACE), UINT16_MAX, true, false, NULL);
			}
			specialmap = mastermapcache;
			break;
		}
		case SECRET_ONLINE:
		{
			V_DrawFixedPatch(-3*FRACUNIT, (y-40)*FRACUNIT,
				FRACUNIT,
				0, W_CachePatchName("EGGASTLA", PU_CACHE),
				NULL);
			break;
		}
		case SECRET_ADDONS:
		{
			V_DrawFixedPatch(28*FRACUNIT, (BASEVIDHEIGHT-28)*FRACUNIT,
				FRACUNIT,
				0, W_CachePatchName("M_ICOADD", PU_CACHE),
				NULL);
			break;
		}
		case SECRET_SOUNDTEST:
		{
			V_DrawFixedPatch(28*FRACUNIT, (BASEVIDHEIGHT-28)*FRACUNIT,
				FRACUNIT,
				0, W_CachePatchName("M_ICOSTM", PU_CACHE),
				NULL);
			break;
		}
		case SECRET_EGGTV:
		{
			V_DrawFixedPatch(3*FRACUNIT, (BASEVIDHEIGHT-40)*FRACUNIT,
				FRACUNIT,
				0, W_CachePatchName(
					va("RHTVSQN%c", (challengesmenu.ticker & 2) ? '5' : '6'),
				PU_CACHE),
				NULL);
			break;
		}
		case SECRET_ALTTITLE:
		{
			x = 4;
			y = BASEVIDHEIGHT-14;
			V_DrawGamemodeString(x, y - 33, 0, R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_MENUCACHE), M_UseAlternateTitleScreen() ? "On" : "Off");

			K_DrawGameControl(x, y, 0, "<a_animated> Toggle", 0, TINY_FONT, 0);
			// K_drawButtonAnim(x, y, 0, kp_button_a[1], challengesmenu.ticker);
			// x += SHORT(kp_button_a[1][0]->width);
			// V_DrawThinString(x, y + 1, highlightflags, "Toggle");


			break;
		}
		case SECRET_ALTMUSIC:
		{
			UINT16 map = M_UnlockableMapNum(ref);
			if (map >= nummapheaders
				|| !mapheaderinfo[map])
			{
				break;
			}

			UINT8 musicid;
			for (musicid = 1; musicid < MAXMUSNAMES; musicid++)
			{
				if (mapheaderinfo[map]->cache_muslock[musicid - 1] == challengesmenu.currentunlock)
					break;
			}

			if (musicid == MAXMUSNAMES)
			{
				break;
			}

			const char *tune = "challenge_altmusic";

			SINT8 pushed = 0;
			const boolean epossible = (M_SecretUnlocked(SECRET_ENCORE, true)
				&& musicid < mapheaderinfo[map]->encoremusname_size);

			if (challengesmenu.nowplayingtile == ((challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy)
			&& Music_Playing(tune))
			{
				const char *song = Music_Song(tune);
				if (epossible
				&& strcmp(song, mapheaderinfo[map]->encoremusname[musicid]) == 0)
					pushed = -1;
				else if (musicid < mapheaderinfo[map]->musname_size
				&& strcmp(song, mapheaderinfo[map]->musname[musicid]) == 0)
					pushed = 1;
			}

			// Draw CD
			{
				spritedef_t *sprdef = &sprites[SPR_ALTM];
				spriteframe_t *sprframe;
				patch_t *patch = NULL;
				UINT32 addflags = 0;

				x -= 10;
				y += 15;

				if (sprdef->numframes)
				{
#ifdef ROTSPRITE
					spriteinfo_t *sprinfo = &spriteinfo[SPR_ALTM];
					INT32 rollangle = 0;
					if (pushed != 0)
					{
						rollangle = (Music_Elapsed(tune) % (ROTANGLES/2))*2;
						if (rendertimefrac >= FRACUNIT/2)
						{
							// A fun interp ability: inbetweens
							rollangle++;
						}
						if (pushed > 0)
						{
							rollangle = ((ROTANGLES-1) - rollangle);
						}
					}
#endif

					sprframe = &sprdef->spriteframes[0];

#ifdef ROTSPRITE
					if (rollangle)
					{
						patch = Patch_GetRotatedSprite(sprframe, 0, 0, (sprframe->flip & 1), false, sprinfo, rollangle);
					}
#endif
					if (!patch)
					{
						patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
						if (sprframe->flip & 1) // Only for first sprite
						{
							addflags ^= V_FLIP; // This sprite is left/right flipped!
						}
					}

					V_DrawFixedPatch(x*FRACUNIT, (y+2)*FRACUNIT, FRACUNIT/2, addflags, patch, NULL);
				}
			}

			if (musicid < mapheaderinfo[map]->musname_size)
			{
				actiontext = (pushed > 0)
					? "<a_animated> <sky>Stop CD"
					: "<a_animated> <sky>Play CD";
			}

			if (epossible)
			{
				const char *secondtext = (pushed < 0)
					? "<l_animated> <magenta>E Stop"
					: "<l_animated> <magenta>E Side";
				if (actiontext)
				{
					// weird encoded height
					actiontext = va("\x1""%s\n%s",
						(pushed < 0)
							? "<l_animated> <magenta>E Stop"
							: "<l_animated> <magenta>E Side",
						actiontext
					);
				}
				else
				{
					actiontext = secondtext;
				}
			}
		}
		default:
		{
			break;
		}
	}

	if (specialmap == NEXTMAP_INVALID || !ref)
		return actiontext;

	x -= 50;
	y = 146+2;

	K_DrawMapThumbnail(
		(x)<<FRACBITS, (y)<<FRACBITS,
		80<<FRACBITS,
		(ref->type == SECRET_ENCORE) ? V_FLIP : 0,
		specialmap,
		NULL);

	if (ref->type == SECRET_ENCORE)
	{
		static angle_t rubyfloattime = 0;
		const fixed_t rubyheight = FINESINE(rubyfloattime>>ANGLETOFINESHIFT);
		V_DrawFixedPatch((x+40)<<FRACBITS, ((y+25)<<FRACBITS) - (rubyheight<<1), FRACUNIT, 0, W_CachePatchName("RUBYICON", PU_CACHE), NULL);
		rubyfloattime += FixedMul(ANGLE_MAX/NEWTICRATE, renderdeltatics);
	}
	else if (ref->type == SECRET_SPBATTACK)
	{
		V_DrawFixedPatch((x+40-25)<<FRACBITS, ((y+25-25)<<FRACBITS),
			FRACUNIT, 0,
			W_CachePatchName(K_GetItemPatch(KITEM_SPB, false), PU_CACHE),
			NULL);
	}
	else if (ref->type == SECRET_HARDSPEED)
	{
		V_DrawFixedPatch((x+40-25)<<FRACBITS, ((y+25-25)<<FRACBITS),
			FRACUNIT, 0,
			W_CachePatchName(K_GetItemPatch(KITEM_ROCKETSNEAKER, false), PU_CACHE),
			NULL);
	}
	else if (ref->type == SECRET_MASTERMODE)
	{
		V_DrawFixedPatch((x+40-25)<<FRACBITS, ((y+25-25)<<FRACBITS),
			FRACUNIT, 0,
			W_CachePatchName(K_GetItemPatch(KITEM_JAWZ, false), PU_CACHE),
			NULL);
	}
	else
	{
		colormap = R_GetTranslationColormap(TC_DEFAULT, M_GetCvPlayerColor(0), GTC_MENUCACHE);
		V_DrawFixedPatch((x+40)<<FRACBITS, ((y+25)<<FRACBITS),
			FRACUNIT/2, 0,
			W_CachePatchName("K_LAPE02", PU_CACHE),
			colormap);
	}

	return actiontext;
}

#define challengesgridstep 22

static void M_DrawChallengeKeys(INT32 tilex, INT32 tiley)
{
	const UINT8 pid = 0;

	patch_t *key = W_CachePatchName("UN_CHA00", PU_CACHE);
	INT32 offs = challengesmenu.unlockcount[CMC_CHAONOPE];
	if (offs & 1)
		offs = -offs;
	offs /= 2;

	fixed_t keyx = (8+offs)*FRACUNIT, keyy = 0;

	const boolean keybuttonpress = (menumessage.active == false && M_MenuExtraHeld(pid) == true);

	// Button prompt
	K_DrawGameControl(
		24, 16,
		0, keybuttonpress ? "<c_pressed>" : "<c>",
		0, TINY_FONT, 0
	);

	// Metyr of rounds played that contribute to Chao Key generation
	{
		const INT32 keybarlen = 32, keybary = 28;

		offs = keybarlen;
		if (gamedata->chaokeys < GDMAX_CHAOKEYS)
		{
		#if (GDCONVERT_ROUNDSTOKEY != 32)
			offs = ((gamedata->pendingkeyroundoffset * keybarlen)/GDCONVERT_ROUNDSTOKEY);
		#else
			offs = gamedata->pendingkeyroundoffset;
		#endif
		}

		if (offs > 0)
			V_DrawFill(1+2, keybary, offs, 1, 0);
		if (offs < keybarlen)
			V_DrawFadeFill(1+2+offs, keybary, keybarlen-offs, 1, 0, 31, challengetransparentstrength);
	}

	// Counter
	{
		INT32 textx = 4, texty = 20-challengesmenu.unlockcount[CMC_CHAOANIM];
		UINT8 numbers[4];
		numbers[0] = ((gamedata->chaokeys / 100) % 10);
		numbers[1] = ((gamedata->chaokeys / 10) % 10);
		numbers[2] = (gamedata->chaokeys % 10);

		numbers[3] = ((gamedata->chaokeys / 1000) % 10);
		if (numbers[3] != 0)
		{
			V_DrawScaledPatch(textx - 4, texty, 0, kp_facenum[numbers[3]]);
			textx += 2;
		}

		UINT8 i = 0;
		while (i < 3)
		{
			V_DrawScaledPatch(textx, texty, 0, kp_facenum[numbers[i]]);
			textx += 6;
			i++;
		}
	}

	// Hand
	if (challengesmenu.keywasadded == true)
	{
		INT32 handx = 32 + 16;
		if (keybuttonpress == false)
		{
			// Only animate if it's the focus
			handx -= (skullAnimCounter/5);
		}

		V_DrawScaledPatch(handx, 8, V_FLIP,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}

	UINT8 keysbeingused = 0;

	// The Chao Key swooping animation
	if (challengesmenu.currentunlock < MAXUNLOCKABLES && challengesmenu.chaokeyhold)
	{
		fixed_t baseradius = challengesgridstep;

		boolean major = false, ending = false;
		if (unlockables[challengesmenu.currentunlock].majorunlock == true)
		{
			major = true;
			tilex += challengesgridstep/2;
			tiley += challengesgridstep/2;
			baseradius = (7*baseradius)/4;
		}

		const INT32 chaohold_duration =
			CHAOHOLD_PADDING
			+ (major
				? CHAOHOLD_MAJOR
				: CHAOHOLD_STANDARD
			);

		if (challengesmenu.chaokeyhold >= chaohold_duration - CHAOHOLD_END)
		{
			ending = true;
			baseradius = ((chaohold_duration - challengesmenu.chaokeyhold)*baseradius)*(FRACUNIT/CHAOHOLD_END);
		}

		INT16 specifickeyholdtime = challengesmenu.chaokeyhold;

		for (; keysbeingused < (major ? 10 : 1); keysbeingused++, specifickeyholdtime -= (CHAOHOLD_STANDARD/10))
		{
			fixed_t radius = baseradius;
			fixed_t thiskeyx, thiskeyy;
			fixed_t keyholdrotation = 0;

			if (specifickeyholdtime < CHAOHOLD_BEGIN)
			{
				if (specifickeyholdtime <= 0)
				{
					// Nothing following will be relevant
					break;
				}

				radius = (specifickeyholdtime*radius)*(FRACUNIT/CHAOHOLD_BEGIN);
				thiskeyx = keyx + specifickeyholdtime*((tilex*FRACUNIT) - keyx)/CHAOHOLD_BEGIN;
				thiskeyy = keyy + specifickeyholdtime*((tiley*FRACUNIT) - keyy)/CHAOHOLD_BEGIN;
			}
			else
			{
				keyholdrotation = (-36 * keysbeingused) * FRACUNIT; // 360/10

				if (ending == false)
				{
					radius <<= FRACBITS;

					keyholdrotation += 360 * ((challengesmenu.chaokeyhold - CHAOHOLD_BEGIN))
						* (FRACUNIT/(CHAOHOLD_STANDARD)); // intentionally not chaohold_duration

					if (keysbeingused == 0)
					{
						INT32 time = (major ? 5 : 3) - (keyholdrotation - 1) / (90 * FRACUNIT);
						if (time <= 5 && time >= 0)
							V_DrawScaledPatch(tilex + 2, tiley - 2, 0, kp_eggnum[time]);
					}
				}

				thiskeyx = tilex*FRACUNIT;
				thiskeyy = tiley*FRACUNIT;
			}

			if (radius != 0)
			{
				angle_t ang = (FixedAngle(
					keyholdrotation
					) >> ANGLETOFINESHIFT) & FINEMASK;

				thiskeyx += FixedMul(radius, FINESINE(ang));
				thiskeyy -= FixedMul(radius, FINECOSINE(ang));
			}

			V_DrawFixedPatch(thiskeyx, thiskeyy, FRACUNIT, 0, key, NULL);
		}
	}

	// The final Chao Key on the stack
	{
		UINT8 *lastkeycolormap = NULL;

		if (gamedata->chaokeys <= keysbeingused)
		{
			// Greyed out if there's going to be none left
			lastkeycolormap = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_MENUCACHE);
		}

		V_DrawFixedPatch(keyx, keyy, FRACUNIT, 0, key, lastkeycolormap);

		// Extra glowverlay if you can use a Chao Key
		if (keysbeingused == 0 && M_CanKeyHiliTile())
		{
			INT32 trans = (((challengesmenu.ticker/5) % 6) - 3);
			if (trans)
			{
				trans = ((trans < 0)
					? (10 + trans)
					: (10 - trans)
				) << V_ALPHASHIFT;

				V_DrawFixedPatch(keyx, keyy, FRACUNIT, trans, key,
					R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_MENUCACHE)
				);
			}
		}
	}
}

static void M_DrawChallengeScrollBar(UINT8 *flashmap)
{
#ifdef DEVELOP
	extern consvar_t cv_debugchallenges;
#endif

	const INT32 bary = 4, barh = 1, hiliw = 1;

	if (!gamedata->challengegrid || !gamedata->challengegridwidth)
		return;

	const INT32 barlen = gamedata->challengegridwidth*hiliw;

	INT32 barx = (BASEVIDWIDTH - barlen)/2;
	if (barlen > 200)
	{
		// TODO I DONT KNOW IF THE MATHS IS WRONG BUT WE DON'T HAVE
		// 200 COLUMNS YET SO KICKING CAN DOWN THE ROAD ~toast 190324
		INT32 shif = barlen - 200;
		barx -= (shif/2 + (shif * challengesmenu.col)/barlen);
	}

	// bg
	V_DrawFadeFill(barx, bary, barlen, barh, 0, 31, challengetransparentstrength);

	// This was a macro for experimentation
	#define COLTOPIX(col) (col*hiliw)
		//((col * barlen)/gamedata->challengegridwidth)

	INT32 hilix, nextstep, i, numincolumn, completionamount, skiplevel;

	// selection
	hilix = COLTOPIX(challengesmenu.col);
	V_DrawFill(barx + hilix, bary-1,    hiliw, 1, 0);
	V_DrawFill(barx + hilix, bary+barh, hiliw, 1, 0);

	// unbounded so that we can do the last remaining completionamount draw
	nextstep = numincolumn = completionamount = skiplevel = 0;
	for (i = 0; ; i++)
	{
		INT32 prevstep = nextstep;
		nextstep = (i % CHALLENGEGRIDHEIGHT);
		if (prevstep >= nextstep)
		{
			if (completionamount > 0 && numincolumn > 0)
			{
				if (completionamount >= numincolumn)
				{
					// If any have been skipped, we subtract a little for awareness...
					completionamount = skiplevel ? 9 : 10;
				}
				else
				{
					// Ordinary 0-10 calculation.
					completionamount = (completionamount*10)/numincolumn;
				}

				V_DrawFadeFill(barx + hilix, bary, hiliw, barh, 0, 1, completionamount);
			}

			numincolumn = completionamount = skiplevel = 0;
			hilix = i/CHALLENGEGRIDHEIGHT;
			hilix = COLTOPIX(hilix);
		}

		// DO NOT DEREFERENCE gamedata->challengegrid[i] UNTIL AFTER THIS
		if (i >= gamedata->challengegridwidth*CHALLENGEGRIDHEIGHT)
			break;

		if (gamedata->challengegrid[i] >= MAXUNLOCKABLES)
			continue;

		// Okay, confirmed not a gap.
		numincolumn++;

#ifdef DEVELOP
		if (cv_debugchallenges.value > 0
		&& cv_debugchallenges.value == gamedata->challengegrid[i]+1)
		{
			V_DrawFill(barx + hilix, bary, hiliw, barh, (challengesmenu.ticker & 2) ? 0 : 32);

			// The debug fill overrides everything else.
			completionamount = -1;
		}
#endif

		if (i == challengesmenu.nowplayingtile && Music_Playing("challenge_altmusic"))
		{
			V_DrawFill(barx + hilix, bary, hiliw, barh, (challengesmenu.ticker & 2) ? 177 : 122);

			// The now-playing fill overrides everything else.
			completionamount = -1;
		}

		if (completionamount == -1)
			continue;

		if (gamedata->unlocked[gamedata->challengegrid[i]])
		{
			completionamount++;

			unlockable_t *ref = &unlockables[gamedata->challengegrid[i]];

			if (!skiplevel && M_Achieved(ref->conditionset - 1) == false)
			{
				skiplevel = 1;
			}
		}

		if (gamedata->unlockpending[gamedata->challengegrid[i]] == false)
			continue;

		INT32 val = (hilix + challengesmenu.ticker) % 40;
		if (val >= 20)
			val = 40 - val;
		val = (val + 6)/10;

		V_DrawFill(barx + hilix, bary, hiliw, barh, flashmap[99 + val]);

		// The pending fill overrides everything else.
		completionamount = -1;
	}

	#undef COLTOPIX
}

void M_DrawChallenges(void)
{
	INT32 x = currentMenu->x, explodex, selectx = 0, selecty = 0;
	INT32 y;
	INT16 i, j;
	const char *str;
	INT16 offset;

	{
#define questionslow 4 // slows down the scroll by this factor
#define questionloop (questionslow*100) // modulo
		INT32 questionoffset;
		double questionoffset_f;
		patch_t *bg = W_CachePatchName("BGUNLCKG", PU_CACHE);
		patch_t *qm = W_CachePatchName("BGUNLSC", PU_CACHE);

		questionoffset_f = fmod(challengesmenu.ticker + FixedToFloat(rendertimefrac), questionloop);
		questionoffset = floor(questionoffset_f);

		// Background gradient
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, bg, NULL);

		// Scrolling question mark overlay
		V_DrawFixedPatch(
			-((160 + questionoffset)*FRACUNIT)/questionslow,
			-(4*FRACUNIT) - (245*(FixedDiv((questionloop - questionoffset)*FRACUNIT, questionloop*FRACUNIT))),
			FRACUNIT,
			V_MODULATE,
			qm,
			NULL);
#undef questionslow
#undef questionloop
	}

	// Do underlay for everything else early so the bottom of the reticule doesn't get shaded over.
	if (challengesmenu.currentunlock < MAXUNLOCKABLES)
	{
		y = 120;

		V_DrawScaledPatch(0, y,
			(10-challengetransparentstrength)<<V_ALPHASHIFT,
			W_CachePatchName("MENUHINT", PU_CACHE));

		V_DrawFadeFill(0, y+27, BASEVIDWIDTH, BASEVIDHEIGHT - (y+27), 0, 31, challengetransparentstrength);
	}

	if (gamedata->challengegrid == NULL || challengesmenu.extradata == NULL)
	{
		V_DrawCenteredMenuString(x, y, V_REDMAP, "No challenges available!?");
		goto challengedesc;
	}

	UINT8 *flashmap = R_GetTranslationColormap(TC_DEFAULT, M_GetCvPlayerColor(0), GTC_MENUCACHE);

	y = currentMenu->y;

	V_DrawFadeFill(0, y-2, BASEVIDWIDTH, (challengesgridstep * CHALLENGEGRIDHEIGHT) + 2, 0, 31, challengetransparentstrength);

	x -= (challengesgridstep-1);

	x += challengesmenu.offset;

	x += Easing_OutQuad(M_DueFrac(challengesmenu.move.start, 4), challengesgridstep * challengesmenu.move.dist, 0);

	if (challengegridloops)
	{
		if (!challengesmenu.col && challengesmenu.hilix)
			x -= gamedata->challengegridwidth*challengesgridstep;
		i = challengesmenu.col + challengesmenu.focusx;
		explodex = x - (i*challengesgridstep);

		while (x < BASEVIDWIDTH-challengesgridstep)
		{
			i = (i + 1) % gamedata->challengegridwidth;
			x += challengesgridstep;
		}
	}
	else
	{
		if (gamedata->challengegridwidth & 1)
			x += (challengesgridstep/2);

		i = gamedata->challengegridwidth-1;
		explodex = x - (i*challengesgridstep)/2;
		x += (i*challengesgridstep)/2;
	}

	selectx = explodex + (challengesmenu.hilix*challengesgridstep);
	selecty = currentMenu->y + (challengesmenu.hiliy*challengesgridstep);

	while (i >= 0 && x >= -(challengesgridstep*2))
	{
		y = currentMenu->y-challengesgridstep;
		for (j = 0; j < CHALLENGEGRIDHEIGHT; j++)
		{
			y += challengesgridstep;

			if (challengesmenu.extradata[(i * CHALLENGEGRIDHEIGHT) + j].flags & CHE_DONTDRAW)
			{
				continue;
			}

			if (x == selectx && j == challengesmenu.hiliy)
			{
				continue;
			}

			M_DrawChallengeTile(i, j, x, y, flashmap, false);
		}

		x -= challengesgridstep;
		i--;
		if (challengegridloops && i < 0)
		{
			i = (i + gamedata->challengegridwidth)
				% gamedata->challengegridwidth;
		}
	}

	if (challengesmenu.fade)
		V_DrawFadeScreen(31, challengesmenu.fade);

	M_DrawChallengeScrollBar(flashmap);

	M_DrawChallengeTile(
		challengesmenu.hilix,
		challengesmenu.hiliy,
		selectx,
		selecty,
		flashmap,
		true);
	M_DrawCharSelectExplosions(false, explodex, currentMenu->y);

challengedesc:

	// Name bar
	{
		y = 120;

		if (challengesmenu.currentunlock < MAXUNLOCKABLES)
		{
			str = unlockables[challengesmenu.currentunlock].name;
			if (!gamedata->unlocked[challengesmenu.currentunlock])
			{
				str = "???"; //M_CreateSecretMenuOption(str);
			}

			offset = V_LSTitleLowStringWidth(str, 0) / 2;
			V_DrawLSTitleLowString(BASEVIDWIDTH/2 - offset, y+6, 0, str);
		}
	}

	// Wings
	{
		const INT32 endy = 18, endlen = 38;
		patch_t *endwing = W_CachePatchName("K_BOSB01", PU_CACHE);

		V_DrawFill(0, endy, endlen, 11, 24);
		V_DrawFixedPatch(endlen*FRACUNIT, endy*FRACUNIT, FRACUNIT, V_FLIP, endwing, NULL);

		V_DrawFill(BASEVIDWIDTH - endlen, endy, endlen, 11, 24);
		V_DrawFixedPatch((BASEVIDWIDTH - endlen)*FRACUNIT, endy*FRACUNIT, FRACUNIT, 0, endwing, NULL);
	}

	// Percentage
	{
		patch_t *medal = W_CachePatchName(
			va("UN_MDL%c", '0' + challengesmenu.unlockcount[CMC_MEDALID]),
			PU_CACHE
		);

		fixed_t medalchopy = 1;

		for (i = CMC_MEDALBLANK; i <= CMC_MEDALFILLED; i++)
		{
			if (challengesmenu.unlockcount[i] == 0)
				continue;

			V_SetClipRect(
				0,
				medalchopy << FRACBITS,
				BASEVIDWIDTH << FRACBITS,
				(medalchopy + challengesmenu.unlockcount[i]) << FRACBITS,
				0
			);

			UINT8 *medalcolormap = NULL;
			if (i == CMC_MEDALBLANK)
			{
				medalcolormap = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_MENUCACHE);
			}
			else if (challengesmenu.unlockcount[CMC_MEDALID] == 0)
			{
				medalcolormap = R_GetTranslationColormap(TC_DEFAULT, M_GetCvPlayerColor(0), GTC_MENUCACHE);
			}

			V_DrawFixedPatch((BASEVIDWIDTH - 31)*FRACUNIT, 1*FRACUNIT, FRACUNIT, 0, medal, medalcolormap);

			V_ClearClipRect();

			medalchopy += challengesmenu.unlockcount[i];
		}

		INT32 textx = BASEVIDWIDTH - 21, texty = 20-challengesmenu.unlockcount[CMC_ANIM];
		UINT8 numbers[3];
		numbers[0] = ((challengesmenu.unlockcount[CMC_PERCENT] / 100) % 10);
		numbers[1] = ((challengesmenu.unlockcount[CMC_PERCENT] / 10) % 10);
		numbers[2] = (challengesmenu.unlockcount[CMC_PERCENT] % 10);

		patch_t *percent = W_CachePatchName("K_SPDML1", PU_CACHE);

		V_DrawScaledPatch(textx + 2, texty, 0, percent);

		i = 3;
		while (i)
		{
			i--;
			textx -= 6;
			V_DrawScaledPatch(textx, texty, 0, kp_facenum[numbers[i]]);
		}
	}

	// Chao Key information
	M_DrawChallengeKeys(selectx, selecty);

	// Derived from M_DrawCharSelectPreview
	x = 40;
	y = BASEVIDHEIGHT-16;

	// Unlock preview
	const char *actiontext = M_DrawChallengePreview(x, y);

	// Conditions for unlock
	// { -- please don't call va() anywhere between here...
	i = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;

	if (challengesmenu.unlockcondition != NULL
	&& challengesmenu.currentunlock < MAXUNLOCKABLES
	&& ((gamedata->unlocked[challengesmenu.currentunlock] == true)
		|| ((challengesmenu.extradata != NULL)
		&& (challengesmenu.extradata[i].flags & CHE_HINT))
		)
	)
	{
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 120 + 32, 0, challengesmenu.unlockcondition);
	}

	// Extracted from M_DrawCharSelectPreview for ordering reasons
	if (actiontext && actiontext[0])
	{
		x = 4;
		y = (BASEVIDHEIGHT-14);
		if (actiontext[0] < '\x5')
		{
			// weird encoded height, supports max 5 rows
			y -= (13 * actiontext[0]);
			actiontext++;
		}
		K_DrawGameControl(
			x, y, 0,
			actiontext,
			0, TINY_FONT, 0
		);
		// } -- ...and here (since actiontext needs it)
	}
}

#undef challengetransparentstrength
#undef challengesgridstep

// Statistics menu

#define STATSSTEP 10

static INT32 M_DrawMapMedals(INT32 mapnum, INT32 x, INT32 y, boolean allowtime, boolean allowencore, boolean allowspb, boolean allowbonus, boolean draw)
{
	UINT8 lasttype = UINT8_MAX, curtype;

	// M_GetLevelEmblems is ONE-indexed, urgh
	emblem_t *emblem = M_GetLevelEmblems(mapnum+1);

	boolean collected = false, hasmedals = false;

	while (emblem)
	{
		collected = gamedata->collected[emblem-emblemlocations];
		switch (emblem->type)
		{
			case ET_TIME:
			{
				if (!allowtime
				|| (!collected && emblem->tag == AUTOMEDAL_PLATINUM))
				{
					emblem = M_GetLevelEmblems(-1);
					continue;
				}
				curtype = 1;
				break;
			}
			case ET_GLOBAL:
			{
				if (emblem->flags & GE_NOTMEDAL)
				{
					emblem = M_GetLevelEmblems(-1);
					continue;
				}
				curtype = 2;
				break;
			}
			case ET_MAP:
			{
				if (((emblem->flags & ME_ENCORE) && !allowencore)
				|| ((emblem->flags & ME_SPBATTACK) && !allowspb))
				{
					emblem = M_GetLevelEmblems(-1);
					continue;
				}
				curtype = 0;
				break;
			}
			default:
			{
				curtype = 0;
				break;
			}
		}

		// Shift over if emblem is of a different discipline
		if (lasttype != curtype)
		{
			if (lasttype != UINT8_MAX)
				x -= 4;
			else
				hasmedals = true;

			lasttype = curtype;
		}

		if (!draw)
			;
		else if (collected)
			V_DrawMappedPatch(x, y, 0, W_CachePatchName(M_GetEmblemPatch(emblem, false), PU_CACHE),
				R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_MENUCACHE));
		else
			V_DrawScaledPatch(x, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));

		emblem = M_GetLevelEmblems(-1);
		x -= 8;
	}

	if (!allowbonus)
		return x;

	if (hasmedals)
		x -= 4;

	if (mapheaderinfo[mapnum]->records.spraycan == MCAN_BONUS)
	{
		if (draw)
			V_DrawScaledPatch(x, y, 0, W_CachePatchName("GOTBON", PU_CACHE));

		x -= 8;
	}
	else if (mapheaderinfo[mapnum]->records.spraycan < gamedata->numspraycans)
	{
		UINT16 col = gamedata->spraycans[mapheaderinfo[mapnum]->records.spraycan].col;

		if (draw && col < numskincolors)
		{
			V_DrawMappedPatch(x, y, 0, W_CachePatchName("GOTCAN", PU_CACHE),
				R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE));
			//V_DrawRightAlignedThinString(x - 2, y, 0, skincolors[col].name);
		}
		x -= 8;
	}

	if (mapheaderinfo[mapnum]->records.mapvisited & MV_MYSTICMELODY)
	{
		if (draw)
			V_DrawScaledPatch(x, y, 0, W_CachePatchName("GOTMEL", PU_CACHE));

		x -= 8;
	}

	return x;
}

static void M_DrawStatsMaps(void)
{
	INT32 y = 70, i;
	INT16 mnum;
	boolean dotopname = true, dobottomarrow = (statisticsmenu.location < statisticsmenu.maxscroll);
	INT32 location = statisticsmenu.location;

	tic_t besttime = 0;

	if (!statisticsmenu.maplist)
	{
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 70, 0, "No maps!?");
		return;
	}

	INT32 mapsunfinished = 0, medalspos;

	char *medalcountstr = va("x %d/%d", statisticsmenu.gotmedals, statisticsmenu.nummedals);

	V_DrawThinString(30, 60, 0, medalcountstr);
	V_DrawMappedPatch(20, 60, 0, W_CachePatchName("GOTITA", PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_GOLD, GTC_MENUCACHE));

	if (gamedata->numspraycans)
	{
		medalspos = 30 + V_ThinStringWidth(medalcountstr, 0);
		medalcountstr = va("x %d/%d", gamedata->gotspraycans + statisticsmenu.numcanbonus, gamedata->numspraycans);
		V_DrawThinString(20 + medalspos, 60, 0, medalcountstr);
		V_DrawMappedPatch(10 + medalspos, 60, 0, W_CachePatchName("GOTCAN", PU_CACHE),
										   R_GetTranslationColormap(TC_DEFAULT, gamedata->spraycans[0].col, GTC_MENUCACHE));
	}
	else if (statisticsmenu.numcanbonus)
	{
		medalspos = 30 + V_ThinStringWidth(medalcountstr, 0);
		medalcountstr = va("x %d", statisticsmenu.numcanbonus);
		V_DrawThinString(20 + medalspos, 60, 0, medalcountstr);
		V_DrawScaledPatch(10 + medalspos, 60, 0, W_CachePatchName("GOTBON", PU_CACHE));
	}

	medalspos = BASEVIDWIDTH - 20;

	boolean timeattack[3];
	timeattack[0] = M_SecretUnlocked(SECRET_TIMEATTACK, true);
	timeattack[1] = M_SecretUnlocked(SECRET_PRISONBREAK, true);
	timeattack[2] = M_SecretUnlocked(SECRET_SPECIALATTACK, true);

	if (timeattack[0] || timeattack[1] || timeattack[2])
	{
		medalspos -= 64;

		for (i = 0; i < nummapheaders; i++)
		{
			// Check for no visibility
			if (!mapheaderinfo[i] || (mapheaderinfo[i]->menuflags & (LF2_NOTIMEATTACK|LF2_HIDEINMENU)))
				continue;

			// Has to be accessible via time attack
			if (!(mapheaderinfo[i]->typeoflevel & (TOL_RACE|TOL_BATTLE|TOL_SPECIAL|TOL_VERSUS)))
				continue;

			if (mapheaderinfo[i]->records.timeattack.time <= 0)
			{
				mapsunfinished++;
				continue;
			}

			besttime += mapheaderinfo[i]->records.timeattack.time;
		}

		V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 60, 0,
			va(
				"Combined time: %c%i:%02i:%02i.%02i (%s)",
				(mapsunfinished ? '\x85' : '\x80'),
				G_TicsToHours(besttime),
				G_TicsToMinutes(besttime, false),
				G_TicsToSeconds(besttime),
				G_TicsToCentiseconds(besttime),
				(mapsunfinished ? "incomplete" : "complete")
			)
		);
	}

	if (location)
		V_DrawMenuString(10, 80-(skullAnimCounter/5),
			highlightflags, "\x1A"); // up arrow

	i = -1;

	const boolean allowsealed = M_SecretUnlocked(SECRET_SPECIALATTACK, true);
	const boolean allowencore = M_SecretUnlocked(SECRET_ENCORE, true);
	const boolean allowspb = M_SecretUnlocked(SECRET_SPBATTACK, true);
	boolean allowtime = false;

	while ((mnum = statisticsmenu.maplist[++i]) != NEXTMAP_INVALID)
	{
		if (location)
		{
			--location;
			continue;
		}

		if (dotopname || mnum >= nummapheaders)
		{
			if (mnum >= nummapheaders)
			{
				mnum = statisticsmenu.maplist[1+i];
				if (mnum >= nummapheaders)
					mnum = statisticsmenu.maplist[i-1];
			}

			if (mnum < nummapheaders)
			{
				const char *str;

				if (mapheaderinfo[mnum]->typeoflevel & TOL_TUTORIAL)
					str = "TUTORIAL MODE";
				else if (mapheaderinfo[mnum]->cup
				&& (!(mapheaderinfo[mnum]->typeoflevel & TOL_SPECIAL) // not special
				|| gamedata->sealedswaps[GDMAX_SEALEDSWAPS-1] != NULL // all found
				|| mapheaderinfo[mnum]->cup->id >= basenumkartcupheaders // custom content
				|| allowsealed)) // true order
					str = va("%s CUP", mapheaderinfo[mnum]->cup->realname);
				else
					str = "LOST & FOUND";

				V_DrawThinString(20,  y, highlightflags, str);
			}

			if (dotopname)
			{
				V_DrawRightAlignedThinString(medalspos, y, highlightflags, "MEDALS");

				if (timeattack[0] || timeattack[1] || timeattack[2])
					V_DrawRightAlignedThinString((BASEVIDWIDTH-20), y, highlightflags, "TIME");

				dotopname = false;
			}

			y += STATSSTEP;
			if (y >= BASEVIDHEIGHT-STATSSTEP)
				goto bottomarrow;

			continue;
		}

		V_DrawFadeFill(24, y + 5, (BASEVIDWIDTH - 24) - 24, 3, 0, 31, 8 - (i & 1)*2);

		allowtime = (
				(timeattack[0] && (mapheaderinfo[mnum]->typeoflevel & TOL_RACE))
			|| (timeattack[1] && (mapheaderinfo[mnum]->typeoflevel & TOL_BATTLE))
			|| (timeattack[2] && (mapheaderinfo[mnum]->typeoflevel & (TOL_SPECIAL|TOL_VERSUS)))
		);

		if (!(mapheaderinfo[mnum]->menuflags & LF2_NOTIMEATTACK) && allowtime)
		{
			besttime = mapheaderinfo[mnum]->records.timeattack.time;

			const char *todrawtext = "--'--\"--";

			if (besttime)
			{
				todrawtext = va("%02d'%02d\"%02d",
					G_TicsToMinutes(besttime, true),
					G_TicsToSeconds(besttime),
					G_TicsToCentiseconds(besttime)
				);
			}

			K_drawKartMicroTime(
				todrawtext,
				(BASEVIDWIDTH-24),
				y,
				(besttime ? 0 : V_TRANSLUCENT)
			);
		}

		M_DrawMapMedals(mnum, medalspos - 8, y, allowtime, allowencore, allowspb, true, true);

		if (mapheaderinfo[mnum]->menuttl[0])
		{
			V_DrawThinString(24, y, V_FORCEUPPERCASE, mapheaderinfo[mnum]->menuttl);
		}
		else
		{
			char *title = G_BuildMapTitle(mnum+1);
			V_DrawThinString(24, y, V_FORCEUPPERCASE, title);
			Z_Free(title);
		}

		y += STATSSTEP;

		if (y >= BASEVIDHEIGHT-STATSSTEP)
			goto bottomarrow;
	}
	if (location)
		--location;

	if (statisticsmenu.numextramedals == 0)
		goto bottomarrow;

	// Extra Emblem headers
	for (i = 0; i < 2; ++i)
	{
		if (i == 1)
		{
			V_DrawThinString(20, y, highlightflags, "EXTRA MEDALS");
			if (location)
			{
				y += STATSSTEP;
				location++;
			}
		}
		if (location)
		{
			--location;
			continue;
		}

		y += STATSSTEP;

		if (y >= BASEVIDHEIGHT-STATSSTEP)
			goto bottomarrow;
	}

	// Extra Emblems
	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (unlockables[i].type != SECRET_EXTRAMEDAL)
		{
			continue;
		}

		if (location)
		{
			--location;
			continue;
		}

		{
			if (gamedata->unlocked[i])
			{
				UINT16 color = min(unlockables[i].color, numskincolors-1);
				if (!color)
					color = SKINCOLOR_GOLD;
				V_DrawMappedPatch(291, y+1, 0, W_CachePatchName("GOTITA", PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, color, GTC_MENUCACHE));
			}
			else
			{
				V_DrawScaledPatch(291, y+1, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			V_DrawThinString(24, y, 0, va("%s", unlockables[i].name));
		}

		y += STATSSTEP;

		if (y >= BASEVIDHEIGHT-STATSSTEP)
			goto bottomarrow;
	}
bottomarrow:
	if (dobottomarrow)
		V_DrawMenuString(10, BASEVIDHEIGHT-20 + (skullAnimCounter/5),
			highlightflags, "\x1B"); // down arrow
}

#undef STATSSTEP
#define STATSSTEP 18

static void M_DrawStatsChars(void)
{
	INT32 y = 80, i, j;
	INT16 skin;
	boolean dobottomarrow = (statisticsmenu.location < statisticsmenu.maxscroll);
	INT32 location = statisticsmenu.location;

	if (!statisticsmenu.maplist || !statisticsmenu.nummaps)
	{
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 70, 0, "No chars!?");
		return;
	}

	if (location)
		V_DrawMenuString(10, y-(skullAnimCounter/5),
			highlightflags, "\x1A"); // up arrow

	i = -1;

	V_DrawThinString(20, y - 10, highlightflags, "CHARACTER");
	V_DrawRightAlignedThinString(BASEVIDWIDTH/2 + 34, y - 10, highlightflags, "WINS/ROUNDS");

	while ((skin = statisticsmenu.maplist[++i]) < numskins)
	{
		if (location)
		{
			--location;
			continue;
		}

		{
			UINT8 *colormap = R_GetTranslationColormap(skin, skins[skin]->prefcolor, GTC_MENUCACHE);

			M_DrawCharacterIconAndEngine(24, y, skin, colormap, skin);
		}

		V_DrawThinString(24+32+2, y+3, 0, skins[skin]->realname);

		V_DrawRightAlignedThinString(BASEVIDWIDTH/2 + 30, y+3, 0, va("%d/%d", skins[skin]->records.wins, skins[skin]->records.rounds));

		y += STATSSTEP;

		if (y >= BASEVIDHEIGHT-STATSSTEP)
			goto bottomarrow;
	}

bottomarrow:
	if (dobottomarrow)
		V_DrawMenuString(10, BASEVIDHEIGHT-20 + (skullAnimCounter/5),
			highlightflags, "\x1B"); // down arrow

	UINT32 x = BASEVIDWIDTH - 20 - 90;
	y = 88;

	V_DrawCenteredThinString(x + 45, y - 10, highlightflags, "HEATMAP");

	V_DrawFadeFill(x, y, 91, 91, 0, 31, 8); // challengetransparentstrength

	V_DrawFill(x+30, y+1,  1, 89,  0);
	V_DrawFill(x+60, y+1,  1, 89,  0);
	V_DrawFill(x+1,  y+30, 89, 1,  0);
	V_DrawFill(x+1,  y+60, 89, 1,  0);

	x++;
	y++;

	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
		{
			if (statisticsmenu.statgridplayed[i][j] == 0)
				continue;

			V_DrawFill(
				x + (i * 10),
				y + (j * 10),
				9,
				9,
				31 - ((statisticsmenu.statgridplayed[i][j] - 1) * 32) / FRACUNIT
			);
		}
	}
}

#undef STATSSTEP
#define STATSSTEP 21

static void M_DrawStatsGP(void)
{
	INT32 y = 80, i, x, j, endj;
	INT16 id;
	boolean dobottomarrow = (statisticsmenu.location < statisticsmenu.maxscroll);
	INT32 location = statisticsmenu.location;

	if (!statisticsmenu.maplist || !statisticsmenu.nummaps)
	{
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 70, 0, "No cups!?");
		return;
	}

	if (location)
		V_DrawMenuString(10, y-(skullAnimCounter/5),
			highlightflags, "\x1A"); // up arrow

	const INT32 width = 53;

	endj = KARTSPEED_NORMAL;
	if (M_SecretUnlocked(SECRET_HARDSPEED, true))
	{
		endj = M_SecretUnlocked(SECRET_MASTERMODE, true)
			? KARTGP_MASTER
			: KARTSPEED_HARD;
	}

	const INT32 h = (21 * min(5, statisticsmenu.nummaps)) - 1;

	x = 7 + BASEVIDWIDTH - 20 - width;
	for (j = endj; j >= KARTSPEED_EASY; j--, x -= width)
	{
		if (j == KARTSPEED_EASY || !gamedata->everseenspecial)
		{
			V_DrawFadeFill(x + 6, y + 1, width - (12 + 1), h, 0, 31, 6 + (j & 1)*2);
			V_DrawCenteredThinString(x + 19 + 7, y - 10, highlightflags|V_FORCEUPPERCASE, gpdifficulty_cons_t[j].strvalue);
			x += (12 + 1);
		}
		else
		{
			V_DrawFadeFill(x - 7, y + 1, width, h, 0, 31, 6 + (j & 1)*2);
			V_DrawCenteredThinString(x + 19, y - 10, highlightflags|V_FORCEUPPERCASE, gpdifficulty_cons_t[j].strvalue);
		}
	}

	i = -1;

	V_DrawThinString(20, y - 10, highlightflags, "CUP");

	cupheader_t *cup = kartcupheaders;

	while ((id = statisticsmenu.maplist[++i]) < numkartcupheaders)
	{
		if (location)
		{
			--location;
			continue;
		}

		// If we have ANY sort of sorting other than instantiation, this won't work
		while (cup && cup->id != id)
		{
			cup = cup->next;
		}

		if (!cup)
		{
			goto bottomarrow;
		}

		V_DrawFill(24, y+1, 21, 20, 31);

		V_DrawScaledPatch(24-1, y, 0, W_CachePatchName(cup->icon, PU_CACHE));
		V_DrawScaledPatch(24-1, y, 0, W_CachePatchName("CUPBOX", PU_CACHE));

		V_DrawThinString(24+21+2, y + 7, 0, cup->realname);

		x = 7 + BASEVIDWIDTH - 20 - width;
		for (j = endj; j >= KARTSPEED_EASY; j--)
		{
			x -= (M_DrawCupWinData(x, y + 5, cup, j, false, true) + 2);
		}

		y += STATSSTEP;

		if (y >= BASEVIDHEIGHT-STATSSTEP)
			goto bottomarrow;
	}

bottomarrow:
	if (dobottomarrow)
		V_DrawMenuString(10, BASEVIDHEIGHT-20 + (skullAnimCounter/5),
			highlightflags, "\x1B"); // down arrow
}

#undef STATSSTEP

static void M_GetStatsTime(char *beststr, UINT32 totaltime)
{
	beststr[0] = 0;

	boolean showallsubsequent = false;

	UINT32 besttime = G_TicsToHours(totaltime);
	if (besttime)
	{
		showallsubsequent = true;
		if (besttime >= 24)
		{
			strcat(beststr, va("%u day%s, ", besttime/24, (besttime < 48 ? "" : "s")));
			besttime %= 24;
		}

		strcat(beststr, va("%u hour%s, ", besttime, (besttime == 1 ? "" : "s")));
	}
	besttime = G_TicsToMinutes(totaltime, false);
	if (besttime || showallsubsequent)
	{
		showallsubsequent = true;
		strcat(beststr, va("%u minute%s, ", besttime, (besttime == 1 ? "" : "s")));
	}
	besttime = G_TicsToSeconds(totaltime);
	strcat(beststr, va("%i second%s", besttime, (besttime == 1 ? "" : "s")));
}

static void M_DrawStatsTimeTracked(void)
{
	INT32 y = 70;
	char beststr[256];

	#define DISPLAYAMODE(str, besttime) \
	{ \
		V_DrawThinString(24, y, 0, str); \
		M_GetStatsTime(beststr, besttime); \
		V_DrawRightAlignedThinString(BASEVIDWIDTH-24, y, 0, beststr); \
		y += 10; \
	}

	DISPLAYAMODE("Race Mode", gamedata->modeplaytime[GDGT_RACE]);

	if (gamedata->roundsplayed[GDGT_PRISONS])
	{
		DISPLAYAMODE("Prison Break", gamedata->modeplaytime[GDGT_PRISONS]);
	}

	DISPLAYAMODE("Battle Mode", gamedata->modeplaytime[GDGT_BATTLE]);

	if (gamedata->roundsplayed[GDGT_SPECIAL])
	{
		DISPLAYAMODE("Special Mode", gamedata->modeplaytime[GDGT_SPECIAL]);
	}

	if (gamedata->roundsplayed[GDGT_CUSTOM])
	{
		DISPLAYAMODE("All Custom Modes", gamedata->modeplaytime[GDGT_CUSTOM]);
	}

	if (M_SecretUnlocked(SECRET_ONLINE, true))
	{
		y += 2;

		DISPLAYAMODE("Playing Online", gamedata->totalnetgametime);
	}

	if (M_SecretUnlocked(SECRET_TIMEATTACK, true)
		|| M_SecretUnlocked(SECRET_PRISONBREAK, true)
		|| M_SecretUnlocked(SECRET_SPECIALATTACK, true))
	{
		y += 2;

		DISPLAYAMODE("Time Attack Modes", gamedata->timeattackingtotaltime);

		if (M_SecretUnlocked(SECRET_SPBATTACK, true))
		{
			DISPLAYAMODE(" (SPB Attack)", gamedata->spbattackingtotaltime);
		}
	}

	if (gamedata->totaltumbletime)
	{
		y += 2;

		DISPLAYAMODE("Tumbling through the air", gamedata->totaltumbletime);
	}

	y += 2;

	DISPLAYAMODE("On Menus", gamedata->totalmenutime);
	DISPLAYAMODE(" (staring at this screen)", gamedata->totaltimestaringatstatistics);
}

void M_DrawStatistics(void)
{
	char beststr[256];

	{
		const char *pagename = NULL;
		INT32 pagenamewidth = 0;

		V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);

		switch (statisticsmenu.page)
		{
			case statisticspage_gp:
			{
				pagename = gamedata->everseenspecial
					? "GRAND PRIX & EMERALDS"
					: "GRAND PRIX";
				M_DrawStatsGP();
				break;
			}

			case statisticspage_maps:
			{
				pagename = "COURSES & MEDALS";
				M_DrawStatsMaps();
				break;
			}

			case statisticspage_chars:
			{
				pagename = "CHARACTERS & ENGINE CLASSES";
				M_DrawStatsChars();
				break;
			}

			case statisticspage_time:
			{
				pagename = "TIME TRACKED";
				M_DrawStatsTimeTracked();
				break;
			}

			default:
				break;
		}

		if (pagename)
		{
			pagenamewidth = V_ThinStringWidth(pagename, 0);
			V_DrawThinString((BASEVIDWIDTH - pagenamewidth)/2, 12, 0, pagename);
		}

		V_DrawMenuString((BASEVIDWIDTH - pagenamewidth)/2 - 10 - (skullAnimCounter/5), 12,
			0, "\x1C"); // left arrow

		V_DrawMenuString((BASEVIDWIDTH + pagenamewidth)/2 + 2 + (skullAnimCounter/5), 12,
			0, "\x1D"); // right arrow
	}

	V_DrawThinString(20, 30, highlightflags, "Total Play Time:");

	M_GetStatsTime(beststr, gamedata->totalplaytime);
	V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 30, 0, beststr);
	beststr[0] = 0;

	V_DrawThinString(20, 40, highlightflags, "Total Rings:");
	if (gamedata->totalrings > GDMAX_RINGS)
	{
		sprintf(beststr, "%c999,999,999+", '\x82');
	}
	else if (gamedata->totalrings >= 1000000)
	{
		sprintf(beststr, "%u,%03u,%03u", (gamedata->totalrings/1000000), (gamedata->totalrings/1000)%1000, (gamedata->totalrings%1000));
	}
	else if (gamedata->totalrings >= 1000)
	{
		sprintf(beststr, "%u,%03u", (gamedata->totalrings/1000), (gamedata->totalrings%1000));
	}
	else
	{
		sprintf(beststr, "%u", gamedata->totalrings);
	}
	V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 40, 0, va("%s collected", beststr));

	beststr[0] = 0;
	V_DrawThinString(20, 50, highlightflags, "Total Rounds:");

	strcat(beststr, va("%u Race", gamedata->roundsplayed[GDGT_RACE]));

	if (gamedata->roundsplayed[GDGT_PRISONS] > 0)
	{
		strcat(beststr, va(", %u Prisons", gamedata->roundsplayed[GDGT_PRISONS]));
	}

	strcat(beststr, va(", %u Battle", gamedata->roundsplayed[GDGT_BATTLE]));

	if (gamedata->roundsplayed[GDGT_SPECIAL] > 0)
	{
		strcat(beststr, va(", %u Special", gamedata->roundsplayed[GDGT_SPECIAL]));
	}

	if (gamedata->roundsplayed[GDGT_CUSTOM] > 0)
	{
		strcat(beststr, va(", %u Custom", gamedata->roundsplayed[GDGT_CUSTOM]));
	}

	V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 50, 0, beststr);
}

static void M_DrawWrongPlayer(UINT8 i)
{
#define wrongpl wrongwarp.wrongplayers[i]
	if (wrongpl.skin >= numskins)
		return;

	UINT8 *colormap = R_GetTranslationColormap(wrongpl.skin, skins[wrongpl.skin]->prefcolor, GTC_MENUCACHE);

	M_DrawCharacterSprite(
		wrongpl.across,
		160 - ((i & 1) ? 0 : 32),
		wrongpl.skin,
		wrongpl.spinout ? SPR2_SPIN : SPR2_SLWN,
		wrongpl.spinout ? ((wrongpl.across/8) & 7) : 6,
		(wrongwarp.ticker+i),
		0, colormap
	);
#undef wrongpl
}

void M_DrawWrongWarp(void)
{
	INT32 titleoffset = 0, titlewidth, x, y;
	const char *titletext = "WRONG GAME? WRONG GAME! ";

	if (wrongwarp.ticker < 2*TICRATE/3)
		return;

	V_DrawFadeScreen(31, min((wrongwarp.ticker - 2*TICRATE/3), 5));

	// SMK title screen recreation!?

	if (wrongwarp.ticker >= 2*TICRATE)
	{
		// Done as four calls and not a loop for the sake of render order
		M_DrawWrongPlayer(0);
		M_DrawWrongPlayer(2);
		M_DrawWrongPlayer(1);
		M_DrawWrongPlayer(3);
	}

	y = 20;

	x = BASEVIDWIDTH - 8;

	if (wrongwarp.ticker < TICRATE)
	{
		INT32 adjust = floor(pow(2, (double)(TICRATE - wrongwarp.ticker)));
		x += adjust/2;
		y += adjust;
	}

	titlewidth = V_LSTitleHighStringWidth(titletext, 0);
	titleoffset = (-wrongwarp.ticker) % titlewidth;

	while (titleoffset < BASEVIDWIDTH)
	{
		V_DrawLSTitleHighString(titleoffset, y, 0, titletext);
		titleoffset += titlewidth;
	}

	patch_t *bumper = W_CachePatchName((M_UseAlternateTitleScreen() ? "MTSJUMPR1" : "MTSBUMPR1"), PU_CACHE);
	V_DrawScaledPatch(x-(SHORT(bumper->width)), (BASEVIDHEIGHT-8)-(SHORT(bumper->height)), 0, bumper);
}

void M_DrawSoundTest(void)
{
	const UINT8 pid = 0;

	INT32 x, y, i, cursorx = 0;
	INT32 titleoffset = 0, titlewidth;
	const char *titletext;

	patch_t *btn = W_CachePatchName("STER_BTN", PU_CACHE);

	const char *tune = S_SoundTestTune(0);

	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("STER_BG", PU_CACHE), NULL);

	x = 24;
	y = 18;

	V_SetClipRect(
		x << FRACBITS, y << FRACBITS,
		272 << FRACBITS, 106 << FRACBITS,
		0
	);

	y += 32;

	if (soundtest.current != NULL)
	{
		if (soundtest.current->sequence.map < nummapheaders)
		{
			K_DrawMapThumbnail(
				0, 0,
				BASEVIDWIDTH<<FRACBITS,
				V_20TRANS|V_ADD,
				soundtest.current->sequence.map,
				NULL);

			V_DrawFixedPatch(
				0, 0,
				FRACUNIT,
				V_60TRANS|V_SUBTRACT,
				W_CachePatchName("STER_DOT", PU_CACHE),
				NULL
			);
		}

		titletext = soundtest.current->title;
		if (!titletext)
			titletext = "Untitled"; // Har har.

		V_DrawThinString(x+1, y, 0, titletext);
		if (soundtest.current->numtracks > 1)
			V_DrawThinString(x+1, (y += 10), 0, va("Track %c", 'A'+soundtest.currenttrack));
		if (soundtest.current->author)
			V_DrawThinString(x+1, (y += 10), 0, soundtest.current->author);
		if (soundtest.current->source)
			V_DrawThinString(x+1, (y += 10), 0, soundtest.current->source);
		if (soundtest.current->composers)
		{
			char *wrappedcomposers = V_ScaledWordWrap(
				(BASEVIDWIDTH - ((x+1)*2)) << FRACBITS,
				FRACUNIT, FRACUNIT, FRACUNIT,
				0, TINY_FONT,
				soundtest.current->composers
			);
			V_DrawThinString(x+1, (y += 10), 0, wrappedcomposers);
			Z_Free(wrappedcomposers);
		}
	}
	else
	{
		const char *sfxstr = (cv_soundtest.value) ? S_sfx[cv_soundtest.value].name : "N/A";

		titletext = "Sound Test";

		V_DrawThinString(x+1, y, 0, "Track ");
		V_DrawThinString(
			x+1 + V_ThinStringWidth("Track ", 0),
			y,
			0,
			va("%04X - %s", cv_soundtest.value, sfxstr)
		);
	}

	titletext = va("%s - ", titletext);
	titlewidth = V_LSTitleHighStringWidth(titletext, 0);
	titleoffset = (-soundtest.menutick) % titlewidth;

	while (titleoffset < 272)
	{
		V_DrawLSTitleHighString(x + titleoffset, 18+1, 0, titletext);
		titleoffset += titlewidth;
	}

	{
		UINT32 currenttime = min(Music_Elapsed(tune), Music_TotalDuration(tune));

		V_DrawRightAlignedThinString(x + 272-1, 18+32, 0,
			va("%02u:%02u",
				G_TicsToMinutes(currenttime, true),
				G_TicsToSeconds(currenttime)
			)
		);
	}

	if ((soundtest.playing && soundtest.current)
		&& (soundtest.current->basenoloop[soundtest.currenttrack] == true
		|| soundtest.autosequence == true))
	{
		UINT32 exittime = Music_TotalDuration(tune);

		V_DrawRightAlignedThinString(x + 272-1, 18+32+10, 0,
			va("%02u:%02u",
				G_TicsToMinutes(exittime, true),
				G_TicsToSeconds(exittime)
			)
		);
	}

	V_ClearClipRect();

	x = currentMenu->x;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (currentMenu->menuitems[i].status == IT_SPACE)
		{
			if (currentMenu->menuitems[i].mvar2 != 0)
			{
				x = currentMenu->menuitems[i].mvar2;
			}

			x += currentMenu->menuitems[i].mvar1;

			continue;
		}

		y = currentMenu->y;

		if (i == itemOn)
		{
			cursorx = x + 13;
		}

		if (currentMenu->menuitems[i].tooltip)
		{
			V_SetClipRect(
				x << FRACBITS, y << FRACBITS,
				27 << FRACBITS, 22 << FRACBITS,
				0
			);

			// Special cases
			if (currentMenu->menuitems[i].mvar2 == stereospecial_back) // back
			{
				if (!soundtest.justopened && M_MenuBackHeld(pid))
				{
					y = currentMenu->y + 7;
				}
			}
			// The following are springlocks.
			else if (currentMenu->menuitems[i].mvar2 == stereospecial_pause) // pause
			{
				if (Music_Paused(tune) == true)
					y = currentMenu->y + 6;
			}
			else if (currentMenu->menuitems[i].mvar2 == stereospecial_play) // play
			{
				if (soundtest.playing == true && Music_Paused(tune) == false)
					y = currentMenu->y + 6;
			}
			else if (currentMenu->menuitems[i].mvar2 == stereospecial_seq) // seq
			{
				if (soundtest.autosequence == true)
					y = currentMenu->y + 6;
			}
			else if (currentMenu->menuitems[i].mvar2 == stereospecial_shf) // shf
			{
				if (soundtest.shuffle == true)
					y = currentMenu->y + 6;
			}

			// Button is being pressed
			if (i == itemOn && !soundtest.justopened && M_MenuConfirmHeld(pid))
			{
				y = currentMenu->y + 7;
			}

			// Button itself
			V_DrawFixedPatch(x << FRACBITS, y << FRACBITS, FRACUNIT, 0, btn, NULL);

			// Icon
			V_DrawFixedPatch(x << FRACBITS, y << FRACBITS,
				FRACUNIT, 0,
				W_CachePatchName(currentMenu->menuitems[i].tooltip, PU_CACHE),
				NULL
			);

			// Text
			V_DrawCenteredThinString(x + 13, y + 1, 0, currentMenu->menuitems[i].text);

			V_ClearClipRect();

			V_DrawFill(x+2, currentMenu->y + 22, 23, 1, 30);
		}
		else if (currentMenu->menuitems[i].mvar2 == stereospecial_vol) // Vol
		{
			consvar_t *voltoadjust = M_GetSoundTestVolumeCvar();
			INT32 j = 0, vol = 0;
			const INT32 barheight = 22;
			patch_t *knob = NULL;
			INT32 knobflags = 0;

			if (i == itemOn)
			{
				if ((menucmd[pid].dpad_ud < 0 && (soundtest.menutick & 2)) || M_MenuConfirmPressed(pid))
				{
					knob = W_CachePatchName("STER_KNT", PU_CACHE);
					knobflags = V_FLIP;
					j = 24;
				}
				else if (menucmd[pid].dpad_ud > 0 && (soundtest.menutick & 2))
				{
					knob = W_CachePatchName("STER_KNT", PU_CACHE);
				}
			}

			if (knob == NULL)
				knob = W_CachePatchName("STER_KNB", PU_CACHE);

			V_DrawFixedPatch((x+1+j) << FRACBITS, y << FRACBITS,
				FRACUNIT, knobflags,
				knob,
				NULL
			);

			V_DrawFill(x+1+24, y+1, 5, barheight, 30);

			if (voltoadjust != NULL)
			{
				vol = (barheight*voltoadjust->value)/(MAX_SOUND_VOLUME*3);
			}

			for (j = 0; j <= barheight/3; j++)
			{
				UINT8 col = 130;

				if (j == 0)
				{
					continue;
				}

				if (j > vol)
				{
					col = 20;
				}
				else if (j > (barheight/3)-2)
				{
					col = 34;
				}

				V_DrawFill(x+1+24+2, y+1 + (barheight-(j*3)), 1, 2, col);
			}

			x += 5;
		}
		else if (currentMenu->menuitems[i].mvar2 == stereospecial_track) // Track
		{
			if (i == itemOn)
			{
				if (menucmd[pid].dpad_ud < 0 || M_MenuConfirmPressed(pid))
				{
					y--;
				}
				else if (menucmd[pid].dpad_ud > 0)
				{
					y++;
				}
			}

			V_DrawFixedPatch(x << FRACBITS, (y-1) << FRACBITS,
				FRACUNIT, 0,
				W_CachePatchName("STER_WH0", PU_CACHE),
				NULL
			);
		}
		else
		{
			V_DrawCenteredThinString(x + 13, y + 1, 0, currentMenu->menuitems[i].text);
		}

		x += 25;
	}

	V_DrawMenuString(cursorx - 4, currentMenu->y - 8 - (skullAnimCounter/5),
		V_SNAPTOTOP|highlightflags, "\x1B"); // up arrow
}

#ifdef HAVE_DISCORDRPC
void M_DrawDiscordRequests(void)
{
	discordRequest_t *curRequest = discordRequestList;
	UINT8 *colormap;
	patch_t *hand = NULL;

	const char *wantText = "...would like to join!";

	INT32 x = 100;
	INT32 y = 133;

	INT32 slide = 0;
	INT32 maxYSlide = 18;

	if (discordrequestmenu.confirmDelay > 0)
	{
		if (discordrequestmenu.confirmAccept == true)
		{
			colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_GREEN, GTC_MENUCACHE);
			hand = W_CachePatchName("K_LAPH02", PU_CACHE);
		}
		else
		{
			colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_RED, GTC_MENUCACHE);
			hand = W_CachePatchName("K_LAPH03", PU_CACHE);
		}

		slide = discordrequestmenu.confirmLength - discordrequestmenu.confirmDelay;
	}
	else
	{
		colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_GREY, GTC_MENUCACHE);
	}

	V_DrawFixedPatch(56*FRACUNIT, 150*FRACUNIT, FRACUNIT, 0, W_CachePatchName("K_LAPE01", PU_CACHE), colormap);

	if (hand != NULL)
	{
		fixed_t handoffset = (4 - abs((signed)(skullAnimCounter - 4))) * FRACUNIT;
		V_DrawFixedPatch(56*FRACUNIT, 150*FRACUNIT + handoffset, FRACUNIT, 0, hand, NULL);
	}

	K_DrawSticker(x + (slide * 32), y - 2, V_ThinStringWidth(M_GetDiscordName(curRequest), 0), 0, false);
	V_DrawThinString(x + (slide * 32), y - 1, V_YELLOWMAP, M_GetDiscordName(curRequest));

	K_DrawSticker(x, y + 12, V_ThinStringWidth(wantText, 0), 0, true);
	V_DrawThinString(x, y + 10, 0, wantText);

	/*
	K_DrawSticker(x, y + 26, stickerWidth, 0, true);
	K_DrawGameControl(x, y+22, 0, "<a_animated>", 0, TINY_FONT, V_SNAPTORIGHT);
	// K_drawButtonAnim(x, y + 22, V_SNAPTORIGHT, kp_button_a[1], discordrequestmenu.ticker);
	*/

	UINT32 bigwidth = K_DrawGameControl(x, y+22, 0, "<a_animated> Accept   <b_animated> <x_animated> Decline", 0, TINY_FONT, V_SNAPTORIGHT);
	K_DrawSticker(x, y + 26, bigwidth, 0, true);
	K_DrawGameControl(x, y+22, 0, "<a_animated> Accept   <b_animated> <x_animated> Decline", 0, TINY_FONT, V_SNAPTORIGHT);

	/*
	V_DrawThinString((x + xoffs), y + 24, 0, acceptText);
	xoffs += acceptTextWidth;

	K_drawButtonAnim((x + xoffs), y + 22, V_SNAPTORIGHT, kp_button_b[1], discordrequestmenu.ticker);
	xoffs += declineButtonWidth;

	xoffs += K_DrawGameControl(x + xoffs, y+22, 0, "<x_animated>", 0, TINY_FONT, V_SNAPTORIGHT);
	K_drawButtonAnim((x + xoffs), y + 22, V_SNAPTORIGHT, kp_button_x[1], discordrequestmenu.ticker);
	xoffs += altDeclineButtonWidth;

	V_DrawThinString((x + xoffs), y + 24, 0, declineText);

	*/

	y -= 18;

	while (curRequest->next != NULL)
	{
		INT32 ySlide = min(slide * 4, maxYSlide);

		curRequest = curRequest->next;

		const char *discordname = M_GetDiscordName(curRequest);

		K_DrawSticker(x, y - 1 + ySlide, V_ThinStringWidth(discordname, 0), 0, false);
		V_DrawThinString(x, y + ySlide, 0, discordname);

		y -= 12;
		maxYSlide = 12;
	}
}
#endif
