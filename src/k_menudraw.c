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

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

#define SKULLXOFF -32
#define LINEHEIGHT 16
#define STRINGHEIGHT 8
#define FONTBHEIGHT 20
#define SMALLLINEHEIGHT 8
#define SLIDER_RANGE 10
#define SLIDER_WIDTH (8*SLIDER_RANGE+6)
#define SERVERS_PER_PAGE 11


// horizontally centered text
static void M_CentreText(INT32 xoffs, INT32 y, const char *string)
{
	INT32 x;
	//added : 02-02-98 : centre on 320, because V_DrawString centers on vid.width...
	x = ((BASEVIDWIDTH - V_StringWidth(string, V_OLDSPACING))>>1) + xoffs;
	V_DrawString(x,y,V_OLDSPACING,string);
}


//  A smaller 'Thermo', with range given as percents (0-100)
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv, boolean ontop)
{
	INT32 i;
	INT32 range;
	patch_t *p;

	for (i = 0; cv->PossibleValue[i+1].strvalue; i++);

	x = BASEVIDWIDTH - x - SLIDER_WIDTH;

	if (ontop)
	{
		V_DrawCharacter(x - 16 - (skullAnimCounter/5), y,
			'\x1C' | highlightflags, false); // left arrow
		V_DrawCharacter(x+(SLIDER_RANGE*8) + 8 + (skullAnimCounter/5), y,
			'\x1D' | highlightflags, false); // right arrow
	}

	if ((range = atoi(cv->defaultvalue)) != cv->value)
	{
		range = ((range - cv->PossibleValue[0].value) * 100 /
		(cv->PossibleValue[1].value - cv->PossibleValue[0].value));

		if (range < 0)
			range = 0;
		if (range > 100)
			range = 100;

		// draw the default
		p = W_CachePatchName("M_SLIDEC", PU_CACHE);
		V_DrawScaledPatch(x - 4 + (((SLIDER_RANGE)*8 + 4)*range)/100, y, 0, p);
	}

	V_DrawScaledPatch(x - 8, y, 0, W_CachePatchName("M_SLIDEL", PU_CACHE));

	p =  W_CachePatchName("M_SLIDEM", PU_CACHE);
	for (i = 0; i < SLIDER_RANGE; i++)
		V_DrawScaledPatch (x+i*8, y, 0,p);

	p = W_CachePatchName("M_SLIDER", PU_CACHE);
	V_DrawScaledPatch(x+SLIDER_RANGE*8, y, 0, p);

	range = ((cv->value - cv->PossibleValue[0].value) * 100 /
	 (cv->PossibleValue[1].value - cv->PossibleValue[0].value));

	if (range < 0)
		range = 0;
	if (range > 100)
		range = 100;

	// draw the slider cursor
	p = W_CachePatchName("M_SLIDEC", PU_CACHE);
	V_DrawScaledPatch(x - 4 + (((SLIDER_RANGE)*8 + 4)*range)/100, y, 0, p);
}


static patch_t *addonsp[NUM_EXT+5];

static fixed_t bgText1Scroll = 0;
static fixed_t bgText2Scroll = 0;
static fixed_t bgImageScroll = 0;
static char bgImageName[9];

#define MENUBG_TEXTSCROLL 6
#define MENUBG_IMAGESCROLL 32

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
		bgImageScroll = (BASEVIDWIDTH / 2)*FRACUNIT;
	}
}

void M_DrawMenuBackground(void)
{
	patch_t *text1 = W_CachePatchName("MENUBGT1", PU_CACHE);
	patch_t *text2 = W_CachePatchName("MENUBGT2", PU_CACHE);

	fixed_t text1loop = SHORT(text1->height)*FRACUNIT;
	fixed_t text2loop = SHORT(text2->width)*FRACUNIT;

	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUBG4", PU_CACHE), NULL);

	V_DrawFixedPatch(-bgImageScroll, 0, FRACUNIT, 0, W_CachePatchName("MENUBG1", PU_CACHE), NULL);
	V_DrawFixedPatch(-bgImageScroll, 0, FRACUNIT, 0, W_CachePatchName(bgImageName, PU_CACHE), NULL);

	V_DrawFixedPatch(0, (BASEVIDHEIGHT + 16) * FRACUNIT, FRACUNIT, V_SUBTRACT, W_CachePatchName("MENUBG2", PU_CACHE), NULL);

	V_DrawFixedPatch(8 * FRACUNIT, -bgText1Scroll,
		FRACUNIT, V_SUBTRACT, text1, NULL);
	V_DrawFixedPatch(8 * FRACUNIT, -bgText1Scroll + text1loop,
		FRACUNIT, V_SUBTRACT, text1, NULL);

	bgText1Scroll += (MENUBG_TEXTSCROLL*renderdeltatics);
	while (bgText1Scroll > text1loop)
		bgText1Scroll -= text1loop;

	V_DrawFixedPatch(-bgText2Scroll, (BASEVIDHEIGHT-8) * FRACUNIT,
		FRACUNIT, V_ADD, text2, NULL);
	V_DrawFixedPatch(-bgText2Scroll + text2loop, (BASEVIDHEIGHT-8) * FRACUNIT,
		FRACUNIT, V_ADD, text2, NULL);

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

	if (setup_numplayers == 0 || currentMenu == &PLAY_CharSelectDef || currentMenu == &MISC_ChallengesDef)
	{
		return;
	}

	x = 2;
	y = BASEVIDHEIGHT - small->height - 2;

	switch (setup_numplayers)
	{
		case 1:
		{
			x -= 8;
			V_DrawScaledPatch(x, y, 0, small);

			skin = R_SkinAvailable(cv_skin[0].string);
			color = cv_playercolor[0].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		case 2:
		{
			x -= 8;
			V_DrawScaledPatch(x, y, 0, small);
			V_DrawScaledPatch(x + PLATTER_OFFSET, y - PLATTER_STAGGER, 0, small);

			skin = R_SkinAvailable(cv_skin[1].string);
			color = cv_playercolor[1].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 22, y - PLATTER_STAGGER + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			skin = R_SkinAvailable(cv_skin[0].string);
			color = cv_playercolor[0].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		case 3:
		{
			V_DrawScaledPatch(x, y, 0, large);
			V_DrawScaledPatch(x + PLATTER_OFFSET, y - PLATTER_STAGGER, 0, small);

			skin = R_SkinAvailable(cv_skin[1].string);
			color = cv_playercolor[1].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 22, y - PLATTER_STAGGER + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			skin = R_SkinAvailable(cv_skin[0].string);
			color = cv_playercolor[0].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + 12, y - 2, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			skin = R_SkinAvailable(cv_skin[2].string);
			color = cv_playercolor[2].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		case 4:
		{
			V_DrawScaledPatch(x, y, 0, large);
			V_DrawScaledPatch(x + PLATTER_OFFSET, y - PLATTER_STAGGER, 0, large);

			skin = R_SkinAvailable(cv_skin[1].string);
			color = cv_playercolor[1].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 12, y - PLATTER_STAGGER - 2, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			skin = R_SkinAvailable(cv_skin[0].string);
			color = cv_playercolor[0].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + 12, y - 2, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			skin = R_SkinAvailable(cv_skin[3].string);
			color = cv_playercolor[3].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + PLATTER_OFFSET + 22, y - PLATTER_STAGGER + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);

			skin = R_SkinAvailable(cv_skin[2].string);
			color = cv_playercolor[2].value;
			colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

			V_DrawMappedPatch(x + 22, y + 8, 0, faceprefix[skin][FACE_MINIMAP], colormap);
			break;
		}
		default:
		{
			return;
		}
	}

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
	if ((vid.width % BASEVIDWIDTH != 0) || (vid.height % BASEVIDHEIGHT != 0))
	{
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("WEIRDRES", PU_CACHE), NULL);
	}
}

// Draws the typing submenu
static void M_DrawMenuTyping(void)
{

	INT32 i, j;

	INT32 x = 60;
	INT32 y = 100 + (9-menutyping.menutypingfade)*8;
	INT32 tflag = (9 - menutyping.menutypingfade)<<V_ALPHASHIFT;

	consvar_t *cv = currentMenu->menuitems[itemOn].itemaction.cvar;

	char buf[8];	// We write there to use drawstring for convenience.

	V_DrawFadeScreen(31, menutyping.menutypingfade);

	// Draw the string we're editing at the top.
	V_DrawString(x, y-48 + 12, V_ALLOWLOWERCASE|tflag, cv->string);
	if (skullAnimCounter < 4)
		V_DrawCharacter(x + V_StringWidth(cv->string, 0), y - 35, '_' | 0x80, false);

	// Some contextual stuff
	if (menutyping.keyboardtyping)
		V_DrawThinString(10, 175, V_ALLOWLOWERCASE|tflag|V_GRAYMAP, "Type using your keyboard. Press Enter to confirm & exit.\nUse your controller or any directional input to use the Virtual Keyboard.\n");
	else
		V_DrawThinString(10, 175, V_ALLOWLOWERCASE|tflag|V_GRAYMAP, "Type using the Virtual Keyboard. Use the \'OK\' button to confirm & exit.\nPress any keyboard key not bound to a control to use it.");


	// Now the keyboard itself
	for (i=0; i < 5; i++)
	{
		for (j=0; j < 13; j++)
		{
			INT32 mflag = 0;
			INT16 c = virtualKeyboard[i][j];
			if (menutyping.keyboardshift ^ menutyping.keyboardcapslock)
				c = shift_virtualKeyboard[i][j];


			if (c == KEY_BACKSPACE)
				strcpy(buf, "DEL");

			else if (c == KEY_RSHIFT)
				strcpy(buf, "SHIFT");

			else if (c == KEY_CAPSLOCK)
				strcpy(buf, "CAPS");

			else if (c == KEY_ENTER)
				strcpy(buf, "OK");

			else if (c == KEY_SPACE)
				strcpy(buf, "SPACE");

			else
			{
				buf[0] = c;
				buf[1] = '\0';
			}

			// highlight:
			if (menutyping.keyboardx == j && menutyping.keyboardy == i && !menutyping.keyboardtyping)
				mflag |= highlightflags;
			else if (menutyping.keyboardtyping)
				mflag |= V_TRANSLUCENT;	// grey it out if we can't use it.

			V_DrawString(x, y, V_ALLOWLOWERCASE|tflag|mflag, buf);

			x += V_StringWidth(buf, 0)+8;
		}
		x = 60;
		y += 12;
	}
}

// Draw the message popup submenu
void M_DrawMenuMessage(void)
{

	INT32 y = menumessage.y + (9-menumessage.fadetimer)*20;
	size_t i, start = 0;
	INT16 max;
	char string[MAXMENUMESSAGE];
	INT32 mlines;
	const char *msg = menumessage.message;

	if (!menumessage.active)
		return;

	mlines = menumessage.m>>8;
	max = (INT16)((UINT8)(menumessage.m & 0xFF)*8);

	V_DrawFadeScreen(31, menumessage.fadetimer);
	M_DrawTextBox(menumessage.x, y - 8, (max+7)>>3, mlines);

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

		V_DrawString((BASEVIDWIDTH - V_StringWidth(string, 0))/2, y, V_ALLOWLOWERCASE, string);
		y += 8;
	}
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

	if (menuactive)
	{
		if (gamestate == GS_MENU)
		{
			M_DrawMenuBackground();
		}
		else if (!WipeInAction && currentMenu != &PAUSE_PlaybackMenuDef)
		{
			V_DrawFadeScreen(122, 3);
		}

		if (currentMenu->drawroutine)
			currentMenu->drawroutine(); // call current menu Draw routine

		M_DrawMenuForeground();

		// Draw version down in corner
		// ... but only in the MAIN MENU.  I'm a picky bastard.
		if (currentMenu == &MainDef)
		{
			if (customversionstring[0] != '\0')
			{
				V_DrawThinString(vid.dupx, vid.height - 20*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT, "Mod version:");
				V_DrawThinString(vid.dupx, vid.height - 10*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, customversionstring);
			}
			else
			{
				F_VersionDrawer();
			}


		}

		// Draw message overlay when needed
		M_DrawMenuMessage();

		// Draw typing overlay when needed, above all other menu elements.
		if (menutyping.active)
			M_DrawMenuTyping();
	}

	if (menuwipe)
	{
		F_WipeEndScreen();
		F_RunWipe(wipe_menu_final, wipedefs[wipe_menu_final], false, "FADEMAP0", true, false);
		menuwipe = false;
	}

	// focus lost notification goes on top of everything, even the former everything
	if (window_notinfocus && cv_showfocuslost.value)
	{
		M_DrawTextBox((BASEVIDWIDTH/2) - (60), (BASEVIDHEIGHT/2) - (16), 13, 2);
		if (gamestate == GS_LEVEL && (P_AutoPause() || paused))
			V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2) - (4), highlightflags, "Game Paused");
		else
			V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2) - (4), highlightflags, "Focus Lost");
	}
}

// ==========================================================================
// GENERIC MENUS
// ==========================================================================

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
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 12, V_ALLOWLOWERCASE|V_6WIDTHSPACE, currentMenu->menuitems[itemOn].tooltip);
	}
}

// Converts a string into question marks.
// Used for the secrets menu, to hide yet-to-be-unlocked stuff.
static const char *M_CreateSecretMenuOption(const char *str)
{
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
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, highlightflags, currentMenu->menuitems[i].text);

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
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								w = V_StringWidth(cv->string, 0);
								V_DrawString(BASEVIDWIDTH - x - w, y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? warningflags : highlightflags), cv->string);
								if (i == itemOn)
								{
									V_DrawCharacter(BASEVIDWIDTH - x - 10 - w - (skullAnimCounter/5), y,
											'\x1C' | highlightflags, false); // left arrow
									V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
											'\x1D' | highlightflags, false); // right arrow
								}
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
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
				V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_HEADERTEXT: // draws 16 pixels to the left, in yellow text
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawString(x-16, y, highlightflags, currentMenu->menuitems[i].text);
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
		V_DrawString(currentMenu->x, cursory, highlightflags, currentMenu->menuitems[itemOn].text);
	}
}

#define GM_STARTX 128
#define GM_STARTY 80
#define GM_XOFFSET 17
#define GM_YOFFSET 34

//
// M_DrawKartGamemodeMenu
//
// Huge gamemode-selection list for main menu
//
void M_DrawKartGamemodeMenu(void)
{
	UINT8 n = 0;
	INT32 i, x, y;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (currentMenu->menuitems[i].status == IT_DISABLED)
		{
			continue;
		}

		n++;
	}

	n--;
	x = GM_STARTX - ((GM_XOFFSET / 2) * (n-1));
	y = GM_STARTY - ((GM_YOFFSET / 2) * (n-1));

	M_DrawMenuTooltips();

	if (menutransition.tics)
	{
		x += 48 * menutransition.tics;
	}

	for (i = 0; i < currentMenu->numitems; i++)
	{
		INT32 type;

		if (currentMenu->menuitems[i].status == IT_DISABLED)
		{
			continue;
		}

		if (i >= currentMenu->numitems-1)
		{
			x = GM_STARTX + (GM_XOFFSET * 5 / 2);
			y = GM_STARTY + (GM_YOFFSET * 5 / 2);

			if (menutransition.tics)
			{
				x += 48 * menutransition.tics;
			}
		}

		type = (currentMenu->menuitems[i].status & IT_DISPLAY);

		switch (type)
		{
			case IT_STRING:
			case IT_TRANSTEXT2:
				{
					UINT8 *colormap = NULL;

					if (i == itemOn)
					{
						colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);
					}
					else
					{
						colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_MOSS, GTC_CACHE);
					}

					V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, W_CachePatchName("MENUPLTR", PU_CACHE), colormap);
					V_DrawGamemodeString(x + 16, y - 3,
						(type == IT_TRANSTEXT2
							? V_TRANSLUCENT
							: 0
						)|V_ALLOWLOWERCASE,
						colormap,
						currentMenu->menuitems[i].text);
				}
				break;
		}

		x += GM_XOFFSET;
		y += GM_YOFFSET;
	}
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

		V_DrawString((BASEVIDWIDTH - V_StringWidth(string, 0))/2,y,V_ALLOWLOWERCASE,string);
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
			numoptions = nummenucolors;
			break;
		case CSSTEP_FOLLOWERCATEGORY:
			numoptions = setup_numfollowercategories+1;
			break;
		case CSSTEP_FOLLOWER:
			numoptions = setup_followercategories[p->followercategory][0];
			break;
		case CSSTEP_FOLLOWERCOLORS:
			numoptions = nummenucolors+2;
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
				colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);
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
					n = l = r = M_GetColorBefore(p->color, numoptions/2, false);
				}
				else if (subtract)
				{
					n = l = M_GetColorBefore(l, 1, false);
				}
				else
				{
					n = r = M_GetColorAfter(r, 1, false);
				}

				colormap = R_GetTranslationColormap(TC_DEFAULT, n, GTC_MENUCACHE);

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
						K_GetEffectiveFollowerColor(fl->defaultcolor, p->color),
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
					n = l = r = M_GetColorBefore(p->followercolor, numoptions/2, true);
				}
				else if (subtract)
				{
					n = l = M_GetColorBefore(l, 1, true);
				}
				else
				{
					n = r = M_GetColorAfter(r, 1, true);
				}

				col = K_GetEffectiveFollowerColor(n, p->color);

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
static boolean M_DrawCharacterSprite(INT16 x, INT16 y, INT16 skin, boolean charflip, boolean animate, INT32 addflags, UINT8 *colormap)
{
	UINT8 spr;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *sprpatch;
	UINT8 rotation = (charflip ? 1 : 7);
	UINT32 frame = animate ? setup_animcounter : 0;

	spr = P_GetSkinSprite2(&skins[skin], SPR2_STIN, NULL);
	sprdef = &skins[skin].sprites[spr];

	if (!sprdef->numframes) // No frames ??
		return false; // Can't render!

	frame %= sprdef->numframes;

	sprframe = &sprdef->spriteframes[frame];
	sprpatch = W_CachePatchNum(sprframe->lumppat[rotation], PU_CACHE);

	if (sprframe->flip & (1<<rotation)) // Only for first sprite
	{
		addflags ^= V_FLIP; // This sprite is left/right flipped!
	}

	if (skins[skin].highresscale != FRACUNIT)
	{
		V_DrawFixedPatch(x<<FRACBITS,
					y<<FRACBITS,
					skins[skin].highresscale,
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
	follower_t fl;
	UINT8 rotation = (charflip ? 1 : 7);

	if (p != NULL)
		followernum = p->followern;
	else
		followernum = num;

	// Don't draw if we're outta bounds.
	if (followernum < 0 || followernum >= numfollowers)
		return false;

	fl = followers[followernum];

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
			(p->mdepth < CSSTEP_FOLLOWERCOLORS) ? fl.defaultcolor : p->followercolor,
			p->color);
		sine = FixedMul(fl.bobamp, FINESINE(((FixedMul(4 * M_TAU_FIXED, fl.bobspeed) * p->follower_timer)>>ANGLETOFINESHIFT) & FINEMASK));
		colormap = R_GetTranslationColormap(TC_DEFAULT, color, GTC_MENUCACHE);
	}

	V_DrawFixedPatch((x*FRACUNIT), ((y-12)*FRACUNIT) + sine, fl.scale, addflags, patch, colormap);

	return true;
}

static void M_DrawCharSelectSprite(UINT8 num, INT16 x, INT16 y, boolean charflip)
{
	setup_player_t *p = &setup_player[num];
	UINT8 color;
	UINT8 *colormap;

	if (p->skin < 0)
		return;

	if (p->mdepth < CSSTEP_COLORS)
		color = skins[p->skin].prefcolor;
	else
		color = p->color;
	colormap = R_GetTranslationColormap(p->skin, color, GTC_MENUCACHE);

	M_DrawCharacterSprite(x, y, p->skin, charflip, (p->mdepth == CSSTEP_READY), 0, colormap);
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

	if (p->mdepth >= CSSTEP_CHARS)
	{
		M_DrawCharSelectSprite(num, x+32, y+75, charflip);
		M_DrawCharSelectCircle(p, x+32, y+64);
	}

	if (p->showextra == false)
	{
		V_DrawScaledPatch(x+9, y+2, 0, W_CachePatchName("FILEBACK", PU_CACHE));
		V_DrawScaledPatch(x, y+2, 0, W_CachePatchName(va("CHARSEL%c", letter), PU_CACHE));
		if (p->mdepth > CSSTEP_PROFILE)
		{
			profile_t *pr = PR_GetProfile(p->profilen);
			V_DrawCenteredFileString(x+16+18, y+2, 0, pr->profilename);
		}
		else
		{
			V_DrawFileString(x+16, y+2, 0, "PLAYER");
		}
	}

	if (p->mdepth >= CSSTEP_FOLLOWER)
	{
		M_DrawFollowerSprite(x+32+((charflip ? 1 : -1)*16), y+75, -1, charflip, 0, 0, p);
	}

	if ((setup_animcounter/10) & 1 && gamestate == GS_MENU)	// Not drawn outside of GS_MENU.
	{
		if (p->mdepth == CSSTEP_NONE && num == setup_numplayers)
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
		INT16 py = y+48 - p->profilen*12;
		UINT8 maxp = PR_GetNumProfiles();

		UINT8 i = 0;
		UINT8 j;

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

			if (dist > 2)
			{
				py += 12;
				continue;
			}

			if (dist == 2)
			{
				V_DrawCenteredFileString(px+26, py, notSelectable, pr->version ? pr->profilename : "NEW");
				V_DrawScaledPatch(px, py, V_TRANSLUCENT, W_CachePatchName("FILEBACK", PU_CACHE));
			}
			else
			{
				V_DrawScaledPatch(px, py, 0, W_CachePatchName("FILEBACK", PU_CACHE));

				if (i != p->profilen || ((setup_animcounter/10) & 1))
				{
					V_DrawCenteredFileString(px+26, py, notSelectable, pr->version ? pr->profilename : "NEW");
				}
			}
			py += 12;
		}

	}
	// "Changes?"
	else if (p->mdepth == CSSTEP_ASKCHANGES)
	{
		UINT8 i;
		char choices[2][9] = {"All good", "Change"};
		INT32 xpos = x+8;
		INT32 ypos = y+38;

		V_DrawFileString(xpos, ypos, 0, "READY?");

		for (i = 0; i < 2; i++)
		{
			UINT8 cy = ypos+16 + (i*10);

			if (p->changeselect == i)
				V_DrawScaledPatch(xpos, cy, 0, W_CachePatchName("M_CURSOR", PU_CACHE));

			V_DrawThinString(xpos+16, cy, (p->changeselect == i ? highlightflags : 0)|V_6WIDTHSPACE, choices[i]);
		}
	}

	if (p->showextra == true)
	{
		INT32 randomskin = 0;
		switch (p->mdepth)
		{
			case CSSTEP_ALTS: // Select clone
			case CSSTEP_READY:
				if (p->clonenum < setup_chargrid[p->gridx][p->gridy].numskins
					&& setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum] < numskins)
				{
					V_DrawThinString(x-3, y+12, V_6WIDTHSPACE,
						skins[setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum]].name);
					randomskin = (skins[setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum]].flags & SF_IRONMAN);
				}
				else
				{
					V_DrawThinString(x-3, y+12, V_6WIDTHSPACE, va("BAD CLONENUM %u", p->clonenum));
				}
				/* FALLTHRU */
			case CSSTEP_CHARS: // Character Select grid
				V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, va("Class %c (s %c - w %c)",
					('A' + R_GetEngineClass(p->gridx+1, p->gridy+1, randomskin)),
					(randomskin
						? '?' : ('1'+p->gridx)),
					(randomskin
						? '?' : ('1'+p->gridy))
					));
				break;
			case CSSTEP_COLORS: // Select color
				if (p->color < numskincolors)
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, skincolors[p->color].name);
				}
				else
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, va("BAD COLOR %u", p->color));
				}
				break;
			case CSSTEP_FOLLOWERCATEGORY:
				if (p->followercategory == -1)
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, "None");
				}
				else
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE,
						followercategories[setup_followercategories[p->followercategory][1]].name);
				}
				break;
			case CSSTEP_FOLLOWER:
				if (p->followern == -1)
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, "None");
				}
				else
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE,
						followers[p->followern].name);
				}
				break;
			case CSSTEP_FOLLOWERCOLORS:
				if (p->followercolor == FOLLOWERCOLOR_MATCH)
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, "Match");
				}
				else if (p->followercolor == FOLLOWERCOLOR_OPPOSITE)
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, "Opposite");
				}
				else if (p->followercolor < numskincolors)
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, skincolors[p->followercolor].name);
				}
				else
				{
					V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, va("BAD FOLLOWERCOLOR %u", p->followercolor));
				}
				break;
			default:
				V_DrawThinString(x-3, y+2, V_6WIDTHSPACE, "[extrainfo mode]");
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

	colormap = R_GetTranslationColormap(TC_DEFAULT, (p->color != SKINCOLOR_NONE ? p->color : SKINCOLOR_GREY), GTC_MENUCACHE);

	if (p->mdepth >= CSSTEP_READY)
	{
		V_DrawMappedPatch(x, y, 0, W_CachePatchName("CHCNFRM0", PU_CACHE), colormap);
	}
	else if (p->mdepth > CSSTEP_CHARS)
	{
		V_DrawMappedPatch(x, y, 0, W_CachePatchName(selectframesa[setup_animcounter % SELECTLEN], PU_CACHE), colormap);
		if (selectframesb[(setup_animcounter-1) % SELECTLEN] != NULL)
			V_DrawMappedPatch(x, y, V_TRANSLUCENT, W_CachePatchName(selectframesb[(setup_animcounter-1) % SELECTLEN], PU_CACHE), colormap);
	}
	else
	{
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
static void M_DrawProfileCard(INT32 x, INT32 y, boolean greyedout, profile_t *p)
{
	setup_player_t *sp = &setup_player[0];	// When editing profile character, we'll always be checking for what P1 is doing.
	patch_t *card = W_CachePatchName("PR_CARD", PU_CACHE);
	patch_t *cardbot = W_CachePatchName("PR_CARDB", PU_CACHE);
	patch_t *pwrlv = W_CachePatchName("PR_PWR", PU_CACHE);
	UINT16 truecol = SKINCOLOR_BLACK;
	UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_BLACK, GTC_CACHE);
	INT32 skinnum = -1;
	INT32 powerlevel = -1;

	char pname[PROFILENAMELEN+1] = "NEW";

	if (p != NULL && p->version)
	{
		truecol = PR_GetProfileColor(p);
		colormap = R_GetTranslationColormap(TC_DEFAULT, truecol, GTC_CACHE);
		strcpy(pname, p->profilename);
		skinnum = R_SkinAvailable(p->skinname);
		powerlevel = p->powerlevels[0];	// Only display race power level.
	}

	// check setup_player for colormap for the card.
	// we'll need to check again for drawing afterwards unfortunately.
	if (sp->mdepth >= CSSTEP_CHARS)
	{
		truecol = PR_GetProfileColor(p);
		colormap = R_GetTranslationColormap(skinnum, sp->color, GTC_MENUCACHE);
	}

	// Card
	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, greyedout ? V_TRANSLUCENT : 0, card, colormap);

	if (greyedout)
		return;	// only used for profiles we can't select.

	// Draw pwlv if we can
	if (powerlevel > -1)
	{
		V_DrawFixedPatch((x+30)*FRACUNIT, (y+84)*FRACUNIT, FRACUNIT, 0, pwrlv, colormap);
		V_DrawCenteredKartString(x+30, y+87, 0, va("%d\n", powerlevel));
	}

	// check what setup_player is doing in priority.
	if (sp->mdepth >= CSSTEP_CHARS)
	{
		skinnum = setup_chargrid[sp->gridx][sp->gridy].skinlist[sp->clonenum];

		if (skinnum >= 0)
		{
			if (M_DrawCharacterSprite(x-22, y+119, skinnum, false, false, 0, colormap))
				V_DrawMappedPatch(x+14, y+66, 0, faceprefix[skinnum][FACE_RANK], colormap);
		}

		M_DrawCharSelectCircle(sp, x-22, y+104);

		if (sp->mdepth >= CSSTEP_FOLLOWER)
		{
			if (M_DrawFollowerSprite(x-22 - 16, y+119, 0, false, 0, 0, sp))
			{
				UINT16 col = K_GetEffectiveFollowerColor(sp->followercolor, sp->color);;
				patch_t *ico = W_CachePatchName(followers[sp->followern].icon, PU_CACHE);
				UINT8 *fcolormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);
				V_DrawMappedPatch(x+14+18, y+66, 0, ico, fcolormap);
			}
		}
	}
	else if (skinnum > -1)	// otherwise, read from profile.
	{
		UINT8 *ccolormap, *fcolormap;
		UINT16 col = K_GetEffectiveFollowerColor(p->followercolor, p->color);
		UINT8 fln = K_FollowerAvailable(p->follower);

		if (R_SkinUsable(g_localplayers[0], skinnum, false))
			ccolormap = colormap;
		else
			ccolormap = R_GetTranslationColormap(TC_BLINK, truecol, GTC_MENUCACHE);

		fcolormap = R_GetTranslationColormap(
			(K_FollowerUsable(fln) ? TC_DEFAULT : TC_BLINK),
				col, GTC_MENUCACHE);

		if (M_DrawCharacterSprite(x-22, y+119, skinnum, false, false, 0, ccolormap))
		{
			V_DrawMappedPatch(x+14, y+66, 0, faceprefix[skinnum][FACE_RANK], ccolormap);
		}

		if (M_DrawFollowerSprite(x-22 - 16, y+119, fln, false, 0, fcolormap, NULL))
		{
			patch_t *ico = W_CachePatchName(followers[fln].icon, PU_CACHE);
			V_DrawMappedPatch(x+14+18, y+66, 0, ico, fcolormap);
		}
	}

	V_DrawCenteredGamemodeString(x, y+24, 0, 0, pname);

	// Card bottom to overlay the skin preview
	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, cardbot, colormap);

	// Profile number & player name

	if (p != NULL)
	{
		V_DrawProfileNum(x + 37 + 10, y + 131, 0, PR_GetProfileNum(p));
		V_DrawCenteredThinString(x, y + 141, V_GRAYMAP|V_6WIDTHSPACE, p->playername);
		V_DrawCenteredThinString(x, y + 151, V_GRAYMAP|V_6WIDTHSPACE, GetPrettyRRID(p->public_key, true));
	}
}

void M_DrawCharacterSelect(void)
{
	UINT8 i, j, k;
	UINT8 priority = 0;
	INT16 quadx, quady;
	INT16 skin;
	INT32 basex = optionsmenu.profile != NULL ? 64 : 0;
	boolean forceskin = (Playing() && K_CanChangeRules(true) == true) && (cv_forceskin.value != -1);

	if (setup_numplayers > 0)
	{
		priority = setup_animcounter % setup_numplayers;
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
		if ((forceskin == true) && (i != skins[cv_forceskin.value].kartspeed-1))
			continue;

		for (j = 0; j < 9; j++)
		{
			if (forceskin == true)
			{
				if (j != skins[cv_forceskin.value].kartweight-1)
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
					colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);

				V_DrawMappedPatch(basex + 82 + (i*16) + quadx, 22 + (j*16) + quady, 0, faceprefix[skin][FACE_RANK], colormap);

				// draw dot if there are more alts behind there!
				if (forceskin == false && setup_page+1 < setup_chargrid[i][j].numskins)
					V_DrawScaledPatch(basex + 82 + (i*16) + quadx, 22 + (j*16) + quady + 11, 0, W_CachePatchName("ALTSDOT", PU_CACHE));
			}
		}
	}

	// Explosions when you've made your final selection
	M_DrawCharSelectExplosions(true, 82, 22);

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
}

// DIFFICULTY SELECT
// This is a mix of K_DrawKartGamemodeMenu and the generic menu drawer depending on what we need.
// This is only ever used here (I hope because this is starting to pile up on hacks to look like the old m_menu.c lol...)

void M_DrawRaceDifficulty(void)
{
	patch_t *box = W_CachePatchName("M_DBOX", PU_CACHE);

	INT32 i;
	INT32 x = 120;
	INT32 y = 48;

	M_DrawMenuTooltips();

	// Draw the box for difficulty...
	V_DrawFixedPatch((111 + 48*menutransition.tics)*FRACUNIT, 33*FRACUNIT, FRACUNIT, 0, box, NULL);

	if (menutransition.tics)
	{
		x += 48 * menutransition.tics;
	}

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i >= drace_boxend)
		{
			x = GM_STARTX + (GM_XOFFSET * 5 / 2);
			y = GM_STARTY + (GM_YOFFSET * 5 / 2);

			if (i < currentMenu->numitems-1)
			{
				x -= GM_XOFFSET;
				y -= GM_YOFFSET;
			}


			if (menutransition.tics)
			{
				x += 48 * menutransition.tics;
			}
		}

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			// This is HACKY......

			case IT_STRING2:
			{

				INT32 f = (i == itemOn) ? highlightflags : 0;

				V_DrawString(140 + 48*menutransition.tics, y, f, currentMenu->menuitems[i].text);

				if (currentMenu->menuitems[i].status & IT_CVAR)
				{
					// implicitely we'll only take care of normal cvars
					INT32 cx = 260 + 48*menutransition.tics;
					consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;

					V_DrawCenteredString(cx, y, f, cv->string);

					if (i == itemOn)
					{

						INT32 w = V_StringWidth(cv->string, 0)/2;

						V_DrawCharacter(cx - 10 - w - (skullAnimCounter/5), y, '\x1C' | highlightflags, false); // left arrow
						V_DrawCharacter(cx + w + 2 + (skullAnimCounter/5), y, '\x1D' | highlightflags, false); // right arrow
					}
				}

				y += 10;

				break;
			}

			case IT_STRING:
			{

				UINT8 *colormap = NULL;

				if (i == itemOn)
				{
					colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);
				}
				else
				{
					colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_MOSS, GTC_CACHE);
				}


				if (currentMenu->menuitems[i].status & IT_CVAR)
				{

					INT32 fx = (x - 48*menutransition.tics);
					INT32 centx = fx + (320-fx)/2 + (menutransition.tics*48);	// undo the menutransition movement to redo it here otherwise the text won't move at the same speed lole.

					// implicitely we'll only take care of normal consvars
					consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;

					V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, W_CachePatchName("MENUSHRT", PU_CACHE), colormap);
					V_DrawCenteredGamemodeString(centx, y - 3, V_ALLOWLOWERCASE, colormap, cv->string);

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
					V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, 0, W_CachePatchName("MENUPLTR", PU_CACHE), colormap);
					V_DrawGamemodeString(x + 16, y - 3, V_ALLOWLOWERCASE, colormap, currentMenu->menuitems[i].text);
				}
				x += GM_XOFFSET;
				y += GM_YOFFSET;

				break;
			}
		}
	}
}

// LEVEL SELECT

static void M_DrawCupPreview(INT16 y, levelsearch_t *levelsearch)
{
	UINT8 i = 0;
	INT16 maxlevels = M_CountLevelsToShowInList(levelsearch);
	INT16 x = -(cupgrid.previewanim % 82);
	INT16 add;
	INT16 map, start = M_GetFirstLevelInList(&i, levelsearch);
	UINT8 starti = i;

	if (levelsearch->cup && maxlevels > 0)
	{
		add = (cupgrid.previewanim / 82) % maxlevels;
		map = start;
		while (add > 0)
		{
			map = M_GetNextLevelInList(map, &i, levelsearch);

			if (map >= nummapheaders)
			{
				break;
			}

			add--;
		}
		while (x < BASEVIDWIDTH)
		{
			if (map >= nummapheaders)
			{
				map = start;
				i = starti;
			}

			K_DrawMapThumbnail(
				(x+1)<<FRACBITS, (y+2)<<FRACBITS,
				80<<FRACBITS,
				0,
				map,
				NULL);

			x += 82;

			map = M_GetNextLevelInList(map, &i, levelsearch);
		}
	}
	else
	{
		patch_t *st = W_CachePatchName(va("PREVST0%d", (cupgrid.previewanim % 4) + 1), PU_CACHE);
		while (x < BASEVIDWIDTH)
		{
			V_DrawScaledPatch(x+1, y+2, 0, st);
			x += 82;
		}
	}
}

static void M_DrawCupTitle(INT16 y, levelsearch_t *levelsearch)
{
	UINT8 temp = 0;

	V_DrawScaledPatch(0, y, 0, W_CachePatchName("MENUHINT", PU_CACHE));

	if (levelsearch->cup == &dummy_lostandfound)
	{
		V_DrawCenteredLSTitleLowString(BASEVIDWIDTH/2, y+6, 0, "Lost and Found");
	}
	else if (levelsearch->cup)
	{
		boolean unlocked = (M_GetFirstLevelInList(&temp, levelsearch) != NEXTMAP_INVALID);
		UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE);
		patch_t *icon = W_CachePatchName(levelsearch->cup->icon, PU_CACHE);
		const char *str = (unlocked ? va("%s Cup", levelsearch->cup->name) : "???");
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

void M_DrawCupSelect(void)
{
	UINT8 i, j, temp = 0;
	UINT8 *colormap = NULL;
	cupwindata_t *windata = NULL;
	levelsearch_t templevelsearch = levellist.levelsearch; // full copy

	for (i = 0; i < CUPMENU_COLUMNS; i++)
	{
		for (j = 0; j < CUPMENU_ROWS; j++)
		{
			size_t id = (i + (j * CUPMENU_COLUMNS)) + (cupgrid.pageno * (CUPMENU_COLUMNS * CUPMENU_ROWS));
			patch_t *patch = NULL;
			INT16 x, y;
			INT16 icony = 7;
			char status =  'A';
			char monitor;
			INT32 rankx = 0;

			if (!cupgrid.builtgrid[id])
				break;

			templevelsearch.cup = cupgrid.builtgrid[id];

			if (cupgrid.grandprix
				&& (cv_dummygpdifficulty.value >= 0 && cv_dummygpdifficulty.value < KARTGP_MAX))
			{
				UINT16 col = SKINCOLOR_NONE;

				windata = &templevelsearch.cup->windata[cv_dummygpdifficulty.value];

				switch (windata->best_placement)
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

			if (templevelsearch.cup == &dummy_lostandfound)
			{
				// No cup? Lost and found!
				monitor = '0';
			}
			else
			{
				if (templevelsearch.cup->monitor < 10)
				{
					monitor = '0' + templevelsearch.cup->monitor;

					if (monitor == '2')
					{
						icony = 5;
						rankx = 2;
					}
				}
				else
				{
					monitor = 'A' + (templevelsearch.cup->monitor - 10);
				}
			}

			patch = W_CachePatchName(va("CUPMON%c%c", monitor, status), PU_CACHE);

			x = 14 + (i*42);
			y = 20 + (j*44) - (30*menutransition.tics);

			V_DrawFixedPatch((x)*FRACUNIT, (y)<<FRACBITS, FRACUNIT, 0, patch, colormap);

			if (templevelsearch.cup == &dummy_lostandfound)
				; // Only ever placed on the list if valid
			else if (M_GetFirstLevelInList(&temp, &templevelsearch) == NEXTMAP_INVALID)
			{
				patch_t *st = W_CachePatchName(va("ICONST0%d", (cupgrid.previewanim % 4) + 1), PU_CACHE);
				V_DrawScaledPatch(x + 8, y + icony, 0, st);
			}
			else
			{
				V_DrawScaledPatch(x + 8, y + icony, 0, W_CachePatchName(templevelsearch.cup->icon, PU_CACHE));
				V_DrawScaledPatch(x + 8, y + icony, 0, W_CachePatchName("CUPBOX", PU_CACHE));

				if (!windata)
					;
				else if (windata->best_placement != 0)
				{
					char gradeChar = '?';

					switch (windata->best_grade)
					{
						case GRADE_E: { gradeChar = 'E'; break; }
						case GRADE_D: { gradeChar = 'D'; break; }
						case GRADE_C: { gradeChar = 'C'; break; }
						case GRADE_B: { gradeChar = 'B'; break; }
						case GRADE_A: { gradeChar = 'A'; break; }
						case GRADE_S: { gradeChar = 'S'; break; }
						default: { break; }
					}

					V_DrawCharacter(x + 5 + rankx, y + icony + 14, gradeChar, false); // rank

					if (windata->got_emerald == true)
					{
						if (templevelsearch.cup->emeraldnum == 0)
							V_DrawCharacter(x + 26 - rankx, y + icony + 14, '*', false); // rank
						else
						{
							UINT16 col = SKINCOLOR_CHAOSEMERALD1 + (templevelsearch.cup->emeraldnum-1) % 7;
							colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);

							V_DrawFixedPatch((x + 26 - rankx)*FRACUNIT, (y + icony + 13)*FRACUNIT, FRACUNIT, 0, W_CachePatchName("K_EMERC", PU_CACHE), colormap);
						}
					}
				}
			}
		}
	}

	V_DrawScaledPatch(14 + (cupgrid.x*42) - 4,
		20 + (cupgrid.y*44) - 1 - (24*menutransition.tics),
		0, W_CachePatchName("CUPCURS", PU_CACHE)
	);

	templevelsearch.cup = cupgrid.builtgrid[CUPMENU_CURSORID];

	V_DrawFill(0, 146 + (24*menutransition.tics), BASEVIDWIDTH, 54, 31);
	M_DrawCupPreview(146 + (24*menutransition.tics), &templevelsearch);

	M_DrawCupTitle(120 - (24*menutransition.tics), &templevelsearch);
}

static void M_DrawHighLowLevelTitle(INT16 x, INT16 y, INT16 map)
{
	char word1[22];
	char word2[22];
	UINT8 word1len = 0;
	UINT8 word2len = 0;
	INT16 x2 = x;
	UINT8 i;

	if (!mapheaderinfo[map] || !mapheaderinfo[map]->lvlttl[0])
		return;

	if (mapheaderinfo[map]->zonttl[0])
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
		boolean donewithone = false;

		for (i = 0; i < 22; i++)
		{
			if (!mapheaderinfo[map]->lvlttl[i])
				break;

			if (mapheaderinfo[map]->lvlttl[i] == ' ')
			{
				if (!donewithone)
				{
					donewithone = true;
					continue;
				}
			}

			if (donewithone)
			{
				word2[word2len] = mapheaderinfo[map]->lvlttl[i];
				word2len++;
			}
			else
			{
				word1[word1len] = mapheaderinfo[map]->lvlttl[i];
				word1len++;
			}
		}
	}

	if (mapheaderinfo[map]->actnum)
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

static void M_DrawLevelSelectBlock(INT16 x, INT16 y, INT16 map, boolean redblink, boolean greyscale)
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
}

void M_DrawLevelSelect(void)
{
	INT16 i = 0;
	UINT8 j = 0;
	INT16 map = M_GetFirstLevelInList(&j, &levellist.levelsearch);
	INT16 t = (64*menutransition.tics), tay = 0;
	INT16 y = 80 - (12 * levellist.y);
	boolean tatransition = ((menutransition.startmenu == &PLAY_TimeAttackDef || menutransition.endmenu == &PLAY_TimeAttackDef) && menutransition.tics);

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
}

void M_DrawTimeAttack(void)
{
	UINT16 map = levellist.choosemap;
	INT16 t = (48*menutransition.tics);
	INT16 leftedge = 149+t+16;
	INT16 rightedge = 149+t+155;
	INT16 opty = 140;
	INT32 w;
	patch_t *minimap = NULL;
	UINT8 i;

	M_DrawLevelSelectBlock(0, 2, map, true, false);

	//V_DrawFill(24-t, 82, 100, 100, 36); // size test

	V_DrawScaledPatch(149+t, 70, 0, W_CachePatchName("BESTTIME", PU_CACHE));

	if (currentMenu == &PLAY_TimeAttackDef && mapheaderinfo[map])
	{
		tic_t timerec = 0;
		tic_t laprec = 0;
		UINT32 timeheight = 82;

		if ((minimap = mapheaderinfo[map]->minimapPic))
			V_DrawScaledPatch(24-t, 82, 0, minimap);

		if (mapheaderinfo[map]->mainrecord)
		{
			timerec = mapheaderinfo[map]->mainrecord->time;
			laprec = mapheaderinfo[map]->mainrecord->lap;
		}

		if ((gametypes[levellist.newgametype]->rules & GTR_CIRCUIT)
			&& (mapheaderinfo[map]->numlaps != 1))
		{
			V_DrawRightAlignedString(rightedge-12, timeheight, highlightflags, "BEST LAP:");
			K_drawKartTimestamp(laprec, 162+t, timeheight+6, 0, 2);
			timeheight += 30;
		}
		else
		{
			timeheight += 15;
		}

		V_DrawRightAlignedString(rightedge-12, timeheight, highlightflags, "BEST TIME:");
		K_drawKartTimestamp(timerec, 162+t, timeheight+6, 0, 1);

		// SPB Attack control hint + menu overlay
		if (levellist.newgametype == GT_RACE && levellist.levelsearch.timeattack == true && M_SecretUnlocked(SECRET_SPBATTACK, true))
		{
			const UINT8 anim_duration = 16;
			const UINT8 anim = (timeattackmenu.ticker % (anim_duration * 2)) < anim_duration;

			INT32 buttonx = 162 + t;
			INT32 buttony = timeheight;

			if (anim)
				V_DrawScaledPatch(buttonx + 35, buttony - 3, V_SNAPTOLEFT, W_CachePatchName("TLB_I", PU_CACHE));
			else
				V_DrawScaledPatch(buttonx + 35, buttony - 3, V_SNAPTOLEFT, W_CachePatchName("TLB_IB", PU_CACHE));

			if ((timeattackmenu.spbflicker == 0 || timeattackmenu.ticker % 2) == (cv_dummyspbattack.value == 1))
			{
				V_DrawMappedPatch(buttonx + 7, buttony - 1, 0, W_CachePatchName("K_SPBATK", PU_CACHE), R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_RED, GTC_MENUCACHE));
			}
		}

	}
	else
		opty = 80;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		UINT32 f = (i == itemOn) ? highlightflags : 0;

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{

			case IT_HEADERTEXT:

				V_DrawString(leftedge, opty, highlightflags, currentMenu->menuitems[i].text);
				opty += 10;
				break;

			case IT_STRING:

				if (i >= currentMenu->numitems-1)
					V_DrawRightAlignedString(rightedge, opty, f, currentMenu->menuitems[i].text);
				else
					V_DrawString(leftedge, opty, f, currentMenu->menuitems[i].text);
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

						optflags |= V_ALLOWLOWERCASE;
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
						w = V_StringWidth(str, optflags);
						V_DrawString(leftedge+12, opty, optflags, str);
						if (drawarrows)
						{
							V_DrawCharacter(leftedge+12 - 10 - (skullAnimCounter/5), opty, '\x1C' | f, false); // left arrow
							V_DrawCharacter(leftedge+12 + w + 2+ (skullAnimCounter/5), opty, '\x1D' | f, false); // right arrow
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
					V_DrawCenteredGamemodeString(x, y - 3, V_ALLOWLOWERCASE, colormap, m->menuitems[i].text);
				}
				break;
		}

		x += GM_XOFFSET;
		y += GM_YOFFSET + extend[i][2];
	}
}

// Draws the EGGA CHANNEL background.
void M_DrawEggaChannel(void)
{
	patch_t *background = W_CachePatchName("M_EGGACH", PU_CACHE);

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 25);
	V_DrawFixedPatch(160<<FRACBITS, 104<<FRACBITS, FRACUNIT, 0, background, NULL);
	V_DrawVhsEffect(false);	// VHS the background! (...sorry OGL my love)
}

// Multiplayer mode option select
void M_DrawMPOptSelect(void)
{
	M_DrawEggaChannel();
	M_DrawMenuTooltips();
	M_MPOptDrawer(&PLAY_MP_OptSelectDef, mpmenu.modewinextend);
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
			V_DrawCenteredGamemodeString(xp + (gobutt->width/2), yp -3, V_ALLOWLOWERCASE, colormap, currentMenu->menuitems[i].text);
		}
		else
		{
			switch (currentMenu->menuitems[i].status & IT_DISPLAY)
			{
				case IT_TRANSTEXT2:
				{
					V_DrawThinString(xp, yp, V_ALLOWLOWERCASE|V_6WIDTHSPACE|V_TRANSLUCENT, currentMenu->menuitems[i].text);
					xp += 5;
					yp += 11;
					break;
				}
				case IT_STRING:
				{
					V_DrawThinString(xp, yp, V_ALLOWLOWERCASE|V_6WIDTHSPACE | (i == itemOn ? highlightflags : 0), currentMenu->menuitems[i].text);

					// Cvar specific handling
					switch (currentMenu->menuitems[i].status & IT_TYPE)
					{
						case IT_CVAR:
						{
							consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
							switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
							{
								case IT_CV_STRING:
									V_DrawThinString(xp + 96, yp, V_ALLOWLOWERCASE|V_6WIDTHSPACE, cv->string);
									if (skullAnimCounter < 4 && i == itemOn)
										V_DrawString(xp + 96 + V_ThinStringWidth(cv->string, V_ALLOWLOWERCASE|V_6WIDTHSPACE), yp+1, 0, "_");

									break;

								default:
									w = V_ThinStringWidth(cv->string, V_6WIDTHSPACE);
									V_DrawThinString(xp + 138 - w, yp, ((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? warningflags : highlightflags)|V_6WIDTHSPACE, cv->string);
									if (i == itemOn)
									{
										V_DrawCharacter(xp + 138 - 10 - w - (skullAnimCounter/5), yp, '\x1C' | highlightflags, false); // left arrow
										V_DrawCharacter(xp + 138 + 2 + (skullAnimCounter/5), yp, '\x1D' | highlightflags, false); // right arrow
									}
									break;
							}
							break;
						}
						case IT_KEYHANDLER:
						{
							if (currentMenu->menuitems[i].itemaction.routine != M_HandleHostMenuGametype)
								break;

							w = V_ThinStringWidth(gametypes[menugametype]->name, V_6WIDTHSPACE);
							V_DrawThinString(xp + 138 - w, yp, highlightflags|V_6WIDTHSPACE, gametypes[menugametype]->name);
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

				V_DrawThinString(xp, yp, V_ALLOWLOWERCASE | ((i == itemOn || currentMenu->menuitems[i].status & IT_SPACE) ? highlightflags : 0)|V_ALLOWLOWERCASE|V_6WIDTHSPACE, str);

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
								V_DrawThinString(xp + 18, yp, V_ALLOWLOWERCASE|V_6WIDTHSPACE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawString(xp + 18 + V_ThinStringWidth(cv->string, V_ALLOWLOWERCASE|V_6WIDTHSPACE), yp+1, 0, "_");

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

	if (!mpmenu.roomforced || mpmenu.room == 0)
		V_DrawFixedPatch(160<<FRACBITS, 100<<FRACBITS, FRACUNIT, mpmenu.room ? (5<<V_ALPHASHIFT) : 0, butt1[(mpmenu.room) ? 1 : 0], NULL);

	if (!mpmenu.roomforced || mpmenu.room == 1)
		V_DrawFixedPatch(160<<FRACBITS, 100<<FRACBITS, FRACUNIT, (!mpmenu.room) ? (5<<V_ALPHASHIFT) : 0, butt2[(!mpmenu.room) ? 1 : 0], NULL);
}

// SERVER BROWSER
void M_DrawMPServerBrowser(void)
{
	patch_t *text1 = W_CachePatchName("MENUBGT1", PU_CACHE);
	patch_t *text2 = W_CachePatchName("MENUBGT2", PU_CACHE);

	patch_t *raceh = W_CachePatchName("M_SERV1", PU_CACHE);
	patch_t *batlh = W_CachePatchName("M_SERV2", PU_CACHE);

	patch_t *racehs = W_CachePatchName("M_SERV12", PU_CACHE);
	patch_t *batlhs = W_CachePatchName("M_SERV22", PU_CACHE);

	fixed_t text1loop = SHORT(text1->height)*FRACUNIT;
	fixed_t text2loop = SHORT(text2->width)*FRACUNIT;

	const UINT8 startx = 18;
	const UINT8 basey = 56;
	const INT32 starty = basey - 18*mpmenu.scrolln + mpmenu.slide;
	INT32 ypos = 0;
	UINT8 i;

	// background stuff
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("BG_MPS3", PU_CACHE), NULL);

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

		boolean racegt = strcmp(serverlist[i].info.gametypename, "Race") == 0;
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

			if (itemOn == 2 && mpmenu.servernum == i)
				V_DrawFixedPatch(startx*FRACUNIT, (starty + ypos)*FRACUNIT, FRACUNIT, transflag, racegt ? racehs : batlhs, NULL);
			else
				V_DrawFixedPatch(startx*FRACUNIT, (starty + ypos)*FRACUNIT, FRACUNIT, transflag, racegt ? raceh : batlh, NULL);

			// Server name:
			V_DrawString(startx+11, starty + ypos + 6, V_ALLOWLOWERCASE|transflag, serverlist[i].info.servername);

			// Ping:
			V_DrawThinString(startx + 191, starty + ypos + 7, V_6WIDTHSPACE|transflag, va("%03d", serverlist[i].info.time));

			// Playercount
			V_DrawThinString(startx + 214, starty + ypos + 7, V_6WIDTHSPACE|transflag, va("%02d/%02d", serverlist[i].info.numberofplayer, serverlist[i].info.maxplayer));

			// Power Level
			V_DrawThinString(startx + 248, starty + ypos, V_6WIDTHSPACE|transflag, va("%04d PLv", serverlist[i].info.avgpwrlv));

			// game speed if applicable:
			if (racegt)
			{
				UINT8 speed = serverlist[i].info.kartvars & SV_SPEEDMASK;
				patch_t *pp = W_CachePatchName(va("M_SDIFF%d", speed), PU_CACHE);

				V_DrawFixedPatch((startx + 251)*FRACUNIT, (starty + ypos + 9)*FRACUNIT, FRACUNIT, transflag, pp, NULL);
			}
		}
		ypos += SERVERSPACE;
	}

	// Draw genericmenu ontop!
	V_DrawFill(0, 0, 320, 52, 31);
	V_DrawFill(0, 53, 320, 1, 31);
	V_DrawFill(0, 55, 320, 1, 31);

	V_DrawCenteredGamemodeString(160, 2, V_ALLOWLOWERCASE, 0, "Server Browser");

	// normal menu options
	M_DrawGenericMenu();

}

// OPTIONS MENU

// Draws the cogs and also the options background!
static void M_DrawOptionsCogs(void)
{
	// the background isn't drawn outside of being in the main menu state.
	if (gamestate == GS_MENU)
	{
		patch_t *back[3] = {W_CachePatchName("OPT_BG1", PU_CACHE), W_CachePatchName("OPT_BG2", PU_CACHE), W_CachePatchName("OPT_BG3", PU_CACHE)};
		INT32 tflag = 0;
		UINT8 *c;
		UINT8 *c2;	// colormap for the one we're changing

		if (optionsmenu.fade)
		{
			c2 = R_GetTranslationColormap(TC_DEFAULT, optionsmenu.lastcolour, GTC_CACHE);
			V_DrawFixedPatch(0, 0, FRACUNIT, 0, back[(optionsmenu.ticker/10) %3], c2);

			// prepare fade flag:
			tflag = min(V_90TRANS, (optionsmenu.fade)<<V_ALPHASHIFT);

		}
		c = R_GetTranslationColormap(TC_DEFAULT, optionsmenu.currcolour, GTC_CACHE);
		V_DrawFixedPatch(0, 0, FRACUNIT, tflag, back[(optionsmenu.ticker/10) %3], c);
	}
	else
	{
		patch_t *back_pause[3] = {W_CachePatchName("OPT_BAK1", PU_CACHE), W_CachePatchName("OPT_BAK2", PU_CACHE), W_CachePatchName("OPT_BAK3", PU_CACHE)};
		V_DrawFixedPatch(0, 0, FRACUNIT, V_MODULATE, back_pause[(optionsmenu.ticker/10) %3], NULL);
	}
}

void M_DrawOptionsMovingButton(void)
{
	patch_t *butt = W_CachePatchName("OPT_BUTT", PU_CACHE);
	UINT8 *c = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

	V_DrawFixedPatch((optionsmenu.optx)*FRACUNIT, (optionsmenu.opty)*FRACUNIT, FRACUNIT, 0, butt, c);
	V_DrawCenteredGamemodeString((optionsmenu.optx)-3, (optionsmenu.opty) - 16, V_ALLOWLOWERCASE, c, OPTIONS_MainDef.menuitems[OPTIONS_MainDef.lastOn].text);
}

void M_DrawOptions(void)
{
	UINT8 i;
	INT32 x = 140 - (48*itemOn) + optionsmenu.offset;
	INT32 y = 70 + optionsmenu.offset;
	patch_t *buttback = W_CachePatchName("OPT_BUTT", PU_CACHE);

	UINT8 *c = NULL;

	M_DrawOptionsCogs();

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
			V_DrawCenteredGamemodeString(px-3, py - 16, V_ALLOWLOWERCASE|tflag, (i == itemOn ? c : NULL), currentMenu->menuitems[i].text);
		}

		y += 48;
		x += 48;
	}

	M_DrawMenuTooltips();

	if (menutransition.tics)
		M_DrawOptionsMovingButton();

}

void M_DrawGenericOptions(void)
{
	INT32 x = currentMenu->x - menutransition.tics*48, y = currentMenu->y, w, i, cursory = 0;

	M_DrawOptionsCogs();
	M_DrawMenuTooltips();
	M_DrawOptionsMovingButton();

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
				y += SMALLLINEHEIGHT;
				break;
#if 0
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, currentMenu->menuitems[i].itemaction.cvar);
				y += LINEHEIGHT;
				break;
#endif
			case IT_STRING:
			case IT_WHITESTRING:
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, highlightflags, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								w = V_StringWidth(cv->string, 0);
								V_DrawString(BASEVIDWIDTH - x - w, y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? warningflags : highlightflags), cv->string);
								if (i == itemOn)
								{
									V_DrawCharacter(BASEVIDWIDTH - x - 10 - w - (skullAnimCounter/5), y,
											'\x1C' | highlightflags, false); // left arrow
									V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
											'\x1D' | highlightflags, false); // right arrow
								}
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
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
				V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_HEADERTEXT: // draws 16 pixels to the left, in yellow text
				if (currentMenu->menuitems[i].mvar1)
					y = currentMenu->y+currentMenu->menuitems[i].mvar1;

				V_DrawString(x-16, y, highlightflags, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(x - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawString(x, cursory, highlightflags, currentMenu->menuitems[itemOn].text);
	}
}

// *Heavily* simplified version of the generic options menu, cattered only towards erasing profiles.
void M_DrawProfileErase(void)
{
	INT32 x = currentMenu->x - menutransition.tics*48, y = currentMenu->y-SMALLLINEHEIGHT, i, cursory = 0;
	UINT8 np = PR_GetNumProfiles();

	M_DrawOptionsCogs();
	M_DrawMenuTooltips();
	M_DrawOptionsMovingButton();

	for (i = 1; i < np; i++)
	{

		profile_t *pr = PR_GetProfile(i);

		if (i == optionsmenu.eraseprofilen)
		{
			cursory = y;
			V_DrawScaledPatch(x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_CACHE));
		}

		V_DrawString(x, y,
			(i == optionsmenu.eraseprofilen ? highlightflags : 0)|V_ALLOWLOWERCASE,
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
	INT32 x = 160 - optionsmenu.profilen*(128 + 128/8) + optionsmenu.offset;
	INT32 y = 35 + menutransition.tics*32;

	M_DrawOptionsCogs();
	M_DrawMenuTooltips();

	// This shouldn't be drawn when a profile is selected as optx/opty are used to move the card.
	if (optionsmenu.profile == NULL && menutransition.tics)
		M_DrawOptionsMovingButton();

	for (i=0; i < MAXPROFILES+1; i++)	// +1 because the default profile does not count
	{
		profile_t *p = PR_GetProfile(i);

		// don't draw the card in this specific scenario
		if (!(menutransition.tics && optionsmenu.profile != NULL && optionsmenu.profilen == i))
			M_DrawProfileCard(x, y, i > maxp, p);

		x += 128 + 128/8;
	}

	// needs to be drawn since it happens on the transition
	if (optionsmenu.profile != NULL)
		M_DrawProfileCard(optionsmenu.optx, optionsmenu.opty, false, optionsmenu.profile);


}

// Profile edition menu
void M_DrawEditProfile(void)
{

	INT32 y = 34;
	INT32 x = 145;
	INT32 i;

	M_DrawOptionsCogs();

	// Tooltip
	// The text is slightly shifted hence why we don't just use M_DrawMenuTooltips()
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);
	if (currentMenu->menuitems[itemOn].tooltip != NULL)
	{
		V_DrawCenteredThinString(224, 12, V_ALLOWLOWERCASE|V_6WIDTHSPACE, currentMenu->menuitems[itemOn].tooltip);
	}

	// Draw the menu options...
	for (i = 0; i < currentMenu->numitems; i++)
	{

		UINT8 *colormap = NULL;
		INT32 tflag = (currentMenu->menuitems[i].status & IT_TRANSTEXT) ? V_TRANSLUCENT : 0;

		if (i == itemOn)
			colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

		// Background
		V_DrawFill(0, y, 400 - (menutransition.tics*64), 24, itemOn == i ? 169 : 30);	// 169 is the plague colourization
		// Text
		V_DrawGamemodeString(x + (menutransition.tics*32), y - 6, V_ALLOWLOWERCASE|tflag, colormap, currentMenu->menuitems[i].text);

		// Cvar specific handling
		/*switch (currentMenu->menuitems[i].status & IT_TYPE)
		{
			case IT_CVAR:
			{
				consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;
				switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
				{
					case IT_CV_STRING:
						V_DrawFill(0, y+24, 400 - (menutransition.tics*64), 16, itemOn == i ? 169 : 30);	// 169 is the plague colourization
						V_DrawString(x + 8, y + 29, V_ALLOWLOWERCASE, cv->string);
						if (skullAnimCounter < 4 && i == itemOn)
						V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 29, '_' | 0x80, false);
						y += 16;
					}
				}
			}*/

		y += 34;
	}

	// Finally, draw the card ontop
	if (optionsmenu.profile != NULL)
	{
		M_DrawProfileCard(optionsmenu.optx, optionsmenu.opty, false, optionsmenu.profile);
	}
}

// Controller offsets to center on each button.
INT16 controlleroffsets[][2] = {
	{0, 0},			// gc_none
	{70, 112},		// gc_up
	{70, 112},		// gc_down
	{70, 112},		// gc_left
	{70, 112},		// gc_right
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

// Controller patches for button presses.
// {patch if not pressed, patch if pressed}
// if NULL, draws nothing.
// reminder that lumpnames can only be 8 chars at most. (+1 for \0)

char controllerpresspatch[9][2][9] = {
	{"", "BTP_A"},	// MBT_A
	{"", "BTP_B"},	// MBT_B
	{"", "BTP_C"},	// MBT_C
	{"", "BTP_X"},	// MBT_X
	{"", "BTP_Y"},	// MBT_Y
	{"", "BTP_Z"},	// MBT_Z
	{"BTNP_L", "BTP_L"},// MBT_L
	{"BTNP_R", "BTP_R"},// MBT_R
	{"", "BTP_ST"}	// MBT_START
};


// the control stuff.
// Dear god.
void M_DrawProfileControls(void)
{
	const UINT8 spacing = 34;
	INT32 y = 16 - (optionsmenu.controlscroll*spacing);
	INT32 x = 8;
	INT32 i, j, k;
	const UINT8 pid = 0;

	M_DrawOptionsCogs();

	V_DrawScaledPatch(BASEVIDWIDTH*2/3 - optionsmenu.contx, BASEVIDHEIGHT/2 -optionsmenu.conty, 0, W_CachePatchName("PR_CONT", PU_CACHE));

	// Draw button presses...
	// @TODO: Dpad when we get the sprites for it.

	for (i = 0; i < 9; i++)
	{
		INT32 bt = 1<<i;
		if (M_MenuButtonHeld(pid, bt))
		{
			if (controllerpresspatch[i][1][0] != '\0')
				V_DrawScaledPatch(BASEVIDWIDTH*2/3 - optionsmenu.contx, BASEVIDHEIGHT/2 -optionsmenu.conty, 0, W_CachePatchName(controllerpresspatch[i][1], PU_CACHE));
		}
		else
		{
			if (controllerpresspatch[i][0][0] != '\0')
				V_DrawScaledPatch(BASEVIDWIDTH*2/3 - optionsmenu.contx, BASEVIDHEIGHT/2 -optionsmenu.conty, 0, W_CachePatchName(controllerpresspatch[i][0], PU_CACHE));
		}
	}

	if (optionsmenu.trycontroller)
	{
		optionsmenu.tcontx = BASEVIDWIDTH*2/3 - 10;
		optionsmenu.tconty = BASEVIDHEIGHT/2 +70;

		V_DrawCenteredString(160, 180, highlightflags, va("PRESS NOTHING FOR %d SEC TO GO BACK", optionsmenu.trycontroller/TICRATE));
		return;	// Don't draw the rest if we're trying the controller.
	}

	// Tooltip
	// The text is slightly shifted hence why we don't just use M_DrawMenuTooltips()
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);
	if (currentMenu->menuitems[itemOn].tooltip != NULL)
	{
		V_DrawCenteredThinString(229, 12, V_ALLOWLOWERCASE|V_6WIDTHSPACE, currentMenu->menuitems[itemOn].tooltip);
	}

	V_DrawFill(0, 0, 138, 200, 31);	// Black border

	// Draw the menu options...
	for (i = 0; i < currentMenu->numitems; i++)
	{
		char buf[256];
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
				V_DrawFill(0, y+17, 124, 1, 0);	// underline
				V_DrawString(x, y+8, 0, currentMenu->menuitems[i].text);
				y += spacing;
				break;

			case IT_STRING:
				V_DrawString(x, y+1, (i == itemOn ? highlightflags : 0), currentMenu->menuitems[i].text);
				y += spacing;
				break;

			case IT_STRING2:
			{
				boolean drawnpatch = false;

				if (currentMenu->menuitems[i].patch)
				{
					V_DrawScaledPatch(x+12, y+12, 0, W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					drawnpatch = true;
				}
				else
					V_DrawString(x, y+1, (i == itemOn ? highlightflags : 0), currentMenu->menuitems[i].text);

				if (currentMenu->menuitems[i].status & IT_CVAR)	// not the proper way to check but this menu only has normal onoff cvars.
				{
					INT32 w;
					consvar_t *cv = currentMenu->menuitems[i].itemaction.cvar;

					w = V_StringWidth(cv->string, 0);
					V_DrawString(x + 12, y + 12, ((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? warningflags : highlightflags), cv->string);
					if (i == itemOn)
					{
						V_DrawCharacter(x - (skullAnimCounter/5), y+12, '\x1C' | highlightflags, false); // left arrow
						V_DrawCharacter(x + 12 + w + 2 + (skullAnimCounter/5) , y+12, '\x1D' | highlightflags, false); // right arrow
					}
				}
				else if (currentMenu->menuitems[i].status & IT_CONTROL)
				{
					UINT32 vflags = V_6WIDTHSPACE;
					INT32 gc = currentMenu->menuitems[i].mvar1;

					UINT8 available = 0, set = 0;

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
									vflags = V_REDMAP|V_6WIDTHSPACE;
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

					if (buf[0])
						;
					else if (!set)
						strcpy(buf, "\x85NOT BOUND");
					else
					{
						for (k = 0; k < MAXINPUTMAPPING; k++)
						{
							if (keys[k] == KEY_NULL)
								continue;

							if (k > 0)
								strcat(buf," / ");

							if (k == 2 && drawnpatch)	// hacky...
								strcat(buf, "\n");

							strcat(buf, G_KeynumToString (keys[k]));
						}
					}

					// don't shift the text if we didn't draw a patch.
					V_DrawThinString(x + (drawnpatch ? 32 : 0), y + (drawnpatch ? 2 : 12), vflags, buf);

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

	// Overlay for control binding
	if (optionsmenu.bindcontrol)
	{
		INT16 reversetimer = TICRATE*5 - optionsmenu.bindtimer;
		INT32 fade = reversetimer;
		INT32 ypos;

		if (fade > 9)
			fade = 9;

		ypos = (BASEVIDHEIGHT/2) - 4 +16*(9 - fade);
		V_DrawFadeScreen(31, fade);

		M_DrawTextBox((BASEVIDWIDTH/2) - (120), ypos - 12, 30, 4);

		V_DrawCenteredString(BASEVIDWIDTH/2, ypos, 0, va("Press key #%d for control", optionsmenu.bindcontrol));
		V_DrawCenteredString(BASEVIDWIDTH/2, ypos +10, 0, va("\"%s\"", currentMenu->menuitems[itemOn].text));
		V_DrawCenteredString(BASEVIDWIDTH/2, ypos +20, highlightflags, va("(WAIT %d SECONDS TO SKIP)", optionsmenu.bindtimer/TICRATE));
	}
}

// Draw the video modes list, a-la-Quake
void M_DrawVideoModes(void)
{
	INT32 i, j, row, col;

	M_DrawOptionsCogs();
	M_DrawMenuTooltips();
	M_DrawOptionsMovingButton();

	V_DrawCenteredString(BASEVIDWIDTH/2 + menutransition.tics*64, currentMenu->y,
		highlightflags, "Choose mode, reselect to change default");

	row = 41 + menutransition.tics*64;
	col = currentMenu->y + 14;
	for (i = 0; i < optionsmenu.vidm_nummodes; i++)
	{
		if (i == optionsmenu.vidm_selected)
			V_DrawString(row, col, highlightflags, optionsmenu.modedescs[i].desc);
		// Show multiples of 320x200 as green.
		else
			V_DrawString(row, col, (optionsmenu.modedescs[i].goodratio) ? recommendedflags : 0, optionsmenu.modedescs[i].desc);

		col += 8;
		if ((i % optionsmenu.vidm_column_size) == (optionsmenu.vidm_column_size-1))
		{
			row += 7*13;
			col = currentMenu->y + 14;
		}
	}

	if (optionsmenu.vidm_testingmode > 0)
	{
		INT32 testtime = (optionsmenu.vidm_testingmode/TICRATE) + 1;

		M_CentreText(menutransition.tics*64, currentMenu->y + 75,
			va("Previewing mode %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : 0x80,
				vid.width, vid.height));
		M_CentreText(menutransition.tics*64, currentMenu->y + 75+8,
			"Press ENTER again to keep this mode");
		M_CentreText(menutransition.tics*64, currentMenu->y + 75+16,
			va("Wait %d second%s", testtime, (testtime > 1) ? "s" : ""));
		M_CentreText(menutransition.tics*64, currentMenu->y + 75+24,
			"or press ESC to return");

	}
	else
	{
		M_CentreText(menutransition.tics*64, currentMenu->y + 75,
			va("Current mode is %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : 0x80,
				vid.width, vid.height));
		M_CentreText(menutransition.tics*64, currentMenu->y + 75+8,
			va("Default mode is %c%dx%d",
				(SCR_IsAspectCorrect(cv_scr_width.value, cv_scr_height.value)) ? 0x83 : 0x80,
				cv_scr_width.value, cv_scr_height.value));

		V_DrawCenteredString(BASEVIDWIDTH/2 + menutransition.tics*64, currentMenu->y + 75+16,
			recommendedflags, "Marked modes are recommended.");
		V_DrawCenteredString(BASEVIDWIDTH/2 + menutransition.tics*64, currentMenu->y + 75+24,
			highlightflags, "Other modes may have visual errors.");
		V_DrawCenteredString(BASEVIDWIDTH/2 + menutransition.tics*64, currentMenu->y + 75+32,
			highlightflags, "Larger modes may have performance issues.");
	}

	// Draw the cursor for the VidMode menu
	i = 41 - 10 + ((optionsmenu.vidm_selected / optionsmenu.vidm_column_size)*7*13) + menutransition.tics*64;
	j = currentMenu->y + 14 + ((optionsmenu.vidm_selected % optionsmenu.vidm_column_size)*8);

	V_DrawScaledPatch(i - 8, j, 0,
		W_CachePatchName("M_CURSOR", PU_CACHE));
}

// Gameplay Item Tggles:
tic_t shitsfree = 0;

void M_DrawItemToggles(void)
{
	const INT32 edges = 8;
	const INT32 height = 4;
	const INT32 spacing = 35;
	const INT32 column = itemOn/height;
	//const INT32 row = itemOn%height;
	INT32 leftdraw, rightdraw, totaldraw;
	INT32 x = currentMenu->x + menutransition.tics*64, y = currentMenu->y;
	INT32 onx = 0, ony = 0;
	consvar_t *cv;
	INT32 i, translucent, drawnum;

	M_DrawOptionsCogs();
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
				V_DrawScaledPatch(x, y, 0, W_CachePatchName("K_ISBG", PU_CACHE));
				V_DrawScaledPatch(x, y, 0, W_CachePatchName("K_ISTOGL", PU_CACHE));
				y += spacing;
				continue;
			}

			if (currentMenu->menuitems[thisitem].mvar1 == 255)
			{
				V_DrawScaledPatch(x, y, 0, W_CachePatchName("K_ISBGD", PU_CACHE));
				y += spacing;
				continue;
			}

			cv = &cv_items[currentMenu->menuitems[thisitem].mvar1-1];
			translucent = (cv->value ? 0 : V_TRANSLUCENT);

			drawnum = K_ItemResultToAmount(currentMenu->menuitems[thisitem].mvar1);

			if (cv->value)
				V_DrawScaledPatch(x, y, 0, W_CachePatchName("K_ISBG", PU_CACHE));
			else
				V_DrawScaledPatch(x, y, 0, W_CachePatchName("K_ISBGD", PU_CACHE));

			if (drawnum > 1)
			{
				V_DrawScaledPatch(x, y, 0, W_CachePatchName("K_ISMUL", PU_CACHE));
				V_DrawScaledPatch(x, y, translucent, W_CachePatchName(K_GetItemPatch(currentMenu->menuitems[thisitem].mvar1, true), PU_CACHE));
				V_DrawString(x+24, y+31, V_ALLOWLOWERCASE|translucent, va("x%d", drawnum));
			}
			else
				V_DrawScaledPatch(x, y, translucent, W_CachePatchName(K_GetItemPatch(currentMenu->menuitems[thisitem].mvar1, true), PU_CACHE));

			y += spacing;
		}

		x += spacing;
		y = currentMenu->y;
	}

	{
		if (currentMenu->menuitems[itemOn].mvar1 == 0)
		{
			V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITBG", PU_CACHE));
			V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITTOGL", PU_CACHE));
		}
		else if (currentMenu->menuitems[itemOn].mvar1 == 255)
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
			translucent = (cv->value ? 0 : V_TRANSLUCENT);

			drawnum = K_ItemResultToAmount(currentMenu->menuitems[itemOn].mvar1);

			if (cv->value)
				V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITBG", PU_CACHE));
			else
				V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITBGD", PU_CACHE));

			if (drawnum > 1)
			{
				V_DrawScaledPatch(onx-1, ony-2, 0, W_CachePatchName("K_ITMUL", PU_CACHE));
				V_DrawScaledPatch(onx-1, ony-2, translucent, W_CachePatchName(K_GetItemPatch(currentMenu->menuitems[itemOn].mvar1, false), PU_CACHE));
				V_DrawScaledPatch(onx+27, ony+39, translucent, W_CachePatchName("K_ITX", PU_CACHE));
				V_DrawKartString(onx+37, ony+34, translucent, va("%d", drawnum));
			}
			else
				V_DrawScaledPatch(onx-1, ony-2, translucent, W_CachePatchName(K_GetItemPatch(currentMenu->menuitems[itemOn].mvar1, false), PU_CACHE));
		}
	}
}


// EXTRAS:
// Copypasted from options but separate either way in case we want it to look more unique later down the line.
void M_DrawExtrasMovingButton(void)
{
	patch_t *butt = W_CachePatchName("OPT_BUTT", PU_CACHE);
	UINT8 *c = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_CACHE);

	V_DrawFixedPatch((extrasmenu.extx)*FRACUNIT, (extrasmenu.exty)*FRACUNIT, FRACUNIT, 0, butt, c);
	V_DrawCenteredGamemodeString((extrasmenu.extx)-3, (extrasmenu.exty) - 16, V_ALLOWLOWERCASE, c, EXTRAS_MainDef.menuitems[EXTRAS_MainDef.lastOn].text);
}

void M_DrawExtras(void)
{
	UINT8 i;
	INT32 x = 140 - (48*itemOn) + extrasmenu.offset;
	INT32 y = 70 + extrasmenu.offset;
	patch_t *buttback = W_CachePatchName("OPT_BUTT", PU_CACHE);
	patch_t *bg = W_CachePatchName("M_XTRABG", PU_CACHE);

	UINT8 *c = NULL;

	V_DrawFixedPatch(0, 0, FRACUNIT, 0, bg, NULL);

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
			V_DrawCenteredGamemodeString(px-3, py - 16, V_ALLOWLOWERCASE|tflag, (i == itemOn ? c : NULL), currentMenu->menuitems[i].text);
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

// PAUSE

// PAUSE MAIN MENU
void M_DrawPause(void)
{

	SINT8 i;
	SINT8 itemsdrawn = 0;
	SINT8 countdown = 0;
	INT16 ypos = -50;	// Draw 3 items from selected item (y=100 - 3 items spaced by 50 px each... you get the idea.)
	INT16 dypos;

	INT16 offset = menutransition.tics ? floor(pow(2, (double)menutransition.tics)) : pausemenu.openoffset;
	INT16 arrxpos = 150 + 2*offset;	// To draw the background arrow.

	INT16 j = 0;
	char word1[MAXSTRINGLENGTH];
	INT16 word1len = 0;
	char word2[MAXSTRINGLENGTH];
	INT16 word2len = 0;
	boolean sok = false;

	patch_t *pausebg = W_CachePatchName("M_STRIPU", PU_CACHE);
	patch_t *vertbg = W_CachePatchName("M_STRIPV", PU_CACHE);
	patch_t *pausetext = W_CachePatchName("M_PAUSET", PU_CACHE);

	patch_t *arrstart = W_CachePatchName("M_PTIP", PU_CACHE);
	patch_t *arrfill = W_CachePatchName("M_PFILL", PU_CACHE);

	//V_DrawFadeScreen(0xFF00, 16);

	// "PAUSED"
	V_DrawFixedPatch(-offset*FRACUNIT, 0, FRACUNIT, V_ADD, pausebg, NULL);
	V_DrawFixedPatch(-offset*FRACUNIT, 0, FRACUNIT, 0, pausetext, NULL);

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

				dypos = ypos + pausemenu.offset;
				V_DrawFixedPatch( ((i == itemOn ? (294 - pausemenu.offset*2/3 * (dypos > 100 ? 1 : -1)) : 261) + offset) << FRACBITS, (dypos)*FRACUNIT, FRACUNIT, 0, pp, colormap);

				ypos += 50;
				itemsdrawn++;	// We drew that!
				break;
			}
		}


		i++;	// Regardless of whether we drew or not, go to the next item in the menu.
		if (i > currentMenu->numitems)
		{
			i = 0;
			while (!(currentMenu->menuitems[i].status & IT_DISPLAY))
				i++;

		}
	}

	// Draw the string!
	// ...but first get what we need to get.
	while (currentMenu->menuitems[itemOn].text[j] && j < MAXSTRINGLENGTH)
	{
		char c = currentMenu->menuitems[itemOn].text[j];

		if (c == ' ' && !sok)
		{
			sok = true;
			j++;
			continue;	// We don't care about this :moyai:
		}

		if (sok)
		{
			word2[word2len] = c;
			word2len++;
		}
		else
		{
			word1[word1len] = c;
			word1len++;
		}

		j++;
	}

	word1[word1len] = '\0';
	word2[word2len] = '\0';

	if (itemOn == mpause_changegametype)
	{
		INT32 w = V_LSTitleLowStringWidth(gametypes[menugametype]->name, 0)/2;

		if (word1len)
			V_DrawCenteredLSTitleHighString(220 + offset*2, 75, 0, word1);

		V_DrawLSTitleLowString(220-w + offset*2, 103, V_YELLOWMAP, gametypes[menugametype]->name);
		V_DrawCharacter(220-w + offset*2 - 8 - (skullAnimCounter/5), 103+6, '\x1C' | V_YELLOWMAP, false); // left arrow
		V_DrawCharacter(220+w + offset*2 + 4 + (skullAnimCounter/5), 103+6, '\x1D' | V_YELLOWMAP, false); // right arrow
	}
	else
	{
		// If there's no 2nd word, take this opportunity to center this line of text.
		if (word1len)
			V_DrawCenteredLSTitleHighString(220 + offset*2, 75 + (!word2len ? 10 : 0), 0, word1);

		if (word2len)
			V_DrawCenteredLSTitleLowString(220 + offset*2, 103, 0, word2);
	}
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

		if ((i == playback_fastforward && cv_playbackspeed.value > 1) || (i == playback_rewind && demo.rewinding))
			V_DrawMappedPatch(currentMenu->x + currentMenu->menuitems[i].mvar1, currentMenu->y, V_SNAPTOTOP, icon, R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_JAWZ, GTC_MENUCACHE));
		else
			V_DrawMappedPatch(currentMenu->x + currentMenu->menuitems[i].mvar1, currentMenu->y, V_SNAPTOTOP, icon, (i == itemOn) ? activemap : inactivemap);

		if (i == itemOn)
		{
			V_DrawCharacter(currentMenu->x + currentMenu->menuitems[i].mvar1 + 4, currentMenu->y + 14,
				'\x1A' | V_SNAPTOTOP|highlightflags, false);

			V_DrawCenteredString(BASEVIDWIDTH/2, currentMenu->y + 18, V_SNAPTOTOP|V_ALLOWLOWERCASE, currentMenu->menuitems[i].text);

			if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_ARROWS)
			{
				char *str;

				if (!(i == playback_viewcount && r_splitscreen == 3))
					V_DrawCharacter(BASEVIDWIDTH/2 - 4, currentMenu->y + 28 - (skullAnimCounter/5),
						'\x1A' | V_SNAPTOTOP|highlightflags, false); // up arrow

				if (!(i == playback_viewcount && r_splitscreen == 0))
					V_DrawCharacter(BASEVIDWIDTH/2 - 4, currentMenu->y + 48 + (skullAnimCounter/5),
						'\x1B' | V_SNAPTOTOP|highlightflags, false); // down arrow

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

				V_DrawCenteredString(BASEVIDWIDTH/2, currentMenu->y + 38, V_SNAPTOTOP|V_ALLOWLOWERCASE|highlightflags, str);
			}
		}
	}
}


// replay hut...
// ...dear lord this is messy, but Ima be real I ain't fixing this.

#define SCALEDVIEWWIDTH (vid.width/vid.dupx)
#define SCALEDVIEWHEIGHT (vid.height/vid.dupy)
static void M_DrawReplayHutReplayInfo(menudemo_t *demoref)
{
	patch_t *patch = NULL;
	UINT8 *colormap;
	INT32 x, y;

	switch (demoref->type)
	{
	case MD_NOTLOADED:
		V_DrawCenteredString(160, 40, 0, "Loading replay information...");
		break;

	case MD_INVALID:
		V_DrawCenteredString(160, 40, warningflags, "This replay cannot be played.");
		break;

	case MD_SUBDIR:
		break; // Can't think of anything to draw here right now

	case MD_OUTDATED:
		V_DrawThinString(17, 64, V_ALLOWLOWERCASE|V_TRANSLUCENT|highlightflags, "Recorded on an outdated version.");
		/* FALLTHRU */
	default:
		// Draw level stuff
		x = 15; y = 15;

		K_DrawMapThumbnail(
			x<<FRACBITS, y<<FRACBITS,
			80<<FRACBITS,
			((demoref->kartspeed & DF_ENCORE) ? V_FLIP : 0),
			demoref->map,
			NULL);

		if (demoref->kartspeed & DF_ENCORE)
		{
			static angle_t rubyfloattime = 0;
			const fixed_t rubyheight = FINESINE(rubyfloattime>>ANGLETOFINESHIFT);
			V_DrawFixedPatch((x+40)<<FRACBITS, ((y+25)<<FRACBITS) - (rubyheight<<1), FRACUNIT, 0, W_CachePatchName("RUBYICON", PU_CACHE), NULL);
			rubyfloattime += FixedMul(ANGLE_MAX/NEWTICRATE, renderdeltatics);
		}

		x += 85;

		if (demoref->map < nummapheaders && mapheaderinfo[demoref->map])
		{
			char *title = G_BuildMapTitle(demoref->map+1);
			V_DrawString(x, y, 0, title);
			Z_Free(title);
		}
		else
			V_DrawString(x, y, V_ALLOWLOWERCASE|V_TRANSLUCENT, "Level is not loaded.");

		if (demoref->numlaps)
			V_DrawThinString(x, y+9, V_ALLOWLOWERCASE, va("(%d laps)", demoref->numlaps));

		{
			const char *gtstring;
			if (demoref->gametype < 0)
			{
				gtstring = "Custom (not loaded)";
			}
			else
			{
				gtstring = gametypes[demoref->gametype]->name;

				if ((gametypes[demoref->gametype]->rules & GTR_CIRCUIT))
					gtstring = va("%s (%s)", gtstring, kartspeed_cons_t[(demoref->kartspeed & ~DF_ENCORE) + 1].strvalue);
			}

			V_DrawString(x, y+20, V_ALLOWLOWERCASE, gtstring);
		}

		if (!demoref->standings[0].ranking)
		{
			// No standings were loaded!
			V_DrawString(x, y+39, V_ALLOWLOWERCASE|V_TRANSLUCENT, "No standings available.");

			break;
		}

		V_DrawThinString(x, y+29, highlightflags, "WINNER");
		V_DrawString(x+38, y+30, V_ALLOWLOWERCASE, demoref->standings[0].name);

		if (demoref->gametype >= 0)
		{
			if (gametypes[demoref->gametype]->rules & GTR_POINTLIMIT)
			{
				V_DrawThinString(x, y+39, highlightflags, "SCORE");
			}
			else
			{
				V_DrawThinString(x, y+39, highlightflags, "TIME");
			}

			if (demoref->standings[0].timeorscore == (UINT32_MAX-1))
			{
				V_DrawThinString(x+32, y+39, 0, "NO CONTEST");
			}
			else if (gametypes[demoref->gametype]->rules & GTR_POINTLIMIT)
			{
				V_DrawString(x+32, y+40, 0, va("%d", demoref->standings[0].timeorscore));
			}
			else
			{
				V_DrawRightAlignedString(x+84, y+40, 0, va("%d'%02d\"%02d",
												G_TicsToMinutes(demoref->standings[0].timeorscore, true),
												G_TicsToSeconds(demoref->standings[0].timeorscore),
												G_TicsToCentiseconds(demoref->standings[0].timeorscore)
				));
			}
		}

		// Character face!

		// Lat: 08/06/2020: For some reason missing skins have their value set to 255 (don't even ask me why I didn't write this)
		// and for an even STRANGER reason this passes the first check below, so we're going to make sure that the skin here ISN'T 255 before we do anything stupid.

		if (demoref->standings[0].skin < numskins)
		{
			patch = faceprefix[demoref->standings[0].skin][FACE_WANTED];
			colormap = R_GetTranslationColormap(
				demoref->standings[0].skin,
				demoref->standings[0].color,
				GTC_MENUCACHE);
		}
		else
		{
			patch = W_CachePatchName("M_NOWANT", PU_CACHE);
			colormap = R_GetTranslationColormap(
				TC_RAINBOW,
				demoref->standings[0].color,
				GTC_MENUCACHE);
		}

		V_DrawMappedPatch(BASEVIDWIDTH-15 - SHORT(patch->width), y+20, 0, patch, colormap);

		break;
	}
}

void M_DrawReplayHut(void)
{
	INT32 x, y, cursory = 0;
	INT16 i;
	INT16 replaylistitem = currentMenu->numitems-2;
	boolean processed_one_this_frame = false;

	static UINT16 replayhutmenuy = 0;

	M_DrawEggaChannel();

	// Draw menu choices
	x = currentMenu->x;
	y = currentMenu->y;

	if (itemOn > replaylistitem)
	{
		itemOn = replaylistitem;
		dir_on[menudepthleft] = sizedirmenu-1;
		extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;
	}
	else if (itemOn < replaylistitem)
	{
		dir_on[menudepthleft] = 0;
		extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;
	}

	if (itemOn == replaylistitem)
	{
		INT32 maxy;
		// Scroll menu items if needed
		cursory = y + currentMenu->menuitems[replaylistitem].mvar1 + dir_on[menudepthleft]*10;
		maxy = y + currentMenu->menuitems[replaylistitem].mvar1 + sizedirmenu*10;

		if (cursory > maxy - 20)
			cursory = maxy - 20;

		if (cursory - replayhutmenuy > SCALEDVIEWHEIGHT-50)
			replayhutmenuy += (cursory-SCALEDVIEWHEIGHT-replayhutmenuy + 51)/2;
		else if (cursory - replayhutmenuy < 110)
			replayhutmenuy += (max(0, cursory-110)-replayhutmenuy - 1)/2;
	}
	else
		replayhutmenuy /= 2;

	y -= replayhutmenuy;

	// Draw static menu items
	for (i = 0; i < replaylistitem; i++)
	{
		INT32 localy = y + currentMenu->menuitems[i].mvar1;

		if (localy < 65)
			continue;

		if (i == itemOn)
			cursory = localy;

		if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
			V_DrawString(x, localy, 0, currentMenu->menuitems[i].text);
		else
			V_DrawString(x, localy, highlightflags, currentMenu->menuitems[i].text);
	}

	y += currentMenu->menuitems[replaylistitem].mvar1;

	for (i = 0; i < (INT16)sizedirmenu; i++)
	{
		INT32 localy = y+i*10;
		INT32 localx = x;

		if (localy < 65)
			continue;
		if (localy >= SCALEDVIEWHEIGHT)
			break;

		if (extrasmenu.demolist[i].type == MD_NOTLOADED && !processed_one_this_frame)
		{
			processed_one_this_frame = true;
			G_LoadDemoInfo(&extrasmenu.demolist[i]);
		}

		if (extrasmenu.demolist[i].type == MD_SUBDIR)
		{
			localx += 8;
			V_DrawScaledPatch(x - 4, localy, 0, W_CachePatchName(dirmenu[i][DIR_TYPE] == EXT_UP ? "M_RBACK" : "M_RFLDR", PU_CACHE));
		}

		if (itemOn == replaylistitem && i == (INT16)dir_on[menudepthleft])
		{
			cursory = localy;

			if (extrasmenu.replayScrollDelay)
				extrasmenu.replayScrollDelay--;
			else if (extrasmenu.replayScrollDir > 0)
			{
				if (extrasmenu.replayScrollTitle < (V_StringWidth(extrasmenu.demolist[i].title, 0) - (SCALEDVIEWWIDTH - (x<<1)))<<1)
					extrasmenu.replayScrollTitle++;
				else
				{
					extrasmenu.replayScrollDelay = TICRATE;
					extrasmenu.replayScrollDir = -1;
				}
			}
			else
			{
				if (extrasmenu.replayScrollTitle > 0)
					extrasmenu.replayScrollTitle--;
				else
				{
					extrasmenu.replayScrollDelay = TICRATE;
					extrasmenu.replayScrollDir = 1;
				}
			}

			V_DrawString(localx - (extrasmenu.replayScrollTitle>>1), localy, highlightflags|V_ALLOWLOWERCASE, extrasmenu.demolist[i].title);
		}
		else
			V_DrawString(localx, localy, V_ALLOWLOWERCASE, extrasmenu.demolist[i].title);
	}

	// Draw scrollbar
	y = sizedirmenu*10 + currentMenu->menuitems[replaylistitem].mvar1 + 30;
	if (y > SCALEDVIEWHEIGHT-80)
	{
		V_DrawFill(BASEVIDWIDTH-4, 75, 4, SCALEDVIEWHEIGHT-80, 159);
		V_DrawFill(BASEVIDWIDTH-3, 76 + (SCALEDVIEWHEIGHT-80) * replayhutmenuy / y, 2, (((SCALEDVIEWHEIGHT-80) * (SCALEDVIEWHEIGHT-80))-1) / y - 1, 149);
	}

	// Draw the cursor
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_CACHE));
	V_DrawString(currentMenu->x, cursory, highlightflags, currentMenu->menuitems[itemOn].text);

	// Now draw some replay info!
	V_DrawFill(10, 10, 300, 60, 159);

	if (itemOn == replaylistitem)
	{
		M_DrawReplayHutReplayInfo(&extrasmenu.demolist[dir_on[menudepthleft]]);
	}
}

void M_DrawReplayStartMenu(void)
{
	const char *warning;
	UINT8 i;
	menudemo_t *demoref = &extrasmenu.demolist[dir_on[menudepthleft]];

	M_DrawEggaChannel();
	M_DrawGenericMenu();

#define STARTY 62-(extrasmenu.replayScrollTitle>>1)
	// Draw rankings beyond first
	for (i = 1; i < MAXPLAYERS && demoref->standings[i].ranking; i++)
	{
		patch_t *patch;
		UINT8 *colormap;

		V_DrawRightAlignedString(BASEVIDWIDTH-100, STARTY + i*20,highlightflags, va("%2d", demoref->standings[i].ranking));
		V_DrawThinString(BASEVIDWIDTH-96, STARTY + i*20, V_ALLOWLOWERCASE, demoref->standings[i].name);

		if (demoref->standings[i].timeorscore == UINT32_MAX-1)
			V_DrawThinString(BASEVIDWIDTH-92, STARTY + i*20 + 9, 0, "NO CONTEST");
		else if (demoref->gametype < 0)
			;
		else if (gametypes[demoref->gametype]->rules & GTR_POINTLIMIT)
			V_DrawString(BASEVIDWIDTH-92, STARTY + i*20 + 9, 0, va("%d", demoref->standings[i].timeorscore));
		else
			V_DrawRightAlignedString(BASEVIDWIDTH-40, STARTY + i*20 + 9, 0, va("%d'%02d\"%02d",
											G_TicsToMinutes(demoref->standings[i].timeorscore, true),
											G_TicsToSeconds(demoref->standings[i].timeorscore),
											G_TicsToCentiseconds(demoref->standings[i].timeorscore)
			));

		// Character face!

		// Lat: 08/06/2020: For some reason missing skins have their value set to 255 (don't even ask me why I didn't write this)
		// and for an even STRANGER reason this passes the first check below, so we're going to make sure that the skin here ISN'T 255 before we do anything stupid.

		if (demoref->standings[i].skin < numskins)
		{
			patch = faceprefix[demoref->standings[i].skin][FACE_RANK];
			colormap = R_GetTranslationColormap(
				demoref->standings[i].skin,
				demoref->standings[i].color,
				GTC_MENUCACHE);
		}
		else
		{
			patch = W_CachePatchName("M_NORANK", PU_CACHE);
			colormap = R_GetTranslationColormap(
				TC_RAINBOW,
				demoref->standings[i].color,
				GTC_MENUCACHE);
		}

		V_DrawMappedPatch(BASEVIDWIDTH-5 - SHORT(patch->width), STARTY + i*20, 0, patch, colormap);
	}
#undef STARTY

	// Handle scrolling rankings
	if (extrasmenu.replayScrollDelay)
		extrasmenu.replayScrollDelay--;
	else if (extrasmenu.replayScrollDir > 0)
	{
		if (extrasmenu.replayScrollTitle < (i*20 - SCALEDVIEWHEIGHT + 100)<<1)
			extrasmenu.replayScrollTitle++;
		else
		{
			extrasmenu.replayScrollDelay = TICRATE;
			extrasmenu.replayScrollDir = -1;
		}
	}
	else
	{
		if (extrasmenu.replayScrollTitle > 0)
			extrasmenu.replayScrollTitle--;
		else
		{
			extrasmenu.replayScrollDelay = TICRATE;
			extrasmenu.replayScrollDir = 1;
		}
	}

	V_DrawFill(10, 10, 300, 60, 159);
	M_DrawReplayHutReplayInfo(demoref);

	V_DrawString(10, 72, highlightflags|V_ALLOWLOWERCASE, demoref->title);

	// Draw a warning prompt if needed
	switch (demoref->addonstatus)
	{
	case DFILE_ERROR_CANNOTLOAD:
		warning = "Some addons in this replay cannot be loaded.\nYou can watch anyway, but desyncs may occur.";
		break;

	case DFILE_ERROR_NOTLOADED:
	case DFILE_ERROR_INCOMPLETEOUTOFORDER:
		warning = "Loading addons will mark your game as modified, and Record Attack may be unavailable.\nYou can watch without loading addons, but desyncs may occur.";
		break;

	case DFILE_ERROR_EXTRAFILES:
		warning = "You have addons loaded that were not present in this replay.\nYou can watch anyway, but desyncs may occur.";
		break;

	case DFILE_ERROR_OUTOFORDER:
		warning = "You have this replay's addons loaded, but they are out of order.\nYou can watch anyway, but desyncs may occur.";
		break;

	default:
		return;
	}

	V_DrawSmallString(4, BASEVIDHEIGHT-14, V_ALLOWLOWERCASE, warning);
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
	ssize_t i, m;
	const UINT8 *flashcol = NULL;
	UINT8 hilicol;

	M_CacheAddonPatches();

	// hack: If we're calling this from GS_MENU, that means we're in the extras menu!
	// so draw the apropriate background
	if (gamestate == GS_MENU)
	{
		patch_t *bg = W_CachePatchName("M_XTRABG", PU_CACHE);
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, bg, NULL);
	}

	if (Playing())
		V_DrawCenteredString(BASEVIDWIDTH/2, 4, warningflags, "Adding files mid-game may cause problems.");
	else
		V_DrawCenteredString(BASEVIDWIDTH/2, 4, 0,
		LOCATIONSTRING1);

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y - 1;

	hilicol = V_GetStringColormap(highlightflags)[0];

	y -= 16;

	V_DrawString(x-21, y + (lsheadingheight - 12), highlightflags|V_ALLOWLOWERCASE, M_AddonsHeaderPath());
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
			V_DrawString(x + (skullAnimCounter/5) - 20, y+8, highlightflags, "\x1D");
		}
		V_DrawString(x + xoffs - 18, y+8, V_ALLOWLOWERCASE|tflag, str);
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
		ssize_t q = m;
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
	if (m > (ssize_t)sizedirmenu)
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
		V_DrawString(19, y+4 - (skullAnimCounter/5), highlightflags, "\x1A");

	if (skullAnimCounter < 4)
		flashcol = V_GetStringColormap(highlightflags);

	y -= (16-addonsseperation);

	for (; i < m; i++)
	{
		UINT32 flags = V_ALLOWLOWERCASE;
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
				flags = V_ALLOWLOWERCASE|highlightflags;
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

	if (m != (ssize_t)sizedirmenu)
		V_DrawString(19, y-12 + (skullAnimCounter/5), highlightflags, "\x1B");

	if (m < (2*numaddonsshown + 1))
	{
		y += ((2*numaddonsshown + 1)-m)*addonsseperation;
	}

	y -= 2;

	V_DrawSmallScaledPatch(x, y, ((!majormods) ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+4]);
	if (modifiedgame)
		V_DrawSmallScaledPatch(x, y, 0, addonsp[NUM_EXT+2]);

	m = numwadfiles-(mainwads+musicwads+1);

	V_DrawCenteredString(BASEVIDWIDTH/2, y+4, (majormods ? highlightflags : V_TRANSLUCENT), va("%ld ADD-ON%s LOADED", (long)m, (m == 1) ? "" : "S")); //+2 for music, sounds, +1 for main.kart
}

#undef addonsseperation

// Challenges Menu

static void M_DrawChallengeTile(INT16 i, INT16 j, INT32 x, INT32 y, boolean hili)
{
	unlockable_t *ref = NULL;
	patch_t *pat = missingpat;
	UINT8 *colormap = NULL, *bgmap = NULL;
	fixed_t siz, accordion;
	UINT8 id, num;
	boolean unlockedyet;
	boolean categoryside;

	id = (i * CHALLENGEGRIDHEIGHT) + j;
	num = gamedata->challengegrid[id];

	// Empty spots in the grid are always unconnected.
	if (num >= MAXUNLOCKABLES)
	{
		goto drawborder;
	}

	// Okay, this is what we want to draw.
	ref = &unlockables[num];

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

	if (challengesmenu.extradata[id].flip != 0
		&& challengesmenu.extradata[id].flip != (TILEFLIP_MAX/2))
	{
		angle_t bad = (FixedAngle((fixed_t)(challengesmenu.extradata[id].flip) * (360*FRACUNIT/TILEFLIP_MAX)) >> ANGLETOFINESHIFT) & FINEMASK;
		accordion = FINECOSINE(bad);
		if (accordion < 0)
			accordion = -accordion;
	}

	pat = W_CachePatchName(
		(ref->majorunlock ? "UN_BORDB" : "UN_BORDA"),
		PU_CACHE);

	bgmap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_SILVER, GTC_MENUCACHE);

	V_DrawStretchyFixedPatch(
		(x*FRACUNIT) + (SHORT(pat->width)*(FRACUNIT-accordion)/2), y*FRACUNIT,
		accordion,
		FRACUNIT,
		0, pat,
		bgmap
	);

	pat = missingpat;

	categoryside = (challengesmenu.extradata[id].flip <= TILEFLIP_MAX/4
		|| challengesmenu.extradata[id].flip > (3*TILEFLIP_MAX)/4);

	if (categoryside)
	{
		char categoryid = '8';
		colormap = bgmap;
		switch (ref->type)
		{
			case SECRET_SKIN:
				categoryid = '1';
				break;
			case SECRET_FOLLOWER:
				categoryid = '2';
				break;
			/*case SECRET_COLOR:
				categoryid = '3';
				break;*/
			case SECRET_CUP:
				categoryid = '4';
				break;
			case SECRET_HARDSPEED:
			case SECRET_MASTERMODE:
			case SECRET_ENCORE:
				categoryid = '5';
				break;
			case SECRET_ONLINE:
			case SECRET_ADDONS:
			case SECRET_EGGTV:
			case SECRET_ALTTITLE:
			case SECRET_SOUNDTEST:
				categoryid = '6';
				break;
			case SECRET_TIMEATTACK:
			case SECRET_PRISONBREAK:
			case SECRET_SPECIALATTACK:
			case SECRET_SPBATTACK:
				categoryid = '7';
				break;
		}
		pat = W_CachePatchName(va("UN_RR0%c%c",
			categoryid,
			(ref->majorunlock) ? 'B' : 'A'),
			PU_CACHE);
		if (pat == missingpat)
		{
			pat = W_CachePatchName(va("UN_RR0%c%c",
				categoryid,
				(ref->majorunlock) ? 'A' : 'B'),
				PU_CACHE);
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
		UINT8 iconid = 0;
		switch (ref->type)
		{
			case SECRET_SKIN:
			{
				INT32 skin = M_UnlockableSkinNum(ref);
				if (skin != -1)
				{
					colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);
					pat = faceprefix[skin][(ref->majorunlock) ? FACE_WANTED : FACE_RANK];
				}
				break;
			}
			case SECRET_FOLLOWER:
			{
				INT32 skin = M_UnlockableFollowerNum(ref);
				if (skin != -1)
				{
					UINT16 col = K_GetEffectiveFollowerColor(followers[skin].defaultcolor, cv_playercolor[0].value);
					colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);
					pat = W_CachePatchName(followers[skin].icon, PU_CACHE);
				}
				break;
			}

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
			case SECRET_ALTTITLE:
				iconid = 6;
				break;
			case SECRET_SOUNDTEST:
				iconid = 1;
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
				iconid = 0; // TEMPORARY
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

	siz = (SHORT(pat->width) << FRACBITS);

	if (!siz)
		; // prevent div/0
	else if (ref->majorunlock)
	{
		V_DrawStretchyFixedPatch(
			((x + 5)*FRACUNIT) + (32*(FRACUNIT-accordion)/2), (y + 5)*FRACUNIT,
			FixedDiv(32*accordion, siz),
			FixedDiv(32 << FRACBITS, siz),
			0, pat,
			colormap
		);
	}
	else
	{
		V_DrawStretchyFixedPatch(
			((x + 2)*FRACUNIT) + (16*(FRACUNIT-accordion)/2), (y + 2)*FRACUNIT,
			FixedDiv(16*accordion, siz),
			FixedDiv(16 << FRACBITS, siz),
			0, pat,
			colormap
		);
	}

drawborder:
	if (!hili)
	{
		return;
	}

	{
		boolean maj = (ref != NULL && ref->majorunlock);
		char buffer[9];
		sprintf(buffer, "UN_RETA1");
		buffer[6] = maj ? 'B' : 'A';
		buffer[7] = (skullAnimCounter/5) ? '2' : '1';
		pat = W_CachePatchName(buffer, PU_CACHE);

		colormap = R_GetTranslationColormap(TC_DEFAULT, cv_playercolor[0].value, GTC_MENUCACHE);

		V_DrawFixedPatch(
			x*FRACUNIT, y*FRACUNIT,
			FRACUNIT,
			0, pat,
			colormap
		);
	}
}

#define challengetransparentstrength 8

static void M_DrawChallengePreview(INT32 x, INT32 y)
{
	unlockable_t *ref = NULL;
	UINT8 *colormap = NULL;
	UINT16 specialmap = NEXTMAP_INVALID;

	if (challengesmenu.currentunlock >= MAXUNLOCKABLES)
	{
		return;
	}

	// Okay, this is what we want to draw.
	ref = &unlockables[challengesmenu.currentunlock];

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
			return;
		}

		useframe = (challengesmenu.ticker / 2) % sprdef->numframes;

		sprframe = &sprdef->spriteframes[useframe];
		patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);

		if (sprframe->flip & 1) // Only for first sprite
		{
			addflags ^= V_FLIP; // This sprite is left/right flipped!
		}

		V_DrawFixedPatch(x*FRACUNIT, (y+6)*FRACUNIT, FRACUNIT, addflags, patch, NULL);
		return;
	}

	switch (ref->type)
	{
		case SECRET_SKIN:
		{
			INT32 skin = M_UnlockableSkinNum(ref), i;
			// Draw our character!
			if (skin != -1)
			{
				colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);
				M_DrawCharacterSprite(x, y, skin, false, false, 0, colormap);

				for (i = 0; i < skin; i++)
				{
					if (!R_SkinUsable(-1, i, false))
						continue;
					if (skins[i].kartspeed != skins[skin].kartspeed)
						continue;
					if (skins[i].kartweight != skins[skin].kartweight)
						continue;

					colormap = R_GetTranslationColormap(i, skins[i].prefcolor, GTC_MENUCACHE);
					break;
				}

				V_DrawFixedPatch(4*FRACUNIT, (BASEVIDHEIGHT-(4+16))*FRACUNIT,
					FRACUNIT,
					0, faceprefix[i][FACE_RANK],
					colormap);

				if (i != skin)
				{
					V_DrawScaledPatch(4, (11 + BASEVIDHEIGHT-(4+16)), 0, W_CachePatchName("ALTSDOT", PU_CACHE));
				}

				V_DrawFadeFill(4+16, (BASEVIDHEIGHT-(4+16)), 16, 16, 0, 31, challengetransparentstrength);

				V_DrawFill(4+16+5,   (BASEVIDHEIGHT-(4+16))+1,    1, 14,  0);
				V_DrawFill(4+16+5+5, (BASEVIDHEIGHT-(4+16))+1,    1, 14,  0);
				V_DrawFill(4+16+1,   (BASEVIDHEIGHT-(4+16))+5,   14,  1,  0);
				V_DrawFill(4+16+1,   (BASEVIDHEIGHT-(4+16))+5+5, 14,  1,  0);

				// The following is a partial duplication of R_GetEngineClass
				{
					INT32 s = (skins[skin].kartspeed - 1)/3;
					INT32 w = (skins[skin].kartweight - 1)/3;

					#define LOCKSTAT(stat) \
						if (stat < 0) { stat = 0; } \
						if (stat > 2) { stat = 2; }
						LOCKSTAT(s);
						LOCKSTAT(w);
					#undef LOCKSTAT

					V_DrawFill(4+16 + (s*5), (BASEVIDHEIGHT-(4+16)) + (w*5), 6, 6, 0);
				}
			}
			break;
		}
		case SECRET_FOLLOWER:
		{
			INT32 skin = R_SkinAvailable(cv_skin[0].string);
			INT32 fskin = M_UnlockableFollowerNum(ref);

			// Draw proximity reference for character
			if (skin == -1)
				skin = 0;
			colormap = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_MENUCACHE);
			M_DrawCharacterSprite(x, y, skin, false, false, 0, colormap);

			// Draw follower next to them
			if (fskin != -1)
			{
				UINT16 col = K_GetEffectiveFollowerColor(followers[fskin].defaultcolor, cv_playercolor[0].value);
				colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_MENUCACHE);
				M_DrawFollowerSprite(x - 16, y, fskin, false, 0, colormap, NULL);

				if (followers[fskin].category < numfollowercategories)
				{
					V_DrawFixedPatch(4*FRACUNIT, (BASEVIDHEIGHT-(4+16))*FRACUNIT,
						FRACUNIT,
						0, W_CachePatchName(followercategories[followers[fskin].category].icon, PU_CACHE),
						NULL);
				}
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
			templevelsearch.typeoflevel = G_TOLFlag(GT_RACE)|G_TOLFlag(GT_BATTLE);
			templevelsearch.cupmode = true;
			templevelsearch.timeattack = false;
			templevelsearch.checklocked = false;

			M_DrawCupPreview(146, &templevelsearch);

			maxid = id = (temp->id % 14);
			offset = (temp->id - id) * 2;
			while (temp && maxid < 14)
			{
				maxid++;
				temp = temp->next;
			}

			V_DrawFadeFill(4, (BASEVIDHEIGHT-(4+16)), 28 + offset, 16, 0, 31, challengetransparentstrength);

			for (i = 0; i < offset; i += 4)
			{
				V_DrawFill(4+1 + i, (BASEVIDHEIGHT-(4+16))+3,   2, 2, 15);
				V_DrawFill(4+1 + i, (BASEVIDHEIGHT-(4+16))+8+3, 2, 2, 15);
			}

			for (i = 0; i < 7; i++)
			{
				if (templevelsearch.cup && id == i)
				{
					V_DrawFill(offset + 4   + (i*4), (BASEVIDHEIGHT-(4+16)),     4, 8, 0);
				}
				else if (i < maxid)
				{
					V_DrawFill(offset + 4+1 + (i*4), (BASEVIDHEIGHT-(4+16))+3, 2, 2, 0);
				}

				if (templevelsearch.cup && (templevelsearch.cup->id % 14) == i+7)
				{
					V_DrawFill(offset + 4 + (i*4), (BASEVIDHEIGHT-(4+16))+8, 4, 8, 0);
				}
				else if (i+7 < maxid)
				{
					V_DrawFill(offset + 4+1 + (i*4), (BASEVIDHEIGHT-(4+16))+8+3, 2, 2, 0);
				}
			}

			break;
		}
		case SECRET_MAP:
		{
			const char *gtname = "INVALID HEADER";
			UINT16 mapnum = M_UnlockableMapNum(ref);

			K_DrawMapThumbnail(
				(x-50)<<FRACBITS, (146+2)<<FRACBITS,
				80<<FRACBITS,
				0,
				mapnum,
				NULL);

			if (mapnum < nummapheaders && mapheaderinfo[mapnum] != NULL)
			{
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

					if (guessgt == GT_SPECIAL && !M_SecretUnlocked(SECRET_SPECIALATTACK, true))
					{
						gtname = "???";
					}
					else
					{
						gtname = gametypes[guessgt]->name;
					}
				}
			}

			V_DrawThinString(1, BASEVIDHEIGHT-(9+3), V_ALLOWLOWERCASE|V_6WIDTHSPACE, gtname);
			
			break;
		}
		case SECRET_ENCORE:
		{
			static UINT16 encoremapcache = NEXTMAP_INVALID;
			if (encoremapcache > nummapheaders)
			{
				encoremapcache = G_RandMap(G_TOLFlag(GT_RACE), -1, 2, 0, false, NULL);
			}
			specialmap = encoremapcache;
			break;
		}
		case SECRET_TIMEATTACK:
		{
			static UINT16 tamapcache = NEXTMAP_INVALID;
			if (tamapcache > nummapheaders)
			{
				tamapcache = G_RandMap(G_TOLFlag(GT_RACE), -1, 2, 0, false, NULL);
			}
			specialmap = tamapcache;
			break;
		}
		case SECRET_PRISONBREAK:
		{
			static UINT16 btcmapcache = NEXTMAP_INVALID;
			if (btcmapcache > nummapheaders)
			{
				btcmapcache = G_RandMap(G_TOLFlag(GT_BATTLE), -1, 2, 0, false, NULL);
			}
			specialmap = btcmapcache;
			break;
		}
		case SECRET_SPECIALATTACK:
		{
			static UINT16 sscmapcache = NEXTMAP_INVALID;
			if (sscmapcache > nummapheaders)
			{
				sscmapcache = G_RandMap(G_TOLFlag(GT_SPECIAL), -1, 2, 0, false, NULL);
			}
			specialmap = sscmapcache;
			break;
		}
		case SECRET_SPBATTACK:
		{
			static UINT16 spbmapcache = NEXTMAP_INVALID;
			if (spbmapcache > nummapheaders)
			{
				spbmapcache = G_RandMap(G_TOLFlag(GT_RACE), -1, 2, 0, false, NULL);
			}
			specialmap = spbmapcache;
			break;
		}
		case SECRET_HARDSPEED:
		{
			static UINT16 hardmapcache = NEXTMAP_INVALID;
			if (hardmapcache > nummapheaders)
			{
				hardmapcache = G_RandMap(G_TOLFlag(GT_RACE), -1, 2, 0, false, NULL);
			}
			specialmap = hardmapcache;
			break;
		}
		case SECRET_MASTERMODE:
		{
			static UINT16 mastermapcache = NEXTMAP_INVALID;
			if (mastermapcache > nummapheaders)
			{
				mastermapcache = G_RandMap(G_TOLFlag(GT_RACE), -1, 2, 0, false, NULL);
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
		case SECRET_ALTTITLE:
		{
			x = 8;
			y = BASEVIDHEIGHT-16;
			V_DrawGamemodeString(x, y - 32, V_ALLOWLOWERCASE, R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_PLAGUE, GTC_MENUCACHE), cv_alttitle.string);
			V_DrawThinString(x, y, V_6WIDTHSPACE|V_ALLOWLOWERCASE|highlightflags, "Press (A)");
			break;
		}
		default:
		{
			break;
		}
	}

	if (specialmap == NEXTMAP_INVALID || !ref)
		return;

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
		colormap = R_GetTranslationColormap(TC_DEFAULT, cv_playercolor[0].value, GTC_MENUCACHE);
		V_DrawFixedPatch((x+40)<<FRACBITS, ((y+25)<<FRACBITS),
			FRACUNIT/2, 0,
			W_CachePatchName("K_LAPE02", PU_CACHE),
			colormap);
	}
}

#define challengesgridstep 22
#define challengekeybarwidth 50

void M_DrawChallenges(void)
{
	INT32 x = currentMenu->x, explodex, selectx;
	INT32 y;
	INT16 i, j;
	const char *str;
	INT16 offset;

	{
#define questionslow 4 // slows down the scroll by this factor
#define questionloop (questionslow*100) // modulo
		INT32 questionoffset = (challengesmenu.ticker % questionloop);
		patch_t *bg = W_CachePatchName("BGUNLCKG", PU_CACHE);
		patch_t *qm = W_CachePatchName("BGUNLSC", PU_CACHE);

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
	{
		y = 120;

		V_DrawScaledPatch(0, y,
			(10-challengetransparentstrength)<<V_ALPHASHIFT,
			W_CachePatchName("MENUHINT", PU_CACHE));

		V_DrawFadeFill(0, y+27, BASEVIDWIDTH, BASEVIDHEIGHT - (y+27), 0, 31, challengetransparentstrength);
	}

	if (gamedata->challengegrid == NULL || challengesmenu.extradata == NULL)
	{
		V_DrawCenteredString(x, y, V_REDMAP, "No challenges available!?");
		goto challengedesc;
	}

	y = currentMenu->y;

	V_DrawFadeFill(0, y-2, BASEVIDWIDTH, 90, 0, 31, challengetransparentstrength);

	x -= (challengesgridstep-1);

	x += challengesmenu.offset;

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

			M_DrawChallengeTile(i, j, x, y, false);
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

	M_DrawChallengeTile(
		challengesmenu.hilix,
		challengesmenu.hiliy,
		selectx,
		currentMenu->y + (challengesmenu.hiliy*challengesgridstep),
		true);
	M_DrawCharSelectExplosions(false, explodex, currentMenu->y);

challengedesc:

	// Chao Keys
	{
		patch_t *key = W_CachePatchName("UN_CHA00", PU_CACHE);
		INT32 offs = challengesmenu.unlockcount[CC_CHAONOPE];
		if (offs & 1)
			offs = -offs;
		offs /= 2;

		if (gamedata->chaokeys > 9)
		{
			offs -= 6;
			if (gamedata->chaokeys > 99)
				offs -= 2; // as far as we can go
		}

		V_DrawFixedPatch((8+offs)*FRACUNIT, 5*FRACUNIT, FRACUNIT, 0, key, NULL);
		V_DrawKartString((27+offs), 9-challengesmenu.unlockcount[CC_CHAOANIM], 0, va("%u", gamedata->chaokeys));

		offs = challengekeybarwidth;
		if (gamedata->chaokeys < GDMAX_CHAOKEYS)
			offs = ((gamedata->pendingkeyroundoffset * challengekeybarwidth)/GDCONVERT_ROUNDSTOKEY);

		if (offs > 0)
			V_DrawFill(1, 25, offs, 2, 0);
		if (offs < challengekeybarwidth)
			V_DrawFadeFill(1+offs, 25, challengekeybarwidth-offs, 2, 0, 31, challengetransparentstrength);
	}

	// Tally
	{
		str = va("%d/%d",
			challengesmenu.unlockcount[CC_UNLOCKED] + challengesmenu.unlockcount[CC_TALLY],
			challengesmenu.unlockcount[CC_TOTAL]
			);
		V_DrawRightAlignedKartString(BASEVIDWIDTH-7, 9-challengesmenu.unlockcount[CC_ANIM], 0, str);
	}

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
		}
		else
		{
			str = "---";
		}

		offset = V_LSTitleLowStringWidth(str, 0) / 2;
		V_DrawLSTitleLowString(BASEVIDWIDTH/2 - offset, y+6, 0, str);
	}

	// Derived from M_DrawCharSelectPreview
	x = 40;
	y = BASEVIDHEIGHT-16;

	// Unlock preview
	M_DrawChallengePreview(x, y);

	// Conditions for unlock
	i = (challengesmenu.hilix * CHALLENGEGRIDHEIGHT) + challengesmenu.hiliy;

	if (challengesmenu.unlockcondition != NULL
	&& challengesmenu.currentunlock < MAXUNLOCKABLES
	&& ((gamedata->unlocked[challengesmenu.currentunlock] == true)
		|| ((challengesmenu.extradata != NULL)
		&& (challengesmenu.extradata[i].flags & CHE_HINT))
		)
	)
	{
		V_DrawCenteredString(BASEVIDWIDTH/2, 120 + 32, V_ALLOWLOWERCASE, challengesmenu.unlockcondition);
	}
}

#undef challengetransparentstrength
#undef challengesgridstep
#undef challengekeybarwidth

// Statistics menu

#define STATSSTEP 10

static void M_DrawMapMedals(INT32 mapnum, INT32 x, INT32 y)
{
	UINT8 lasttype = UINT8_MAX, curtype;
	emblem_t *emblem = M_GetLevelEmblems(mapnum);

	while (emblem)
	{
		switch (emblem->type)
		{
			case ET_TIME:
				curtype = 1;
				break;
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
				if (((emblem->flags & ME_ENCORE) && !M_SecretUnlocked(SECRET_ENCORE, true))
					|| ((emblem->flags & ME_SPBATTACK) && !M_SecretUnlocked(SECRET_SPBATTACK, true)))
				{
					emblem = M_GetLevelEmblems(-1);
					continue;
				}
				curtype = 0;
				break;
			}
			default:
				curtype = 0;
				break;
		}

		// Shift over if emblem is of a different discipline
		if (lasttype != UINT8_MAX && lasttype != curtype)
			x -= 4;
		lasttype = curtype;

		if (gamedata->collected[emblem-emblemlocations])
			V_DrawSmallMappedPatch(x, y, 0, W_CachePatchName(M_GetEmblemPatch(emblem, false), PU_CACHE),
			                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_MENUCACHE));
		else
			V_DrawSmallScaledPatch(x, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));

		emblem = M_GetLevelEmblems(-1);
		x -= 8;
	}
}

static void M_DrawStatsMaps(void)
{
	INT32 y = 80, i = -1;
	INT16 mnum;
	boolean dotopname = true, dobottomarrow = (statisticsmenu.location < statisticsmenu.maxscroll);
	INT32 location = statisticsmenu.location;

	if (location)
		V_DrawCharacter(10, y-(skullAnimCounter/5),
			'\x1A' | highlightflags, false); // up arrow

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

				if (mapheaderinfo[mnum]->cup)
					str = va("%s CUP", mapheaderinfo[mnum]->cup->name);
				else
					str = "LOST AND FOUND";

				V_DrawThinString(20,  y, V_6WIDTHSPACE|highlightflags, str);
			}

			if (dotopname)
			{
				V_DrawRightAlignedThinString(BASEVIDWIDTH-20, y, V_6WIDTHSPACE|highlightflags, "MEDALS");
				dotopname = false;
			}

			y += STATSSTEP;
			if (y >= BASEVIDHEIGHT-STATSSTEP)
				goto bottomarrow;

			continue;
		}

		M_DrawMapMedals(mnum+1, 291, y);

		{
			char *title = G_BuildMapTitle(mnum+1);
			V_DrawThinString(24, y, V_6WIDTHSPACE, title);
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
			V_DrawThinString(20, y, V_6WIDTHSPACE|highlightflags, "EXTRA MEDALS");
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
				V_DrawSmallMappedPatch(291, y+1, V_6WIDTHSPACE, W_CachePatchName("GOTITA", PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, color, GTC_MENUCACHE));
			}
			else
			{
				V_DrawSmallScaledPatch(291, y+1, V_6WIDTHSPACE, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			V_DrawThinString(24, y, V_6WIDTHSPACE, va("%s", unlockables[i].name));
		}

		y += STATSSTEP;

		if (y >= BASEVIDHEIGHT-STATSSTEP)
			goto bottomarrow;
	}
bottomarrow:
	if (dobottomarrow)
		V_DrawCharacter(10, y-STATSSTEP + (skullAnimCounter/5),
			'\x1B' | highlightflags, false); // down arrow
}

void M_DrawStatistics(void)
{
	char beststr[256];

	tic_t besttime = 0;

	INT32 i;
	INT32 mapsunfinished = 0;

	{
		patch_t *bg = W_CachePatchName("M_XTRABG", PU_CACHE);
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, bg, NULL);
	}

	beststr[0] = 0;
	V_DrawThinString(20, 22, V_6WIDTHSPACE|V_ALLOWLOWERCASE|highlightflags, "Total Play Time:");
	besttime = G_TicsToHours(gamedata->totalplaytime);
	if (besttime)
	{
		if (besttime >= 24)
		{
			strcat(beststr, va("%u day%s, ", besttime/24, (besttime < 48 ? "" : "s")));
			besttime %= 24;
		}

		strcat(beststr, va("%u hour%s, ", besttime, (besttime == 1 ? "" : "s")));
	}
	besttime = G_TicsToMinutes(gamedata->totalplaytime, false);
	if (besttime)
	{
		strcat(beststr, va("%u minute%s, ", besttime, (besttime == 1 ? "" : "s")));
	}
	besttime = G_TicsToSeconds(gamedata->totalplaytime);
	strcat(beststr, va("%i second%s", besttime, (besttime == 1 ? "" : "s")));
	V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 22, V_6WIDTHSPACE, beststr);
	beststr[0] = 0;

	V_DrawThinString(20, 32, V_6WIDTHSPACE|V_ALLOWLOWERCASE|highlightflags, "Total Rings:");
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
	V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 32, V_6WIDTHSPACE, va("%s collected", beststr));

	beststr[0] = 0;
	V_DrawThinString(20, 42, V_6WIDTHSPACE|V_ALLOWLOWERCASE|highlightflags, "Total Rounds:");

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

	V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 42, V_6WIDTHSPACE, beststr);

	if (!statisticsmenu.maplist)
	{
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 62, V_6WIDTHSPACE|V_ALLOWLOWERCASE, "No maps!?");
		return;
	}

	besttime = 0;

	for (i = 0; i < nummapheaders; i++)
	{
		if (!mapheaderinfo[i] || (mapheaderinfo[i]->menuflags & (LF2_NOTIMEATTACK|LF2_HIDEINSTATS|LF2_HIDEINMENU)))
			continue;

		if (!mapheaderinfo[i]->mainrecord || mapheaderinfo[i]->mainrecord->time <= 0)
		{
			mapsunfinished++;
			continue;
		}

		besttime += mapheaderinfo[i]->mainrecord->time;
	}

	V_DrawThinString(20, 60, V_6WIDTHSPACE|V_ALLOWLOWERCASE, "Combined time records:");

	sprintf(beststr, "%i:%02i:%02i.%02i", G_TicsToHours(besttime), G_TicsToMinutes(besttime, false), G_TicsToSeconds(besttime), G_TicsToCentiseconds(besttime));
	V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 60, V_6WIDTHSPACE|V_ALLOWLOWERCASE|(mapsunfinished ? V_REDMAP : 0), beststr);

	if (mapsunfinished)
		V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 70, V_6WIDTHSPACE|V_ALLOWLOWERCASE|V_REDMAP, va("(%d unfinished)", mapsunfinished));
	else
		V_DrawRightAlignedThinString(BASEVIDWIDTH-20, 70, V_6WIDTHSPACE|V_ALLOWLOWERCASE, "(complete)");

	V_DrawThinString(32, 70, V_6WIDTHSPACE, va("x %d/%d", M_CountMedals(false, false), M_CountMedals(true, false)));
	V_DrawSmallMappedPatch(20, 70, 0, W_CachePatchName("GOTITA", PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_GOLD, GTC_MENUCACHE));

	M_DrawStatsMaps();
}

#undef STATSSTEP
