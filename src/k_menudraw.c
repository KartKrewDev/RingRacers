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
			V_DrawCustomFadeScreen("FADEMAP0", 4); // now that's more readable with a faded background (yeah like Quake...)

		if (currentMenu->drawroutine)
			currentMenu->drawroutine(); // call current menu Draw routine

		// draw non-green resolution border
		if ((vid.width % BASEVIDWIDTH != 0) || (vid.height % BASEVIDHEIGHT != 0))
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
		F_RunWipe(wipedefs[wipe_menu_final], false, "FADEMAP0", true, false);
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
	{
		V_DrawCenteredThinString(BASEVIDWIDTH/2, 12, V_ALLOWLOWERCASE|V_6WIDTHSPACE, currentMenu->menuitems[itemOn].tooltip);
	}
}

//
// M_DrawMenuPreviews
//
// Draw a box with a preview image of the current option
//
static void M_DrawMenuPreviews(void)
{
	V_DrawFixedPatch(172<<FRACBITS, 29<<FRACBITS, FRACUNIT, 0, W_CachePatchName("MENUPREV", PU_CACHE), NULL);

	if (currentMenu->menuitems[itemOn].patch == NULL)
	{
		patch_t *st = W_CachePatchName(va("MIMGST0%d", (skullAnimCounter % 4) + 1), PU_CACHE);
		V_DrawFixedPatch(181<<FRACBITS, 39<<FRACBITS, FRACUNIT, 0, st, NULL);
	}
	else
	{
		V_DrawFixedPatch(181<<FRACBITS, 39<<FRACBITS, FRACUNIT, 0, W_CachePatchName(currentMenu->menuitems[itemOn].patch, PU_CACHE), NULL);
	}
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
	UINT8 n = currentMenu->numitems-1;

	M_DrawMenuTooltips();
	M_DrawMenuPreviews();

	if (menutransition.tics)
		x -= 24 * menutransition.tics;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		INT16 y;

		if (i == n)
			y = 160;
		else
			y = 80 - (16 * (n-1)) + (32 * i);

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_STRING:
				V_DrawRightAlignedGamemodeString(x, y, 0, currentMenu->menuitems[i].text,
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
	angle_t angamt = ANGLE_MAX;
	UINT8 numoptions;
	UINT8 i;

	if (p->mdepth == CSSTEP_ALTS)
		numoptions = setup_chargrid[p->gridx][p->gridy].numskins;
	else
		numoptions = MAXSKINCOLORS-1;

	angamt /= numoptions;

	for (i = 0; i < numoptions; i++)
	{
		fixed_t cx = x << FRACBITS, cy = y << FRACBITS;
		boolean subtract = (i & 1);
		angle_t ang = ((i+1)/2) * angamt;
		patch_t *patch = NULL;
		UINT8 *colormap;
		fixed_t radius;
		INT16 n;

		if (p->mdepth == CSSTEP_ALTS)
		{
			SINT8 skin;

			n = (p->clonenum) + numoptions/2;
			if (subtract)
				n -= ((i+1)/2);
			else
				n += ((i+1)/2);
			n %= numoptions;

			skin = setup_chargrid[p->gridx][p->gridy].skinlist[n];
			patch = facerankprefix[skin];
			colormap = R_GetTranslationColormap(skin, skins[skin].prefcolor, GTC_MENUCACHE);
			radius = 24<<FRACBITS;

			cx -= (SHORT(patch->width) << FRACBITS) >> 1;
			cy -= (SHORT(patch->height) << FRACBITS) >> 1;
		}
		else
		{
			INT16 diff;

			n = (p->color-1) + numoptions/2;
			if (subtract)
				n -= ((i+1)/2);
			else
				n += ((i+1)/2);
			n %= numoptions;
			n++;

			colormap = R_GetTranslationColormap(TC_DEFAULT, n, GTC_MENUCACHE);

			if (n > p->color)
				diff = n - p->color;
			else
				diff = p->color - n;

			if (diff == 0)
				patch = W_CachePatchName("COLORSP2", PU_CACHE);
			else if (abs(diff) < 25)
				patch = W_CachePatchName("COLORSP1", PU_CACHE);
			else
				patch = W_CachePatchName("COLORSP0", PU_CACHE);

			radius = 28<<FRACBITS;
			//radius -= SHORT(patch->width) << FRACBITS;

			cx -= (SHORT(patch->width) << FRACBITS) >> 1;
		}

		if (subtract)
			ang = (signed)(ANGLE_90 - ang);
		else
			ang = ANGLE_90 + ang;

		if (numoptions % 2)
			ang = (signed)(ang - (angamt/2));

		if (p->rotate)
			ang = (signed)(ang + ((angamt / CSROTATETICS) * p->rotate));

		cx += FixedMul(radius, FINECOSINE(ang >> ANGLETOFINESHIFT));
		cy -= FixedMul(radius, FINESINE(ang >> ANGLETOFINESHIFT)) / 3;

		V_DrawFixedPatch(cx, cy, FRACUNIT, 0, patch, colormap);
		if (p->mdepth == CSSTEP_ALTS && n != p->clonenum)
			V_DrawFixedPatch(cx, cy, FRACUNIT, V_TRANSLUCENT, W_CachePatchName("ICONDARK", PU_CACHE), NULL);
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

	if (p->mdepth >= CSSTEP_CHARS)
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

		if (p->mdepth == CSSTEP_ALTS || p->mdepth == CSSTEP_COLORS)
			M_DrawCharSelectCircle(p, x+32, y+64);
	}

	if ((setup_animcounter/10) & 1)
	{
		if (p->mdepth == CSSTEP_NONE && num == setup_numplayers)
			V_DrawScaledPatch(x+1, y+36, 0, W_CachePatchName("4PSTART", PU_CACHE));
		//else if (p->mdepth >= CSSTEP_READY)
		//	V_DrawScaledPatch(x+1, y+36, 0, W_CachePatchName("4PREADY", PU_CACHE));
	}

	V_DrawScaledPatch(x+9, y+2, 0, W_CachePatchName("FILEBACK", PU_CACHE));
	V_DrawScaledPatch(x, y+2, 0, W_CachePatchName(va("CHARSEL%c", letter), PU_CACHE));
	V_DrawFileString(x+16, y+2, 0, "PLAYER");
}

static void M_DrawCharSelectExplosions(void)
{
	UINT8 i;

	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		INT16 quadx, quady;
		UINT8 *colormap;
		UINT8 frame;

		if (setup_explosions[i].tics == 0 || setup_explosions[i].tics > 5)
			continue;

		frame = 6 - setup_explosions[i].tics;

		quadx = 4 * (setup_explosions[i].x / 3);
		quady = 4 * (setup_explosions[i].y / 3);

		colormap = R_GetTranslationColormap(TC_DEFAULT, setup_explosions[i].color, GTC_MENUCACHE);

		V_DrawMappedPatch(
			82 + (setup_explosions[i].x*16) + quadx - 6,
			22 + (setup_explosions[i].y*16) + quady - 6,
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

	quadx = 4 * (p->gridx / 3);
	quady = 4 * (p->gridy / 3);

	x = 82 + (p->gridx*16) + quadx - 13,
	y = 22 + (p->gridy*16) + quady - 12,

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

void M_DrawCharacterSelect(void)
{
	UINT8 i, j, k;
	UINT8 priority = setup_animcounter % setup_numplayers;
	INT16 quadx, quady;
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

				if (setup_chargrid[i][j].numskins > 1)
					V_DrawScaledPatch(82 + (i*16) + quadx, 22 + (j*16) + quady + 11, 0, W_CachePatchName("ALTSDOT", PU_CACHE));
			}
		}
	}

	// Explosions when you've made your final selection
	M_DrawCharSelectExplosions();

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		// Draw a preview for each player
		M_DrawCharSelectPreview(i);

		if (i >= setup_numplayers)
			continue;

		// Draw the cursors
		if (i != priority)
			M_DrawCharSelectCursor(i);
	}

	// Draw the priority player over the other ones
	M_DrawCharSelectCursor(priority);
}

// LEVEL SELECT

static void M_DrawCupPreview(INT16 y, cupheader_t *cup)
{
	UINT8 i;
	INT16 x = -(cupgrid.previewanim % 82);

	V_DrawFill(0, y, BASEVIDWIDTH, 54, 31);

	if (cup && (cup->unlockrequired == -1 || unlockables[cup->unlockrequired].unlocked))
	{
		for (i = 0; i < cup->numlevels; i++)
		{
			lumpnum_t lumpnum;
			patch_t *PictureOfLevel;
			UINT8 lvloff = (i + (cupgrid.previewanim / 82)) % cup->numlevels;

			lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cup->levellist[lvloff]+1)));
			if (lumpnum != LUMPERROR)
				PictureOfLevel = W_CachePatchNum(lumpnum, PU_CACHE);
			else
				PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

			V_DrawSmallScaledPatch(x + 1 + (i*82), y+2, 0, PictureOfLevel);
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

static void M_DrawCupTitle(INT16 y, cupheader_t *cup)
{
	V_DrawScaledPatch(0, y, 0, W_CachePatchName("MENUHINT", PU_CACHE));

	if (cup)
	{
		boolean unlocked = (cup->unlockrequired == -1 || unlockables[cup->unlockrequired].unlocked);
		UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE);
		patch_t *icon = W_CachePatchName(cup->icon, PU_CACHE);
		const char *str = (unlocked ? va("%s Cup", cup->name) : "???");
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
			V_DrawCenteredLSTitleLowString(BASEVIDWIDTH/2, y+6, 0, va("%s Mode", Gametype_Names[levellist.newgametype]));
	}
}

void M_DrawCupSelect(void)
{
	UINT8 i, j;
	cupheader_t *cup = kartcupheaders;

	while (cup)
	{
		if (cup->id == CUPMENU_CURSORID)
			break;
		cup = cup->next;
	}

	for (i = 0; i < CUPMENU_COLUMNS; i++)
	{
		for (j = 0; j < CUPMENU_ROWS; j++)
		{
			UINT8 id = (i + (j * CUPMENU_COLUMNS)) + (cupgrid.pageno * (CUPMENU_COLUMNS * CUPMENU_ROWS));
			cupheader_t *iconcup = kartcupheaders;
			patch_t *patch = NULL;
			INT16 x, y;
			INT16 icony = 7;

			while (iconcup)
			{
				if (iconcup->id == id)
					break;
				iconcup = iconcup->next;
			}

			if (!iconcup)
				break;

			/*if (iconcup->emeraldnum == 0)
				patch = W_CachePatchName("CUPMON3A", PU_CACHE);
			else*/ if (iconcup->emeraldnum > 7)
			{
				patch = W_CachePatchName("CUPMON2A", PU_CACHE);
				icony = 5;
			}
			else
				patch = W_CachePatchName("CUPMON1A", PU_CACHE);

			x = 14 + (i*42);
			y = 20 + (j*44) - (15*menutransition.tics);

			V_DrawScaledPatch(x, y, 0, patch);

			if (iconcup->unlockrequired != -1 && !unlockables[iconcup->unlockrequired].unlocked)
			{
				patch_t *st = W_CachePatchName(va("ICONST0%d", (cupgrid.previewanim % 4) + 1), PU_CACHE);
				V_DrawScaledPatch(x + 8, y + icony, 0, st);
			}
			else
			{
				V_DrawScaledPatch(x + 8, y + icony, 0, W_CachePatchName(iconcup->icon, PU_CACHE));
				V_DrawScaledPatch(x + 8, y + icony, 0, W_CachePatchName("CUPBOX", PU_CACHE));
			}
		}
	}

	V_DrawScaledPatch(14 + (cupgrid.x*42) - 4,
		20 + (cupgrid.y*44) - 1 - (12*menutransition.tics),
		0, W_CachePatchName("CUPCURS", PU_CACHE)
	);

	M_DrawCupPreview(146 + (12*menutransition.tics), cup);
	M_DrawCupTitle(120 - (12*menutransition.tics), cup);
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

	if (mapheaderinfo[map]->actnum[0])
	{
		word2[word2len] = ' ';
		word2len++;

		for (i = 0; i < 3; i++)
		{
			if (!mapheaderinfo[map]->actnum[i])
				break;

			word2[word2len] = mapheaderinfo[map]->actnum[i];
			word2len++;
		}
	}

	word1[word1len] = '\0';
	word2[word2len] = '\0';

	for (i = 0; i < 2; i++)
	{
		INT32 c;

		if (i >= word1len)
			break;

		c = toupper(word1[i]) - LT_FONTSTART;

		if (!title_font_high[c])
			x2 += 16;
		else
			x2 += SHORT(title_font_high[c]->width) - 4;
	}

	if (word1len)
		V_DrawLSTitleHighString(x, y, 0, word1);
	if (word2len)
		V_DrawLSTitleLowString(x2, y+28, 0, word2);
}

static void M_DrawLevelSelectBlock(INT16 x, INT16 y, INT16 map, boolean redblink, boolean greyscale)
{
	lumpnum_t lumpnum;
	patch_t *PictureOfLevel;
	UINT8 *colormap = NULL;

	if (greyscale)
		colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE);

	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(map+1)));
	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchNum(lumpnum, PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	if (redblink)
		V_DrawScaledPatch(3+x, y, 0, W_CachePatchName("LVLSEL2", PU_CACHE));
	else
		V_DrawScaledPatch(3+x, y, 0, W_CachePatchName("LVLSEL", PU_CACHE));

	V_DrawSmallMappedPatch(9+x, y+6, 0, PictureOfLevel, colormap);
	M_DrawHighLowLevelTitle(98+x, y+8, map);
}

void M_DrawLevelSelect(void)
{
	INT16 i;
	INT16 start = M_GetFirstLevelInList(levellist.newgametype);
	INT16 map = start;
	INT16 t = (32*menutransition.tics), tay = 0;
	INT16 y = 80 - (12 * levellist.y);
	boolean tatransition = ((menutransition.startmenu == &PLAY_TimeAttackDef || menutransition.endmenu == &PLAY_TimeAttackDef) && menutransition.tics);

	if (tatransition)
	{
		t = -t;
		tay = t/2;
	}

	for (i = 0; i < M_CountLevelsToShowInList(levellist.newgametype); i++)
	{
		INT16 lvlx = t, lvly = y;

		while (!M_CanShowLevelInList(map, levellist.newgametype) && map < NUMMAPS)
			map++;

		if (map >= NUMMAPS)
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
		map++;
	}

	M_DrawCupTitle(tay, levellist.selectedcup);
}

void M_DrawTimeAttack(void)
{
	INT16 map = levellist.choosemap;
	INT16 t = (24*menutransition.tics);
	INT16 leftedge = 149+t+16;
	INT16 rightedge = 149+t+155;
	INT16 opty = 152;
	lumpnum_t lumpnum;
	UINT8 i;

	M_DrawLevelSelectBlock(0, 2, map, true, false);

	//V_DrawFill(24-t, 82, 100, 100, 36); // size test

	lumpnum = W_CheckNumForName(va("%sR", G_BuildMapName(map+1)));
	if (lumpnum != LUMPERROR)
		V_DrawScaledPatch(24-t, 82, 0, W_CachePatchNum(lumpnum, PU_CACHE));

	V_DrawScaledPatch(149+t, 70, 0, W_CachePatchName("BESTTIME", PU_CACHE));

	V_DrawRightAlignedString(rightedge-12, 82, highlightflags, "BEST LAP:");
	K_drawKartTimestamp(0, 162+t, 88, 0, 2);

	V_DrawRightAlignedString(rightedge-12, 112, highlightflags, "BEST TIME:");
	K_drawKartTimestamp(0, 162+t, 118, map, 1);

	for (i = 0; i < currentMenu->numitems; i++)
	{
		UINT32 f = (i == itemOn) ? recommendedflags : highlightflags;

		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_STRING:
				if (i >= currentMenu->numitems-1)
					V_DrawRightAlignedString(rightedge, opty, f, currentMenu->menuitems[i].text);
				else
					V_DrawString(leftedge, opty, f, currentMenu->menuitems[i].text);
				opty += 10;
				break;
			case IT_SPACE:
				opty += 4;
				break;
		}
	}
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
