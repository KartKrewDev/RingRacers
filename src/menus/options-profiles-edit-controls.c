// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by "Lat'".
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-profiles-edit-controls.c
/// \brief Profile Controls Editor

#include "../g_input.h"
#include "../k_menu.h"
#include "../s_sound.h"
#include "../i_joy.h" // for joystick menu controls

menuitem_t OPTIONS_ProfileControls[] = {

	{IT_HEADER, "DEVICE SETTINGS", "",
		NULL, {NULL}, 0, 0},

	{IT_STRING2 | IT_CVAR, "Button Display", "DESCRIPTIVEINPUT-SENTINEL",
		NULL, {.cvar = &cv_dummyprofiledescriptiveinput}, 0, 0},

	{IT_HEADER, "MAIN CONTROLS", "That's the stuff on the controller!!",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL, "Accel / Confirm", "Accelerate / Confirm",
		"TLB_A", {.routine = M_ProfileSetControl}, gc_a, 0},

	{IT_CONTROL, "Look back", "Look backwards / Go back",
		"TLB_B", {.routine = M_ProfileSetControl}, gc_b, 0},

	{IT_CONTROL, "Spin Dash", "Spin Dash / Extra",
		"TLB_C", {.routine = M_ProfileSetControl}, gc_c, 0},

	{IT_CONTROL, "Brake / Go back", "Brake / Go back",
		"TLB_X", {.routine = M_ProfileSetControl}, gc_x, 0},

	{IT_CONTROL, "Ring Bail", "Ring Bail / Burst / Time Attack Quick Restart",
		"TLB_Y", {.routine = M_ProfileSetControl}, gc_y, 0},

	{IT_CONTROL, "Action", "Quick Vote / Advance Dialogue",
		"TLB_Z", {.routine = M_ProfileSetControl}, gc_z, 0},

	{IT_CONTROL, "Use Item", "Use item",
		"TLB_L1", {.routine = M_ProfileSetControl}, gc_l, 0},

	{IT_CONTROL, "Drift", "Drift",
		"TLB_R1", {.routine = M_ProfileSetControl}, gc_r, 0},

	{IT_CONTROL, "Turn Left", "Turn left",
		"TLB_ARL", {.routine = M_ProfileSetControl}, gc_left, 0},

	{IT_CONTROL, "Turn Right", "Turn right",
		"TLB_ARR", {.routine = M_ProfileSetControl}, gc_right, 0},

	{IT_CONTROL, "Aim Forward", "Aim forwards",
		"TLB_ARU", {.routine = M_ProfileSetControl}, gc_up, 0},

	{IT_CONTROL, "Aim Backwards", "Aim backwards",
		"TLB_ARD", {.routine = M_ProfileSetControl}, gc_down, 0},

	{IT_CONTROL, "Open pause menu", "Open pause menu",
		"TLB_S", {.routine = M_ProfileSetControl}, gc_start, 0},

	{IT_HEADER, "OPTIONAL CONTROLS", "Take a screenshot, chat...",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL, "SCREENSHOT", "Take a high resolution screenshot.",
		NULL, {.routine = M_ProfileSetControl}, gc_screenshot, 0},

	{IT_CONTROL, "RECORD VIDEO", "Record a video with sound.",
		NULL, {.routine = M_ProfileSetControl}, gc_startmovie, 0},

	{IT_CONTROL, "RECORD GIF", "Record a pixel perfect GIF.",
		NULL, {.routine = M_ProfileSetControl}, gc_startlossless, 0},

	{IT_CONTROL, "SHOW RANKINGS", "Display the current rankings mid-game.",
		NULL, {.routine = M_ProfileSetControl}, gc_rankings, 0},

	{IT_CONTROL, "OPEN CHAT", "Opens full keyboard chatting for online games.",
		NULL, {.routine = M_ProfileSetControl}, gc_talk, 0},

	{IT_CONTROL, "OPEN TEAM CHAT", "Opens team-only full chat for online games.",
		NULL, {.routine = M_ProfileSetControl}, gc_teamtalk, 0},

	{IT_CONTROL, "PUSH-TO-TALK", "Activates voice chat transmission in Push-to-Talk (PTT) mode.",
		NULL, {.routine = M_ProfileSetControl}, gc_voicepushtotalk, 0},

	{IT_CONTROL, "LUA/1", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_lua1, 0},

	{IT_CONTROL, "LUA/2", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_lua2, 0},

	{IT_CONTROL, "LUA/3", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_lua3, 0},

	{IT_CONTROL, "OPEN CONSOLE", "Opens the developer options console.",
		NULL, {.routine = M_ProfileSetControl}, gc_console, 0},

	{IT_HEADER, "TEST AND CONFIRM", "",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "TRY MAPPINGS", "Test your controls.",
		NULL, {.routine = M_ProfileTryController}, 0, 0},

	{IT_STRING | IT_CALL, "RESET TO DEFAULT", "Reset all controls back to default.",
		NULL, {.routine = M_ProfileDefaultControls}, 0, 0},

	{IT_STRING | IT_CALL, "CLEAR ALL", "Unbind all controls.",
		NULL, {.routine = M_ProfileClearControls}, 0, 0},

	{IT_STRING | IT_CALL, "CONFIRM", "Go back to profile setup.",
		NULL, {.routine = M_ProfileControlsConfirm}, 0, 0},
};

menu_t OPTIONS_ProfileControlsDef = {
	sizeof (OPTIONS_ProfileControls) / sizeof (menuitem_t),
	&OPTIONS_EditProfileDef,
	0,
	OPTIONS_ProfileControls,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	MBF_DRAWBGWHILEPLAYING,
	"FILE",
	3, 5,
	M_DrawProfileControls,
	M_DrawOptionsCogs,
	M_HandleProfileControls,
	NULL,
	NULL,
	M_ProfileControlsInputs,
};

// sets whatever device has had its key pressed to the active device.
// 20/05/22: Commented out for now but not deleted as it might still find some use in the future?
/*
static void SetDeviceOnPress(void)
{
	UINT8 i;

	for (i=0; i < MAXDEVICES; i++)
	{
		if (deviceResponding[i])
		{
			G_SetDeviceForPlayer(0, i); // Force-set this joystick as the current joystick we're using for P1 (which is the only one controlling menus)
			CONS_Printf("SetDeviceOnPress: Device for %d set to %d\n", 0, i);
			return;
		}
	}
}
*/

static boolean M_ClearCurrentControl(void)
{
	// check if we're on a valid menu option...
	if (currentMenu->menuitems[itemOn].mvar1)
	{
		// clear controls for that key
		INT32 i;

		for (i = 0; i < MAXINPUTMAPPING; i++)
			optionsmenu.tempcontrols[currentMenu->menuitems[itemOn].mvar1][i] = KEY_NULL;

		return true;
	}

	return false;
}

void M_HandleProfileControls(void)
{
	const UINT8 pid = 0;
	UINT8 maxscroll = currentMenu->numitems - 5;
	M_OptionsTick();

	optionsmenu.contx += (optionsmenu.tcontx - optionsmenu.contx)/2;
	optionsmenu.conty += (optionsmenu.tconty - optionsmenu.conty)/2;

	if (abs(optionsmenu.contx - optionsmenu.tcontx) < 2 && abs(optionsmenu.conty - optionsmenu.tconty) < 2)
	{
		optionsmenu.contx = optionsmenu.tcontx;
		optionsmenu.conty = optionsmenu.tconty;	// Avoid awkward 1 px errors.
	}

	optionsmenu.controlscroll = itemOn - 3;	// very barebones scrolling, but it works just fine for our purpose.
	if (optionsmenu.controlscroll > maxscroll)
		optionsmenu.controlscroll = maxscroll;

	if (optionsmenu.controlscroll < 0)
		optionsmenu.controlscroll = 0;

	// bindings, cancel if timer is depleted.
	if (optionsmenu.bindtimer)
	{
		if (optionsmenu.bindtimer > 0)
			optionsmenu.bindtimer--;
	}
	else if (currentMenu->menuitems[itemOn].mvar1) // check if we're on a valid menu option...
	{
		// Hold right to begin clearing the control.
		//
		// If bindben timer increases enough, bindben_swallow
		// will be set.
		// This is a commitment to clear the control.
		// You can keep holding right to postpone the clear
		// but once you let go, you are locked out of
		// pressing it again until the animation finishes.
		if (menucmd[pid].dpad_lr > 0 && (optionsmenu.bindben || !optionsmenu.bindben_swallow))
		{
			optionsmenu.bindben++;
		}
		else
		{
			optionsmenu.bindben = 0;

			if (optionsmenu.bindben_swallow)
			{
				optionsmenu.bindben_swallow--;

				if (optionsmenu.bindben_swallow == 100) // special countdown for the "quick" animation
					optionsmenu.bindben_swallow = 0;
				else if (!optionsmenu.bindben_swallow) // long animation, clears control when done
					M_ClearCurrentControl();
			}
		}
	}
}

void M_ProfileTryController(INT32 choice)
{
	(void)choice;

	optionsmenu.trycontroller = TICRATE*5;

	// Apply these controls right now on P1's end.
	G_ApplyControlScheme(0, optionsmenu.tempcontrols);
}

static void M_ProfileControlSaveResponse(INT32 choice)
{
	if (choice == MA_YES)
	{
		SINT8 belongsto = PR_ProfileUsedBy(optionsmenu.profile);
		// Save the profile
		memcpy(&optionsmenu.profile->controls, optionsmenu.tempcontrols, sizeof(gamecontroldefault));

		// If this profile is in-use by anyone, apply the changes immediately upon exiting.
		// Don't apply the profile itself as that would lead to issues mid-game.
		if (belongsto > -1 && belongsto < MAXSPLITSCREENPLAYERS)
		{
			G_ApplyControlScheme(belongsto, optionsmenu.tempcontrols);
		}
	}
	else
	{
		// Revert changes
		memcpy(optionsmenu.tempcontrols, optionsmenu.profile->controls, sizeof(gamecontroldefault));
	}

	M_GoBack(0);
}

void M_ProfileControlsConfirm(INT32 choice)
{
	if (!memcmp(optionsmenu.profile->controls, optionsmenu.tempcontrols, sizeof(gamecontroldefault)))
	{
		M_GoBack(0); // no change
	}
	else if (choice == 0)
	{
		M_StartMessage(
			"Profiles",
			"You have unsaved changes to your controls.\n"
			"Please confirm if you wish to save them.\n",
			&M_ProfileControlSaveResponse,
			MM_YESNO,
			NULL,
			NULL
		);
	}
	else
		M_ProfileControlSaveResponse(MA_YES);


	// Reapply player 1's real profile ID.
	if (cv_currprofile.value > -1)
	{
		PR_ApplyProfilePretend(cv_lastprofile[0].value, 0);
	}
}

boolean M_ProfileControlsInputs(INT32 ch)
{
	const UINT8 pid = 0;
	(void)ch;

	// By default, accept all inputs.
	if (optionsmenu.trycontroller)
	{
		if (menucmd[pid].dpad_ud || menucmd[pid].dpad_lr || menucmd[pid].buttons)
		{
			if (menucmd[pid].dpad_ud != menucmd[pid].prev_dpad_ud || menucmd[pid].dpad_lr != menucmd[pid].prev_dpad_lr)
				S_StartSound(NULL, sfx_s3k5b);

			UINT32 newbuttons = menucmd[pid].buttons & ~(menucmd[pid].buttonsHeld);

			if (newbuttons & MBT_L)
				S_StartSound(NULL, sfx_kc69);
			if (newbuttons & MBT_R)
				S_StartSound(NULL, sfx_s3ka2);

			if (newbuttons & MBT_A)
				S_StartSound(NULL, sfx_kc3c);
			if (newbuttons & MBT_B)
				S_StartSound(NULL, sfx_3db09);
			if (newbuttons & MBT_C)
				S_StartSound(NULL, sfx_s1be);

			if (newbuttons & MBT_X)
				S_StartSound(NULL, sfx_s1a4);
			if (newbuttons & MBT_Y)
				S_StartSound(NULL, sfx_s3kcas);
			if (newbuttons & MBT_Z)
				S_StartSound(NULL, sfx_s3kc3s);

			if (newbuttons & MBT_START)
				S_StartSound(NULL, sfx_gshdc);

			optionsmenu.trycontroller = 5*TICRATE;
		}
		else
		{
			optionsmenu.trycontroller--;
		}

		if (optionsmenu.trycontroller == 0)
		{
			// Reset controls to that of the current profile.
			profile_t *cpr = PR_GetProfile(cv_currprofile.value);
			if (cpr == NULL)
				cpr = PR_GetProfile(0); // Creating a profile at boot, revert to guest profile
			G_ApplyControlScheme(0, cpr->controls);
		}

		return true;
	}

	if (optionsmenu.bindtimer)
		return true;	// Eat all inputs there. We'll use a stupid hack in M_Responder instead.

	//SetDeviceOnPress();	// Update device constantly so that we don't stay stuck with otpions saying a device is unavailable just because we're mapping multiple devices...

	if (M_MenuExtraPressed(pid))
	{
		if (M_ClearCurrentControl())
			S_StartSound(NULL, sfx_monch);
		optionsmenu.bindben = 0;
		optionsmenu.bindben_swallow = M_OPTIONS_BINDBEN_QUICK;
		M_SetMenuDelay(pid);
		return true;
	}
	else if (M_MenuBackPressed(pid))
	{
		M_ProfileControlsConfirm(0);
		M_SetMenuDelay(pid);
		return true;
	}

	if (menucmd[pid].dpad_ud)
	{
		if (optionsmenu.bindben_swallow)
		{
			// Control would be cleared, but we're
			// interrupting the animation so clear it
			// immediately.
			M_ClearCurrentControl();
		}
		optionsmenu.bindben = 0;
		optionsmenu.bindben_swallow = 0;
	}

	return false;
}

void M_ProfileSetControl(INT32 ch)
{
	(void) ch;

	optionsmenu.bindtimer = TICRATE*5;
	memset(optionsmenu.bindinputs, 0, sizeof optionsmenu.bindinputs);
	G_ResetAllDeviceGameKeyDown();
}

static void M_ProfileDefaultControlsResponse(INT32 ch)
{
	if (ch == MA_YES)
	{
		memcpy(&optionsmenu.tempcontrols, gamecontroldefault, sizeof optionsmenu.tempcontrols);
		S_StartSound(NULL, sfx_s24f);
	}
}

void M_ProfileDefaultControls(INT32 ch)
{
	(void)ch;
	M_StartMessage(
		"Profiles",
		"Reset all controls to the default mappings?",
		&M_ProfileDefaultControlsResponse,
		MM_YESNO,
		NULL,
		NULL
	);
}

static void M_ProfileClearControlsResponse(INT32 ch)
{
	if (ch == MA_YES)
	{
		memset(&optionsmenu.tempcontrols, 0, sizeof optionsmenu.tempcontrols);
		S_StartSound(NULL, sfx_s3k66);
	}
}

void M_ProfileClearControls(INT32 ch)
{
	(void)ch;
	M_StartMessage(
		"Profiles",
		"Clear all control bindings?",
		&M_ProfileClearControlsResponse,
		MM_YESNO,
		NULL,
		NULL
	);
}

// Map the event to the profile.

#define KEYHOLDFOR 1
void M_MapProfileControl(event_t *ev)
{
	if (ev->type == ev_keydown && ev->data2) // ignore repeating keys
		return;

	if (optionsmenu.bindtimer > TICRATE*5 - 9) // grace period after entering the bind dialog
		return;

	INT32 *DeviceGameKeyDownArray = G_GetDeviceGameKeyDownArray(ev->device);

	if (!DeviceGameKeyDownArray)
		return;

	// Find every held button.
	boolean noinput = true;
	for (INT32 c = 1; c < NUMINPUTS; ++c)
	{
		if (DeviceGameKeyDownArray[c] < 3*JOYAXISRANGE/4)
			continue;

		noinput = false;

		for (UINT8 i = 0; i < MAXINPUTMAPPING; ++i)
		{
			// If this key is already bound, don't bind it again.
			if (optionsmenu.bindinputs[i] == c)
				break;

			// Find the first available slot.
			if (!optionsmenu.bindinputs[i])
			{
				optionsmenu.bindinputs[i] = c;
				break;
			}
		}
	}

	if (noinput)
	{
		{
			// You can hold a button before entering this
			// dialog, then buffer a keyup without pressing
			// anything else. If this happens, don't wipe the
			// binds, just ignore it.
			const UINT8 zero[sizeof optionsmenu.bindinputs] = {0};
			if (!memcmp(zero, optionsmenu.bindinputs, sizeof zero))
				return;
		}

		INT32 controln = currentMenu->menuitems[itemOn].mvar1;
		memcpy(&optionsmenu.tempcontrols[controln], optionsmenu.bindinputs, sizeof optionsmenu.bindinputs);
		optionsmenu.bindtimer = 0;

		// Set menu delay regardless of what we're doing to avoid stupid stuff.
		M_SetMenuDelay(0);
	}
	else
		optionsmenu.bindtimer = -1; // prevent skip countdown
}
#undef KEYHOLDFOR
