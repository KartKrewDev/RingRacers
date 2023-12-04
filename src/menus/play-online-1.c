/// \file  menus/play-online-1.c
/// \brief MULTIPLAYER OPTION SELECT

#include "../k_menu.h"
#include "../m_cond.h"
#include "../s_sound.h"
#include "../mserv.h" // cv_masterserver

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

menuitem_t PLAY_MP_OptSelect[] =
{
	{IT_STRING_CALL_NOTESTERS, "Host Game", "Start your own online game!",
		NULL, {.routine = M_PreMPHostInit}, 0, 0},

	{IT_STRING | IT_CALL, "Server Browser", "Search for game servers to play in.",
		NULL, {.routine = M_PreMPRoomSelectInit}, 0, 0},

	{IT_STRING | IT_CALL, "Join by IP", "Join an online game by its IP address.",
		NULL, {.routine = M_MPJoinIPInit}, 0, 0},
};

#undef IT_STRING_CALL_NOTESTERS


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
	-1, 1,
	M_DrawMPOptSelect,
	M_DrawEggaChannel,
	M_MPOptSelectTick,
	NULL,
	NULL,
	NULL
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
