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
#include "k_kart.h" // SRB2kart
#include "d_player.h" // KITEM_ constants
#include "doomstat.h" // MAXSPLITSCREENPLAYERS

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

#if defined(HAVE_SDL)
#include "SDL.h"
#if SDL_VERSION_ATLEAST(2,0,0)
#include "sdl/sdlmain.h" // JOYSTICK_HOTPLUG
#endif
#endif

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

// flags for text highlights
#define highlightflags V_ORANGEMAP
#define recommendedflags V_GREENMAP
#define warningflags V_GRAYMAP

#define SKULLXOFF -32
#define LINEHEIGHT 16
#define STRINGHEIGHT 8
#define FONTBHEIGHT 20
#define SMALLLINEHEIGHT 8
#define SLIDER_RANGE 10
#define SLIDER_WIDTH (8*SLIDER_RANGE+6)
#define SERVERS_PER_PAGE 11

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
	if (currentMenu == &MessageDef)
		menuactive = true;

	if (menuwipe)
		F_WipeStartScreen();

	if (menuactive)
	{
		if (gamestate == GS_MENU) // draw BG
			V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUBG", PU_CACHE), NULL);
		else if (!WipeInAction && currentMenu != &PAUSE_PlaybackMenuDef)
			V_DrawFadeScreen(0xFF00, 16); // now that's more readable with a faded background (yeah like Quake...)

		if (currentMenu->drawroutine)
			currentMenu->drawroutine(); // call current menu Draw routine

		// draw non-green resolution border
		if ((gamestate == GS_MENU) && ((vid.width % BASEVIDWIDTH != 0) || (vid.height % BASEVIDHEIGHT != 0)))
			V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("WEIRDRES", PU_CACHE), NULL);

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
#ifdef DEVELOP // Development -- show revision / branch info
				V_DrawThinString(vid.dupx, vid.height - 20*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, compbranch);
				V_DrawThinString(vid.dupx, vid.height - 10*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, comprevision);
#else // Regular build
				V_DrawThinString(vid.dupx, vid.height - 10*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, va("%s", VERSIONSTRING));
#endif
			}
		}
	}

	if (menuwipe)
	{
		F_WipeEndScreen();
		F_RunWipe(wipedefs[wipe_menu_final], false, "FADEMAP0", true);
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
	V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName("MENUHINT", PU_CACHE), NULL);
	if (currentMenu->menuitems[itemOn].tooltip != NULL)
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 12, V_ALLOWLOWERCASE|V_6WIDTHSPACE, currentMenu->menuitems[itemOn].tooltip);
}

//
// M_DrawMenuPreviews
//
// Draw a box with a preview image of the current option
//
static void M_DrawMenuPreviews(void)
{
	V_DrawFixedPatch(172<<FRACBITS, 29<<FRACBITS, FRACUNIT, 0, W_CachePatchName("MENUPREV", PU_CACHE), NULL);
	if (currentMenu->menuitems[itemOn].patch != NULL)
		V_DrawFixedPatch(0, 0, FRACUNIT, 0, W_CachePatchName(currentMenu->menuitems[itemOn].patch, PU_CACHE), NULL);
}

// Converts a string into question marks.
// Used for the secrets menu, to hide yet-to-be-unlocked stuff.
static const char *M_CreateSecretMenuOption(const char *str)
{
	static char qbuf[32];
	int i;

	for (i = 0; i < 31; ++i)
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

	qbuf[31] = '\0';
	return qbuf;
}

//
// M_DrawGenericMenu
//
// Default, generic text-based list menu, used for Options
//
void M_DrawGenericMenu(void)
{
	INT32 x = 0, y = 0, w, i, cursory = 0;

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
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
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
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						char asterisks[MAXSTRINGLENGTH+1];
						size_t sl;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
#if 0
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
#endif
							case IT_CV_PASSWORD:
								if (i == itemOn)
								{
									V_DrawRightAlignedThinString(x + MAXSTRINGLENGTH*8 + 10, y, V_ALLOWLOWERCASE, va(M_GetText("Tab: %s password"), cv->value ? "hide" : "show"));
								}

								if (!cv->value || i != itemOn)
								{
									sl = strlen(cv->string);
									memset(asterisks, '*', sl);
									memset(asterisks + sl, 0, MAXSTRINGLENGTH+1-sl);

									M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
									V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, asterisks);
									if (skullAnimCounter < 4 && i == itemOn)
										V_DrawCharacter(x + 8 + V_StringWidth(asterisks, 0), y + 12,
											'_' | 0x80, false);
									y += 16;
									break;
								}
								/* fallthru */
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

//
// M_DrawKartGamemodeMenu
//
// Huge gamemode-selection list for main menu
//
void M_DrawKartGamemodeMenu(void)
{
	INT16 i, x = 170;

	M_DrawMenuTooltips();
	M_DrawMenuPreviews();

	if (menutransition.tics)
		x -= 24 * menutransition.tics;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_STRING:
				V_DrawRightAlignedGamemodeString(x, currentMenu->menuitems[i].mvar1, 0, currentMenu->menuitems[i].text,
					(i == itemOn) ? SKINCOLOR_PLAGUE : SKINCOLOR_PIGEON);
				break;
		}
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
	char string[MAXMSGLINELEN];
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
				memset(string, 0, MAXMSGLINELEN);
				if (i >= MAXMSGLINELEN)
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
			if (i >= MAXMSGLINELEN)
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
	// Grr.  Need to autodetect for pic_ts.
	pic_t *pictest = (pic_t *)W_CachePatchName(currentMenu->menuitems[itemOn].patch, PU_CACHE);
	if (!pictest->zero)
		V_DrawScaledPic(0,0,0,W_GetNumForName(currentMenu->menuitems[itemOn].patch));
	else
	{
		patch_t *patch = W_CachePatchName(currentMenu->menuitems[itemOn].patch, PU_CACHE);
		if (patch->height <= BASEVIDHEIGHT)
			V_DrawScaledPatch(0,0,0,patch);
		else
			V_DrawSmallScaledPatch(0,0,0,patch);
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
	const fixed_t rad = 32<<FRACBITS;
	fixed_t angamt = 360;
	patch_t *patch = NULL;
	UINT8 numoptions;
	UINT8 i;

	if (p->mdepth == 2)
		numoptions = setup_chargrid[p->gridx][p->gridy].numskins;
	else
	{
		numoptions = 16;
		patch = W_CachePatchName("COLORSPH", PU_CACHE);
	}

	angamt /= numoptions*2;

	for (i = 0; i < numoptions; i++)
	{
		fixed_t cx = x << FRACBITS, cy = y << FRACBITS;
		fixed_t ang = -(angamt * i);
		UINT8 *colormap;
		INT16 n;

		ang += 90;

		cx += FixedMul(rad, FINECOSINE(FixedAngle(ang << FRACBITS) >> ANGLETOFINESHIFT));
		cy += FixedMul(rad, FINESINE(FixedAngle(ang << FRACBITS) >> ANGLETOFINESHIFT)) >> 2;

		if (p->mdepth == 2)
		{
			SINT8 skin;

			cx -= 8<<FRACBITS;
			n = (p->clonenum + i) % setup_chargrid[p->gridx][p->gridy].numskins;

			skin = setup_chargrid[p->gridx][p->gridy].skinlist[n];

			colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);
			patch = facerankprefix[skin];
		}
		else
		{
			cx -= 4<<FRACBITS;
			n = ((p->color + i) % (MAXSKINCOLORS-1));
			colormap = R_GetTranslationColormap(TC_DEFAULT, n, GTC_MENUCACHE);
		}

		V_DrawFixedPatch(cx, cy, FRACUNIT, 0, patch, colormap);
	}
}

static void M_DrawCharSelectPreview(UINT8 num)
{
	INT16 x = 11, y = 5;
	char letter = 'A' + num;
	SINT8 skin;
	UINT8 color;
	UINT8 *colormap;
	patch_t *sprpatch;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	setup_player_t *p = &setup_player[num];

	if (num & 1)
		x += 233;

	if (num > 1)
		y += 99;

	V_DrawScaledPatch(x, y+6, V_TRANSLUCENT, W_CachePatchName("PREVBACK", PU_CACHE));

	if (p->mdepth > 0)
	{
		skin = setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum];

		color = p->color;
		colormap = R_GetTranslationColormap(skin, color, GTC_MENUCACHE);

		if (skin != -1)
		{
			statenum_t st = S_KART_WALK1;
			UINT32 flags = 0;
			UINT32 frame;

			if (skullAnimCounter & 1)
				st++;

			sprdef = &skins[skin].spritedef;

			if (sprdef->numframes) // No frames ??
			{
				frame = states[st].frame & FF_FRAMEMASK;
				if (frame >= sprdef->numframes) // Walking animation missing
					frame = 0; // Try to use standing frame

				sprframe = &sprdef->spriteframes[frame];
				sprpatch = W_CachePatchNum(sprframe->lumppat[1], PU_CACHE);
				if (sprframe->flip & 1) // Only for first sprite
					flags |= V_FLIP; // This sprite is left/right flipped!

				// Flip for left-side players
				if (!(num & 1))
					flags ^= V_FLIP;

				if (skins[skin].flags & SF_HIRES)
				{
					V_DrawFixedPatch((x+32)<<FRACBITS,
								(y+75)<<FRACBITS,
								skins[skin].highresscale,
								flags, sprpatch, colormap);
				}
				else
					V_DrawMappedPatch(x+32, y+75, flags, sprpatch, colormap);
			}
		}

		if (p->mdepth == 2 || p->mdepth == 3)
			M_DrawCharSelectCircle(p, x+32, y+48);
	}

	V_DrawScaledPatch(x+9, y+2, 0, W_CachePatchName("FILEBACK", PU_CACHE));
	V_DrawScaledPatch(x, y, 0, W_CachePatchName(va("CHARSEL%c", letter), PU_CACHE));

	if (p->mdepth == 0 && (skullAnimCounter/5))
		V_DrawScaledPatch(x+1, y+36, 0, W_CachePatchName("4PSTART", PU_CACHE));
}

void M_DrawCharacterSelect(void)
{
	UINT8 i, j, k;
	UINT16 quadx, quady;
	SINT8 skin;

	// We have to loop twice -- first time to draw the drop shadows, a second time to draw the icons.
	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
		{
			skin = setup_chargrid[i][j].skinlist[0];
			quadx = 4 * (i / 3);
			quady = 4 * (j / 3);

			// Here's a quick little cheat to save on drawing time!
			// Don't draw a shadow if it'll get covered by another icon
			if ((i % 3 < 2) && (j % 3 < 2))
			{
				if ((setup_chargrid[i+1][j].skinlist[0] != -1)
				&& (setup_chargrid[i][j+1].skinlist[0] != -1)
				&& (setup_chargrid[i+1][j+1].skinlist[0] != -1))
					continue;
			}

			if (skin != -1)
				V_DrawScaledPatch(82 + (i*16) + quadx + 1, 22 + (j*16) + quady + 1, 0, W_CachePatchName("ICONBACK", PU_CACHE));
		}
	}

	// Draw this inbetween. These drop shadows should be covered by the stat graph, but the icons shouldn't.
	V_DrawScaledPatch(3, 2, 0, W_CachePatchName("STATGRPH", PU_CACHE));

	// Draw the icons now
	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
		{
			for (k = 0; k < setup_numplayers; k++)
			{
				if (setup_player[k].gridx == i && setup_player[k].gridy == j)
					break; // k == setup_numplayers means no one has it selected
			}

			skin = setup_chargrid[i][j].skinlist[0];
			quadx = 4 * (i / 3);
			quady = 4 * (j / 3);

			if (skin != -1)
			{
				UINT8 *colormap;

				if (k == setup_numplayers)
					colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE);
				else
					colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);

				V_DrawMappedPatch(82 + (i*16) + quadx, 22 + (j*16) + quady, 0, facerankprefix[skin], colormap);
			}
		}
	}

	// Draw a preview for each player
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		M_DrawCharSelectPreview(i);
}

//
// INGAME / PAUSE MENUS
//
void M_DrawPlaybackMenu(void)
{
	INT16 i;
	patch_t *icon = NULL;
	UINT8 *activemap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GOLD, GTC_MENUCACHE);

	// Toggle items
	if (paused && !demo.rewinding)
	{
		PAUSE_PlaybackMenu[playback_pause].status = PAUSE_PlaybackMenu[playback_fastforward].status = PAUSE_PlaybackMenu[playback_rewind].status = IT_DISABLED;
		PAUSE_PlaybackMenu[playback_resume].status = PAUSE_PlaybackMenu[playback_advanceframe].status = PAUSE_PlaybackMenu[playback_backframe].status = IT_CALL|IT_STRING;

		if (itemOn >= playback_rewind && itemOn <= playback_fastforward)
			itemOn += playback_backframe - playback_rewind;
	}
	else
	{
		PAUSE_PlaybackMenu[playback_pause].status = PAUSE_PlaybackMenu[playback_fastforward].status = PAUSE_PlaybackMenu[playback_rewind].status = IT_CALL|IT_STRING;
		PAUSE_PlaybackMenu[playback_resume].status = PAUSE_PlaybackMenu[playback_advanceframe].status = PAUSE_PlaybackMenu[playback_backframe].status = IT_DISABLED;

		if (itemOn >= playback_backframe && itemOn <= playback_advanceframe)
			itemOn -= playback_backframe - playback_rewind;
	}

	if (modeattacking)
	{
		for (i = playback_viewcount; i <= playback_view4; i++)
			PAUSE_PlaybackMenu[i].status = IT_DISABLED;

		//PAUSE_PlaybackMenu[playback_moreoptions].mvar1 = 72;
		//PAUSE_PlaybackMenu[playback_quit].mvar1 = 88;
		PAUSE_PlaybackMenu[playback_quit].mvar1 = 72;

		//currentMenu->x = BASEVIDWIDTH/2 - 52;
		currentMenu->x = BASEVIDWIDTH/2 - 44;
	}
	else
	{
		PAUSE_PlaybackMenu[playback_viewcount].status = IT_ARROWS|IT_STRING;

		for (i = 0; i <= splitscreen; i++)
			PAUSE_PlaybackMenu[playback_view1+i].status = IT_ARROWS|IT_STRING;
		for (i = splitscreen+1; i < 4; i++)
			PAUSE_PlaybackMenu[playback_view1+i].status = IT_DISABLED;

		//PAUSE_PlaybackMenu[playback_moreoptions].mvar1 = 156;
		//PAUSE_PlaybackMenu[playback_quit].mvar1 = 172;
		PAUSE_PlaybackMenu[playback_quit].mvar1 = 156;

		//currentMenu->x = BASEVIDWIDTH/2 - 94;
		currentMenu->x = BASEVIDWIDTH/2 - 88;
	}

	// wip
	//M_DrawTextBox(currentMenu->x-68, currentMenu->y-7, 15, 15);
	//M_DrawCenteredMenu();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		UINT8 *inactivemap = NULL;

		if (i >= playback_view1 && i <= playback_view4)
		{
			if (modeattacking) continue;

			if (splitscreen >= i - playback_view1)
			{
				INT32 ply = displayplayers[i - playback_view1];

				icon = facerankprefix[players[ply].skin];
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

				if (!(i == playback_viewcount && splitscreen == 3))
					V_DrawCharacter(BASEVIDWIDTH/2 - 4, currentMenu->y + 28 - (skullAnimCounter/5),
						'\x1A' | V_SNAPTOTOP|highlightflags, false); // up arrow

				if (!(i == playback_viewcount && splitscreen == 0))
					V_DrawCharacter(BASEVIDWIDTH/2 - 4, currentMenu->y + 48 + (skullAnimCounter/5),
						'\x1B' | V_SNAPTOTOP|highlightflags, false); // down arrow

				switch (i)
				{
				case playback_viewcount:
					str = va("%d", splitscreen+1);
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
