// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'".
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-profiles-edit-1.c
/// \brief Profile Editor

#include "../i_time.h"
#include "../k_menu.h"
#include "../s_sound.h"
#include "../m_cond.h"

// These are placed in descending order next to the things they modify, for clarity.
// Try to keep the mvar2 in order, if you add new profile info!!
menuitem_t OPTIONS_EditProfile[] = {
	{IT_STRING | IT_CVAR | IT_CV_STRING, "Profile ID", "6-character long name to display on menus.",
		NULL, {.cvar = &cv_dummyprofilename}, 0, 41},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Player Tag", "Full name, displayed online and in replays.",
		NULL, {.cvar = &cv_dummyprofileplayername}, 0, 61},

	{IT_STRING | IT_CALL, "Controls", "Change the button mappings.",
		NULL, {.routine = M_ProfileDeviceSelect}, 0, 91},

	{IT_STRING | IT_SUBMENU, "Accessibility", "Accessibility and quality of life options.",
		NULL, {.submenu = &OPTIONS_ProfileAccessibilityDef}, 0, 111},

	{IT_STRING | IT_CALL, "Character", NULL, // tooltip set in M_StartEditProfile
		NULL, {.routine = M_CharacterSelect}, 0, 131},

	{IT_STRING | IT_CALL, "Confirm", "Confirm changes.",
		NULL, {.routine = M_ConfirmProfile}, 0, 171},

};

menu_t OPTIONS_EditProfileDef = {
	sizeof (OPTIONS_EditProfile) / sizeof (menuitem_t),
	&OPTIONS_ProfilesDef,
	0,
	OPTIONS_EditProfile,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	MBF_DRAWBGWHILEPLAYING,
	"FILE",
	2, 5,
	M_DrawEditProfile,
	M_DrawOptionsCogs,
	M_HandleProfileEdit,
	NULL,
	NULL,
	M_ProfileEditInputs,
};

// Returns true if the profile can be saved, false otherwise. Also starts messages if necessary.
static boolean M_ProfileEditEnd(const UINT8 pid)
{
	UINT8 i;

	// Guest profile, you can't edit that one!
	if (optionsmenu.profilen == 0)
	{
		S_StartSound(NULL, sfx_s3k7b);
		M_StartMessage("Profiles", M_GetText("Guest profile cannot be edited.\nCreate a new profile instead."), NULL, MM_NOTHING, NULL, NULL);
		M_SetMenuDelay(pid);
		return false;
	}

	// check if some profiles have the same name
	for (i = 0; i < PR_GetNumProfiles(); i++)
	{
		profile_t *check = PR_GetProfile(i);

		// For obvious reasons don't check if our name is the same as our name....
		if (check != optionsmenu.profile)
		{
			if (!(strcmp(optionsmenu.profile->profilename, check->profilename)))
			{
				S_StartSound(NULL, sfx_s3k7b);
				M_StartMessage("Profiles", M_GetText("Another profile uses the same name.\nThis must be changed to be able to save."), NULL, MM_NOTHING, NULL, NULL);
				M_SetMenuDelay(pid);
				return false;
			}
		}
	}

	return true;
}

static void M_ProfileEditApply(void)
{
	SINT8 belongsto = PR_ProfileUsedBy(optionsmenu.profile);
	// Save the profile
	optionsmenu.profile->kickstartaccel = cv_dummyprofilekickstart.value;
	optionsmenu.profile->autoroulette = cv_dummyprofileautoroulette.value;
	optionsmenu.profile->litesteer = cv_dummyprofilelitesteer.value;
	optionsmenu.profile->strictfastfall = cv_dummyprofilestrictfastfall.value;
	optionsmenu.profile->descriptiveinput = cv_dummyprofiledescriptiveinput.value;
	optionsmenu.profile->autoring = cv_dummyprofileautoring.value;
	optionsmenu.profile->rumble = cv_dummyprofilerumble.value;
	optionsmenu.profile->fov = cv_dummyprofilefov.value;

	// If this profile is in-use by anyone, apply the changes immediately upon exiting.
	// Don't apply the full profile itself as that would lead to issues mid-game.
	if (belongsto > -1 && belongsto < MAXSPLITSCREENPLAYERS)
	{
		PR_ApplyProfileToggles(optionsmenu.profilen, belongsto);
		if (gamestate == GS_MENU)
		{
			// Safe to apply skin, etc here.
			PR_ApplyProfileLight(optionsmenu.profilen, belongsto);
		}
	}

	// Reapply player 1's real profile ID.
	if (cv_currprofile.value > -1)
	{
		PR_ApplyProfilePretend(cv_lastprofile[0].value, 0);
	}
}

static void M_ProfileEditExit(void)
{
	M_ProfileEditApply();

	if (M_GameTrulyStarted() == true)
	{
		optionsmenu.toptx = 160;
		optionsmenu.topty = 35;
		optionsmenu.resetprofile = true;	// Reset profile after the transition is done.
	}
	else
	{
		M_ResetOptions(); // Reset all options variables otherwise things are gonna go reaaal bad lol.
	}

	PR_SaveProfiles();					// save profiles after we do that.
}

// For profile edit, just make sure going back resets the card to its position, the rest is taken care of automatically.
boolean M_ProfileEditInputs(INT32 ch)
{

	(void) ch;
	const UINT8 pid = 0;

	if (M_MenuBackPressed(pid))
	{
		if (M_ProfileEditEnd(pid))
		{
			M_ProfileEditExit();
			if (cv_currprofile.value == -1)
				M_SetupNextMenu(&MAIN_ProfilesDef, false);
			else
				M_GoBack(0);
			M_SetMenuDelay(pid);
		}
		return true;
	}
	else if (M_MenuConfirmPressed(pid))
	{
		if (currentMenu->menuitems[itemOn].status & IT_TRANSTEXT)
			return true;	// No.
	}

	if (menucmd[pid].dpad_ud != 0)
	{
		optionsmenu.offset.start = I_GetTime();
	}

	return false;
}

// Handle some actions in profile editing
void M_HandleProfileEdit(void)
{
	// Always copy the profile name and player name in the profile.
	if (optionsmenu.profile && !menutyping.active)
	{
		// Copy the first 6 chars for profile name
		if (cv_dummyprofilename.string[0])
		{
			char *s;
			// convert dummyprofilename to uppercase
			strncpy(optionsmenu.profile->profilename, cv_dummyprofilename.string, PROFILENAMELEN);
			s = optionsmenu.profile->profilename;
			while (*s)
			{
				*s = toupper(*s);
				s++;
			}
		}

		if (cv_dummyprofileplayername.string[0])
		{
			strncpy(optionsmenu.profile->playername, cv_dummyprofileplayername.string, MAXPLAYERNAME);
		}
	}

	M_OptionsTick();	//  Has to be afterwards because this can unset optionsmenu.profile
}

// Confirm Profile edi via button.
void M_ConfirmProfile(INT32 choice)
{
	const UINT8 pid = 0;
	(void) choice;

	if (M_ProfileEditEnd(pid))
	{
		if (cv_currprofile.value > -1)
		{
			M_ProfileEditExit();
			M_GoBack(0);
			M_SetMenuDelay(pid);
		}
		else
		{
			M_StartMessage("Profiles", M_GetText("Are you sure you wish to\nselect this profile?\n"), &M_FirstPickProfile, MM_YESNO, NULL, NULL);
			M_SetMenuDelay(pid);
		}
	}
	return;
}

// Prompt a device selection window (just tap any button on the device you want)
void M_ProfileDeviceSelect(INT32 choice)
{
	(void)choice;

	// While we're here, setup the incoming controls menu to reset the scroll & bind status:
	optionsmenu.controlscroll = 0;
	optionsmenu.bindtimer = 0;

	optionsmenu.lastkey = 0;
	optionsmenu.keyheldfor = 0;

	optionsmenu.contx = optionsmenu.tcontx = controlleroffsets[gc_a][0];
	optionsmenu.conty = optionsmenu.tconty = controlleroffsets[gc_a][1];

	M_SetupNextMenu(&OPTIONS_ProfileControlsDef, false);	// Don't set device here anymore.
}
