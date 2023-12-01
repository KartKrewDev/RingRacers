/// \file  menus/main-goner.c
/// \brief The Goner Setup.

#include "../k_menu.h"

menuitem_t MAIN_Goner[] =
{
	{IT_STRING | IT_CVAR | IT_CV_STRING, "Password",
		"ATTEMPT ADMINISTRATOR ACCESS.", NULL,
		{.cvar = &cv_dummyextraspassword}, 0, 0},

	{IT_STRING | IT_CALL, "Quit",
		"CONCLUDE OBSERVATIONS NOW.", NULL,
		{.routine = M_QuitSRB2}, 0, 0},

	{IT_STRING | IT_CALL, "Video Options",
		"CONFIGURE OCULAR PATHWAYS.", NULL,
		={.routine = M_VideoOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Sound Options",
		"CALIBRATE AURAL DATASTREAM.", NULL, 
		{.routine = M_SoundOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Profile Setup",
		"ASSIGN VEHICLE INPUTS.", NULL,
		{.routine = M_GonerProfile}, 0, 0},

	{IT_STRING | IT_CALL, "Begin Tutorial",
		"PREPARE FOR INTEGRATION.", NULL,
		{.routine = M_GonerTutorial}, 0, 0},
};

static boolean M_GonerInputs(INT32 ch);

menu_t MAIN_GonerDef = {
	sizeof (MAIN_Goner) / sizeof (menuitem_t),
	NULL,
	0,
	MAIN_Goner,
	32, 160,
	0, 0,
	MBF_UD_LR_FLIPPED,
	"_GONER",
	0, 0,
	M_DrawHorizontalMenu,
	M_GonerTick,
	NULL,
	NULL,
	M_GonerInputs,
};

void M_GonerTick(void)
{
	if (menutyping.active == false && cv_dummyextraspassword.string[0] != '\0')
	{
		// Challenges are not interpreted at this stage.
		// See M_ExtraTick for the full behaviour.

		cht_Interpret(cv_dummyextraspassword.string);
		CV_StealthSet(&cv_dummyextraspassword, "");
	}
}

void M_GonerProfile(INT32 choice)
{
	(void)choice;

	optionsmenu.profilen = cv_ttlprofilen.value;

	const INT32 maxp = PR_GetNumProfiles();
	if (optionsmenu.profilen > maxp)
		optionsmenu.profilen = maxp;
	else if (optionsmenu.profilen < 1)
	{
		// Assume the first one is what we want..??
		CV_StealthSetValue(&cv_ttlprofilen, 1);
		optionsmenu.profilen = 1;
	}

	M_ResetOptions();

	// This will create a new profile if necessary.
	M_StartEditProfile(MA_YES);
	PR_ApplyProfilePretend(optionsmenu.profilen, 0);
}

void M_GonerTutorial(INT32 choice)
{
	(void)choice;

	if (cv_currprofile.value == -1)
	{
		const INT32 maxp = PR_GetNumProfiles();
		INT32 profilen = cv_ttlprofilen.value;
		if (profilen >= maxp)
			profilen = maxp-1;
		else if (profilen < 1)
			profilen = 1;

		PR_ApplyProfile(profilen, 0);
	}

	// Please also see M_LevelSelectInit as called in extras-1.c
	levellist.netgame = false;
	levellist.levelsearch.checklocked = true;
	cupgrid.grandprix = false;
	levellist.levelsearch.timeattack = false;

	if (!M_LevelListFromGametype(GT_TUTORIAL))
	{
		// The game is incapable of progression, but I can't bring myself to put an I_Error here.
		M_StartMessage("SURVEY_PROGRAM",
			"YOU ACCEPT EVERYTHING THAT WILL HAPPEN FROM NOW ON.",
			&M_QuitResponse, MM_YESNO, "I agree", "Cancel");
	}
}

static boolean M_GonerInputs(INT32 ch)
{
	const UINT8 pid = 0;
	(void)ch;

	if (M_MenuBackPressed(pid))
	{
		// No returning to the title screen.
		M_QuitSRB2(-1);
		return true;
	}

	return false;
}
