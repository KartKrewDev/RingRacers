// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_main.c
/// \brief SRB2 main program
///
///        SRB2 main program (D_SRB2Main) and game loop (D_SRB2Loop),
///        plus functions to parse command line parameters, configure game
///        parameters, and call the startup functions.

#include <tracy/tracy/Tracy.hpp>

#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __GNUC__
#include <unistd.h> // for getcwd
#endif

#ifdef _WIN32
#include <direct.h>
#include <malloc.h>
#endif

#include <time.h>

#include "doomdef.h"
#include "am_map.h"
#include "console.h"
#include "d_net.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_joy.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_time.h"
#include "i_threads.h"
#include "i_video.h"
#include "m_argv.h"
#include "k_menu.h"
#include "m_misc.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "r_main.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_main.h"
#include "d_netfil.h"
#include "m_cheat.h"
#include "y_inter.h"
#include "p_local.h" // chasecam
#include "m_misc.h" // screenshot functionality
#include "deh_tables.h" // Dehacked list test
#include "m_cond.h" // condition initialization
#include "fastcmp.h"
#include "r_fps.h" // Frame interpolation/uncapped
#include "keys.h"
#include "g_input.h" // tutorial mode control scheming
#include "m_perfstats.h"
#include "core/memory.h"

#include "monocypher/monocypher.h"
#include "stun.h"

// SRB2Kart
#include "k_grandprix.h"
#include "doomstat.h"
#include "m_random.h" // P_ClearRandom
#include "k_specialstage.h"
#include "acs/interface.h"
#include "k_podium.h"
#include "k_vote.h"
#include "k_serverstats.h"
#include "music.h"
#include "k_dialogue.h"
#include "k_bans.h"
#include "k_credits.h"
#include "r_debug.hpp"
#include "k_director.h"
#include "m_pw.h"

#ifdef HWRENDER
#include "hardware/hw_main.h" // 3D View Rendering
#endif

#include "lua_script.h"

#include "lua_profile.h"

extern "C" consvar_t cv_lua_profile, cv_menuframeskip;

/* Manually defined asset hashes
 */

#define ASSET_HASH_BIOS_PK3						"2f3d5ac37fccd77a2bf7376f60a70ab1"
#define ASSET_HASH_SCRIPTS_PK3					"15e65f7f6d5460f9362c646714f57578"
#define ASSET_HASH_GFX_PK3						"142df1ca805fd80a688a318cc4d24ca0"
#define ASSET_HASH_TEXTURES_GENERAL_PK3			"1c91e9d6f407ba8350f7c2dce0035936"
#define ASSET_HASH_TEXTURES_SEGAZONES_PK3		"a029a0993e5f04056eb6c139b1ffa924"
#define ASSET_HASH_TEXTURES_ORIGINALZONES_PK3	"ee5c04df39386e6cd93d346769b37693"
#define ASSET_HASH_CHARS_PK3					"bf014478cdda5e9208e3dea3c51f58c5"
#define ASSET_HASH_FOLLOWERS_PK3				"a37b8796fc1d83d3398f79767aa0de47"
#define ASSET_HASH_MAPS_PK3						"a8bd1f924531c483f500d96583b7b837"
#define ASSET_HASH_UNLOCKS_PK3					"ebc06ff46c2cc80e93dadf5f7099d7b8"
#define ASSET_HASH_STAFFGHOSTS_PK3				"9cb77f6c0e801c1bc61ca84870b65707"
#define ASSET_HASH_SHADERS_PK3					"7aefd2aa55129b31210aa094cf782695"
#ifdef USE_PATCH_FILE
#define ASSET_HASH_PATCH_PK3					"00000000000000000000000000000000"
#endif

// Version numbers for netplay :upside_down_face:
int    VERSION;
int SUBVERSION;

UINT8 comprevision_abbrev_bin[GIT_SHA_ABBREV];

#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif

// platform independant focus loss
UINT8 window_notinfocus = false;
INT32 window_x;
INT32 window_y;

//
// DEMO LOOP
//
static char *startupiwads[MAX_WADFILES];
static char *startuppwads[MAX_WADFILES];

boolean devparm = false; // started game with -devparm

boolean g_singletics = false; // timedemo
boolean lastdraw = false;

tic_t g_fast_forward = 0;
tic_t g_fast_forward_clock_stop = INFTICS;

postimg_t postimgtype[MAXSPLITSCREENPLAYERS];
INT32 postimgparam[MAXSPLITSCREENPLAYERS];

// These variables are in effect
// whether the respective sound system is disabled
// or they're init'ed, but the player just toggled them

boolean sound_disabled = false;
boolean digital_disabled = false;

#ifdef DEBUGFILE
INT32 debugload = 0;
#endif

UINT16 numskincolors = SKINCOLOR_FIRSTFREESLOT;
menucolor_t *menucolorhead, *menucolortail;

char savegamename[256];
char gpbackup[256];

char srb2home[256] = ".";
char srb2path[256] = ".";
boolean usehome = true;
const char *pandf = "%s" PATHSEP "%s";
char addonsdir[MAX_WADPATH];
char downloaddir[sizeof addonsdir + sizeof DOWNLOADDIR_PART] = "DOWNLOAD";

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
// referenced from i_system.c for I_GetKey()

event_t events[MAXEVENTS];
INT32 eventhead, eventtail;

boolean dedicated = false;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent(const event_t *ev)
{
	events[eventhead] = *ev;
	eventhead = (eventhead+1) & (MAXEVENTS-1);
}

// modifier keys
// Now handled in I_OsPolling
UINT8 shiftdown = 0; // 0x1 left, 0x2 right
UINT8 ctrldown = 0; // 0x1 left, 0x2 right
UINT8 altdown = 0; // 0x1 left, 0x2 right
boolean capslock = 0;	// gee i wonder what this does.

static void HandleGamepadDeviceAdded(event_t *ev)
{
	char guid[64];
	char name[256];

	I_Assert(ev != NULL);
	I_Assert(ev->type == ev_gamepad_device_added);

	G_RegisterAvailableGamepad(ev->device);
	I_GetGamepadGuid(ev->device, guid, sizeof(guid));
	I_GetGamepadName(ev->device, name, sizeof(name));
	CONS_Alert(CONS_NOTICE, "Gamepad device %d connected: %s (%s)\n", ev->device, name, guid);
}

static void HandleGamepadDeviceRemoved(event_t *ev)
{
	int i = 0;
	I_Assert(ev != NULL);
	I_Assert(ev->type == ev_gamepad_device_removed);
	CONS_Alert(CONS_NOTICE, "Gamepad device %d disconnected\n", ev->device);

	boolean playerinterrupted = false;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		INT32 device = G_GetDeviceForPlayer(i);
		if (device == ev->device)
		{
			G_SetDeviceForPlayer(i, -1);
			playerinterrupted = true;
		}
	}

	// Downstream responders need to update player gamepad assignments, pause, etc
	G_UnregisterAvailableGamepad(ev->device);

	if (playerinterrupted && Playing() && !netgame && !demo.playback)
	{
		M_StartControlPanel();
	}
}

/// Respond to added/removed device events, for bookkeeping available gamepads.
void HandleGamepadDeviceEvents(event_t *ev)
{
	I_Assert(ev != NULL);

	switch (ev->type)
	{
	case ev_gamepad_device_added:
		HandleGamepadDeviceAdded(ev);
		break;
	case ev_gamepad_device_removed:
		HandleGamepadDeviceRemoved(ev);
		break;
	default:
		break;
	}
}

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents(boolean callresponders)
{
	event_t *ev;
	int i;

	boolean eaten;

	G_ResetAllDeviceResponding();

	// Save these in local variables because eventtail !=
	// eventhead was evaluating true when they were equal,
	// but only when using the Y button to restart a Time
	// Attack??
	INT32 tail = eventtail;
	INT32 head = eventhead;

	eventtail = eventhead;

	for (; tail != head; tail = (tail+1) & (MAXEVENTS-1))
	{
		ev = &events[tail];

		HandleGamepadDeviceEvents(ev);

		// console input
#ifdef HAVE_THREADS
		I_lock_mutex(&con_mutex);
#endif
		{
			eaten = CON_Responder(ev);
		}
#ifdef HAVE_THREADS
		I_unlock_mutex(con_mutex);
#endif

		if (eaten)
		{
			hu_keystrokes = true;
			continue; // ate the event
		}

		// update keys current state
		G_MapEventsToControls(ev);

		if (!callresponders)
			continue; // eat

		// Menu input
#ifdef HAVE_THREADS
		I_lock_mutex(&k_menu_mutex);
#endif
		{
			eaten = M_Responder(ev);
		}
#ifdef HAVE_THREADS
		I_unlock_mutex(k_menu_mutex);
#endif

		if (eaten)
			continue; // menu ate the event

		G_Responder(ev);
	}

	// Update menu CMD
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		M_UpdateMenuCMD(i, false);
	}
}

//
// D_Display
// draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
// added comment : there is a wipe eatch change of the gamestate
gamestate_t wipegamestate = GS_LEVEL;
// -1: Default; 0-n: Wipe index; INT16_MAX: do not wipe
INT16 wipetypepre = -1;
INT16 wipetypepost = -1;

static bool D_Display(bool world)
{
	bool ranwipe = false;
	boolean forcerefresh = false;
	static boolean wipe = false;
	INT32 wipedefindex = 0;
	UINT8 i;

	ZoneScoped;

	if (!dedicated)
	{
		if (nodrawers)
			return false; // for comparative timing/profiling

		// Lactozilla: Switching renderers works by checking
		// if the game has to do it right when the frame
		// needs to render. If so, five things will happen:
		// 1. Interface functions will be called so
		//    that switching to OpenGL creates a
		//    GL context, and switching to Software
		//    allocates screen buffers.
		// 2. Software will set drawer functions,
		//    and OpenGL will load textures and
		//    create plane polygons, if necessary.
		// 3. Functions related to switching video
		//    modes (resolution) are called.
		// 4. The frame is ready to be drawn!

		// Check for change of renderer or screen size (video mode)
		if ((setrenderneeded || setmodeneeded) && !wipe)
			SCR_SetMode(); // change video mode

		// Recalc the screen
		if (vid.recalc)
			SCR_Recalc(); // NOTE! setsizeneeded is set by SCR_Recalc()

		if (rendermode == render_soft)
		{
			for (i = 0; i <= r_splitscreen; ++i)
			{
				R_SetViewContext(static_cast<viewcontext_e>(VIEWCONTEXT_PLAYER1 + i));
				R_InterpolateViewRollAngle(rendertimefrac);
				R_CheckViewMorph(i);
			}
		}

		// Change the view size if needed
		// Set by changing video mode or renderer
		if (setsizeneeded)
		{
			R_ExecuteSetViewSize();
			forcerefresh = true; // force background redraw
		}

		// draw buffered stuff to screen
		// Used only by linux GGI version
		I_UpdateNoBlit();
	}

	// save the current screen if about to wipe
	wipe = (gamestate != wipegamestate);
	if (wipe)
	{
		// MUST be set for all later
		wipedefindex = gamestate; // wipe_xxx_toblack
		if (gamestate == GS_TITLESCREEN && wipegamestate != GS_INTRO)
			wipedefindex = wipe_titlescreen_toblack;
	}

	if (wipe && wipetypepre != INT16_MAX)
	{
		if (wipetypepre < 0 || !F_WipeExists(wipetypepre))
			wipetypepre = wipedefs[wipedefindex];

		if (rendermode != render_none)
		{
			// Fade to black first
			if (G_GamestateUsesLevel() == false // fades to black on its own timing, always
				// Wipe between credits slides.
				// But not back from attract demo, since F_CreditsDemoExitFade exists.
				&& (gamestate != GS_CREDITS || wipegamestate != GS_LEVEL)
				&& wipetypepre != UINT8_MAX)
			{
				F_WipeStartScreen();
				F_WipeColorFill(31);
				F_WipeEndScreen();
				F_RunWipe(wipedefindex, wipetypepre, gamestate != GS_MENU, "FADEMAP0", false, false);
				ranwipe = true;
			}

			if (G_GamestateUsesLevel() == false && rendermode != render_none)
			{
				V_SetPaletteLump("PLAYPAL"); // Reset the palette
				R_ReInitColormaps(0, NULL, 0);
			}

			F_WipeStartScreen();
		}
		else //dedicated servers
		{
			F_RunWipe(wipedefindex, wipedefs[wipedefindex], gamestate != GS_MENU, "FADEMAP0", false, false);
			ranwipe = true;
			wipegamestate = gamestate;
		}

		wipetypepre = -1;
	}
	else
		wipetypepre = -1;

	if (dedicated) //bail out after wipe logic
		return false;

	// Catch runaway clipping rectangles.
	V_ClearClipRect();

	// do buffered drawing
	switch (gamestate)
	{
		case GS_TITLESCREEN:
			if (!titlemapinaction || !curbghide)
			{
				F_TitleScreenDrawer();
			}
			break;

		case GS_INTERMISSION:
			Y_IntermissionDrawer();
			break;

		case GS_VOTING:
			Y_VoteDrawer();
			break;

		case GS_INTRO:
			F_IntroDrawer();
			if (wipegamestate == (gamestate_t)-1)
			{
				wipe = true;
				wipedefindex = gamestate; // wipe_xxx_toblack
			}
			break;

		case GS_CUTSCENE:
			F_CutsceneDrawer();
			break;

		case GS_EVALUATION:
			F_GameEvaluationDrawer();
			break;

		case GS_CREDITS:
			F_CreditDrawer();
			break;

		case GS_WAITINGPLAYERS:
			// The clientconnect drawer is independent...
			if (netgame)
			{
				// I don't think HOM from nothing drawing is independent...
				F_WaitingPlayersDrawer();
			}
		case GS_DEDICATEDSERVER:
		case GS_NULL:
		default:
			break;
	}

	HU_Erase();

	// STUPID race condition...
	{
		wipegamestate = gamestate;

		// clean up border stuff
		// see if the border needs to be initially drawn
		if (G_GamestateUsesLevel() == true)
		{
			if (!automapactive && !dedicated && cv_renderview.value && (world || forcerefresh))
			{
				R_ApplyLevelInterpolators(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);

				viewwindowy = 0;
				viewwindowx = 0;

				topleft = screens[0] + viewwindowy*vid.width + viewwindowx;
				objectsdrawn = 0;

				ps_rendercalltime = I_GetPreciseTime();

				if (rendermode == render_soft)
				{
					if (cv_homremoval.value)
					{
						if (cv_homremoval.value == 1)
						{
							// Clear the software screen buffer to remove HOM
							memset(screens[0], 31, vid.width * vid.height * vid.bpp);
						}
						else
						{
							//'development' HOM removal -- makes it blindingly obvious if HOM is spotted.
							memset(screens[0], 32+(timeinmap&15), vid.width * vid.height * vid.bpp);
						}
					}

					if (r_splitscreen == 2)
					{
						// Draw over the fourth screen so you don't have to stare at a HOM :V
						V_DrawFill(viewwidth, viewheight, viewwidth, viewheight, 31|V_NOSCALESTART);
					}
				}

				for (i = 0; i <= r_splitscreen; i++)
				{
					if (players[displayplayers[i]].mo || players[displayplayers[i]].playerstate == PST_DEAD)
					{
						viewssnum = i;

#ifdef HWRENDER
						if (rendermode == render_opengl)
							HWR_RenderPlayerView();
						else
#endif
						if (rendermode != render_none)
						{
							if (i > 0) // Splitscreen-specific
							{
								switch (i)
								{
									case 1:
										if (r_splitscreen > 1)
										{
											viewwindowx = viewwidth;
											viewwindowy = 0;
										}
										else
										{
											viewwindowx = 0;
											viewwindowy = viewheight;
										}
										M_Memcpy(ylookup, ylookup2, viewheight*sizeof (ylookup[0]));
										break;
									case 2:
										viewwindowx = 0;
										viewwindowy = viewheight;
										M_Memcpy(ylookup, ylookup3, viewheight*sizeof (ylookup[0]));
										break;
									case 3:
										viewwindowx = viewwidth;
										viewwindowy = viewheight;
										M_Memcpy(ylookup, ylookup4, viewheight*sizeof (ylookup[0]));
									default:
										break;
								}


								topleft = screens[0] + viewwindowy*vid.width + viewwindowx;
							}

							R_RenderPlayerView();

							if (i > 0)
								M_Memcpy(ylookup, ylookup1, viewheight*sizeof (ylookup[0]));
						}
					}
				}

				if (rendermode == render_soft)
				{
					for (i = 0; i <= r_splitscreen; i++)
					{
						R_ApplyViewMorph(i);
					}
				}

				ps_rendercalltime = I_GetPreciseTime() - ps_rendercalltime;
				R_RestoreLevelInterpolators();
			}

			// rhi: display the software framebuffer to the screen
			//if (rendermode == render_soft)
			{
				// TODO: THIS SHOULD IDEALLY BE IN REGULAR HUD CODE !!
				// (st_stuff.c ST_Drawer, also duplicated in k_podium.c)
				// Unfortunately this is the latest place we can do it
				// If we could immediately tint the GPU data a lot
				// of problems could be solved (including GL support)
				// ---
				// last minute toast edit: We need to run most of this so
				// that the fallback GL behaviour activates at the right time

				if (gamestate != GS_TITLESCREEN
				&& G_GamestateUsesLevel() == true
				&& lt_fade < 16)
				{
					// Level fade-in
					V_DrawCustomFadeScreen(((levelfadecol == 0) ? "FADEMAP1" : "FADEMAP0"), 31-(lt_fade*2));
				}

				if (demo.attract == DEMO_ATTRACT_CREDITS)
				{
					INT32 val = F_CreditsDemoExitFade();
					if (val >= 0)
					{
						V_DrawCustomFadeScreen("FADEMAP0", val);
					}
				}
				else if (demo.attract == DEMO_ATTRACT_TITLE)
				{
					if (INT32 fade = F_AttractDemoExitFade())
					{
						V_DrawCustomFadeScreen("FADEMAP0", fade);
					}
				}

				if (rendermode == render_soft)
				{
					VID_DisplaySoftwareScreen();
				}
			}

			if (lastdraw)
			{
				if (rendermode == render_soft)
				{
					VID_BlitLinearScreen(screens[0], screens[1], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
				}

				lastdraw = false;
			}

			ps_uitime = I_GetPreciseTime();

			switch (gamestate)
			{
				case GS_LEVEL:
				{
					AM_Drawer();
					ST_Drawer();
					srb2::r_debug::draw_frame_list();
					F_TextPromptDrawer();
					break;
				}
				case GS_TITLESCREEN:
				{
					F_TitleScreenDrawer();
					break;
				}
				case GS_CEREMONY:
				{
					K_CeremonyDrawer();
					break;
				}
				default:
				{
					break;
				}
			}
		}
		else
		{
			ps_uitime = I_GetPreciseTime();
		}
	}

	if (Playing() || demo.playback)
	{
		HU_Drawer();
	}

	// change gamma if needed
	// (GS_LEVEL handles this already due to level-specific palettes)
	if (forcerefresh && G_GamestateUsesLevel() == false)
		V_SetPalette(0);

	if (demo.rewinding)
		V_DrawFadeScreen(TC_RAINBOW, (leveltime & 0x20) ? SKINCOLOR_PASTEL : SKINCOLOR_MOONSET);

	// vid size change is now finished if it was on...
	vid.recalc = 0;

#ifdef HAVE_THREADS
	I_lock_mutex(&k_menu_mutex);
#endif
	M_Drawer(); // menu is drawn even on top of everything
#ifdef HAVE_THREADS
	I_unlock_mutex(k_menu_mutex);
#endif
	// focus lost moved to M_Drawer

	CON_Drawer();

	ps_uitime = I_GetPreciseTime() - ps_uitime;

	//
	// wipe update
	//
	if (wipe && wipetypepost != INT16_MAX)
	{
		// note: moved up here because NetUpdate does input changes
		// and input during wipe tends to mess things up
		wipedefindex += WIPEFINALSHIFT;

		if (wipetypepost < 0 || !F_WipeExists(wipetypepost))
			wipetypepost = wipedefs[wipedefindex];

		if (rendermode != render_none)
		{
			F_WipeEndScreen();

			F_RunWipe(wipedefindex, wipedefs[wipedefindex], gamestate != GS_MENU && gamestate != GS_TITLESCREEN, "FADEMAP0", true, false);
			ranwipe = true;
		}

		// reset counters so timedemo doesn't count the wipe duration
		if (demo.timing)
		{
			framecount = 0;
			demostarttime = I_GetTime();
		}

		wipetypepost = -1;
	}
	else
		wipetypepost = -1;

	// It's safe to end the game now.
	if (G_GetExitGameFlag())
	{
		Command_ExitGame_f();
		G_ClearExitGameFlag();
	}

	//
	// normal update
	//
	if (!wipe)
	{
		if (cv_netstat.value)
		{
			char s[50];
			Net_GetNetStat();

			s[sizeof s - 1] = '\0';

			snprintf(s, sizeof s - 1, "get %d b/s", getbps);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-ST_HEIGHT-40, V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "send %d b/s", sendbps);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-ST_HEIGHT-30, V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "GameMiss %.2f%%", gamelostpercent);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-ST_HEIGHT-20, V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "SysMiss %.2f%%", lostpercent);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-ST_HEIGHT-10, V_YELLOWMAP, s);
		}

		if (cv_perfstats.value)
		{
			M_DrawPerfStats();
		}

		if (cv_lua_profile.value > 0)
		{
			LUA_RenderTimers();
		}

		ps_swaptime = I_GetPreciseTime();
		I_FinishUpdate(); // page flip or blit buffer
		ps_swaptime = I_GetPreciseTime() - ps_swaptime;
	}

	return ranwipe;
}

// =========================================================================
// D_SRB2Loop
// =========================================================================

tic_t rendergametic;

void D_SRB2Loop(void)
{
	tic_t entertic = 0, oldentertics = 0, realtics = 0, rendertimeout = INFTICS;
	double deltatics = 0.0;
	double deltasecs = 0.0;

	boolean interp = false;
	boolean doDisplay = false;
	int frameskip = 0;
	bool skiplaggyworld = false;
	double sincelastworld = 0.0;
	double minworldfps = 0.5;

	double worldfpsrun = 0.0;
	int worldfpscount = 0;
	int worldfpsavg = 0;

	if (dedicated)
		server = true;

	// Pushing of + parameters is now done back in D_SRB2Main, not here.

	I_UpdateTime();
	oldentertics = I_GetTime();

	// end of loading screen: CONS_Printf() will no more call FinishUpdate()
	con_startup = false;

	// make sure to do a d_display to init mode _before_ load a level
	SCR_SetMode(); // change video mode
	SCR_Recalc();

	chosenrendermode = render_none;

	// Check and print which version is executed.
	// Use this as the border between setup and the main game loop being entered.
	CONS_Printf(
	"===========================================================================\n"
	"                   We hope you enjoy this game as\n"
	"                     much as we did making it!\n"
	"===========================================================================\n");

	// hack to start on a nice clear console screen.
	COM_ImmedExecute("cls;version");

	I_FinishUpdate(); // page flip or blit buffer
	/*
	LMFAO this was showing garbage under OpenGL
	because I_FinishUpdate was called afterward
	*/

	for (;;)
	{
		// capbudget is the minimum precise_t duration of a single loop iteration
		precise_t capbudget;
		precise_t enterprecise = I_GetPreciseTime();
		precise_t finishprecise = enterprecise;

		g_dc = {};
		Z_Frame_Reset();
		srb2::r_debug::clear_frame_list();

		{
			// Casting the return value of a function is bad practice (apparently)
			double budget = round((1.0 / R_GetFramerateCap()) * I_GetPrecisePrecision());
			capbudget = (precise_t) budget;
		}

		bool ranwipe = false;
		bool world = false;

		I_UpdateTime();

		if (lastwipetic)
		{
			oldentertics = lastwipetic;
			lastwipetic = 0;
		}

		// get real tics
		entertic = I_GetTime();
		realtics = entertic - oldentertics;
		oldentertics = entertic;

		if (demo.playback && gamestate == GS_LEVEL)
		{
			// Nicer place to put this.
			realtics = realtics * cv_playbackspeed.value;
		}

#ifdef DEBUGFILE
		if (!realtics)
			if (debugload)
				debugload--;
#endif

		interp = R_UsingFrameInterpolation() && !dedicated;
		doDisplay = false;

		renderisnewtic = (realtics > 0 || singletics);

		bool timeisprogressing = (!(paused || P_AutoPause()) && !hu_stopped);

		if (renderisnewtic)
		{
			P_ResetInterpHudRandSeed(timeisprogressing);

			// don't skip more than 10 frames at a time
			// (fadein / fadeout cause massive frame skip!)
			if (realtics > 8)
				realtics = 1;

			// process tics (but maybe not if realtic == 0)
			{
				ZoneScopedN("TryRunTics");
				TryRunTics(realtics);
			}

			if (lastdraw || singletics || gametic > rendergametic)
			{
				rendergametic = gametic;
				rendertimeout = entertic + TICRATE/17;

				doDisplay = true;
			}
			else if (rendertimeout < entertic) // in case the server hang or netsplit
			{
				// Lagless camera! Yay!
				if (gamestate == GS_LEVEL && netgame)
				{
					// Evaluate the chase cam once for every local realtic
					// This might actually be better suited inside G_Ticker or TryRunTics
					for (tic_t chasecamtics = 0; chasecamtics < realtics; chasecamtics++)
					{
						P_RunChaseCameras();
					}
					R_UpdateViewInterpolation();
				}

				doDisplay = true;
			}
		}

		if (interp)
		{
			renderdeltatics = FLOAT_TO_FIXED(deltatics);

			if (timeisprogressing)
			{
				rendertimefrac = g_time.timefrac;
			}
			else
			{
				rendertimefrac = FRACUNIT;
			}

			rendertimefrac_unpaused = g_time.timefrac;
		}
		else
		{
			renderdeltatics = realtics * FRACUNIT;
			rendertimefrac = FRACUNIT;
			rendertimefrac_unpaused = FRACUNIT;
		}

		if ((interp || doDisplay) && !frameskip && g_fast_forward == 0)
		{
			if (!renderisnewtic)
				P_ResetInterpHudRandSeed(false);

			world = true;

			// TODO: skipping 3D rendering does not work in
			// Legacy GL -- the screen gets filled with a
			// single color.
			// In software, the last frame is preserved,
			// which is the intended effect.
			if (rendermode == render_soft)
			{
				auto none_freecam = []
				{
					for (UINT8 i = 0; i <= r_splitscreen; ++i)
					{
						if (camera[i].freecam || (players[displayplayers[i]].spectator && !K_DirectorIsAvailable(i)))
							return false;
					}
					return true;
				};
				auto can_skip = [&]
				{
					// Always do 3d rendering, even when paused.
					if (cv_renderview.value == 2)
						return false;

					// Would interfere with "Advanced Frame" button in replays.
					if (demo.playback)
						return false;

					// 3D rendering is stopped ENTIRELY if the game is paused.
					// - In single player, opening the menu pauses the game, so it's perfect.
					// - One exception: freecam is allowed to move when the game is paused.
					if ((paused || P_AutoPause()) && none_freecam())
						return true;

					// 3D framerate is always allowed to at least drop if the menu is open.
					if (skiplaggyworld && menuactive)
						return true;

					return false;
				};
				if (can_skip())
					world = false;
			}

			ranwipe = D_Display(world);
		}

#ifdef HWRENDER
		// Only take screenshots after drawing.
		if (moviemode && rendermode == render_opengl)
			M_LegacySaveFrame();
		if (rendermode == render_opengl && takescreenshot)
			M_DoLegacyGLScreenShot();
#endif

		if ((moviemode || takescreenshot) && rendermode == render_soft)
			I_CaptureVideoFrame();

		// consoleplayer -> displayplayers (hear sounds from viewpoint)
		S_UpdateSounds(); // move positional sounds
		if (realtics > 0 || singletics)
		{
			S_UpdateClosedCaptions();
			S_TickSoundTest();
		}

		LUA_Step();

#ifdef HAVE_DISCORDRPC
		if (! dedicated)
		{
			Discord_RunCallbacks();
		}
#endif

		Music_Tick();

		// Fully completed frame made.
		finishprecise = I_GetPreciseTime();

		// Use the time before sleep for frameskip calculations:
		// post-sleep time is literally being intentionally wasted
		deltasecs = (double)((INT64)(finishprecise - enterprecise)) / I_GetPrecisePrecision();
		deltatics = deltasecs * NEWTICRATE;

		// If time spent this game loop exceeds a single tic,
		// it's probably because of rendering.
		//
		// Skip rendering the next frame, up to a limit of 3
		// frames before a frame is rendered no matter what.
		//
		// Wipes run an inner loop and artificially increase
		// the measured time.
		if (!ranwipe && frameskip < 3 && deltatics > 1.0)
		{
			frameskip++;
		}
		else
		{
			frameskip = 0;
		}

		if (world)
		{
			sincelastworld = 0.0;

			worldfpsrun += deltasecs;
			worldfpscount++;
			if (worldfpsrun > 1.0)
			{
				worldfpsavg = worldfpscount;
				worldfpsrun = 0.0;
				worldfpscount = 0;
			}
		}
		else if (skiplaggyworld)
		{
			sincelastworld += deltasecs;
		}

		// Try to skip 3D rendering if the theoretical framerate drops below 60.
		// This measures the time spent rendering a single frame.
		// If the framrate is capped at a lower value than 60,
		// the time spent on each frame will not artificially increase.
		// So this measurement is accurate regardless of fpscap.
		if (sincelastworld <= minworldfps)
		{
			double goal = cv_menuframeskip.value;
			if (worldfpsavg < goal)
			{
				skiplaggyworld = true;
				minworldfps = 1.0 / std::max(worldfpsavg * worldfpsavg / goal, 2.0);
			}
		}
		else
		{
			skiplaggyworld = false;
		}

		if (!singletics)
		{
			INT64 elapsed = (INT64)(finishprecise - enterprecise);

			// in the case of "match refresh rate" + vsync, don't sleep at all
			const boolean vsync_with_match_refresh = cv_vidwait.value && cv_fpscap.value == 0;

			if (elapsed > 0 && (INT64)capbudget > elapsed && !vsync_with_match_refresh)
			{
				I_SleepDuration(capbudget - (finishprecise - enterprecise));
			}
		}
		// Capture the time once more to get the real delta time.
		finishprecise = I_GetPreciseTime();
		deltasecs = (double)((INT64)(finishprecise - enterprecise)) / I_GetPrecisePrecision();
		deltatics = deltasecs * NEWTICRATE;
	}
}

// =========================================================================
// D_SRB2Main
// =========================================================================

//
// D_ClearState
//
void D_ClearState(void)
{
	INT32 i;

	// okay, stop now
	// (otherwise the game still thinks we're playing!)
	SV_StopServer();
	SV_ResetServer();
	serverlistultimatecount = 0;

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	splitscreen = 0;
	SplitScreen_OnChange();

	cht_debug = 0;
	memset(&luabanks, 0, sizeof(luabanks));

	// In case someone exits out at the same time they start a time attack run,
	// reset modeattacking
	modeattacking = ATTACKING_NONE;
	marathonmode = static_cast<marathonmode_t>(0);

	// Reset GP and roundqueue
	memset(&grandprixinfo, 0, sizeof(struct grandprixinfo));
	memset(&roundqueue, 0, sizeof(struct roundqueue));

	// empty some other semi-important state
	maptol = 0;
	nextmapoverride = 0;
	skipstats = 0;
	tutorialchallenge = TUTORIALSKIP_NONE;
	gamemap = 1;

	gameaction = ga_nothing;
	memset(displayplayers, 0, sizeof(displayplayers));
	memset(g_localplayers, 0, sizeof g_localplayers);
	consoleplayer = 0;
	G_SetGametype(GT_RACE); // SRB2kart
	paused = false;

	// clear cmd building stuff
	G_ResetAllDeviceGameKeyDown();
	G_ResetAllDeviceResponding();

	// Reset the palette
	if (rendermode != render_none)
		V_SetPaletteLump("PLAYPAL");

	if (!Music_Playing("credits"))
		S_StopMusicCredit();

	S_StopSounds();
	S_SetSfxVolume(); // reset sound volume

	if (gamedata && gamedata->deferredsave)
		G_SaveGameData();

	K_UnsetDialogue();

	G_SetGamestate(GS_NULL);
	wipegamestate = GS_NULL;
}

static boolean g_deferredtitle = false;

//
// D_StartTitle
//
void D_StartTitle(void)
{
	demo.attract = DEMO_ATTRACT_OFF;

	Music_StopAll();

	D_ClearState();
	F_StartTitleScreen();
	M_ClearMenus(false);
	g_deferredtitle = false;
}

void D_SetDeferredStartTitle(boolean deferred)
{
	g_deferredtitle = deferred;
}

boolean D_IsDeferredStartTitle(void)
{
	return g_deferredtitle;
}

//
// D_AddFile
//
static void D_AddFile(char **list, const char *file)
{
	size_t pnumwadfiles;
	char *newfile;

	for (pnumwadfiles = 0; list[pnumwadfiles]; pnumwadfiles++)
		;

	newfile = static_cast<char*>(malloc(strlen(file) + 1));
	if (!newfile)
	{
		I_Error("No more free memory to AddFile %s",file);
	}
	strcpy(newfile, file);

	list[pnumwadfiles] = newfile;
}

static inline void D_CleanFile(char **list)
{
	size_t pnumwadfiles;
	for (pnumwadfiles = 0; list[pnumwadfiles]; pnumwadfiles++)
	{
		free(list[pnumwadfiles]);
		list[pnumwadfiles] = NULL;
	}
}

///\brief Checks if a netgame URL is being handled, and changes working directory to the EXE's if so.
///       Done because browsers (at least, Firefox on Windows) launch the game from the browser's directory, which causes problems.
static void ChangeDirForUrlHandler(void)
{
	// URL handlers are opened by web browsers (at least Firefox) from the browser's working directory, not the game's stored directory,
	// so chdir to that directory unless overridden.
	if (M_GetUrlProtocolArg() != NULL && !M_CheckParm("-nochdir"))
	{
		size_t i;

		CONS_Printf("%s connect links load game files from the SRB2 application's stored directory. Switching to ", SERVER_URL_PROTOCOL);
		strlcpy(srb2path, myargv[0], sizeof(srb2path));

		// Get just the directory, minus the EXE name
		for (i = strlen(srb2path)-1; i > 0; i--)
		{
			if (srb2path[i] == '/' || srb2path[i] == '\\')
			{
				srb2path[i] = '\0';
				break;
			}
		}

		CONS_Printf("%s\n", srb2path);

#if defined (_WIN32)
		SetCurrentDirectoryA(srb2path);
#else
		if (chdir(srb2path) == -1)
			I_OutputMsg("Couldn't change working directory\n");
#endif
	}
}

// ==========================================================================
// Identify the SRB2 version, and IWAD file to use.
// ==========================================================================

static boolean AddIWAD(void)
{
	char * path = va(pandf,srb2path,"bios.pk3");

	if (FIL_ReadFileOK(path))
	{
		D_AddFile(startupiwads, path);
		return true;
	}
	else
	{
		return false;
	}
}

static void IdentifyVersion(void)
{
	const char *srb2waddir = NULL;

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	// change to the directory where 'bios.pk3' is found
	srb2waddir = I_LocateWad();
#endif

	// get the current directory (possible problem on NT with "." as current dir)
	if (srb2waddir)
	{
		strlcpy(srb2path,srb2waddir,sizeof (srb2path));
	}
	else
	{
		if (getcwd(srb2path, 256) != NULL)
			srb2waddir = srb2path;
		else
			srb2waddir = ".";
	}

	// Load the IWAD
	if (! AddIWAD())
	{
		I_Error("\"bios.pk3\" not found! Expected in %s\n", srb2waddir);
	}

	// will be overwritten in case of -cdrom or unix/win home
	snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, srb2waddir);
	configfile[sizeof configfile - 1] = '\0';

	// if you change the ordering of this or add/remove a file, be sure to update the md5
	// checking in D_SRB2Main

	D_AddFile(startupiwads, va(pandf,srb2waddir,"scripts.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"gfx.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"textures_general.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"textures_segazones.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"textures_originalzones.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"chars.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"followers.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"maps.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"unlocks.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"staffghosts.pk3"));
	D_AddFile(startupiwads, va(pandf,srb2waddir,"shaders.pk3"));
#ifdef USE_PATCH_FILE
	D_AddFile(startupiwads, va(pandf,srb2waddir,"patch.pk3"));
#endif

#define MUSICTEST(str) \
	{\
		const char *musicpath = va(pandf,srb2waddir,str);\
		int ms = W_VerifyNMUSlumps(musicpath, false); \
		if (ms == 1) \
		{ \
			D_AddFile(startupiwads, musicpath); \
			musicwads++; \
		} \
		else if (ms == 0) \
			I_Error("File " str " has been modified with non-music/sound lumps"); \
	}

	MUSICTEST("sounds.pk3")
	MUSICTEST("music.pk3")
	MUSICTEST("altmusic.pk3")

#undef MUSICTEST
}

static void
D_AbbrevCommit (void)
{
	UINT8 i;

	for (i = 0; i < GIT_SHA_ABBREV; ++i)
	{
		sscanf(&comprevision[i * 2], "%2hhx",
				&comprevision_abbrev_bin[i]);
	}
}

static void
D_ConvertVersionNumbers (void)
{
	/* leave at defaults (0) under DEVELOP */
#ifndef DEVELOP
	sscanf(SRB2VERSION, "%d.%d", &VERSION, &SUBVERSION);
#endif
}

const char *D_GetFancyBranchName(void)
{
	if (!strcmp(compbranch, ""))
	{
		// \x8b = aqua highlight
		return "\x8b" "detached HEAD" "\x80";
	}

	return compbranch;
}

static void Command_assert(void)
{
#if !defined(NDEBUG) || defined(PARANOIA)
	CONS_Printf("Yes, assertions are enabled.\n");
#else
	CONS_Printf("No, ssertions are NOT enabled.\n");
#endif
}

#ifdef DEVELOP
static void Command_crash(void)
{
	I_Error("The game crashed on PURPOSE, because of the 'crash' command. (This is only enabled in DEVELOP builds.)");
}
#endif

//
// D_SRB2Main
//
void D_SRB2Main(void)
{
	INT32 i, j, p;
#ifdef DEVELOP
	INT32 pstartmap = 1; // default to first loaded map (Test Run)
#else
	INT32 pstartmap = 0; // default to random map (0 is not a valid map number)
#endif
	boolean autostart = false;
	INT32 newgametype = -1;

	/* break the version string into version numbers, for netplay */
	D_ConvertVersionNumbers();
	D_AbbrevCommit();

	// Print GPL notice for our console users (Linux)
	CONS_Printf(
	"\n\nDr. Robotnik's Ring Racers\n"
	"Copyright (C) 2024 by Kart Krew\n\n"
	"This program comes with ABSOLUTELY NO WARRANTY.\n\n"
	"This is free software, and you are welcome to redistribute it\n"
	"and/or modify it under the terms of the GNU General Public License\n"
	"as published by the Free Software Foundation; either version 2 of\n"
	"the License, or (at your option) any later version.\n"
	"See the 'LICENSE.txt' file for details.\n\n"
	"Dr. Robotnik and related characters are trademarks of SEGA.\n"
	"We do not claim ownership of SEGA's intellectual property used\n"
	"in this program.\n\n");

	// keep error messages until the final flush(stderr)
#if !defined(NOTERMIOS)
	if (setvbuf(stderr, NULL, _IOFBF, 1000))
		I_OutputMsg("setvbuf didnt work\n");
#endif

	// initialise locale code
	M_StartupLocale();

	// This will be done more properly on
	// level load, but for now at least make
	// sure that it is initalized at all
	P_ClearRandom(0);

	// get parameters from a response file (eg: srb2 @parms.txt)
	M_FindResponseFile();

	// MAINCFG is now taken care of where "OBJCTCFG" is handled
	G_LoadGameSettings();

	// Test Dehacked lists
	DEH_TableCheck();

	// Netgame URL special case: change working dir to EXE folder.
	ChangeDirForUrlHandler();

	// identify the main IWAD file to use
	IdentifyVersion();

#if !defined(NOTERMIOS)
	setbuf(stdout, NULL); // non-buffered output
#endif

#if 0 //defined (_DEBUG)
	devparm = M_CheckParm("-nodebug") == 0;
#else
	devparm = M_CheckParm("-debug") != 0;
#endif

	// for dedicated server
	dedicated = M_CheckParm("-dedicated") != 0;
	if (dedicated)
	{
		usedTourney = true;
	}

	if (devparm)
		CONS_Printf(M_GetText("Development mode ON.\n"));

	// default savegame
	strcpy(savegamename, SAVEGAMENAME"%u.ssg");
	strcpy(gpbackup, "gp" SAVEGAMENAME ".bkp"); // intentionally not ending with .ssg

	// Init the joined IP table for quick rejoining of past games.
	M_InitJoinedIPArray();

	{
		const char *userhome = D_Home(); //Alam: path to home

		if (!userhome)
		{
#if ((defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)) && !defined (__CYGWIN__)
			I_Error("Please set $HOME to your home directory\n");
#else
			if (dedicated)
				snprintf(configfile, sizeof configfile, "d" CONFIGFILENAME);
			else
				snprintf(configfile, sizeof configfile, CONFIGFILENAME);
#endif
		}
		else
		{
			// use user specific config file
#ifdef DEFAULTDIR
			snprintf(srb2home, sizeof srb2home, "%s" PATHSEP DEFAULTDIR, userhome);
			if (dedicated)
				snprintf(configfile, sizeof configfile, "%s" PATHSEP "d" CONFIGFILENAME, srb2home);
			else
				snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, srb2home);

			// can't use sprintf since there is %u in savegamename
			strcatbf(savegamename, srb2home, PATHSEP);
			strcatbf(gpbackup, srb2home, PATHSEP);

			snprintf(luafiledir, sizeof luafiledir, "%s" PATHSEP "luafiles", srb2home);
#else // DEFAULTDIR
			snprintf(srb2home, sizeof srb2home, "%s", userhome);
			if (dedicated)
				snprintf(configfile, sizeof configfile, "%s" PATHSEP "d"CONFIGFILENAME, userhome);
			else
				snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, userhome);

			// can't use sprintf since there is %u in savegamename
			strcatbf(savegamename, userhome, PATHSEP);
			strcatbf(gpbackup, userhome, PATHSEP);

			snprintf(luafiledir, sizeof luafiledir, "%s" PATHSEP "luafiles", userhome);
#endif // DEFAULTDIR
		}

		configfile[sizeof configfile - 1] = '\0';
	}

	// If config isn't writable, tons of behavior will be broken.
	// Fail loudly before things get confusing!
	{
		FILE *tmpfile;
		char testfile[MAX_WADPATH];

		snprintf(testfile, sizeof testfile, "%s" PATHSEP "file.tmp", srb2home);
		testfile[sizeof testfile - 1] = '\0';

		tmpfile = fopen(testfile, "w");
		if (tmpfile == NULL)
		{
#if defined (_WIN32)
			I_Error("Couldn't write game config.\nMake sure the game is installed somewhere it has write permissions.\n\n(Don't use the Downloads folder, Program Files, or your desktop!\nIf unsure, we recommend making a subfolder in your Documents folder.)");
#else
			I_Error("Couldn't write game config.\nMake sure you've installed the game somewhere it has write permissions.");
#endif
		}
		else
		{
			fclose(tmpfile);
			remove(testfile);
		}
	}

	M_LoadJoinedIPs();	// load joined ips

	// Create addons dir
	snprintf(addonsdir, sizeof addonsdir, "%s%s%s", srb2home, PATHSEP, "addons");
	I_mkdir(addonsdir, 0755);

	/* and downloads in a subdirectory */
	snprintf(downloaddir, sizeof downloaddir, "%s%s%s",
			addonsdir, PATHSEP, DOWNLOADDIR_PART);

	// rand() needs seeded regardless of password
	srand((unsigned int)time(NULL));
	rand();
	rand();
	rand();

	if (M_CheckParm("-password") && M_IsNextParm())
		D_SetPassword(M_GetNextParm());

	CONS_Printf("Z_Init(): Init zone memory allocation daemon. \n");
	Z_Init();
	CON_SetLoadingProgress(LOADED_ZINIT);

	M_NewGameDataStruct();

	// Do this up here so that WADs loaded through the command line can use ExecCfg
	COM_Init();

	COM_AddDebugCommand("assert", Command_assert);
#ifdef DEVELOP
	COM_AddDebugCommand("crash", Command_crash);
#endif

#ifndef TESTERS
	// add any files specified on the command line with -file wadfile
	// to the wad list
	if (!((M_GetUrlProtocolArg() || M_CheckParm("-connect")) && !M_CheckParm("-server")))
	{
		if (M_CheckParm("-file"))
		{
			// the parms after p are wadfile/lump names,
			// until end of parms or another - preceded parm
			while (M_IsNextParm())
			{
				const char *s = M_GetNextParm();

				if (s) // Check for NULL?
					D_AddFile(startuppwads, s);
			}
		}
	}
#endif

	// get map from parms

	if (M_CheckParm("-server") || dedicated)
		netgame = server = true;

	// adapt tables to SRB2's needs, including extra slots for dehacked file support
	P_PatchInfoTables();

	//---------------------------------------------------- READY TIME
	// we need to check for dedicated before initialization of some subsystems

	CONS_Printf("I_InitializeTime()...\n");
	I_InitializeTime();
	CON_SetLoadingProgress(LOADED_ISTARTUPTIMER);

	// Make backups of some SOCcable tables.
	P_BackupTables();

	// load wad, including the main wad file
	CONS_Printf("W_InitMultipleFiles(): Adding IWAD and main PWADs.\n");
	W_InitMultipleFiles(startupiwads, false);
	D_CleanFile(startupiwads);

	mainwads = 0;

#ifndef DEVELOP
	// Check MD5s of autoloaded files
	// Note: Do not add any files that ignore MD5!
	W_VerifyFileMD5(mainwads, ASSET_HASH_BIOS_PK3);									// bios.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_SCRIPTS_PK3);					// scripts.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_GFX_PK3);						// gfx.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_TEXTURES_GENERAL_PK3);			// textures_general.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_TEXTURES_SEGAZONES_PK3);		// textures_segazones.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_TEXTURES_ORIGINALZONES_PK3);	// textures_originalzones.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_CHARS_PK3);					// chars.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_FOLLOWERS_PK3);				// followers.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_MAPS_PK3);						// maps.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_UNLOCKS_PK3);					// unlocks.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_STAFFGHOSTS_PK3);				// staffghosts.pk3
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_SHADERS_PK3);					// shaders.pk3
#ifdef USE_PATCH_FILE
	mainwads++; W_VerifyFileMD5(mainwads, ASSET_HASH_PATCH_PK3);					// patch.pk3
#endif
#else
	mainwads++;	// scripts.pk3
	mainwads++;	// gfx.pk3
	mainwads++;	// textures_general.pk3
	mainwads++;	// textures_segazones.pk3
	mainwads++;	// textures_originalzones.pk3
	mainwads++;	// chars.pk3
	mainwads++; // followers.pk3
	mainwads++;	// maps.pk3
	mainwads++; // unlocks.pk3
	mainwads++; // staffghosts.pk3
	mainwads++; // shaders.pk3
#ifdef USE_PATCH_FILE
	mainwads++; // patch.pk3
#endif

#endif //ifndef DEVELOP

	// Load credits_def lump
	F_LoadCreditsDefinitions();

	// Do it before P_InitMapData because PNG patch
	// conversion sometimes needs the palette
	V_ReloadPalette();

	//
	// search for mainwad maps
	//
	P_InitMapData();
	basenummapheaders = nummapheaders;
	basenumkartcupheaders = numkartcupheaders;

	CON_SetLoadingProgress(LOADED_IWAD);

	CONS_Printf("W_InitMultipleFiles(): Adding external PWADs.\n");
	W_InitMultipleFiles(startuppwads, true);
	D_CleanFile(startuppwads);

	//
	// search for pwad maps
	//
	P_InitMapData();

	CON_SetLoadingProgress(LOADED_PWAD);

	M_PasswordInit();

	//---------------------------------------------------- READY SCREEN
	// we need to check for dedicated before initialization of some subsystems

	CONS_Printf("I_StartupGraphics()...\n");
	I_StartupGraphics();
	I_StartDisplayUpdate();

	I_StartupInput();

#ifdef HWRENDER
	// Lactozilla: Add every hardware mode CVAR and CCMD.
	// Has to be done before the configuration file loads,
	// but after the OpenGL library loads.
	HWR_AddCommands();
#endif

	//--------------------------------------------------------- CONSOLE
	// setup loading screen
	SCR_Startup();

	// Do this in background; lots of number crunching
	R_InitTranslucencyTables();

	CON_SetLoadingProgress(LOADED_ISTARTUPGRAPHICS);

	CONS_Printf("HU_Init()...\n");
	HU_Init();

	CON_Init();

	CON_SetLoadingProgress(LOADED_HUINIT);

	D_RegisterServerCommands();
	D_RegisterClientCommands(); // be sure that this is called before D_CheckNetGame
	R_RegisterEngineStuff();
	S_RegisterSoundStuff();

	I_RegisterSysCommands();

	M_Init();

	//--------------------------------------------------------- CONFIG.CFG
	M_FirstLoadConfig(); // WARNING : this do a "COM_BufExecute()"

	// Load Profiles now that default controls have been defined
	PR_LoadProfiles();	// load control profiles

	SV_LoadStats();

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	VID_PrepareModeList(); // Regenerate Modelist according to cv_fullscreen
#endif

	// set user default mode or mode set at cmdline
	SCR_CheckDefaultMode();

	if (M_CheckParm("-noupload"))
		COM_BufAddText("downloading 0\n");

	if (M_CheckParm("-gamedata") && M_IsNextParm())
	{
		// Moved from G_LoadGameData itself, as it would cause some crazy
		// confusion issues when loading mods.
		strlcpy(gamedatafilename, M_GetNextParm(), sizeof gamedatafilename);
	}
	G_LoadGameData();

	wipegamestate = gamestate;

	savedata.lives = 0; // flag this as not-used

	CON_SetLoadingProgress(LOADED_CONFIG);

	CONS_Printf("R_InitTextureData()...\n");
	R_InitTextureData(); // seperated out from below because it takes ages by itself
	CON_SetLoadingProgress(LOADED_INITTEXTUREDATA);

	CONS_Printf("R_InitSprites()...\n");
	R_InitSprites(); // ditto
	CON_SetLoadingProgress(LOADED_INITSPRITES);

	CONS_Printf("R_InitSkins()...\n");
	R_InitSkins(); // ditto
	CON_SetLoadingProgress(LOADED_INITSKINS);

	CONS_Printf("R_Init(): Init SRB2 refresh daemon.\n");
	R_Init();
	CON_SetLoadingProgress(LOADED_RINIT);

	// setting up sound
	if (dedicated)
	{
		sound_disabled = true;
		digital_disabled = true;
	}

	if (M_CheckParm("-noaudio")) // combines -nosound and -nomusic
	{
		sound_disabled = true;
		digital_disabled = true;
	}
	else
	{
		if (M_CheckParm("-nosound"))
			sound_disabled = true;
		if (M_CheckParm("-nomusic")) // combines -nomidimusic and -nodigmusic
		{
			digital_disabled = true;
		}
		else
		{
			if (M_CheckParm("-nodigmusic"))
				digital_disabled = true; // WARNING: DOS version initmusic in I_StartupSound
		}
	}

	if (!( sound_disabled && digital_disabled ))
	{
		CONS_Printf("S_InitSfxChannels(): Setting up sound channels.\n");
		I_StartupSound();
		I_InitMusic();
		S_InitSfxChannels();
		S_SetMusicVolume();
	}

	Music_Init();

	CON_SetLoadingProgress(LOADED_SINITSFXCHANNELS);

	S_InitMusicDefs();

	CONS_Printf("ST_Init(): Init status bar.\n");
	ST_Init();
	CON_SetLoadingProgress(LOADED_STINIT);

	CONS_Printf("ACS_Init(): Init Action Code Script VM.\n");
	ACS_Init();
	CON_SetLoadingProgress(LOADED_ACSINIT);

	//------------------------------------------------ COMMAND LINE PARAMS

	// this must be done after loading gamedata,
	// to avoid setting off the corrupted gamedata code in G_LoadGameData if a SOC with custom gamedata is added
	// -- Monster Iestyn 20/02/20
	if (M_CheckParm("-warp") && M_IsNextParm())
	{
		const char *word = M_GetNextParm();

		if (WADNAMECHECK(word))
		{
			if (!(pstartmap = wadnamemap))
				I_Error("Bad '%s' level warp.\n"
#if defined (_WIN32)
				"Are you using MSDOS 8.3 filenames in Zone Builder?\n"
				"\n"
				"To check: edit the Ring Racers game configuration in Zone Builder.\n"
				"Go to the Testing tab and make sure \"Use short paths and file names\" is turned off.\n"
				"(The option is hidden by default. Check \"Customize parameters\" to show it.)\n"
#endif
				, word);
		}
		else
		{
			if (!(pstartmap = G_FindMapByNameOrCode(word, 0)))
				I_Error("Cannot find a map remotely named '%s'\n", word);
		}

		{
			if (!M_CheckParm("-server") && !dedicated)
			{
				G_SetUsedCheats();

				// Start up a "minor" grand prix session
				memset(&grandprixinfo, 0, sizeof(struct grandprixinfo));
				memset(&roundqueue, 0, sizeof(struct roundqueue));

				grandprixinfo.gamespeed = KARTSPEED_NORMAL;
				grandprixinfo.masterbots = false;

				grandprixinfo.gp = true;
				grandprixinfo.cup = NULL;
				grandprixinfo.wonround = false;

				grandprixinfo.initalize = true;
			}

			autostart = true;
		}
	}

	// Set up splitscreen players before joining!
	if (!dedicated && (M_CheckParm("-splitscreen") && M_IsNextParm()))
	{
		UINT8 num = atoi(M_GetNextParm());
		if (num >= 1 && num <= 4)
		{
			CV_StealthSetValue(&cv_splitplayers, num);
			splitscreen = num-1;
			SplitScreen_OnChange();
		}
	}

	// init all NETWORK
	CONS_Printf("D_CheckNetGame(): Checking network game status.\n");
	if (D_CheckNetGame())
		autostart = true;
	CON_SetLoadingProgress(LOADED_DCHECKNETGAME);

	SV_LoadBans(); // Must be after D_CheckNetGame, or winsock getaddrinfo isn't ready and readback throws a fit

	if (splitscreen && !M_CheckParm("-connect")) // Make sure multiplayer & autostart is set if you have splitscreen, even after D_CheckNetGame
		multiplayer = autostart = true;

	// check for a driver that wants intermission stats
	// start the apropriate game based on parms
	if (M_CheckParm("-record") && M_IsNextParm())
	{
		G_RecordDemo(M_GetNextParm());
		autostart = true;
	}

	// user settings come before "+" parameters.
	if (dedicated)
		COM_ImmedExecute(va("exec \"%s" PATHSEP "ringserv.cfg\"\n", srb2home));
	else
		COM_ImmedExecute(va("exec \"%s" PATHSEP "ringexec.cfg\" -noerror\n", srb2home));

	if (!autostart)
		M_PushSpecialParameters(); // push all "+" parameters at the command buffer

	// demo doesn't need anymore to be added with D_AddFile()
	p = M_CheckParm("-playdemo");
	if (!p)
		p = M_CheckParm("-timedemo");
	if (p && M_IsNextParm())
	{
		char tmp[MAX_WADPATH];
		// add .lmp to identify the EXTERNAL demo file
		// it is NOT possible to play an internal demo using -playdemo,
		// rather push a playdemo command.. to do.

		strcpy(tmp, M_GetNextParm());
		// get spaced filename or directory
		while (M_IsNextParm())
		{
			strcat(tmp, " ");
			strcat(tmp, M_GetNextParm());
		}

		FIL_DefaultExtension(tmp, ".lmp");

		CONS_Printf(M_GetText("Playing demo %s.\n"), tmp);

		if (M_CheckParm("-playdemo"))
		{
			demo.quitafterplaying = true; // quit after one demo
			G_DeferedPlayDemo(tmp);
		}
		else
			G_TimeDemo(tmp);

		G_SetGamestate(GS_NULL);
		wipegamestate = GS_NULL;
		return;
	}

	/*if (M_CheckParm("-ultimatemode"))
	{
		autostart = true;
		ultimatemode = true;
	}*/

	// rei/miru: bootmap (Idea: starts the game on a predefined map)
	if (bootmap && !(M_CheckParm("-warp") && M_IsNextParm()))
	{
		pstartmap = G_MapNumber(bootmap)+1;

		if (pstartmap > nummapheaders)
		{
			I_Error("Cannot warp to map %s (not found)\n", bootmap);
		}

		autostart = true;
	}

	// Has to be done before anything else so skin, color, etc in command buffer has an affect.
	// ttlprofilen used because it's roughly equivalent in functionality - a QoL aid for quickly getting from startup to action
	if (!dedicated)
	{
		PR_ApplyProfile(cv_ttlprofilen.value, 0);

		if (gamedata->importprofilewins == true)
		{
			profile_t *pr = PR_GetProfile(cv_ttlprofilen.value);
			if (pr != NULL)
			{
				INT32 importskin = R_SkinAvailableEx(pr->skinname, false);
				if (importskin != -1)
				{
					skins[importskin].records.wins = pr->wins;

					cupheader_t *cup;
					for (cup = kartcupheaders; cup; cup = cup->next)
					{
						for (i = 0; i < KARTGP_MAX; i++)
						{
							if (cup->windata[i].best_placement == 0)
								continue;
							cup->windata[i].best_skin.id = importskin;
							cup->windata[i].best_skin.unloaded = NULL;
						}
					}

					unloaded_cupheader_t *unloadedcup;
					for (unloadedcup = unloadedcupheaders; unloadedcup; unloadedcup = unloadedcup->next)
					{
						for (i = 0; i < KARTGP_MAX; i++)
						{
							if (unloadedcup->windata[i].best_placement == 0)
								continue;
							unloadedcup->windata[i].best_skin.id = importskin;
							unloadedcup->windata[i].best_skin.unloaded = NULL;
						}
					}

					CONS_Printf(" Wins for profile \"%s\" imported onto character \"%s\"\n", pr->profilename, skins[importskin].name);
				}
			}

			gamedata->importprofilewins = false;
		}

		for (i = 1; i < cv_splitplayers.value; i++)
		{
			PR_ApplyProfile(cv_lastprofile[i].value, i);
		}

		if (M_CheckParm("-profile"))
		{
			UINT8 num = atoi(M_GetNextParm());
			PR_ApplyProfile(num, 0);
		}
		if (M_CheckParm("-profile2"))
		{
			UINT8 num = atoi(M_GetNextParm());
			PR_ApplyProfile(num, 1);
		}
		if (M_CheckParm("-profile3"))
		{
			UINT8 num = atoi(M_GetNextParm());
			PR_ApplyProfile(num, 2);
		}
		if (M_CheckParm("-profile4"))
		{
			UINT8 num = atoi(M_GetNextParm());
			PR_ApplyProfile(num, 3);
		}
	}

	SV_SaveStats();
	SV_SaveBans();

	if (autostart || netgame)
	{
		gameaction = ga_nothing;

		CV_ClearChangedFlags();

		// Do this here so if you run SRB2 with eg +timelimit 5, the time limit counts
		// as having been modified for the first game.
		M_PushSpecialParameters(); // push all "+" parameter at the command buffer

		COM_BufExecute(); // ensure the command buffer gets executed before the map starts (+skin)

		if (M_CheckParm("-gametype") && M_IsNextParm())
		{
			// from Command_Map_f
			const char *sgametype = M_GetNextParm();

			newgametype = G_GetGametypeByName(sgametype);

			if (newgametype == -1) // reached end of the list with no match
			{
				j = atoi(sgametype); // assume they gave us a gametype number, which is okay too
				if (j >= 0 && j < numgametypes)
					newgametype = (INT16)j;
			}

			if (newgametype != -1)
			{
				j = gametype;
				G_SetGametype(newgametype);
				D_GameTypeChanged(j);
			}
		}

		if (M_CheckParm("-skill") && M_IsNextParm())
		{
			INT16 newskill = -1;
			const char *sskill = M_GetNextParm();

			for (j = 0; gpdifficulty_cons_t[j].strvalue; j++)
			{
				if (!strcasecmp(gpdifficulty_cons_t[j].strvalue, sskill))
				{
					newskill = (INT16)gpdifficulty_cons_t[j].value;
					break;
				}
			}

			if (!gpdifficulty_cons_t[j].strvalue) // reached end of the list with no match
			{
				j = atoi(sskill); // assume they gave us a skill number, which is okay too
				if (j >= KARTSPEED_EASY && j <= KARTGP_MASTER)
					newskill = (INT16)j;
			}

			// Invalidate if locked.
			if ((newskill >= KARTSPEED_HARD && !M_SecretUnlocked(SECRET_HARDSPEED, true))
				|| (newskill >= KARTGP_MASTER && !M_SecretUnlocked(SECRET_MASTERMODE, true)))
			{
				newskill = -1;
			}

			if (newskill != -1)
			{
				if (grandprixinfo.gp == true)
				{
					if (newskill == KARTGP_MASTER)
					{
						grandprixinfo.masterbots = true;
						newskill = KARTSPEED_HARD;
					}

					grandprixinfo.gamespeed = newskill;
				}
				else if (newskill == KARTGP_MASTER)
				{
					newskill = KARTSPEED_HARD;
				}

				CV_SetValue(&cv_kartspeed, newskill);
			}
		}

		if (server && (dedicated || !M_CheckParm("+map")))
		{
			if (!pstartmap && (pstartmap = G_GetFirstMapOfGametype(gametype)+1) > nummapheaders)
			{
				I_Error("Can't get first map of gametype\n");
			}

			if (pstartmap != 1 && M_MapLocked(pstartmap))
			{
				G_SetUsedCheats();
			}

			if (grandprixinfo.gp == true && mapheaderinfo[pstartmap-1])
			{
				if (newgametype == -1)
				{
					newgametype = G_GuessGametypeByTOL(mapheaderinfo[pstartmap-1]->typeoflevel);
					if (newgametype != -1)
					{
						j = gametype;
						G_SetGametype(newgametype);
						D_GameTypeChanged(j);
					}

					multiplayer = true;
				}

				G_SetUsedCheats();
			}

			D_MapChange(pstartmap, gametype, (cv_kartencore.value == 1), true, 0, false, false);
		}
	}
	else if (M_CheckParm("-skipintro"))
	{
		F_StartTitleScreen();
		CV_StealthSetValue(&cv_currprofile, -1);
	}
	else
	{
		F_StartIntro(); // Tails 03-03-2002
		CV_StealthSetValue(&cv_currprofile, -1);
	}

	CON_ToggleOff();

#ifdef HAVE_DISCORDRPC
	if (! dedicated)
	{
		DRPC_Init();
	}
#endif

	if (con_startup_loadprogress != LOADED_ALLDONE)
	{
		I_Error("Something is wrong with the loading bar! (got %d, expected %d)\n", con_startup_loadprogress, LOADED_ALLDONE);
		return;
	}
}

const char *D_Home(void)
{
	const char *userhome = NULL;

#ifdef ANDROID
	return "/data/data/org.srb2/";
#endif

	if (M_CheckParm("-home") && M_IsNextParm())
		userhome = M_GetNextParm();
	else
	{
#if !((defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)) && !defined (__APPLE__)
		if (FIL_FileOK(CONFIGFILENAME))
			usehome = false; // Let's NOT use home
		else
#endif
			userhome = I_GetEnv("HOME"); //Alam: my new HOME for srb2
	}
#ifdef _WIN32 //Alam: only Win32 have APPDATA and USERPROFILE
	if (!userhome && usehome) //Alam: Still not?
	{
		char *testhome = NULL;
		testhome = I_GetEnv("APPDATA");
		if (testhome != NULL
			&& (FIL_FileOK(va("%s" PATHSEP "%s" PATHSEP CONFIGFILENAME, testhome, DEFAULTDIR))))
		{
			userhome = testhome;
		}
	}
#ifndef __CYGWIN__
	if (!userhome && usehome) //Alam: All else fails?
	{
		char *testhome = NULL;
		testhome = I_GetEnv("USERPROFILE");
		if (testhome != NULL
			&& (FIL_FileOK(va("%s" PATHSEP "%s" PATHSEP CONFIGFILENAME, testhome, DEFAULTDIR))))
		{
			userhome = testhome;
		}
	}
#endif// !__CYGWIN__
#endif// _WIN32
	if (usehome) return userhome;
	else return NULL;
}
