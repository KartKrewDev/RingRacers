// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_finale.c
/// \brief Title screen, intro, game evaluation, and credits.

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "s_sound.h"
#include "i_time.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_threads.h"
#include "dehacked.h"
#include "g_input.h"
#include "console.h"
#include "m_random.h"
#include "m_misc.h" // moviemode functionality
#include "y_inter.h"
#include "m_cond.h"
#include "p_local.h"
#include "p_setup.h"
#include "st_stuff.h" // hud hiding
#include "fastcmp.h"

#include "lua_hud.h"
#include "lua_hook.h"

// SRB2Kart
#include "k_menu.h"

// Stage of animation:
// 0 = text, 1 = art screen
INT32 finalecount;
INT32 titlescrollxspeed = 16;
INT32 titlescrollyspeed = 0;
UINT8 titlemapinaction = TITLEMAP_OFF;

static INT32 timetonext; // Delay between screen changes

static tic_t animtimer; // Used for some animation timings
static tic_t credbgtimer; // Credits background

static tic_t stoptimer;

static boolean keypressed = false;

static INT32 menuanimtimer; // Title screen: background animation timing
mobj_t *titlemapcameraref = NULL;

// menu presentation state
char curbgname[9];
SINT8 curfadevalue;
INT32 curbgcolor = -1;	// Please stop assaulting my eyes.
INT32 curbgxspeed;
INT32 curbgyspeed;
boolean curbghide;
boolean hidetitlemap;		// WARNING: set to false by M_SetupNextMenu and M_ClearMenus

#if 0
static UINT8  laststaff = 0;
#endif
//static UINT8  curDemo = 0;
static UINT32 demoDelayLeft;
static UINT32 demoIdleLeft;

// customizable title screen graphics

ttmode_enum ttmode = TTMODE_RINGRACERS;
UINT8 ttscale = 1; // FRACUNIT / ttscale
// ttmode user vars
char ttname[9];
INT16 ttx = 0;
INT16 tty = 0;
INT16 ttloop = -1;
UINT16 tttics = 1;

boolean curhidepics;
ttmode_enum curttmode;
UINT8 curttscale;
// ttmode user vars
char curttname[9];
INT16 curttx;
INT16 curtty;
INT16 curttloop;
UINT16 curtttics;

// ttmode old
/*
static patch_t *ttbanner; // SONIC ROBO BLAST 2
static patch_t *ttkart; // *vroom* KART
static patch_t *ttcheckers; // *vroom* KART
static patch_t *ttkflash; // flash screen
*/

static patch_t *kts_bumper; // DR ROBOTNIKS RING RACERS
static patch_t *kts_eggman; // dr. robotnik himself
static patch_t *kts_tails; // tails himself
static patch_t *kts_tails_tails; // tails' tails
static patch_t *kts_electricity[6]; // ring o' electricity
static patch_t *kts_copyright; // (C) SEGA

#define NOWAY

#ifdef NOWAY
static patch_t *driver[2]; // Driving character on the waiting screen
static UINT8 *waitcolormap; // colormap for the spinning character
#endif

// ttmode user
static patch_t *ttuser[TTMAX_USER];
static INT32 ttuser_count = 0;

static boolean goodending;
static patch_t *endbrdr[2]; // border - blue, white, pink - where have i seen those colours before?
static patch_t *endbgsp[3]; // nebula, sun, planet
static patch_t *endegrk[2]; // eggrock - replaced midway through good ending
static patch_t *endfwrk[3]; // firework - replaced with skin when good ending
static patch_t *endspkl[3]; // sparkle
static patch_t *endglow[2]; // glow aura - replaced with black rock's midway through good ending
static patch_t *endxpld[4]; // mini explosion
static patch_t *endescp[5]; // escape pod + flame
static INT32 sparkloffs[3][2]; // eggrock explosions/blackrock sparkles
static INT32 sparklloop;

//
// PROMPT STATE
//
boolean promptactive = false;
static mobj_t *promptmo;
static INT16 promptpostexectag;
static boolean promptblockcontrols;
static char *promptpagetext = NULL;
static INT32 callpromptnum = INT32_MAX;
static INT32 callpagenum = INT32_MAX;
static INT32 callplayer = INT32_MAX;

//
// CUTSCENE TEXT WRITING
//
static const char *cutscene_basetext = NULL;
static char cutscene_disptext[1024];
static INT32 cutscene_baseptr = 0;
static INT32 cutscene_writeptr = 0;
static INT32 cutscene_textcount = 0;
static INT32 cutscene_textspeed = 0;
static UINT8 cutscene_boostspeed = 0;
static tic_t cutscene_lasttextwrite = 0;

// STJR Intro
char stjrintro[9] = "STJRI000";

static huddrawlist_h luahuddrawlist_title;

//
// This alters the text string cutscene_disptext.
// Use the typical string drawing functions to display it.
// Returns 0 if \0 is reached (end of input)
//
static UINT8 F_WriteText(void)
{
	INT32 numtowrite = 1;
	const char *c;
	tic_t ltw = I_GetTime();

	if (cutscene_lasttextwrite == ltw)
		return 1; // singletics prevention
	cutscene_lasttextwrite = ltw;

	if (cutscene_boostspeed)
	{
		// for custom cutscene speedup mode
		numtowrite = 8;
	}
	else
	{
		// Don't draw any characters if the count was 1 or more when we started
		if (--cutscene_textcount >= 0)
			return 1;

		if (cutscene_textspeed < 7)
			numtowrite = 8 - cutscene_textspeed;
	}

	for (;numtowrite > 0;++cutscene_baseptr)
	{
		c = &cutscene_basetext[cutscene_baseptr];
		if (!c || !*c || *c=='#')
			return 0;

		// \xA0 - \xAF = change text speed
		if ((UINT8)*c >= 0xA0 && (UINT8)*c <= 0xAF)
		{
			cutscene_textspeed = (INT32)((UINT8)*c - 0xA0);
			continue;
		}
		// \xB0 - \xD2 = delay character for up to one second (35 tics)
		else if ((UINT8)*c >= 0xB0 && (UINT8)*c <= (0xB0+TICRATE-1))
		{
			cutscene_textcount = (INT32)((UINT8)*c - 0xAF);
			numtowrite = 0;
			continue;
		}

		cutscene_disptext[cutscene_writeptr++] = *c;

		// Ignore other control codes (color)
		if ((UINT8)*c < 0x80)
			--numtowrite;
	}
	// Reset textcount for next tic based on speed
	// if it wasn't already set by a delay.
	if (cutscene_textcount < 0)
	{
		cutscene_textcount = 0;
		if (cutscene_textspeed > 7)
			cutscene_textcount = cutscene_textspeed - 7;
	}
	return 1;
}

static void F_NewCutscene(const char *basetext)
{
	cutscene_basetext = basetext;
	memset(cutscene_disptext,0,sizeof(cutscene_disptext));
	cutscene_writeptr = cutscene_baseptr = 0;
	cutscene_textspeed = 9;
	cutscene_textcount = TICRATE/2;
}

//
// F_TitleBGScroll
//
/*
static void F_TitleBGScroll(INT32 scrollspeed)
{
	INT32 x, y, w;
	patch_t *pat, *pat2;
	INT32 anim2 = 0;

	pat = W_CachePatchName("TITLEBG1", PU_CACHE);
	pat2 = W_CachePatchName("TITLEBG2", PU_CACHE);

	w = vid.width / vid.dupx;

	animtimer = ((finalecount*scrollspeed)/16) % SHORT(pat->width);
	anim2 = SHORT(pat2->width) - (((finalecount*scrollspeed)/16) % SHORT(pat2->width));

	// SRB2Kart: F_DrawPatchCol is over-engineered; recoded to be less shitty and error-prone
	if (rendermode != render_none)
	{
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);

		x = -((INT32)animtimer);
		y = 0;
		while (x < w)
		{
			V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, V_SNAPTOTOP|V_SNAPTOLEFT, pat, NULL);
			x += SHORT(pat->width);
		}

		x = -anim2;
		y = BASEVIDHEIGHT - SHORT(pat2->height);
		while (x < w)
		{
			V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTOLEFT, pat2, NULL);
			x += SHORT(pat2->width);
		}
	}

	W_UnlockCachedPatch(pat);
	W_UnlockCachedPatch(pat2);
}
*/

// =============
//  INTRO SCENE
// =============
#define NUMINTROSCENES 1
INT32 intro_scenenum = 0;
INT32 intro_curtime = 0;

const char *introtext[NUMINTROSCENES];

static tic_t introscenetime[NUMINTROSCENES] =
{
	 4*TICRATE,	// KART KR(eW
};

// custom intros
void F_StartCustomCutscene(INT32 cutscenenum, boolean precutscene, boolean resetplayer);

void F_StartIntro(void)
{
	if (gamestate)
	{
		F_WipeStartScreen();
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
		F_WipeEndScreen();
		F_RunWipe(wipedefs[wipe_intro_toblack], false, "FADEMAP0", false, false);
	}

	S_StopMusic();
	S_StopSounds();

	if (introtoplay)
	{
		if (!cutscenes[introtoplay - 1])
			D_StartTitle();
		else
			F_StartCustomCutscene(introtoplay - 1, false, false);
		return;
	}

	introtext[0] = " #";

	G_SetGamestate(GS_INTRO);
	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();
	F_NewCutscene(introtext[0]);

	intro_scenenum = 0;
	finalecount = animtimer = skullAnimCounter = stoptimer = 0;
	timetonext = introscenetime[intro_scenenum];
	S_StopMusic();
}

//
// F_IntroDrawScene
//
static void F_IntroDrawScene(void)
{
	boolean highres = true;
	INT32 cx = 8, cy = 128;
	patch_t *background = NULL;
	INT32 bgxoffs = 0;

	// DRAW A FULL PIC INSTEAD OF FLAT!
	if (intro_scenenum == 0)
	{
		background = W_CachePatchName("KARTKREW", PU_CACHE);
		highres = true;
	}

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);

	if (background)
	{
		if (highres)
			V_DrawSmallScaledPatch(bgxoffs, 0, 0, background);
		else
			V_DrawScaledPatch(bgxoffs, 0, 0, background);
	}

	W_UnlockCachedPatch(background);

	V_DrawString(cx, cy, 0, cutscene_disptext);
}

//
// F_IntroDrawer
//
void F_IntroDrawer(void)
{
	// Used to be this whole thing, but now...
	F_IntroDrawScene();
}

//
// F_IntroTicker
//
void F_IntroTicker(void)
{
	// advance animation
	finalecount++;

	timetonext--;

	if (intro_scenenum == 0)
	{
		if (timetonext <= 0)
		{
#if 0 // The necessary apparatus for constructing more elaborate intros...
			intro_scenenum++;
			F_NewCutscene(introtext[intro_scenenum]);
			timetonext = introscenetime[intro_scenenum];
			wipegamestate = -1;
			animtimer = stoptimer = 0;
#endif
			if (rendermode != render_none)
			{
				F_WipeStartScreen();
				F_WipeColorFill(31);
				F_WipeEndScreen();
				F_RunWipe(99, true, "FADEMAP0", false, false);
			}

			// Stay on black for a bit. =)
			{
				tic_t nowtime, quittime, lasttime;
				nowtime = lasttime = I_GetTime();
				quittime = nowtime + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds
				while (quittime > nowtime)
				{
					while (!((nowtime = I_GetTime()) - lasttime))
					{
						I_Sleep(cv_sleep.value);
						I_UpdateTime(cv_timescale.value);
					}
					lasttime = nowtime;

					I_OsPolling();
					I_UpdateNoBlit();
#ifdef HAVE_THREADS
					I_lock_mutex(&k_menu_mutex);
#endif
					M_Drawer(); // menu is drawn even on top of wipes
#ifdef HAVE_THREADS
					I_unlock_mutex(k_menu_mutex);
#endif
					I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001

#ifdef HWRENDER
					if (moviemode && rendermode == render_opengl) // make sure we save frames for the white hold too
						M_LegacySaveFrame();
#endif
				}
			}

			D_StartTitle();
			return;
		}
		if (finalecount == 8)
			S_StartSound(NULL, sfx_vroom);
		else if (finalecount == 47)
		{
			// Need to use M_Random otherwise it always uses the same sound
			INT32 rskin = M_RandomKey(numskins);
			UINT8 rtaunt = M_RandomKey(2);
			sfxenum_t rsound = skins[rskin].soundsid[SKSKBST1+rtaunt];
			S_StartSound(NULL, rsound);
		}
	}

	intro_curtime = introscenetime[intro_scenenum] - timetonext;

	F_WriteText();

	// check for skipping
	if (keypressed)
		keypressed = false;

	if (animtimer > 0)
		animtimer--;
}

//
// F_IntroResponder
//
boolean F_IntroResponder(event_t *event)
{
	INT32 key = event->data1;

	// remap virtual keys (mouse & joystick buttons)
	switch (key)
	{
		case KEY_MOUSE1:
			key = KEY_ENTER;
			break;
		case KEY_MOUSE1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_JOY1:
		case KEY_JOY1 + 2:
			key = KEY_ENTER;
			break;
		case KEY_JOY1 + 3:
			key = 'n';
			break;
		case KEY_JOY1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_HAT1:
			key = KEY_UPARROW;
			break;
		case KEY_HAT1 + 1:
			key = KEY_DOWNARROW;
			break;
		case KEY_HAT1 + 2:
			key = KEY_LEFTARROW;
			break;
		case KEY_HAT1 + 3:
			key = KEY_RIGHTARROW;
			break;
	}

	if (event->type != ev_keydown && key != 301)
		return false;

	if (key != 27 && key != KEY_ENTER && key != KEY_SPACE && key != KEY_BACKSPACE)
		return false;

	if (keypressed)
		return false;

	keypressed = true;
	return true;
}

// =========
//  CREDITS
// =========
static const char *credits[] = {
	"\1Dr. Robotnik's Ring Racers",
	"\1Credits",
	"",
	"\1Game Design",
	"Sally \"TehRealSalt\" Cochenour",
	"Jeffery \"Chromatian\" Scott",
	"\"VelocitOni\"",
	"",
	"\1Lead Programming",
	"Sally \"TehRealSalt\" Cochenour",
	"Vivian \"toaster\" Grannell",
	"Sean \"Sryder\" Ryder",
	"Ehab \"wolfs\" Saeed",
	"\"ZarroTsu\"",
	"",
	"\1Support Programming",
	"Colette \"fickleheart\" Bordelon",
	"James R.",
	"\"Lat\'\"",
	"\"Monster Iestyn\"",
	"\"Shuffle\"",
	"\"SteelT\"",
	"",
	"\1Lead Artists",
	"Desmond \"Blade\" DesJardins",
	"\"VelocitOni\"",
	"",
	"\1Support Artists",
	"Sally \"TehRealSalt\" Cochenour",
	"Sherman \"CoatRack\" DesJardins",
	"\"DrTapeworm\"",
	"Jesse \"Jeck Jims\" Emerick",
	"Wesley \"Charyb\" Gillebaard",
	"Vivian \"toaster\" Grannell",
	"James \"SeventhSentinel\" Hall",
	"\"Lat\'\"",
	"\"Tyrannosaur Chao\"",
	"\"ZarroTsu\"",
	"",
	"\1External Artists",
	"\"1-Up Mason\"",
	"\"Chengi\"",
	"\"Chrispy\"",
	"\"DirkTheHusky\"",
	"\"LJSTAR\"",
	"\"MotorRoach\"",
	"\"Nev3r\"",
	"\"rairai104n\"",
	"\"Ritz\"",
	"\"Rob\"",
	"\"SmithyGNC\"",
	"\"Snu\"",
	"\"Spherallic\"",
	"\"TelosTurntable\"",
	"\"VAdaPEGA\"",
	"\"Virt\"",
	"\"Voltrix\"",
	"",
	"\1Sound Design",
	"James \"SeventhSentinel\" Hall",
	"Sonic Team",
	"\"VAdaPEGA\"",
	"\"VelocitOni\"",
	"",
	"\1Music",
	"\"DrTapeworm\"",
	"Wesley \"Charyb\" Gillebaard",
	"James \"SeventhSentinel\" Hall",
	"",
	"\1Lead Level Design",
	"\"Blitz-T\"",
	"Sally \"TehRealSalt\" Cochenour",
	"Desmond \"Blade\" DesJardins",
	"Jeffery \"Chromatian\" Scott",
	"\"Tyrannosaur Chao\"",
	"",
	"\1Support Level Design",
	"\"Chaos Zero 64\"",
	"\"D00D64\"",
	"\"DrTapeworm\"",
	"Paul \"Boinciel\" Clempson",
	"Sherman \"CoatRack\" DesJardins",
	"Colette \"fickleheart\" Bordelon",
	"Vivian \"toaster\" Grannell",
	"\"Gunla\"",
	"James \"SeventhSentinel\" Hall",
	"\"Lat\'\"",
	"\"MK\"",
	"\"Ninferno\"",
	"Sean \"Sryder\" Ryder",
	"\"Ryuspark\"",
	"\"Simsmagic\"",
	"\"SP47\"",
	"\"TG\"",
	"\"Victor Rush Turbo\"",
	"\"ZarroTsu\"",
	"",
	"\1Testing",
	"\"CyberIF\"",
	"\"Dani\"",
	"Karol \"Fooruman\" D""\x1E""browski", // Dąbrowski, <Sryder> accents in srb2 :ytho:
	"\"VirtAnderson\"",
	"",
	"\1Special Thanks",
	"SEGA",
	"Sonic Team",
	"SRB2 & Sonic Team Jr. (www.srb2.org)",
	"\"Chaos Zero 64\"",
	"",
	"\1Produced By",
	"Kart Krew",
	"",
	"\1In Memory of",
	"\"Tyler52\"",
	"",
	"",
	"\1Thank you       ",
	"\1for playing!       ",
	NULL
};

#define CREDITS_LEFT 8
#define CREDITS_RIGHT ((BASEVIDWIDTH) - 8)

static struct {
	UINT32 x, y;
	const char *patch;
	UINT8 colorize;
} credits_pics[] = {
	// We don't have time to be fancy, let's just colorize some item sprites :V
	{224, 80+(200* 1), "K_ITJAWZ", SKINCOLOR_CREAMSICLE},
	{224, 80+(200* 2), "K_ITSPB",  SKINCOLOR_GARDEN},
	{224, 80+(200* 3), "K_ITBANA", SKINCOLOR_LILAC},
	{224, 80+(200* 4), "K_ITHYUD", SKINCOLOR_DREAM},
	{224, 80+(200* 5), "K_ITBHOG", SKINCOLOR_TANGERINE},
	{224, 80+(200* 6), "K_ITSHRK", SKINCOLOR_JAWZ},
	{224, 80+(200* 7), "K_ITSHOE", SKINCOLOR_MINT},
	{224, 80+(200* 8), "K_ITGROW", SKINCOLOR_RUBY},
	{224, 80+(200* 9), "K_ITPOGO", SKINCOLOR_SAPPHIRE},
	{224, 80+(200*10), "K_ITRSHE", SKINCOLOR_YELLOW},
	{224, 80+(200*11), "K_ITORB4", SKINCOLOR_DUSK},
	{224, 80+(200*12), "K_ITEGGM", SKINCOLOR_GREEN},
	{224, 80+(200*13), "K_ITMINE", SKINCOLOR_BRONZE},
	{224, 80+(200*14), "K_ITTHNS", SKINCOLOR_RASPBERRY},
	{224, 80+(200*15), "K_ITINV1", SKINCOLOR_GREY},
	// This Tyler52 gag is troublesome
	// Alignment should be ((spaces+1 * 100) + (headers+1 * 38) + (lines * 15))
	// Current max image spacing: (200*17)
	{112, (15*100)+(17*38)+(88*15), "TYLER52", SKINCOLOR_NONE}
};

#undef CREDITS_LEFT
#undef CREDITS_RIGHT

static UINT32 credits_height = 0;
static const UINT8 credits_numpics = sizeof(credits_pics)/sizeof(credits_pics[0]) - 1;

void F_StartCredits(void)
{
	G_SetGamestate(GS_CREDITS);

	// Just in case they're open ... somehow
	M_ClearMenus(true);

	if (creditscutscene)
	{
		F_StartCustomCutscene(creditscutscene - 1, false, false);
		return;
	}

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();
	S_StopMusic();
	S_StopSounds();

	S_ChangeMusicInternal("_creds", false);
	S_ShowMusicCredit();

	finalecount = 0;
	animtimer = 0;
	timetonext = 2*TICRATE;
}

void F_CreditDrawer(void)
{
	UINT16 i;
	fixed_t y = (80<<FRACBITS) - (animtimer<<FRACBITS>>1);

	//V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	// Draw background
	V_DrawSciencePatch(0, 0 - FixedMul(32<<FRACBITS, FixedDiv(credbgtimer%TICRATE, TICRATE)), V_SNAPTOTOP, W_CachePatchName("CREDTILE", PU_CACHE), FRACUNIT);

	V_DrawSciencePatch(0, 0 - FixedMul(40<<FRACBITS, FixedDiv(credbgtimer%(TICRATE/2), (TICRATE/2))), V_SNAPTOTOP, W_CachePatchName("CREDZIGZ", PU_CACHE), FRACUNIT);
	V_DrawSciencePatch(320<<FRACBITS, 0 - FixedMul(40<<FRACBITS, FixedDiv(credbgtimer%(TICRATE/2), (TICRATE/2))), V_SNAPTOTOP|V_FLIP, W_CachePatchName("CREDZIGZ", PU_CACHE), FRACUNIT);

	// Draw pictures
	for (i = 0; i < credits_numpics; i++)
	{
		UINT8 *colormap = NULL;
		fixed_t sc = FRACUNIT>>1;

		if (credits_pics[i].colorize != SKINCOLOR_NONE)
		{
			colormap = R_GetTranslationColormap(TC_RAINBOW, credits_pics[i].colorize, GTC_MENUCACHE);
			sc = FRACUNIT; // quick hack so I don't have to add another field to credits_pics
		}

		V_DrawFixedPatch(credits_pics[i].x<<FRACBITS, (credits_pics[i].y<<FRACBITS) - 4*(animtimer<<FRACBITS)/5, sc, 0, W_CachePatchName(credits_pics[i].patch, PU_CACHE), colormap);
	}

	// Draw credits text on top
	for (i = 0; credits[i]; i++)
	{
		switch(credits[i][0])
		{
		case 0:
			y += 80<<FRACBITS;
			break;
		case 1:
			if (y>>FRACBITS > -20)
				V_DrawCreditString((160 - (V_CreditStringWidth(&credits[i][1])>>1))<<FRACBITS, y, 0, &credits[i][1]);
			y += 30<<FRACBITS;
			break;
		case 2:
			if (y>>FRACBITS > -10)
				V_DrawStringAtFixed((BASEVIDWIDTH-V_StringWidth(&credits[i][1], V_ALLOWLOWERCASE|V_YELLOWMAP))<<FRACBITS>>1, y, V_ALLOWLOWERCASE|V_YELLOWMAP, &credits[i][1]);
			y += 12<<FRACBITS;
			break;
		default:
			if (y>>FRACBITS > -10)
				V_DrawStringAtFixed(32<<FRACBITS, y, V_ALLOWLOWERCASE, credits[i]);
			y += 12<<FRACBITS;
			break;
		}
		if (((y>>FRACBITS) * vid.dupy) > vid.height)
			break;
	}
}

void F_CreditTicker(void)
{
	// "Simulate" the drawing of the credits so that dedicated mode doesn't get stuck
	UINT16 i;
	fixed_t y = (80<<FRACBITS) - (animtimer<<FRACBITS>>1);

	// Calculate credits height to display art properly
	if (credits_height == 0)
	{
		for (i = 0; credits[i]; i++)
		{
			switch(credits[i][0])
			{
				case 0: credits_height += 80; break;
				case 1: credits_height += 30; break;
				default: credits_height += 12; break;
			}
		}
		credits_height = 131*credits_height/80; // account for scroll speeds. This is a guess now, so you may need to update this if you change the credits length.
	}

	// Draw credits text on top
	for (i = 0; credits[i]; i++)
	{
		switch(credits[i][0])
		{
			case 0: y += 80<<FRACBITS; break;
			case 1: y += 30<<FRACBITS; break;
			default: y += 12<<FRACBITS; break;
		}
		if (FixedMul(y,vid.dupy) > vid.height)
			break;
	}

	// Do this here rather than in the drawer you doofus! (this is why dedicated mode broke at credits)
	if (!credits[i] && y <= 120<<FRACBITS && !finalecount)
	{
		timetonext = 5*TICRATE+1;
		finalecount = 5*TICRATE;
	}

	if (timetonext)
		timetonext--;
	else
		animtimer++;

	credbgtimer++;

	if (finalecount && --finalecount == 0)
		F_StartGameEvaluation();
}

boolean F_CreditResponder(event_t *event)
{
	INT32 key = event->data1;

	// remap virtual keys (mouse & joystick buttons)
	switch (key)
	{
		case KEY_MOUSE1:
			key = KEY_ENTER;
			break;
		case KEY_MOUSE1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_JOY1:
		case KEY_JOY1 + 2:
			key = KEY_ENTER;
			break;
		case KEY_JOY1 + 3:
			key = 'n';
			break;
		case KEY_JOY1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_HAT1:
			key = KEY_UPARROW;
			break;
		case KEY_HAT1 + 1:
			key = KEY_DOWNARROW;
			break;
		case KEY_HAT1 + 2:
			key = KEY_LEFTARROW;
			break;
		case KEY_HAT1 + 3:
			key = KEY_RIGHTARROW;
			break;
	}

	if (event->type != ev_keydown)
		return false;

	if (key == KEY_DOWNARROW || key == KEY_SPACE)
	{
		if (!timetonext && !finalecount)
			animtimer += 7;
		return false;
	}

	/*if (!(gamedata->timesBeaten) && !(netgame || multiplayer) && !cht_debug)
		return false;*/

	if (key != KEY_ESCAPE && key != KEY_ENTER && key != KEY_BACKSPACE)
		return false;

	if (keypressed)
		return true;

	keypressed = true;
	return true;
}

// ============
//  EVALUATION
// ============

#define SPARKLLOOPTIME 7 // must be odd

void F_StartGameEvaluation(void)
{
	// Credits option in extras menu
	if (cursaveslot == -1)
	{
		S_FadeOutStopMusic(2*MUSICRATE);
		F_StartGameEnd();
		return;
	}

	S_FadeOutStopMusic(5*MUSICRATE);

	G_SetGamestate(GS_EVALUATION);

	// Just in case they're open ... somehow
	M_ClearMenus(true);

	goodending = (ALLCHAOSEMERALDS(emeralds));

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();

	finalecount = -1;
	sparklloop = 0;
}

void F_GameEvaluationDrawer(void)
{
	INT32 x, y, i;
	angle_t fa;
	INT32 eemeralds_cur;
	char patchname[7] = "CEMGx0";
	const char* endingtext;

	if (marathonmode)
		endingtext = "THANKS FOR THE RUN!";
	else if (goodending)
		endingtext = "CONGRATULATIONS!";
	else
		endingtext = "TRY AGAIN...";

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	// Draw all the good crap here.

	if (finalecount > 0 && useBlackRock)
	{
		INT32 scale = FRACUNIT;
		patch_t *rockpat;
		UINT8 *colormap[2] = {NULL, NULL};
		patch_t *glow;
		INT32 trans = 0;

		x = (((BASEVIDWIDTH-82)/2)+11)<<FRACBITS;
		y = (((BASEVIDHEIGHT-82)/2)+12)<<FRACBITS;

		if (finalecount < 5)
		{
			scale = (finalecount<<(FRACBITS-2));
			x += (30*(FRACUNIT-scale));
			y += (30*(FRACUNIT-scale));
		}

		if (goodending)
		{
			rockpat = W_CachePatchName(va("ROID00%.2d", 34 - (finalecount % 35)), PU_PATCH_LOWPRIORITY);
			glow = W_CachePatchName(va("ENDGLOW%.1d", 2+(finalecount & 1)), PU_PATCH_LOWPRIORITY);
			x -= FRACUNIT;
		}
		else
		{
			rockpat = W_CachePatchName("ROID0000", PU_PATCH_LOWPRIORITY);
			glow = W_CachePatchName(va("ENDGLOW%.1d", (finalecount & 1)), PU_PATCH_LOWPRIORITY);
		}

		if (finalecount >= 5)
			trans = (finalecount-5)>>1;
		if (trans < 10)
			V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, glow, NULL);

		trans = (15-finalecount);
		if (trans < 0)
			trans = -trans;

		if (finalecount < 15)
			colormap[0] = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
		V_DrawFixedPatch(x, y, scale, 0, rockpat, colormap[0]);
		if (trans < 10)
		{
			colormap[1] = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_AQUAMARINE, GTC_CACHE);
			V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, rockpat, colormap[1]);
		}
		if (goodending)
		{
			INT32 j = (sparklloop & 1) ? 2 : 3;
			if (j > (finalecount/SPARKLLOOPTIME))
				j = (finalecount/SPARKLLOOPTIME);
			while (j)
			{
				if (j > 1 || sparklloop >= 2)
				{
					// if j == 0 - alternate between 0 and 1
					//         1 -                   1 and 2
					//         2 -                   2 and not rendered
					V_DrawFixedPatch(x+sparkloffs[j-1][0], y+sparkloffs[j-1][1], FRACUNIT, 0,
						W_CachePatchName(va("ENDSPKL%.1d", (j - ((sparklloop & 1) ? 0 : 1))), PU_PATCH),
						R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_AQUAMARINE, GTC_CACHE));
				}
				j--;
			}
		}
		else
		{
			patch_t *eggrock = W_CachePatchName("ENDEGRK5", PU_PATCH_LOWPRIORITY);
			V_DrawFixedPatch(x, y, scale, 0, eggrock, colormap[0]);
			if (trans < 10)
				V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, eggrock, colormap[1]);
			else if (sparklloop)
				V_DrawFixedPatch(x, y, scale, (10-sparklloop)<<V_ALPHASHIFT,
					W_CachePatchName("ENDEGRK0", PU_PATCH_LOWPRIORITY), colormap[1]);
		}
	}

	eemeralds_cur = (finalecount % 360)<<FRACBITS;

	for (i = 0; i < 7; ++i)
	{
		fa = (FixedAngle(eemeralds_cur)>>ANGLETOFINESHIFT) & FINEMASK;
		x = (BASEVIDWIDTH<<(FRACBITS-1)) + (60*FINECOSINE(fa));
		y = ((BASEVIDHEIGHT+16)<<(FRACBITS-1)) + (60*FINESINE(fa));
		eemeralds_cur += (360<<FRACBITS)/7;

		patchname[4] = 'A'+(char)i;
		V_DrawFixedPatch(x, y, FRACUNIT, ((emeralds & (1<<i)) ? 0 : V_80TRANS), W_CachePatchName(patchname, PU_PATCH_LOWPRIORITY), NULL);
	}

	V_DrawCreditString((BASEVIDWIDTH - V_CreditStringWidth(endingtext))<<(FRACBITS-1), (BASEVIDHEIGHT-100)<<(FRACBITS-1), 0, endingtext);

	if (marathonmode)
	{
		const char *rtatext, *cuttext;
		rtatext = (marathonmode & MA_INGAME) ? "In-game timer" : "RTA timer";
		cuttext = (marathonmode & MA_NOCUTSCENES) ? "" : " w/ cutscenes";
		endingtext = va("%s, %s%s", skins[players[consoleplayer].skin].realname, rtatext, cuttext);
		V_DrawCenteredString(BASEVIDWIDTH/2, 182, V_SNAPTOBOTTOM|(ultimatemode ? V_REDMAP : V_YELLOWMAP), endingtext);
	}
}

void F_GameEvaluationTicker(void)
{
	if (++finalecount > 10*TICRATE)
	{
		F_StartGameEnd();
		return;
	}

	if (!useBlackRock)
		;
	else if (!goodending)
	{
		if (sparklloop)
			sparklloop--;

		if (finalecount == (5*TICRATE)/2
			|| finalecount == (7*TICRATE)/2
			|| finalecount == ((7*TICRATE)/2)+5)
		{
			S_StartSound(NULL, sfx_s3k5c);
			sparklloop = 10;
		}
	}
	else if (++sparklloop == SPARKLLOOPTIME) // time to roll the randomisation again
	{
		angle_t workingangle = FixedAngle((M_RandomKey(360))<<FRACBITS)>>ANGLETOFINESHIFT;
		fixed_t workingradius = M_RandomKey(26);

		sparkloffs[2][0] = sparkloffs[1][0];
		sparkloffs[2][1] = sparkloffs[1][1];
		sparkloffs[1][0] = sparkloffs[0][0];
		sparkloffs[1][1] = sparkloffs[0][1];

		sparkloffs[0][0] = (30<<FRACBITS) + workingradius*FINECOSINE(workingangle);
		sparkloffs[0][1] = (30<<FRACBITS) + workingradius*FINESINE(workingangle);
		sparklloop = 0;
	}

	if (finalecount == 5*TICRATE)
	{
		if (!usedCheats)
		{
			++gamedata->timesBeaten;

			M_UpdateUnlockablesAndExtraEmblems(true);
			G_SaveGameData();
		}
		else
		{
			HU_SetCEchoFlags(V_YELLOWMAP);
			HU_SetCEchoDuration(6);
			HU_DoCEcho("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\Cheated games can't unlock extras!");
			S_StartSound(NULL, sfx_s3k68);
		}
	}
}

#undef SPARKLLOOPTIME

// ==========
//   ENDING
// ==========

#define INFLECTIONPOINT (6*TICRATE)
#define STOPPINGPOINT (14*TICRATE)
#define SPARKLLOOPTIME 15 // must be odd

static void F_CacheEnding(void)
{
	endbrdr[1] = W_CachePatchName("ENDBRDR1", PU_PATCH_LOWPRIORITY);

	endegrk[0] = W_CachePatchName("ENDEGRK0", PU_PATCH_LOWPRIORITY);
	endegrk[1] = W_CachePatchName("ENDEGRK1", PU_PATCH_LOWPRIORITY);

	endglow[0] = W_CachePatchName("ENDGLOW0", PU_PATCH_LOWPRIORITY);
	endglow[1] = W_CachePatchName("ENDGLOW1", PU_PATCH_LOWPRIORITY);

	endbgsp[0] = W_CachePatchName("ENDBGSP0", PU_PATCH_LOWPRIORITY);
	endbgsp[1] = W_CachePatchName("ENDBGSP1", PU_PATCH_LOWPRIORITY);
	endbgsp[2] = W_CachePatchName("ENDBGSP2", PU_PATCH_LOWPRIORITY);

	endspkl[0] = W_CachePatchName("ENDSPKL0", PU_PATCH_LOWPRIORITY);
	endspkl[1] = W_CachePatchName("ENDSPKL1", PU_PATCH_LOWPRIORITY);
	endspkl[2] = W_CachePatchName("ENDSPKL2", PU_PATCH_LOWPRIORITY);

	endxpld[0] = W_CachePatchName("ENDXPLD0", PU_PATCH_LOWPRIORITY);
	endxpld[1] = W_CachePatchName("ENDXPLD1", PU_PATCH_LOWPRIORITY);
	endxpld[2] = W_CachePatchName("ENDXPLD2", PU_PATCH_LOWPRIORITY);
	endxpld[3] = W_CachePatchName("ENDXPLD3", PU_PATCH_LOWPRIORITY);

	endescp[0] = W_CachePatchName("ENDESCP0", PU_PATCH_LOWPRIORITY);
	endescp[1] = W_CachePatchName("ENDESCP1", PU_PATCH_LOWPRIORITY);
	endescp[2] = W_CachePatchName("ENDESCP2", PU_PATCH_LOWPRIORITY);
	endescp[3] = W_CachePatchName("ENDESCP3", PU_PATCH_LOWPRIORITY);
	endescp[4] = W_CachePatchName("ENDESCP4", PU_PATCH_LOWPRIORITY);

	// so we only need to check once
	if ((goodending = ALLCHAOSEMERALDS(emeralds)))
	{
		endfwrk[0] = W_CachePatchName("ENDFWRK3", PU_PATCH);
		endfwrk[1] = W_CachePatchName("ENDFWRK4", PU_PATCH);
		endfwrk[2] = W_CachePatchName("ENDFWRK5", PU_PATCH);

		endbrdr[0] = W_CachePatchName("ENDBRDR2", PU_PATCH_LOWPRIORITY);
	}
	else
	{
		// eggman, skin nonspecific
		endfwrk[0] = W_CachePatchName("ENDFWRK0", PU_PATCH_LOWPRIORITY);
		endfwrk[1] = W_CachePatchName("ENDFWRK1", PU_PATCH_LOWPRIORITY);
		endfwrk[2] = W_CachePatchName("ENDFWRK2", PU_PATCH_LOWPRIORITY);

		endbrdr[0] = W_CachePatchName("ENDBRDR0", PU_PATCH_LOWPRIORITY);
	}
}

static void F_CacheGoodEnding(void)
{
	endegrk[0] = W_CachePatchName("ENDEGRK2", PU_PATCH_LOWPRIORITY);
	endegrk[1] = W_CachePatchName("ENDEGRK3", PU_PATCH_LOWPRIORITY);

	endglow[0] = W_CachePatchName("ENDGLOW2", PU_PATCH_LOWPRIORITY);
	endglow[1] = W_CachePatchName("ENDGLOW3", PU_PATCH_LOWPRIORITY);

	endxpld[0] = W_CachePatchName("ENDEGRK4", PU_PATCH_LOWPRIORITY);
}

void F_StartEnding(void)
{
	G_SetGamestate(GS_ENDING);
	wipetypepost = INT16_MAX;

	// Just in case they're open ... somehow
	M_ClearMenus(true);

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();
	S_StopMusic(); // todo: placeholder
	S_StopSounds();

	finalecount = -10; // what? this totally isn't a hack. why are you asking?

	memset(sparkloffs, 0, sizeof(INT32)*3*2);
	sparklloop = 0;

	F_CacheEnding();
}

void F_EndingTicker(void)
{
	if (++finalecount > STOPPINGPOINT)
	{
		F_StartCredits();
		wipetypepre = INT16_MAX;
		return;
	}

	if (finalecount == -8)
		S_ChangeMusicInternal((goodending ? "_endg" : "_endb"), false);

	if (goodending && finalecount == INFLECTIONPOINT) // time to swap some assets
		F_CacheGoodEnding();

	if (++sparklloop == SPARKLLOOPTIME) // time to roll the randomisation again
	{
		angle_t workingangle = FixedAngle((M_RandomRange(-170, 80))<<FRACBITS)>>ANGLETOFINESHIFT;
		fixed_t workingradius = M_RandomKey(26);

		sparkloffs[0][0] = (30<<FRACBITS) + workingradius*FINECOSINE(workingangle);
		sparkloffs[0][1] = (30<<FRACBITS) + workingradius*FINESINE(workingangle);

		sparklloop = 0;
	}
}

void F_EndingDrawer(void)
{
	INT32 x, y, i, j, parallaxticker;
	patch_t *rockpat;

	if (!goodending || finalecount < INFLECTIONPOINT)
		rockpat = W_CachePatchName("ROID0000", PU_PATCH_LOWPRIORITY);
	else
		rockpat = W_CachePatchName(va("ROID00%.2d", 34 - ((finalecount - INFLECTIONPOINT) % 35)), PU_PATCH_LOWPRIORITY);

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	parallaxticker = finalecount - INFLECTIONPOINT;
	x = -((parallaxticker*20)<<FRACBITS)/INFLECTIONPOINT;
	y = ((parallaxticker*7)<<FRACBITS)/INFLECTIONPOINT;
	i = (((BASEVIDWIDTH-82)/2)+11)<<FRACBITS;
	j = (((BASEVIDHEIGHT-82)/2)+12)<<FRACBITS;

	if (finalecount <= -10)
		;
	else if (finalecount < 0)
		V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 0, 10+finalecount);
	else if (finalecount <= 20)
	{
		V_DrawFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0);
		if (finalecount && finalecount < 20)
		{
			INT32 trans = (10-finalecount);
			if (trans < 0)
			{
				trans = -trans;
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[0]);
			}
			V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, trans<<V_ALPHASHIFT, endbrdr[1]);
		}
		else if (finalecount == 20)
			V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[0]);
	}
	else if (goodending && (parallaxticker == -2 || !parallaxticker))
	{
		V_DrawFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0);
		V_DrawFixedPatch(x+i, y+j, FRACUNIT, 0, endegrk[0],
			R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_CACHE));
		//V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[1]);
	}
	else if (goodending && parallaxticker == -1)
	{
		V_DrawFixedPatch(x+i, y+j, FRACUNIT, 0, rockpat,
			R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE));
		V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[1]);
	}
	else
	{
		boolean doexplosions = false;
		boolean borderstuff = false;
		INT32 tweakx = 0, tweaky = 0;

		if (parallaxticker < 75) // f background's supposed to be visible
		{
			V_DrawFixedPatch(-(x/10), -(y/10), FRACUNIT, 0, endbgsp[0], NULL); // nebula
			V_DrawFixedPatch(-(x/5),  -(y/5),  FRACUNIT, 0, endbgsp[1], NULL); // sun
			V_DrawFixedPatch(     0,  -(y/2),  FRACUNIT, 0, endbgsp[2], NULL); // planet

			// player's escape pod
			V_DrawFixedPatch((200<<FRACBITS)+(finalecount<<(FRACBITS-2)),
				(100<<FRACBITS)+(finalecount<<(FRACBITS-2)),
				FRACUNIT, 0, endescp[4], NULL);
			if (parallaxticker > -19)
			{
				INT32 trans = (-parallaxticker)>>1;
				if (trans < 0)
					trans = 0;
				V_DrawFixedPatch((200<<FRACBITS)+(finalecount<<(FRACBITS-2)),
					(100<<FRACBITS)+(finalecount<<(FRACBITS-2)),
					FRACUNIT, trans<<V_ALPHASHIFT, endescp[(finalecount/2)&3], NULL);
			}

			if (goodending && parallaxticker > 0) // gunchedrock
			{
				INT32 scale = FRACUNIT + ((parallaxticker-10)<<7);
				INT32 trans = parallaxticker>>2;
				UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_JET, GTC_CACHE);

				if (parallaxticker < 10)
				{
					tweakx = parallaxticker<<FRACBITS;
					tweaky = ((7*parallaxticker)<<(FRACBITS-2))/5;
				}
				else
				{
					tweakx = 10<<FRACBITS;
					tweaky = 7<<(FRACBITS-1);
				}
				i += tweakx;
				j -= tweaky;

				x <<= 1;
				y <<= 1;

				// center detritrus
				V_DrawFixedPatch(i-x, j-y, FRACUNIT, 0, endegrk[0], colormap);
				if (trans < 10)
					V_DrawFixedPatch(i-x, j-y, FRACUNIT, trans<<V_ALPHASHIFT, endegrk[0], NULL);

				 // ring detritrus
				V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(2*x), (30*(FRACUNIT-scale))+j-(2*y) - ((7<<FRACBITS)/2), scale, 0, endegrk[1], colormap);
				if (trans < 10)
					V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(2*x), (30*(FRACUNIT-scale))+j-(2*y), scale, trans<<V_ALPHASHIFT, endegrk[1], NULL);

				scale += ((parallaxticker-10)<<7);

				 // shard detritrus
				V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(x/2), (30*(FRACUNIT-scale))+j-(y/2) - ((7<<FRACBITS)/2), scale, 0, endxpld[0], colormap);
				if (trans < 10)
					V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(x/2), (30*(FRACUNIT-scale))+j-(y/2), scale, trans<<V_ALPHASHIFT, endxpld[0], NULL);
			}
		}
		else if (goodending)
		{
			tweakx = 10<<FRACBITS;
			tweaky = 7<<(FRACBITS-1);
			i += tweakx;
			j += tweaky;
			x <<= 1;
			y <<= 1;
		}

		if (goodending && parallaxticker > 0)
		{
			i -= (3+(tweakx<<1));
			j += tweaky<<2;
		}

		if (parallaxticker <= 70) // eggrock/blackrock
		{
			INT32 trans;
			fixed_t scale = FRACUNIT;
			UINT8 *colormap[2] = {NULL, NULL};

			x += i;
			y += j;

			if (parallaxticker > 66)
			{
				scale = ((70 - parallaxticker)<<(FRACBITS-2));
				x += (30*(FRACUNIT-scale));
				y += (30*(FRACUNIT-scale));
			}
			else if ((parallaxticker > 60) || (goodending && parallaxticker > 0))
				;
			else
			{
				doexplosions = true;
				if (!sparklloop)
				{
					x += ((sparkloffs[0][0] < 30<<FRACBITS) ? FRACUNIT : -FRACUNIT);
					y += ((sparkloffs[0][1] < 30<<FRACBITS) ? FRACUNIT : -FRACUNIT);
				}
			}

			if (goodending && finalecount > INFLECTIONPOINT)
				parallaxticker -= 40;

			if ((-parallaxticker/4) < 5)
			{
				trans = (-parallaxticker/4) + 5;
				if (trans < 0)
					trans = 0;
				V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, endglow[(finalecount & 1) ? 0 : 1], NULL);
			}

			if (goodending && finalecount > INFLECTIONPOINT)
			{
				if (finalecount < INFLECTIONPOINT+10)
					V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 0, INFLECTIONPOINT+10-finalecount);
				parallaxticker -= 30;
			}

			if ((parallaxticker/2) > -15)
				colormap[0] = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
			V_DrawFixedPatch(x, y, scale, 0, rockpat, colormap[0]);
			if ((parallaxticker/2) > -25)
			{
				trans = (parallaxticker/2) + 15;
				if (trans < 0)
					trans = -trans;
				if (trans < 10)
					V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, rockpat,
						R_GetTranslationColormap(TC_BLINK, SKINCOLOR_AQUAMARINE, GTC_CACHE));
			}

			if (goodending && finalecount > INFLECTIONPOINT)
			{
				if (finalecount < INFLECTIONPOINT+10)
					V_DrawFixedPatch(x, y, scale, (finalecount-INFLECTIONPOINT)<<V_ALPHASHIFT, rockpat,
						R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_CACHE));
			}
			else
			{
				if ((-parallaxticker/2) < -5)
					colormap[1] = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);

				V_DrawFixedPatch(x, y, scale, 0, endegrk[0], colormap[1]);

				if ((-parallaxticker/2) < 5)
				{
					trans = (-parallaxticker/2) + 5;
					if (trans < 0)
						trans = -trans;
					if (trans < 10)
						V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, endegrk[1], NULL);
				}
			}
		}
		else // firework
		{
			fixed_t scale = FRACUNIT;
			INT32 frame;
			UINT8 *colormap = NULL;
			parallaxticker -= 70;
			x += ((BASEVIDWIDTH-3)<<(FRACBITS-1)) - tweakx;
			y += (BASEVIDHEIGHT<<(FRACBITS-1)) + tweaky;
			borderstuff = true;

			if (parallaxticker < 5)
			{
				scale = (parallaxticker<<FRACBITS)/4;
				V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 31, parallaxticker*2);
			}
			else
				scale += (parallaxticker-4)<<5;

			if (goodending)
				colormap = R_GetTranslationColormap(players[consoleplayer].skin, players[consoleplayer].skincolor, GTC_CACHE);

			if ((frame = ((parallaxticker & 1) ? 1 : 0) + (parallaxticker/TICRATE)) < 3)
				V_DrawFixedPatch(x, y, scale, 0, endfwrk[frame], colormap);
		}

		// explosions
		if (sparklloop >= 3 && doexplosions)
		{
			INT32 boomtime = parallaxticker - sparklloop;

			x = ((((BASEVIDWIDTH-82)/2)+11)<<FRACBITS) - ((boomtime*20)<<FRACBITS)/INFLECTIONPOINT;
			y = ((((BASEVIDHEIGHT-82)/2)+12)<<FRACBITS) + ((boomtime*7)<<FRACBITS)/INFLECTIONPOINT;

			V_DrawFixedPatch(x + sparkloffs[0][0], y + sparkloffs[0][1],
				FRACUNIT, 0, endxpld[sparklloop/4], NULL);
		}

		// initial fade
		if (finalecount < 30)
			V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 0, 30-finalecount);

		// border - only emeralds can exist outside it
		{
			INT32 trans = 0;
			if (borderstuff)
				trans = (10*parallaxticker)/(3*TICRATE);
			if (trans < 10)
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, trans<<V_ALPHASHIFT, endbrdr[0]);
			if (borderstuff && parallaxticker < 11)
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, (parallaxticker-1)<<V_ALPHASHIFT, endbrdr[1]);
			else if (goodending && finalecount > INFLECTIONPOINT && finalecount < INFLECTIONPOINT+10)
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, (finalecount-INFLECTIONPOINT)<<V_ALPHASHIFT, endbrdr[1]);
		}

		// emeralds and emerald accessories
		if (goodending && finalecount >= TICRATE && finalecount < INFLECTIONPOINT)
		{
			INT32 workingtime = finalecount - TICRATE;
			fixed_t radius = ((vid.width/vid.dupx)*(INFLECTIONPOINT - TICRATE - workingtime))/(INFLECTIONPOINT - TICRATE);
			angle_t fa;
			INT32 eemeralds_cur[4];
			char patchname[7] = "CEMGx0";

			radius <<= FRACBITS;

			for (i = 0; i < 4; ++i)
			{
				if (i == 1)
					workingtime -= sparklloop;
				else if (i)
					workingtime -= SPARKLLOOPTIME;
				eemeralds_cur[i] = (workingtime % 360)<<FRACBITS;
			}

			// sparkles
			for (i = 0; i < 7; ++i)
			{
				UINT8* colormap;
				skincolornum_t col = SKINCOLOR_GREEN;
				switch (i)
				{
					case 1:
						col = SKINCOLOR_MAGENTA;
						break;
					case 2:
						col = SKINCOLOR_BLUE;
						break;
					case 3:
						col = SKINCOLOR_CYAN;
						break;
					case 4:
						col = SKINCOLOR_ORANGE;
						break;
					case 5:
						col = SKINCOLOR_RED;
						break;
					case 6:
						col = SKINCOLOR_GREY;
					default:
					case 0:
						break;
				}

				colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_CACHE);

				j = (sparklloop & 1) ? 2 : 3;
				while (j)
				{
					fa = (FixedAngle(eemeralds_cur[j])>>ANGLETOFINESHIFT) & FINEMASK;
					x =  (BASEVIDWIDTH<<(FRACBITS-1)) + FixedMul(FINECOSINE(fa),radius);
					y = (BASEVIDHEIGHT<<(FRACBITS-1)) + FixedMul(FINESINE(fa),radius);
					eemeralds_cur[j] += (360<<FRACBITS)/7;

					// if j == 0 - alternate between 0 and 1
					//         1 -                   1 and 2
					//         2 -                   2 and not rendered
					V_DrawFixedPatch(x, y, FRACUNIT, 0, endspkl[(j - ((sparklloop & 1) ? 0 : 1))], colormap);

					j--;
				}
			}

			// ...then emeralds themselves
			for (i = 0; i < 7; ++i)
			{
				fa = (FixedAngle(eemeralds_cur[0])>>ANGLETOFINESHIFT) & FINEMASK;
				x = (BASEVIDWIDTH<<(FRACBITS-1)) + FixedMul(FINECOSINE(fa),radius);
				y = ((BASEVIDHEIGHT+16)<<(FRACBITS-1)) + FixedMul(FINESINE(fa),radius);
				eemeralds_cur[0] += (360<<FRACBITS)/7;

				patchname[4] = 'A'+(char)i;
				V_DrawFixedPatch(x, y, FRACUNIT, 0, W_CachePatchName(patchname, PU_PATCH_LOWPRIORITY), NULL);
			}
		} // if (goodending...
	} // (finalecount > 20)

	// look, i make an ending for you last-minute, the least you could do is let me have this
	if (cv_soundtest.value == 413)
	{
		INT32 trans = 0;
		boolean donttouch = false;
		const char *str;
		if (goodending)
			str = va("[S] %s: Engage.", skins[players[consoleplayer].skin].realname);
		else
			str = "[S] Eggman: Abscond.";

		if (finalecount < 10)
			trans = (10-finalecount)/2;
		else if (finalecount > STOPPINGPOINT - 20)
		{
			trans = 10 + (finalecount - STOPPINGPOINT)/2;
			donttouch = true;
		}

		if (trans < 10)
		{
			//colset(linkmap,  164, 165, 169); -- the ideal purple colour to represent a clicked in-game link, but not worth it just for a soundtest-controlled secret
			V_DrawCenteredString(BASEVIDWIDTH/2, 8, V_ALLOWLOWERCASE|(trans<<V_ALPHASHIFT), str);
			V_DrawCharacter(32, BASEVIDHEIGHT-16, '>'|(trans<<V_ALPHASHIFT), false);
			V_DrawString(40, ((finalecount == STOPPINGPOINT-(20+TICRATE)) ? 1 : 0)+BASEVIDHEIGHT-16, ((gamedata->timesBeaten || finalecount >= STOPPINGPOINT-TICRATE) ? V_PURPLEMAP : V_BLUEMAP)|(trans<<V_ALPHASHIFT), " [S] ===>");
		}

		if (finalecount > STOPPINGPOINT-(20+(2*TICRATE)))
		{
			INT32 trans2 = abs((5*FINECOSINE((FixedAngle((finalecount*5)<<FRACBITS)>>ANGLETOFINESHIFT & FINEMASK)))>>FRACBITS)+2;
			if (!donttouch)
			{
				trans = 10 + (STOPPINGPOINT-(20+(2*TICRATE))) - finalecount;
				if (trans > trans2)
					trans2 = trans;
			}
			else
				trans2 += 2*trans;
			if (trans2 < 10)
				V_DrawCharacter(26, BASEVIDHEIGHT-33, '\x1C'|(trans2<<V_ALPHASHIFT), false);
		}
	}
}

#undef SPARKLLOOPTIME

// ==========
//  GAME END
// ==========
void F_StartGameEnd(void)
{
	G_SetGamestate(GS_GAMEEND);

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();
	S_StopSounds();

	// In case menus are still up?!!
	M_ClearMenus(true);

	timetonext = TICRATE;
}

//
// F_GameEndDrawer
//
void F_GameEndDrawer(void)
{
	// this function does nothing
}

//
// F_GameEndTicker
//
void F_GameEndTicker(void)
{
	if (timetonext > 0)
	{
		timetonext--;
	}
	else
	{
		nextmap = NEXTMAP_TITLE;
		G_EndGame();
	}
}


// ==============
//  TITLE SCREEN
// ==============

static void F_InitMenuPresValues(void)
{
	menuanimtimer = 0;

	// Set defaults for presentation values
	strncpy(curbgname, "TITLESKY", 9);
	curfadevalue = 16;
	curbgcolor = -1;
	curbgxspeed = titlescrollxspeed;
	curbgyspeed = titlescrollyspeed;
	curbghide = false;

	curhidepics = hidetitlepics;
	curttmode = ttmode;
	curttscale = ttscale;
	strncpy(curttname, ttname, 9);
	curttx = ttx;
	curtty = tty;
	curttloop = ttloop;
	curtttics = tttics;

	LUA_HUD_DestroyDrawList(luahuddrawlist_title);
	luahuddrawlist_title = LUA_HUD_CreateDrawList();
}

//
// F_SkyScroll
//
void F_SkyScroll(INT32 scrollxspeed, INT32 scrollyspeed, const char *patchname)
{
	INT32 xscrolled, x, xneg = (scrollxspeed > 0) - (scrollxspeed < 0), tilex;
	INT32 yscrolled, y, yneg = (scrollyspeed > 0) - (scrollyspeed < 0), tiley;
	boolean xispos = (scrollxspeed >= 0), yispos = (scrollyspeed >= 0);
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	INT16 patwidth, patheight;
	INT32 pw, ph; // scaled by dupz
	patch_t *pat;
	INT32 i, j;
	fixed_t fracmenuanimtimer, xscrolltimer, yscrolltimer;

	if (rendermode == render_none)
		return;

	V_DrawFill(0, 0, vid.width, vid.height, 31);

	if (!patchname || !patchname[0])
	{
		return;
	}

	pat = W_CachePatchName(patchname, PU_PATCH_LOWPRIORITY);

	if (scrollxspeed == 0 && scrollyspeed == 0)
	{
		V_DrawPatchFill(pat);
		return;
	}

	patwidth = pat->width;
	patheight = pat->height;
	pw = patwidth * dupz;
	ph = patheight * dupz;

	tilex = max(FixedCeil(FixedDiv(vid.width, pw)) >> FRACBITS, 1)+2; // one tile on both sides of center
	tiley = max(FixedCeil(FixedDiv(vid.height, ph)) >> FRACBITS, 1)+2;

	fracmenuanimtimer = (menuanimtimer * FRACUNIT) - (FRACUNIT - rendertimefrac);
	xscrolltimer = ((fracmenuanimtimer*scrollxspeed)/16 + patwidth*xneg*FRACUNIT) % (patwidth * FRACUNIT);
	yscrolltimer = ((fracmenuanimtimer*scrollyspeed)/16 + patheight*yneg*FRACUNIT) % (patheight * FRACUNIT);

	// coordinate offsets
	xscrolled = FixedInt(xscrolltimer * dupz);
	yscrolled = FixedInt(yscrolltimer * dupz);

	for (x = (xispos) ? -pw*(tilex-1)+pw : 0, i = 0;
		i < tilex;
		x += pw, i++)
	{
		for (y = (yispos) ? -ph*(tiley-1)+ph : 0, j = 0;
			j < tiley;
			y += ph, j++)
		{
			V_DrawScaledPatch(
				(xispos) ? xscrolled - x : x + xscrolled,
				(yispos) ? yscrolled - y : y + yscrolled,
				V_NOSCALESTART, pat);
		}
	}
}

#define LOADTTGFX(arr, name, maxf) \
lumpnum = W_CheckNumForName(name); \
if (lumpnum != LUMPERROR) \
{ \
	arr[0] = W_CachePatchName(name, PU_PATCH_LOWPRIORITY); \
	arr[min(1, maxf-1)] = 0; \
} \
else if (strlen(name) <= 6) \
{ \
	fixed_t cnt = strlen(name); \
	strncpy(lumpname, name, 7); \
	for (i = 0; i < maxf-1; i++) \
	{ \
		sprintf(&lumpname[cnt], "%.2hu", (UINT16)(i+1)); \
		lumpname[8] = 0; \
		lumpnum = W_CheckNumForName(lumpname); \
		if (lumpnum != LUMPERROR) \
			arr[i] = W_CachePatchName(lumpname, PU_PATCH_LOWPRIORITY); \
		else \
			break; \
	} \
	arr[min(i, maxf-1)] = 0; \
} \
else \
	arr[0] = 0;

static void F_CacheTitleScreen(void)
{
	UINT16 i;

	switch(curttmode)
	{
		case TTMODE_NONE:
			break;

		case TTMODE_OLD:
			break; // idk do we still want this?

		case TTMODE_RINGRACERS:
			if (!M_SecretUnlocked(SECRET_ALTTITLE, true))
			{
				CV_StealthSetValue(&cv_alttitle, 0);
			}

			kts_bumper = W_CachePatchName(
				(cv_alttitle.value ? "KTSJUMPR1" : "KTSBUMPR1"),
				PU_PATCH_LOWPRIORITY);
			kts_eggman = W_CachePatchName("KTSEGG01", PU_PATCH_LOWPRIORITY);
			kts_tails = W_CachePatchName("KTSTAL01", PU_PATCH_LOWPRIORITY);
			kts_tails_tails = W_CachePatchName("KTSTAL02", PU_PATCH_LOWPRIORITY);
			for (i = 0; i < 6; i++)
			{
				kts_electricity[i] = W_CachePatchName(va("KTSELCT%.1d", i+1), PU_PATCH_LOWPRIORITY);
			}
			kts_copyright = W_CachePatchName("KTSCR", PU_PATCH_LOWPRIORITY);
			break;

		case TTMODE_USER:
		{
			lumpnum_t lumpnum;
			char lumpname[9];

			LOADTTGFX(ttuser, curttname, TTMAX_USER)
			break;
		}
	}
}

void F_StartTitleScreen(void)
{
	INT32 titleMapNum;
	setup_numplayers = 0;

	if (gamestate != GS_TITLESCREEN && gamestate != GS_WAITINGPLAYERS)
	{
		ttuser_count = 0;
		finalecount = 0;
		wipetypepost = 0;
	}
	else
		wipegamestate = GS_TITLESCREEN;

	if (titlemap
		&& ((titleMapNum = G_MapNumber(titlemap)) < nummapheaders)
		&& mapheaderinfo[titleMapNum]
		&& mapheaderinfo[titleMapNum]->lumpnum != LUMPERROR)
	{
		mapthing_t *startpos;

		gamestate_t prevwipegamestate = wipegamestate;
		titlemapinaction = TITLEMAP_LOADING;
		titlemapcameraref = NULL;
		gamemap = titleMapNum+1;

		maptol = mapheaderinfo[titleMapNum]->typeoflevel;
		globalweather = mapheaderinfo[titleMapNum]->weather;

		G_DoLoadLevel(true);
		if (!titlemap)
			return;

		players[displayplayers[0]].playerstate = PST_DEAD; // Don't spawn the player in dummy (I'm still a filthy cheater)

		// Set Default Position
		if (playerstarts[0])
			startpos = playerstarts[0];
		else if (deathmatchstarts[0])
			startpos = deathmatchstarts[0];
		else
			startpos = NULL;

		if (startpos)
		{
			camera[0].x = startpos->x << FRACBITS;
			camera[0].y = startpos->y << FRACBITS;
			camera[0].subsector = R_PointInSubsector(camera[0].x, camera[0].y);
			camera[0].z = camera[0].subsector->sector->floorheight + (startpos->z << FRACBITS);
			camera[0].angle = (startpos->angle % 360)*ANG1;
			camera[0].aiming = 0;
		}
		else
		{
			camera[0].x = camera[0].y = camera[0].z = camera[0].angle = camera[0].aiming = 0;
			camera[0].subsector = NULL; // toast is filthy too
		}

		camera[0].chase = true;
		camera[0].height = 0;

		wipegamestate = prevwipegamestate;
	}
	else
	{
		titlemapinaction = TITLEMAP_OFF;
		gamemap = 1; // g_game.c
		CON_ClearHUD();
	}

	G_SetGamestate(GS_TITLESCREEN);

	// IWAD dependent stuff.

	animtimer = skullAnimCounter = 0;

	demoDelayLeft = demoDelayTime;
	demoIdleLeft = demoIdleTime;

	F_InitMenuPresValues();
	F_CacheTitleScreen();
}

void F_VersionDrawer(void)
{
	// An adapted thing from old menus - most games have version info on the title screen now...
	INT32 texty = vid.height - 10*vid.dupy;

#define addtext(f, str) {\
	V_DrawThinString(vid.dupx, texty, V_NOSCALESTART|f, str);\
	texty -= 10*vid.dupy;\
}
	if (customversionstring[0] != '\0')
	{
		addtext(V_ALLOWLOWERCASE, customversionstring);
		addtext(0, "Mod version:");
	}
	else
	{
// Development -- show revision / branch info
#if defined(TESTERS)
		addtext(V_ALLOWLOWERCASE|V_SKYMAP, "Tester client");
		addtext(V_ALLOWLOWERCASE|V_TRANSLUCENT, va("%s", compdate));
#elif defined(HOSTTESTERS)
		addtext(V_ALLOWLOWERCASE|V_REDMAP, "Netgame host for testers");
		addtext(V_ALLOWLOWERCASE|V_TRANSLUCENT, va("%s", compdate));
#elif defined(DEVELOP)
		addtext(V_ALLOWLOWERCASE|V_TRANSLUCENT, va("%s %s", comprevision, compnote));
		addtext(V_ALLOWLOWERCASE|V_TRANSLUCENT, D_GetFancyBranchName());
#else // Regular build
		addtext(V_ALLOWLOWERCASE|V_TRANSLUCENT, va("%s", VERSIONSTRING));
#endif
		if (compoptimized)
		{
			addtext(V_ALLOWLOWERCASE|V_TRANSLUCENT, va("%s build", comptype));
		}
		else
		{
			addtext(V_ALLOWLOWERCASE|V_ORANGEMAP, va("%s build (no optimizations)", comptype));
		}

		if (compuncommitted)
		{
			addtext(V_REDMAP|V_STRINGDANCE, "! UNCOMMITTED CHANGES !");
		}
	}
#undef addtext
}

// (no longer) De-Demo'd Title Screen
void F_TitleScreenDrawer(void)
{
	boolean hidepics = false;

	if (modeattacking)
		return; // We likely came here from retrying. Don't do a damn thing.

	// Draw that sky!
	if (curbgcolor >= 0)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, curbgcolor);
	else if (!curbghide || !titlemapinaction || gamestate == GS_WAITINGPLAYERS)
		F_SkyScroll(curbgxspeed, curbgyspeed, curbgname);
	else
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	// Don't draw outside of the title screen, or if the patch isn't there.
	if (gamestate != GS_TITLESCREEN && gamestate != GS_WAITINGPLAYERS)
		return;

	// Don't draw if title mode is set to Old/None and the patch isn't there
	/*
	if (!ttwing && (curttmode == TTMODE_OLD || curttmode == TTMODE_NONE))
		return;
	*/

	// rei|miru: use title pics?
	hidepics = curhidepics;
	if (hidepics)
		goto luahook;

	switch(curttmode)
	{
		case TTMODE_NONE:
			break;

		case TTMODE_RINGRACERS:
		{
			const char *eggName = "eggman";
			INT32 eggSkin = R_SkinAvailable(eggName);
			skincolornum_t eggColor = SKINCOLOR_RED;
			UINT8 *eggColormap = NULL;

			const char *tailsName = "tails";
			INT32 tailsSkin = R_SkinAvailable(tailsName);
			skincolornum_t tailsColor = SKINCOLOR_ORANGE;
			UINT8 *tailsColormap = NULL;

			if (eggSkin != -1)
			{
				eggColor = skins[eggSkin].prefcolor;
			}
			eggColormap = R_GetTranslationColormap(TC_DEFAULT, eggColor, GTC_MENUCACHE);

			if (tailsSkin != -1)
			{
				tailsColor = skins[tailsSkin].prefcolor;
			}
			tailsColormap = R_GetTranslationColormap(TC_DEFAULT, tailsColor, GTC_MENUCACHE);

			V_DrawFixedPatch(0, 0, FRACUNIT, 0, kts_tails_tails, tailsColormap);
			V_DrawFixedPatch(0, 0, FRACUNIT, V_ADD, kts_electricity[finalecount % 6], NULL);

			V_DrawFixedPatch(0, 0, FRACUNIT, 0, kts_eggman, eggColormap);
			V_DrawFixedPatch(0, 0, FRACUNIT, 0, kts_tails, tailsColormap);

			V_DrawFixedPatch(0, 0, FRACUNIT, 0, kts_bumper, NULL);

			V_DrawFixedPatch(0, 0, FRACUNIT, 0, kts_copyright, NULL);

			F_VersionDrawer();
			break;
		}

		case TTMODE_OLD:
/*
			if (finalecount < 50)
			{
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

				V_DrawSmallScaledPatch(84, 36, 0, ttbanner);

				if (finalecount >= 20)
					V_DrawSmallScaledPatch(84, 87, 0, ttkart);
				else if (finalecount >= 10)
					V_DrawSciencePatch((84<<FRACBITS) - FixedDiv(180<<FRACBITS, 10<<FRACBITS)*(20-finalecount), (87<<FRACBITS), 0, ttkart, FRACUNIT/2);
			}
			else if (finalecount < 52)
			{
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);
				V_DrawSmallScaledPatch(84, 36, 0, ttkflash);
			}
			else
			{
				INT32 transval = 0;

				if (finalecount <= (50+(9<<1)))
					transval = (finalecount - 50)>>1;

				V_DrawSciencePatch(0, 0 - FixedMul(40<<FRACBITS, FixedDiv(finalecount%70, 70)), V_SNAPTOTOP|V_SNAPTOLEFT, ttcheckers, FRACUNIT);
				V_DrawSciencePatch(280<<FRACBITS, -(40<<FRACBITS) + FixedMul(40<<FRACBITS, FixedDiv(finalecount%70, 70)), V_SNAPTOTOP|V_SNAPTORIGHT, ttcheckers, FRACUNIT);

				if (transval)
					V_DrawFadeScreen(0, 10 - transval);

				V_DrawSmallScaledPatch(84, 36, 0, ttbanner);

				V_DrawSmallScaledPatch(84, 87, 0, ttkart);

				if (!transval)
					return;

				V_DrawSmallScaledPatch(84, 36, transval<<V_ALPHASHIFT, ttkflash);
			}
*/
			V_DrawCenteredString(BASEVIDWIDTH/2, 64, V_ALLOWLOWERCASE, "Dr. Robotnik's Ring Racers v2.0");

#ifdef DEVELOP
#if defined(TESTERS)
			V_DrawCenteredString(BASEVIDWIDTH/2, 96, V_SKYMAP|V_ALLOWLOWERCASE, "Tester EXE");
#elif defined(HOSTTESTERS)
			V_DrawCenteredThinString(BASEVIDWIDTH/2, 96, V_REDMAP|V_ALLOWLOWERCASE, "Tester netgame host EXE");
#else
			V_DrawCenteredString(BASEVIDWIDTH/2, 96, V_ALLOWLOWERCASE, "Development EXE");
#endif
#endif
			break;

		case TTMODE_USER:
			if (!ttuser[max(0, ttuser_count)])
			{
				if(curttloop > -1 && ttuser[curttloop])
					ttuser_count = curttloop;
				else if (ttuser[max(0, ttuser_count-1)])
					ttuser_count = max(0, ttuser_count-1);
				else
					break; // draw nothing
			}

			V_DrawSciencePatch(curttx<<FRACBITS, curtty<<FRACBITS, 0, ttuser[ttuser_count], FRACUNIT);

			if (!(finalecount % max(1, curtttics)))
				ttuser_count++;
			break;
	}

luahook:
	// The title drawer is sometimes called without first being started
	// In order to avoid use-before-initialization crashes, let's check and
	// create the drawlist if it doesn't exist.
	if (!LUA_HUD_IsDrawListValid(luahuddrawlist_title))
	{
		LUA_HUD_DestroyDrawList(luahuddrawlist_title);
		luahuddrawlist_title = LUA_HUD_CreateDrawList();
	}

	if (renderisnewtic)
	{
		LUA_HUD_ClearDrawList(luahuddrawlist_title);
		LUA_HookHUD(luahuddrawlist_title, HUD_HOOK(title));
	}
	LUA_HUD_DrawList(luahuddrawlist_title);

	if (finalecount > 0)
		M_DrawMenuMessage();
}

// (no longer) De-Demo'd Title Screen
void F_TitleScreenTicker(boolean run)
{
	menuanimtimer++; // title sky

	if (run)
	{
		if (finalecount == 0)
		{
			// Now start the music
			S_ChangeMusicInternal("_title", looptitle);
		}
		else if (menumessage.fadetimer < 9)
			menumessage.fadetimer++;

		finalecount++;
	}

	// don't trigger if doing anything besides idling on title
	if (gameaction != ga_nothing || gamestate != GS_TITLESCREEN)
		return;

	// Execute the titlemap camera settings
	if (titlemapinaction)
	{
		thinker_t *th;
		mobj_t *mo2;
		mobj_t *cameraref = NULL;

		// If there's a Line 422 Switch Cut-Away view, don't force us.
		if (!titlemapcameraref || titlemapcameraref->type != MT_ALTVIEWMAN)
		{
			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo2 = (mobj_t *)th;

				if (!mo2)
					continue;

				if (mo2->type != MT_ALTVIEWMAN)
					continue;

				cameraref = titlemapcameraref = mo2;
				break;
			}
		}
		else
			cameraref = titlemapcameraref;

		if (cameraref)
		{
			camera[0].x = cameraref->x;
			camera[0].y = cameraref->y;
			camera[0].z = cameraref->z;
			camera[0].angle = cameraref->angle;
			camera[0].aiming = cameraref->cusval;
			camera[0].subsector = cameraref->subsector;
		}
		else
		{
			// Default behavior: Do a lil' camera spin if a title map is loaded;
			camera[0].angle += titlescrollxspeed*ANG1/64;
		}
	}

	// no demos to play? or, are they disabled?
	if (!cv_rollingdemos.value)
		return;

	#if defined (TESTERS)
		return;
	#endif

	// Wait for a while (for the music to finish, preferably)
	// before starting demos
	if (demoDelayLeft)
	{
		--demoDelayLeft;
		return;
	}

	// Hold up for a bit if menu or console active
	if (menuactive || CON_Ready())
	{
		demoIdleLeft = demoIdleTime;
		return;
	}

	// is it time?
	if (!(--demoIdleLeft))
	{
		char dname[MAXMAPLUMPNAME+1+8+1];
		UINT16 mapnum;
		UINT8 numstaff;
		static boolean use_netreplay = false;

		if ((use_netreplay = !use_netreplay))
		{
			lumpnum_t l = LUMPERROR;
			numstaff = 0;
			while (numstaff < 99 && (l = W_CheckNumForName(va("TDEMO%03u", numstaff))) != LUMPERROR)
				numstaff++;

			if (numstaff)
			{
				numstaff = M_RandomKey(numstaff)+1;
				snprintf(dname, 9, "TDEMO%03u", numstaff);
				goto loadreplay;
			}
		}

		// prevent console spam if failed
		demoIdleLeft = demoIdleTime;

		mapnum = G_RandMap(TOL_RACE|TOL_BATTLE, -2, 2, 0, false, NULL);
		if (mapnum == 0) // gotta have ONE
		{
			return;
		}

		numstaff = M_RandomKey(mapheaderinfo[mapnum]->ghostCount)+1;

		// Setup demo name
		sprintf(dname, "%s/GHOST_%u", mapheaderinfo[mapnum]->lumpname, numstaff);

loadreplay:
		demo.title = true;
		demo.ignorefiles = true;
		demo.loadfiles = false;
		G_DoPlayDemo(dname);
	}
}

void F_TitleDemoTicker(void)
{
	keypressed = false;
}

// ================
//  WAITINGPLAYERS
// ================

void F_StartWaitingPlayers(void)
{
#ifdef NOWAY
	INT32 i;
	INT32 randskin;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
#endif

	wipegamestate = GS_TITLESCREEN; // technically wiping from title screen
	finalecount = 0;

#ifdef NOWAY
	randskin = M_RandomKey(numskins);

	if (waitcolormap)
		Z_Free(waitcolormap);

	waitcolormap = R_GetTranslationColormap(randskin, skins[randskin].prefcolor, 0);

	sprdef = &skins[randskin].sprites[P_GetSkinSprite2(&skins[randskin], SPR2_FSTN, NULL)];

	for (i = 0; i < 2; i++)
	{
		sprframe = &sprdef->spriteframes[i];
		driver[i] = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
	}
#endif
}

void F_WaitingPlayersTicker(void)
{
	if (paused)
		return;

	finalecount++;

	// dumb hack, only start the music on the 1st tick so if you instantly go into the map you aren't hearing a tic of music
	if (finalecount == 2)
		S_ChangeMusicInternal("WAIT2J", true);
}

void F_WaitingPlayersDrawer(void)
{
#ifdef NOWAY
	UINT32 frame = (finalecount % 8) / 4; // The game only tics every other frame while waitingplayers
#endif
	const char *waittext1 = "You will join";
	const char *waittext2 = "the next race...";
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	V_DrawCreditString((160 - (V_CreditStringWidth(waittext1)>>1))<<FRACBITS, 48<<FRACBITS, 0, waittext1);
	V_DrawCreditString((160 - (V_CreditStringWidth(waittext2)>>1))<<FRACBITS, 64<<FRACBITS, 0, waittext2);
#ifdef NOWAY
	V_DrawFixedPatch((160<<FRACBITS) - driver[frame]->width / 2, 150<<FRACBITS, 1<<FRACBITS, V_FLIP, driver[frame], waitcolormap);
#endif
}

// ==================
//  CUSTOM CUTSCENES
// ==================
static INT32 scenenum, cutnum;
static INT32 picxpos, picypos, picnum, pictime, picmode, numpics, pictoloop;
static INT32 textxpos, textypos;
static boolean dofadenow = false, cutsceneover = false;
static boolean runningprecutscene = false, precutresetplayer = false;


static void F_AdvanceToNextScene(void)
{
	// Don't increment until after endcutscene check
	// (possible overflow / bad patch names from the one tic drawn before the fade)
	if (scenenum+1 >= cutscenes[cutnum]->numscenes)
	{
		F_EndCutScene();
		return;
	}

	++scenenum;

	timetonext = 0;
	stoptimer = 0;
	picnum = 0;
	picxpos = cutscenes[cutnum]->scene[scenenum].xcoord[picnum];
	picypos = cutscenes[cutnum]->scene[scenenum].ycoord[picnum];

	if (cutscenes[cutnum]->scene[scenenum].musswitch[0])
		S_ChangeMusicEx(cutscenes[cutnum]->scene[scenenum].musswitch,
			cutscenes[cutnum]->scene[scenenum].musswitchflags,
			cutscenes[cutnum]->scene[scenenum].musicloop,
			cutscenes[cutnum]->scene[scenenum].musswitchposition, 0, 0);

	// Fade to the next
	F_NewCutscene(cutscenes[cutnum]->scene[scenenum].text);

	picnum = 0;
	picxpos = cutscenes[cutnum]->scene[scenenum].xcoord[picnum];
	picypos = cutscenes[cutnum]->scene[scenenum].ycoord[picnum];
	textxpos = cutscenes[cutnum]->scene[scenenum].textxpos;
	textypos = cutscenes[cutnum]->scene[scenenum].textypos;

	animtimer = pictime = cutscenes[cutnum]->scene[scenenum].picduration[picnum];
}

// See also G_AfterIntermission, the only other place which handles intra-map/ending transitions
void F_EndCutScene(void)
{
	cutsceneover = true; // do this first, just in case G_EndGame or something wants to turn it back false later
	if (runningprecutscene)
	{
		if (server)
			D_MapChange(gamemap, gametype, false, precutresetplayer, 0, true, false);
	}
	else
	{
		if (cutnum == creditscutscene-1)
			F_StartGameEvaluation();
		else if (cutnum == introtoplay-1)
			D_StartTitle();
		else
			G_NextLevel();
	}
}

void F_StartCustomCutscene(INT32 cutscenenum, boolean precutscene, boolean resetplayer)
{
	if (!cutscenes[cutscenenum])
		return;

	G_SetGamestate(GS_CUTSCENE);

	if (wipegamestate == GS_CUTSCENE)
		wipegamestate = -1;

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();

	F_NewCutscene(cutscenes[cutscenenum]->scene[0].text);

	cutsceneover = false;
	runningprecutscene = precutscene;
	precutresetplayer = resetplayer;

	scenenum = picnum = 0;
	cutnum = cutscenenum;
	picxpos = cutscenes[cutnum]->scene[0].xcoord[0];
	picypos = cutscenes[cutnum]->scene[0].ycoord[0];
	textxpos = cutscenes[cutnum]->scene[0].textxpos;
	textypos = cutscenes[cutnum]->scene[0].textypos;

	pictime = cutscenes[cutnum]->scene[0].picduration[0];

	keypressed = false;
	finalecount = 0;
	timetonext = 0;
	animtimer = cutscenes[cutnum]->scene[0].picduration[0]; // Picture duration
	stoptimer = 0;

	if (cutscenes[cutnum]->scene[0].musswitch[0])
		S_ChangeMusicEx(cutscenes[cutnum]->scene[0].musswitch,
			cutscenes[cutnum]->scene[0].musswitchflags,
			cutscenes[cutnum]->scene[0].musicloop,
			cutscenes[cutnum]->scene[scenenum].musswitchposition, 0, 0);
	else
		S_StopMusic();
	S_StopSounds();
}

//
// F_CutsceneDrawer
//
void F_CutsceneDrawer(void)
{
	if (dofadenow && rendermode != render_none)
	{
		F_WipeStartScreen();

		// Fade to any palette color you want.
		if (cutscenes[cutnum]->scene[scenenum].fadecolor)
		{
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, cutscenes[cutnum]->scene[scenenum].fadecolor);

			F_WipeEndScreen();
			F_RunWipe(cutscenes[cutnum]->scene[scenenum].fadeinid, true, NULL, false, false);

			F_WipeStartScreen();
		}
	}
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (cutscenes[cutnum]->scene[scenenum].picname[picnum][0] != '\0')
	{
		if (cutscenes[cutnum]->scene[scenenum].pichires[picnum])
			V_DrawSmallScaledPatch(picxpos, picypos, 0,
				W_CachePatchName(cutscenes[cutnum]->scene[scenenum].picname[picnum], PU_PATCH_LOWPRIORITY));
		else
			V_DrawScaledPatch(picxpos,picypos, 0,
				W_CachePatchName(cutscenes[cutnum]->scene[scenenum].picname[picnum], PU_PATCH_LOWPRIORITY));
	}

	if (dofadenow && rendermode != render_none)
	{
		F_WipeEndScreen();
		F_RunWipe(cutscenes[cutnum]->scene[scenenum].fadeoutid, true, NULL, false, false);
	}

	V_DrawString(textxpos, textypos, V_ALLOWLOWERCASE, cutscene_disptext);
}

void F_CutsceneTicker(void)
{
	INT32 i;

	// Online clients tend not to instantly get the map change, so just wait
	// and don't send 30 of them.
	if (cutsceneover)
		return;

	// advance animation
	finalecount++;
	cutscene_boostspeed = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (netgame && i != serverplayer && !IsPlayerAdmin(i))
			continue;

		if (players[i].cmd.buttons & BT_BRAKE || players[i].cmd.buttons & BT_ACCELERATE) // SRB2kart
		{
			keypressed = false;
			cutscene_boostspeed = 1;
			if (timetonext)
				timetonext = 2;
		}
	}

	if (animtimer)
	{
		animtimer--;
		if (animtimer <= 0)
		{
			if (picnum < 7 && cutscenes[cutnum]->scene[scenenum].picname[picnum+1][0] != '\0')
			{
				picnum++;
				picxpos = cutscenes[cutnum]->scene[scenenum].xcoord[picnum];
				picypos = cutscenes[cutnum]->scene[scenenum].ycoord[picnum];
				pictime = cutscenes[cutnum]->scene[scenenum].picduration[picnum];
				animtimer = pictime;
			}
			else
				timetonext = 2;
		}
	}

	if (timetonext)
		--timetonext;

	if (++stoptimer > 2 && timetonext == 1)
		F_AdvanceToNextScene();
	else if (!timetonext && !F_WriteText())
		timetonext = 5*TICRATE + 1;
}

boolean F_CutsceneResponder(event_t *event)
{
	if (cutnum == introtoplay-1)
		return F_IntroResponder(event);

	return false;
}

// ==================
//  TEXT PROMPTS
// ==================

static void F_GetPageTextGeometry(UINT8 *pagelines, boolean *rightside, INT32 *boxh, INT32 *texth, INT32 *texty, INT32 *namey, INT32 *chevrony, INT32 *textx, INT32 *textr)
{
	// reuse:
	// cutnum -> promptnum
	// scenenum -> pagenum
	lumpnum_t iconlump = W_CheckNumForName(textprompts[cutnum]->page[scenenum].iconname);

	*pagelines = textprompts[cutnum]->page[scenenum].lines ? textprompts[cutnum]->page[scenenum].lines : 4;
	*rightside = (iconlump != LUMPERROR && textprompts[cutnum]->page[scenenum].rightside);

	// Vertical calculations
	*boxh = *pagelines*2;
	*texth = textprompts[cutnum]->page[scenenum].name[0] ? (*pagelines-1)*2 : *pagelines*2; // name takes up first line if it exists
	*texty = BASEVIDHEIGHT - ((*texth * 4) + (*texth/2)*4);
	*namey = BASEVIDHEIGHT - ((*boxh * 4) + (*boxh/2)*4);
	*chevrony = BASEVIDHEIGHT - (((1*2) * 4) + ((1*2)/2)*4); // force on last line

	// Horizontal calculations
	// Shift text to the right if we have a character icon on the left side
	// Add 4 margin against icon
	*textx = (iconlump != LUMPERROR && !*rightside) ? ((*boxh * 4) + (*boxh/2)*4) + 4 : 4;
	*textr = *rightside ? BASEVIDWIDTH - (((*boxh * 4) + (*boxh/2)*4) + 4) : BASEVIDWIDTH-4;
}

static fixed_t F_GetPromptHideHudBound(void)
{
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	if (cutnum == INT32_MAX || scenenum == INT32_MAX || !textprompts[cutnum] || scenenum >= textprompts[cutnum]->numpages ||
		!textprompts[cutnum]->page[scenenum].hidehud ||
		(splitscreen && textprompts[cutnum]->page[scenenum].hidehud != 2)) // don't hide on splitscreen, unless hide all is forced
		return 0;
	else if (textprompts[cutnum]->page[scenenum].hidehud == 2) // hide all
		return BASEVIDHEIGHT;

	F_GetPageTextGeometry(&pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	// calc boxheight (see V_DrawPromptBack)
	boxh *= vid.dupy;
	boxh = (boxh * 4) + (boxh/2)*5; // 4 lines of space plus gaps between and some leeway

	// return a coordinate to check
	// if negative: don't show hud elements below this coordinate (visually)
	// if positive: don't show hud elements above this coordinate (visually)
	return 0 - boxh; // \todo: if prompt at top of screen (someday), make this return positive
}

boolean F_GetPromptHideHudAll(void)
{
	if (cutnum == INT32_MAX || scenenum == INT32_MAX || !textprompts[cutnum] || scenenum >= textprompts[cutnum]->numpages ||
		!textprompts[cutnum]->page[scenenum].hidehud ||
		(splitscreen && textprompts[cutnum]->page[scenenum].hidehud != 2)) // don't hide on splitscreen, unless hide all is forced
		return false;
	else if (textprompts[cutnum]->page[scenenum].hidehud == 2) // hide all
		return true;
	else
		return false;
}

boolean F_GetPromptHideHud(fixed_t y)
{
	INT32 ybound;
	boolean fromtop;
	fixed_t ytest;

	if (!promptactive)
		return false;

	ybound = F_GetPromptHideHudBound();
	fromtop = (ybound >= 0);
	ytest = (fromtop ? ybound : BASEVIDHEIGHT + ybound);

	return (fromtop ? y < ytest : y >= ytest); // true means hide
}

static void F_PreparePageText(char *pagetext)
{
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	F_GetPageTextGeometry(&pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	if (promptpagetext)
		Z_Free(promptpagetext);
	promptpagetext = (pagetext && pagetext[0]) ? V_WordWrap(textx, textr, 0, pagetext) : Z_StrDup("");

	F_NewCutscene(promptpagetext);
	cutscene_textspeed = textprompts[cutnum]->page[scenenum].textspeed ? textprompts[cutnum]->page[scenenum].textspeed : TICRATE/5;
	cutscene_textcount = 0; // no delay in beginning
	cutscene_boostspeed = 0; // don't print 8 characters to start

	// \todo update control hot strings on re-config
	// and somehow don't reset cutscene text counters
}

static void F_AdvanceToNextPage(void)
{
	INT32 nextprompt = textprompts[cutnum]->page[scenenum].nextprompt ? textprompts[cutnum]->page[scenenum].nextprompt - 1 : INT32_MAX,
		nextpage = textprompts[cutnum]->page[scenenum].nextpage ? textprompts[cutnum]->page[scenenum].nextpage - 1 : INT32_MAX,
		oldcutnum = cutnum;

	if (textprompts[cutnum]->page[scenenum].nexttag[0])
		F_GetPromptPageByNamedTag(textprompts[cutnum]->page[scenenum].nexttag, &nextprompt, &nextpage);

	// determine next prompt
	if (nextprompt != INT32_MAX)
	{
		if (nextprompt <= MAX_PROMPTS && textprompts[nextprompt])
			cutnum = nextprompt;
		else
			cutnum = INT32_MAX;
	}

	// determine next page
	if (nextpage != INT32_MAX)
	{
		if (cutnum != INT32_MAX)
		{
			scenenum = nextpage;
			if (scenenum >= MAX_PAGES || scenenum > textprompts[cutnum]->numpages-1)
				scenenum = INT32_MAX;
		}
	}
	else
	{
		if (cutnum != oldcutnum)
			scenenum = 0;
		else if (scenenum + 1 < MAX_PAGES && scenenum < textprompts[cutnum]->numpages-1)
			scenenum++;
		else
			scenenum = INT32_MAX;
	}

	// close the prompt if either num is invalid
	if (cutnum == INT32_MAX || scenenum == INT32_MAX)
		F_EndTextPrompt(false, false);
	else
	{
		// on page mode, number of tics before allowing boost
		// on timer mode, number of tics until page advances
		timetonext = textprompts[cutnum]->page[scenenum].timetonext ? textprompts[cutnum]->page[scenenum].timetonext : TICRATE/10;
		F_PreparePageText(textprompts[cutnum]->page[scenenum].text);

		// gfx
		picnum = textprompts[cutnum]->page[scenenum].pictostart;
		numpics = textprompts[cutnum]->page[scenenum].numpics;
		picmode = textprompts[cutnum]->page[scenenum].picmode;
		pictoloop = textprompts[cutnum]->page[scenenum].pictoloop > 0 ? textprompts[cutnum]->page[scenenum].pictoloop - 1 : 0;
		picxpos = textprompts[cutnum]->page[scenenum].xcoord[picnum];
		picypos = textprompts[cutnum]->page[scenenum].ycoord[picnum];
		animtimer = pictime = textprompts[cutnum]->page[scenenum].picduration[picnum];

		// music change
		if (textprompts[cutnum]->page[scenenum].musswitch[0])
			S_ChangeMusic(textprompts[cutnum]->page[scenenum].musswitch,
				textprompts[cutnum]->page[scenenum].musswitchflags,
				textprompts[cutnum]->page[scenenum].musicloop);
	}
}

void F_EndTextPrompt(boolean forceexec, boolean noexec)
{
	boolean promptwasactive = promptactive;
	promptactive = false;
	callpromptnum = callpagenum = callplayer = INT32_MAX;

	if (promptwasactive)
	{
		if (promptmo && promptmo->player && promptblockcontrols)
			promptmo->reactiontime = TICRATE/4; // prevent jumping right away // \todo account freeze realtime for this)
		// \todo reset frozen realtime?
	}

	// \todo net safety, maybe loop all player thinkers?
	if ((promptwasactive || forceexec) && !noexec && promptpostexectag)
	{
		if (tm.thing) // edge case where starting an invalid prompt immediately on level load will make P_MapStart fail
			P_LinedefExecute(promptpostexectag, promptmo, NULL);
		else
		{
			P_MapStart();
			P_LinedefExecute(promptpostexectag, promptmo, NULL);
			P_MapEnd();
		}
	}
}

void F_StartTextPrompt(INT32 promptnum, INT32 pagenum, mobj_t *mo, UINT16 postexectag, boolean blockcontrols, boolean freezerealtime)
{
	INT32 i;

	// if splitscreen and we already have a prompt active, ignore.
	// \todo Proper per-player splitscreen support (individual prompts)
	if (promptactive && splitscreen && promptnum == callpromptnum && pagenum == callpagenum)
		return;

	// \todo proper netgame support
	if (netgame)
	{
		F_EndTextPrompt(true, false); // run the post-effects immediately
		return;
	}

	// We share vars, so no starting text prompts over cutscenes or title screens!
	keypressed = false;
	finalecount = 0;
	timetonext = 0;
	animtimer = 0;
	stoptimer = 0;
	skullAnimCounter = 0;

	// Set up state
	promptmo = mo;
	promptpostexectag = postexectag;
	promptblockcontrols = blockcontrols;
	(void)freezerealtime; // \todo freeze player->realtime, maybe this needs to cycle through player thinkers

	// Initialize current prompt and scene
	callpromptnum = promptnum;
	callpagenum = pagenum;
	cutnum = (promptnum < MAX_PROMPTS && textprompts[promptnum]) ? promptnum : INT32_MAX;
	scenenum = (cutnum != INT32_MAX && pagenum < MAX_PAGES && pagenum <= textprompts[cutnum]->numpages-1) ? pagenum : INT32_MAX;
	promptactive = (cutnum != INT32_MAX && scenenum != INT32_MAX);

	if (promptactive)
	{
		// on page mode, number of tics before allowing boost
		// on timer mode, number of tics until page advances
		timetonext = textprompts[cutnum]->page[scenenum].timetonext ? textprompts[cutnum]->page[scenenum].timetonext : TICRATE/10;
		F_PreparePageText(textprompts[cutnum]->page[scenenum].text);

		// gfx
		picnum = textprompts[cutnum]->page[scenenum].pictostart;
		numpics = textprompts[cutnum]->page[scenenum].numpics;
		picmode = textprompts[cutnum]->page[scenenum].picmode;
		pictoloop = textprompts[cutnum]->page[scenenum].pictoloop > 0 ? textprompts[cutnum]->page[scenenum].pictoloop - 1 : 0;
		picxpos = textprompts[cutnum]->page[scenenum].xcoord[picnum];
		picypos = textprompts[cutnum]->page[scenenum].ycoord[picnum];
		animtimer = pictime = textprompts[cutnum]->page[scenenum].picduration[picnum];

		// music change
		if (textprompts[cutnum]->page[scenenum].musswitch[0])
			S_ChangeMusic(textprompts[cutnum]->page[scenenum].musswitch,
				textprompts[cutnum]->page[scenenum].musswitchflags,
				textprompts[cutnum]->page[scenenum].musicloop);

		// get the calling player
		if (promptblockcontrols && mo && mo->player)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (players[i].mo == mo)
				{
					callplayer = i;
					break;
				}
			}
		}
	}
	else
		F_EndTextPrompt(true, false); // run the post-effects immediately
}

static boolean F_GetTextPromptTutorialTag(char *tag, INT32 length)
{
	INT32 gcs = 0;
	boolean suffixed = true;

	if (!tag || !tag[0] || !tutorialmode)
		return false;

	/*
	if (!strncmp(tag, "TAA", 3)) // Accelerate
		gcs = G_GetControlScheme(gamecontrol[0], gcl_accelerate, num_gcl_accelerate);
	else if (!strncmp(tag, "TAB", 3)) // Brake
		gcs = G_GetControlScheme(gamecontrol[0], gcl_brake, num_gcl_brake);
	else if (!strncmp(tag, "TAD", 3)) // Drift
		gcs = G_GetControlScheme(gamecontrol[0], gcl_drift, num_gcl_drift);
	else if (!strncmp(tag, "TAS", 3)) // Spindash
		gcs = G_GetControlScheme(gamecontrol[0], gcl_spindash, num_gcl_spindash);
	else if (!strncmp(tag, "TAM", 3)) // Movement
		gcs = G_GetControlScheme(gamecontrol[0], gcl_movement, num_gcl_movement);
	else if (!strncmp(tag, "TAI", 3)) // Item usage
		gcs = G_GetControlScheme(gamecontrol[0], gcl_item, num_gcl_item);
	else
		gcs = G_GetControlScheme(gamecontrol[0], gcl_full, num_gcl_full);
	*/

	switch (gcs)
	{
		default:
			strncat(tag, "CUSTOM", length);
			break;
	}

	return suffixed;
}

void F_GetPromptPageByNamedTag(const char *tag, INT32 *promptnum, INT32 *pagenum)
{
	INT32 nosuffixpromptnum = INT32_MAX, nosuffixpagenum = INT32_MAX;
	INT32 tutorialpromptnum = (tutorialmode) ? TUTORIAL_PROMPT-1 : 0;
	boolean suffixed = false, found = false;
	char suffixedtag[33];

	*promptnum = *pagenum = INT32_MAX;

	if (!tag || !tag[0])
		return;

	strncpy(suffixedtag, tag, 33);
	suffixedtag[32] = 0;

	if (tutorialmode)
		suffixed = F_GetTextPromptTutorialTag(suffixedtag, 33);

	for (*promptnum = 0 + tutorialpromptnum; *promptnum < MAX_PROMPTS; (*promptnum)++)
	{
		if (!textprompts[*promptnum])
			continue;

		for (*pagenum = 0; *pagenum < textprompts[*promptnum]->numpages && *pagenum < MAX_PAGES; (*pagenum)++)
		{
			if (suffixed && fastcmp(suffixedtag, textprompts[*promptnum]->page[*pagenum].tag))
			{
				// this goes first because fastcmp ends early if first string is shorter
				found = true;
				break;
			}
			else if (nosuffixpromptnum == INT32_MAX && nosuffixpagenum == INT32_MAX && fastcmp(tag, textprompts[*promptnum]->page[*pagenum].tag))
			{
				if (suffixed)
				{
					nosuffixpromptnum = *promptnum;
					nosuffixpagenum = *pagenum;
					// continue searching for the suffixed tag
				}
				else
				{
					found = true;
					break;
				}
			}
		}

		if (found)
			break;
	}

	if (suffixed && !found && nosuffixpromptnum != INT32_MAX && nosuffixpagenum != INT32_MAX)
	{
		found = true;
		*promptnum = nosuffixpromptnum;
		*pagenum = nosuffixpagenum;
	}

	if (!found)
		CONS_Debug(DBG_GAMELOGIC, "Text prompt: Can't find a page with named tag %s or suffixed tag %s\n", tag, suffixedtag);
}

void F_TextPromptDrawer(void)
{
	// reuse:
	// cutnum -> promptnum
	// scenenum -> pagenum
	lumpnum_t iconlump;
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	// Data
	patch_t *patch;

	if (!promptactive)
		return;

	iconlump = W_CheckNumForName(textprompts[cutnum]->page[scenenum].iconname);
	F_GetPageTextGeometry(&pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	// Draw gfx first
	if (picnum >= 0 && picnum < numpics && textprompts[cutnum]->page[scenenum].picname[picnum][0] != '\0')
	{
		if (textprompts[cutnum]->page[scenenum].pichires[picnum])
			V_DrawSmallScaledPatch(picxpos, picypos, 0,
				W_CachePatchName(textprompts[cutnum]->page[scenenum].picname[picnum], PU_PATCH_LOWPRIORITY));
		else
			V_DrawScaledPatch(picxpos,picypos, 0,
				W_CachePatchName(textprompts[cutnum]->page[scenenum].picname[picnum], PU_PATCH_LOWPRIORITY));
	}

	// Draw background
	V_DrawPromptBack(boxh, textprompts[cutnum]->page[scenenum].backcolor);

	// Draw narrator icon
	if (iconlump != LUMPERROR)
	{
		INT32 iconx, icony, scale, scaledsize;
		patch = W_CachePatchName(textprompts[cutnum]->page[scenenum].iconname, PU_PATCH_LOWPRIORITY);

		// scale and center
		if (patch->width > patch->height)
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->width);
			scaledsize = FixedMul(patch->height, scale);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = ((namey-4) << FRACBITS) + FixedDiv(BASEVIDHEIGHT - namey + 4 - scaledsize, 2); // account for 4 margin
		}
		else if (patch->height > patch->width)
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->height);
			scaledsize = FixedMul(patch->width, scale);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = namey << FRACBITS;
			iconx += FixedDiv(FixedMul(patch->height, scale) - scaledsize, 2);
		}
		else
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->width);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = namey << FRACBITS;
		}

		if (textprompts[cutnum]->page[scenenum].iconflip)
			iconx += FixedMul(patch->width, scale) << FRACBITS;

		V_DrawFixedPatch(iconx, icony, scale, (V_SNAPTOBOTTOM|(textprompts[cutnum]->page[scenenum].iconflip ? V_FLIP : 0)), patch, NULL);
		W_UnlockCachedPatch(patch);
	}

	// Draw text
	V_DrawString(textx, texty, (V_SNAPTOBOTTOM|V_ALLOWLOWERCASE), cutscene_disptext);

	// Draw name
	// Don't use V_YELLOWMAP here so that the name color can be changed with control codes
	if (textprompts[cutnum]->page[scenenum].name[0])
		V_DrawString(textx, namey, (V_SNAPTOBOTTOM|V_ALLOWLOWERCASE), textprompts[cutnum]->page[scenenum].name);

	// Draw chevron
	if (promptblockcontrols && !timetonext)
		V_DrawString(textr-8, chevrony + (skullAnimCounter/5), (V_SNAPTOBOTTOM|V_YELLOWMAP), "\x1B"); // down arrow
}

#define nocontrolallowed(j) {\
		players[j].nocontrol = 1;\
		if (players[j].mo)\
		{\
			if (players[j].mo->state == states+S_KART_STILL && players[j].mo->tics != -1)\
				players[j].mo->tics++;\
		}\
	}

void F_TextPromptTicker(void)
{
	INT32 i;

	if (!promptactive || paused || P_AutoPause())
		return;

	// advance animation
	finalecount++;
	cutscene_boostspeed = 0;

	// for the chevron
	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8;

	// button handling
	if (textprompts[cutnum]->page[scenenum].timetonext)
	{
		if (promptblockcontrols) // same procedure as below, just without the button handling
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (netgame && i != serverplayer && !IsPlayerAdmin(i))
					continue;
				else
				{
					UINT8 j;

					// Both players' controls are locked,
					// But only consoleplayer can advance the prompt.
					// \todo Proper per-player splitscreen support (individual prompts)
					for (j = 0; j < g_localplayers[j]; j++)
					{
						if (i == g_localplayers[j])
							nocontrolallowed(i)
					}
				}

				if (!splitscreen)
					break;
			}
		}

		if (timetonext >= 1)
			timetonext--;

		if (!timetonext)
			F_AdvanceToNextPage();

		F_WriteText();
	}
	else
	{
		if (promptblockcontrols)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (netgame && i != serverplayer && !IsPlayerAdmin(i))
					continue;
				else
				{
					UINT8 j;

					players[i].nocontrol = 1;

					// Both players' controls are locked,
					// But only the triggering player can advance the prompt.
					for (j = 0; j <= splitscreen; j++)
					{
						if (callplayer == g_localplayers[j])
						{
							if (i != callplayer)
								continue;
						}
						else if (i != consoleplayer)
							continue;
					}
				}

				if (players[i].cmd.buttons & (BT_ACCELERATE|BT_BRAKE|BT_DRIFT))
				{
					if (timetonext > 1)
						timetonext--;
					else if (cutscene_baseptr) // don't set boost if we just reset the string
						cutscene_boostspeed = 1; // only after a slight delay

					if (keypressed)
					{
						if (!splitscreen)
							break;
						else
							continue;
					}

					if (!timetonext) // is 0 when finished generating text
					{
						F_AdvanceToNextPage();
						if (promptactive)
							S_StartSound(NULL, sfx_menu1);
					}
					keypressed = true; // prevent repeat events
				}
				else if (!(players[i].cmd.buttons & (BT_ACCELERATE|BT_BRAKE|BT_DRIFT)))
					keypressed = false;

				if (!splitscreen)
					break;
			}
		}

		// generate letter-by-letter text
		if (scenenum >= MAX_PAGES ||
			!textprompts[cutnum]->page[scenenum].text ||
			!textprompts[cutnum]->page[scenenum].text[0] ||
			!F_WriteText())
			timetonext = !promptblockcontrols; // never show the chevron if we can't toggle pages
	}

	// gfx
	if (picnum >= 0 && picnum < numpics)
	{
		if (animtimer <= 0)
		{
			boolean persistanimtimer = false;

			if (picnum < numpics-1 && textprompts[cutnum]->page[scenenum].picname[picnum+1][0] != '\0')
				picnum++;
			else if (picmode == PROMPT_PIC_LOOP)
				picnum = pictoloop;
			else if (picmode == PROMPT_PIC_DESTROY)
				picnum = -1;
			else // if (picmode == PROMPT_PIC_PERSIST)
				persistanimtimer = true;

			if (!persistanimtimer && picnum >= 0)
			{
				picxpos = textprompts[cutnum]->page[scenenum].xcoord[picnum];
				picypos = textprompts[cutnum]->page[scenenum].ycoord[picnum];
				pictime = textprompts[cutnum]->page[scenenum].picduration[picnum];
				animtimer = pictime;
			}
		}
		else
			animtimer--;
	}
}
