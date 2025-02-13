// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-online-1.c
/// \brief MULTIPLAYER OPTION SELECT

#include "../k_menu.h"
#include "../m_cond.h"
#include "../s_sound.h"
#include "../mserv.h" // cv_masterserver
#include "../m_misc.h"

#if defined (TESTERS)
	#define IT_STRING_CALL_NOTESTERS IT_DISABLED
#else
	#define IT_STRING_CALL_NOTESTERS (IT_STRING | IT_CALL)
#endif // TESTERS

static boolean firstDismissedNagThisBoot = true;

static void M_HandleMasterServerResetChoice(INT32 ch)
{
	if (ch == MA_YES)
	{
		CV_Set(&cv_masterserver, cv_masterserver.defaultvalue);
		CV_Set(&cv_masterserver_nagattempts, cv_masterserver_nagattempts.defaultvalue);
		S_StartSound(NULL, sfx_s221);
	}
	else 
	{
		if (firstDismissedNagThisBoot)
		{
			if (cv_masterserver_nagattempts.value > 0)
			{
				CV_SetValue(&cv_masterserver_nagattempts, cv_masterserver_nagattempts.value - 1);
			}
			firstDismissedNagThisBoot = false;
		}
	}
}

static void M_PreMPHostInitChoice(INT32 ch)
{
	M_HandleMasterServerResetChoice(ch);
	M_MPHostInit(0);
}

static void M_PreMPHostInit(INT32 choice)
{
	(void)choice;

	if (!CV_IsSetToDefault(&cv_masterserver) && cv_masterserver_nagattempts.value > 0)
	{
		M_StartMessage("Server Browser Alert", M_GetText("Hey! You've changed the game's\naddress for the Server Browser.\n\nYou won't be able to host games on\nthe official Server Browser.\n\nUnless you're from the future, this\nprobably isn't what you want.\n"), &M_PreMPHostInitChoice, MM_YESNO, "Fix and continue", "I changed the URL intentionally");
		return;
	}

	M_MPHostInit(0);
}

static void M_PreMPRoomSelectInitChoice(INT32 ch)
{
	M_HandleMasterServerResetChoice(ch);
	M_MPRoomSelectInit(0);
}

static void M_PreMPRoomSelectInit(INT32 choice)
{
	(void)choice;

	if (!CV_IsSetToDefault(&cv_masterserver) && cv_masterserver_nagattempts.value > 0)
	{
		M_StartMessage("Server Browser Alert", M_GetText("Hey! You've changed the game's\naddress for the Server Browser.\n\nYou won't be able to see games from\nthe official Server Browser.\n\nUnless you're from the future, this\nprobably isn't what you want.\n"), &M_PreMPRoomSelectInitChoice, MM_YESNO, "Fix and continue", "I changed the URL intentionally");
		return;
	}

	M_MPRoomSelectInit(0);
}

static const char *query_ip(const char *replace)
{
	if (replace)
		M_JoinIP(replace);
	return "";
}

static boolean uses_gamepad;

static void ip_entry(void)
{
	M_OpenVirtualKeyboard(MAXSTRINGLENGTH, query_ip, NULL);
}

static consvar_t *ip_cvar(void)
{
	extern consvar_t cv_dummyipselect;
	return &cv_dummyipselect;
}

static void confirm_ip_select(INT32 choice)
{
	if (choice == MA_YES)
	{
		consvar_t *cv = ip_cvar();
		M_JoinIP(joinedIPlist[cv->value][0]);
	}
}

static void find_ip(INT32 add)
{
	consvar_t *cv = ip_cvar();
	for (int i = 0; i < NUMLOGIP; ++i)
	{
		CV_AddValue(cv, add);
		if (*joinedIPlist[cv->value][0])
			break;
	}
}

static void direct_join_routine(INT32 choice)
{
	consvar_t *cv = ip_cvar();
	INT32 index = cv->value;

	if (choice == 2)
			ip_entry();
	else if (choice == -1)
	{
		const char *ip = joinedIPlist[index][0];
		if (*ip)
		{
			M_StartMessage("Direct Join", va("Connect to %s?", joinedIPlist[index][0]),
				&confirm_ip_select, MM_YESNO, "Connect", "Back");
		}
	}
	else
		find_ip(choice ? 1 : -1);
}

// mp_e
menuitem_t PLAY_MP_OptSelect[] =
{
	{IT_STRING_CALL_NOTESTERS, "Host Game", "Start your own online game!",
		NULL, {.routine = M_PreMPHostInit}, 0, 0},

	{IT_STRING | IT_CALL, "Browse", "Search for game servers to play in.",
		NULL, {.routine = M_PreMPRoomSelectInit}, 0, 0},

	{IT_STRING | IT_ARROWS, "Direct Join", "Join an online game by its IP address.",
		NULL, {.routine = direct_join_routine}, 0, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

#undef IT_STRING_CALL_NOTESTERS

static void draw_routine(void)
{
	M_DrawKartGamemodeMenu();
	M_DrawMasterServerReminder();
}

static boolean any_stored_ips(void)
{
	for (int i = 0; i < NUMLOGIP; ++i)
	{
		if (*joinedIPlist[i][0])
			return true;
	}
	return false;
}

static void init_routine(void)
{
	menuitem_t *it = &PLAY_MP_OptSelect[mp_directjoin];
	CV_SetValue(ip_cvar(), 0);
	if (any_stored_ips())
	{
		it->status = IT_STRING | IT_ARROWS;
		find_ip(1);
	}
	else
		it->status = IT_STRING | IT_CALL;
}

static boolean input_routine(INT32 key)
{
	uses_gamepad = (key == -1);
	return false;
}

menu_t PLAY_MP_OptSelectDef = {
	sizeof (PLAY_MP_OptSelect) / sizeof (menuitem_t),
	#if defined (TESTERS)
		&PLAY_CharSelectDef,
	#else
		&PLAY_MainDef,
	#endif
	0,
	PLAY_MP_OptSelect,
	0, 0,
	0, 0,
	0,
	"NETMD2",
	4, 5,
	draw_routine,
	M_DrawEggaChannel,
	NULL,
	init_routine,
	NULL,
	input_routine
};

struct mpmenu_s mpmenu;

// Use this as a quit routine within the HOST GAME and JOIN BY IP "sub" menus
boolean M_MPResetOpts(void)
{
	UINT8 i = 0;

	for (; i < 3; i++)
		mpmenu.modewinextend[i][0] = 0;	// Undo this

	return true;
}

void M_MPOptSelectInit(INT32 choice)
{
	INT16 arrcpy[3][3] = {{0,68,0}, {0,12,0}, {0,74,0}};
	const UINT32 forbidden = GTR_FORBIDMP;

#ifndef TESTERS
	if (choice != -1 && !M_SecretUnlocked(SECRET_ONLINE, true))
	{
		M_StartMessage("No Way? No Way!", "Online play is ""\x8B""not yet unlocked""\x80"".\n\nYou'll want experience in ""\x8B""Grand Prix""\x80""\nbefore even thinking about facing\nopponents from across the world.\n", NULL, MM_NOTHING, NULL, NULL);
		S_StartSound(NULL, sfx_s3k36);
		return;
	}
#endif

	mpmenu.modechoice = 0;
	mpmenu.ticker = 0;

	memcpy(&mpmenu.modewinextend, &arrcpy, sizeof(mpmenu.modewinextend));

	// Guarantee menugametype is good
	M_PrevMenuGametype(forbidden);
	M_NextMenuGametype(forbidden);

	if (cv_advertise.value)
	{
		// Try to have the rules available "early" for opening the Host Game menu.
		Get_rules();
	}

	if (choice != -1)
	{
		M_SetupNextMenu(&PLAY_MP_OptSelectDef, false);
	}
}

void M_MPOptSelectTick(void)
{
	UINT8 i = 0;

	// 3 Because we have 3 options in the menu
	for (; i < 3; i++)
	{
		if (mpmenu.modewinextend[i][0] != 0)
		{
			if (mpmenu.modewinextend[i][2] < (mpmenu.modewinextend[i][1] - 8))
			{
				mpmenu.modewinextend[i][2] = (((2*mpmenu.modewinextend[i][1]) + mpmenu.modewinextend[i][2])/3);
				mpmenu.modewinextend[i][2] -= (mpmenu.modewinextend[i][2] & 1); // prevent jitter, bias closed
			}
			else
			{
				mpmenu.modewinextend[i][2] = mpmenu.modewinextend[i][1];
			}
		}
		else if (mpmenu.modewinextend[i][2] > 8)
		{
			mpmenu.modewinextend[i][2] /= 3;
			mpmenu.modewinextend[i][2] += (mpmenu.modewinextend[i][2] & 1); // prevent jitter, bias open
		}
		else
		{
			mpmenu.modewinextend[i][2] = 0;
		}
	}
}
