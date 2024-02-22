/// \file  menus/options-profiles-edit-controls.c
/// \brief Profile Controls Editor

#include "../g_input.h"
#include "../k_menu.h"
#include "../s_sound.h"
#include "../i_joy.h" // for joystick menu controls

menuitem_t OPTIONS_ProfileControls[] = {

	{IT_HEADER, "MAIN CONTROLS", "That's the stuff on the controller!!",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL, "Accel / Confirm", "Accelerate / Confirm",
		"TLB_A", {.routine = M_ProfileSetControl}, gc_a, 0},

	{IT_CONTROL, "Look back", "Look backwards / Go back",
		"TLB_B", {.routine = M_ProfileSetControl}, gc_b, 0},

	{IT_CONTROL, "Spindash", "Spindash / Extra",
		"TLB_C", {.routine = M_ProfileSetControl}, gc_c, 0},

	{IT_CONTROL, "Brake / Go back", "Brake / Go back",
		"TLB_D", {.routine = M_ProfileSetControl}, gc_x, 0},

	{IT_CONTROL, "Respawn", "Respawn",
		"TLB_E", {.routine = M_ProfileSetControl}, gc_y, 0},

	{IT_CONTROL, "Action", "Multiplayer quick-chat / quick-vote",
		"TLB_F", {.routine = M_ProfileSetControl}, gc_z, 0},

	{IT_CONTROL, "Use Item", "Use item",
		"TLB_H", {.routine = M_ProfileSetControl}, gc_l, 0},

	{IT_CONTROL, "Drift", "Drift",
		"TLB_I", {.routine = M_ProfileSetControl}, gc_r, 0},

	{IT_CONTROL, "Turn Left", "Turn left",
		"TLB_M", {.routine = M_ProfileSetControl}, gc_left, 0},

	{IT_CONTROL, "Turn Right", "Turn right",
		"TLB_L", {.routine = M_ProfileSetControl}, gc_right, 0},

	{IT_CONTROL, "Aim Forward", "Aim forwards",
		"TLB_J", {.routine = M_ProfileSetControl}, gc_up, 0},

	{IT_CONTROL, "Aim Backwards", "Aim backwards",
		"TLB_K", {.routine = M_ProfileSetControl}, gc_down, 0},

	{IT_CONTROL, "Open pause menu", "Open pause menu",
		"TLB_G", {.routine = M_ProfileSetControl}, gc_start, 0},

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

	{IT_CONTROL, "LUA/A", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luaa, 0},

	{IT_CONTROL, "LUA/B", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luab, 0},

	{IT_CONTROL, "LUA/C", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luac, 0},

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

void M_HandleProfileControls(void)
{
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
	if (optionsmenu.bindcontrol)
	{
		optionsmenu.bindtimer--;
		if (!optionsmenu.bindtimer)
		{
			optionsmenu.bindcontrol = 0;		// we've gone past the max, just stop.
		}

	}
}

void M_ProfileTryController(INT32 choice)
{
	(void)choice;

	optionsmenu.trycontroller = TICRATE*5;

	// Apply these controls right now on P1's end.
	memcpy(&gamecontrol[0], optionsmenu.tempcontrols, sizeof(gamecontroldefault));
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
			memcpy(&gamecontrol[belongsto], optionsmenu.tempcontrols, sizeof(gamecontroldefault));
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


	// Reapply player 1's real profile.
	if (cv_currprofile.value > -1)
	{
		PR_ApplyProfile(cv_lastprofile[0].value, 0);
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
			memcpy(&gamecontrol[0], cpr->controls, sizeof(gamecontroldefault));
		}

		return true;
	}

	if (optionsmenu.bindcontrol)
		return true;	// Eat all inputs there. We'll use a stupid hack in M_Responder instead.

	//SetDeviceOnPress();	// Update device constantly so that we don't stay stuck with otpions saying a device is unavailable just because we're mapping multiple devices...

	if (M_MenuExtraPressed(pid))
	{
		// check if we're on a valid menu option...
		if (currentMenu->menuitems[itemOn].mvar1)
		{
			// clear controls for that key
			INT32 i;

			for (i = 0; i < MAXINPUTMAPPING; i++)
				optionsmenu.tempcontrols[currentMenu->menuitems[itemOn].mvar1][i] = KEY_NULL;

			S_StartSound(NULL, sfx_s3k66);
		}
		M_SetMenuDelay(pid);
		return true;
	}
	else if (M_MenuBackPressed(pid))
	{
		M_ProfileControlsConfirm(0);
		M_SetMenuDelay(pid);
		return true;
	}

	return false;
}

void M_ProfileSetControl(INT32 ch)
{
	INT32 controln = currentMenu->menuitems[itemOn].mvar1;
	UINT8 i;
	(void) ch;

	optionsmenu.bindcontrol = 1;	// Default to control #1

	for (i = 0; i < MAXINPUTMAPPING; i++)
	{
		if (optionsmenu.tempcontrols[controln][i] == KEY_NULL)
		{
			optionsmenu.bindcontrol = i+1;
			break;
		}
	}

	// If we could find a null key to map into, map there.
	// Otherwise, this will stay at 1 which means we'll overwrite the first bound control.

	optionsmenu.bindtimer = TICRATE*5;
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
	INT32 c = 0;
	UINT8 n = optionsmenu.bindcontrol-1;					// # of input to bind
	INT32 controln = currentMenu->menuitems[itemOn].mvar1;	// gc_
	UINT8 where = n;										// By default, we'll save the bind where we're supposed to map.
	INT32 i;
	INT32 *DeviceGameKeyDownArray = G_GetDeviceGameKeyDownArray(ev->device);

	if (!DeviceGameKeyDownArray)
		return;

	//SetDeviceOnPress();	// Update player gamepad assignments

	// Only consider keydown and joystick events to make sure we ignore ev_mouse and other events
	// See also G_MapEventsToControls
	switch (ev->type)
	{
		case ev_keydown:
			if (ev->data1 < NUMINPUTS)
			{
				c = ev->data1;
			}
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad downkey input %d\n", ev->data1);
			}
#endif
			break;
		case ev_gamepad_axis:
			if (ev->data1 >= JOYAXES)
			{
#ifdef PARANOIA
				CONS_Debug(DBG_GAMELOGIC, "Bad gamepad axis event %d\n", ev->data1);
#endif
				return;
			}
			else
			{
				INT32 deadzone = deadzone = (JOYAXISRANGE * cv_deadzone[0].value) / FRACUNIT; // TODO how properly account for different deadzone cvars for different devices
				boolean responsivelr = ((ev->data2 != INT32_MAX) && (abs(ev->data2) >= deadzone));
				boolean responsiveud = ((ev->data3 != INT32_MAX) && (abs(ev->data3) >= deadzone));

				i = ev->data1;

				if (i >= JOYANALOGS)
				{
					// The trigger axes are handled specially.
					i -= JOYANALOGS;

					if (responsivelr)
					{
						c = KEY_AXIS1 + (JOYANALOGS * 4) + (i * 2);
					}
					else if (responsiveud)
					{
						c = KEY_AXIS1 + (JOYANALOGS * 4) + (i * 2) + 1;
					}
				}
				else
				{
					// Actual analog sticks

					// Only consider unambiguous assignment.
					if (responsivelr == responsiveud)
						return;

					if (responsivelr)
					{
						if (ev->data2 < 0)
						{
							// Left
							c = KEY_AXIS1 + (i * 4);
						}
						else
						{
							// Right
							c = KEY_AXIS1 + (i * 4) + 1;
						}
					}
					else //if (responsiveud)
					{
						if (ev->data3 < 0)
						{
							// Up
							c = KEY_AXIS1 + (i * 4) + 2;
						}
						else
						{
							// Down
							c = KEY_AXIS1 + (i * 4) + 3;
						}
					}
				}
			}
			break;
		default:
			return;
	}

	// safety result
	if (!c)
		return;

	// Set menu delay regardless of what we're doing to avoid stupid stuff.
	M_SetMenuDelay(0);

	// Reset this input so (keyboard keys at least) are not
	// buffered and caught by menucmd.
	DeviceGameKeyDownArray[c] = 0;

	// Check if this particular key (c) is already bound in any slot.
	// If that's the case, simply do nothing.
	for (i = 0; i < MAXINPUTMAPPING; i++)
	{
		if (optionsmenu.tempcontrols[controln][i] == c)
		{
			optionsmenu.bindcontrol = 0;
			return;
		}
	}

	// With the way we do things, there cannot be instances of 'gaps' within the controls, so we don't need to pretend like we need to handle that.
	// Unless of course you tamper with the cfg file, but then it's *your* fault, not mine.

	optionsmenu.tempcontrols[controln][where] = c;
	optionsmenu.bindcontrol = 0;	// not binding anymore

	// If possible, reapply the profile...
	// 19/05/22: Actually, no, don't do that, it just fucks everything up in too many cases.

	/*
	if (gamestate == GS_MENU)	// In menu? Apply this to P1, no questions asked.
	{
		// Apply the profile's properties to player 1 but keep the last profile cv to p1's ACTUAL profile to revert once we exit.
		UINT8 lastp = cv_lastprofile[0].value;
		PR_ApplyProfile(PR_GetProfileNum(optionsmenu.profile), 0);
		CV_StealthSetValue(&cv_lastprofile[0], lastp);
	}
	else	// != GS_MENU
	{
		// ONLY apply the profile if it's in use by anything currently.
		UINT8 pnum = PR_GetProfileNum(optionsmenu.profile);
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			if (cv_lastprofile[i].value == pnum)
			{
				PR_ApplyProfile(pnum, i);
				break;
			}
		}
	}
	*/
}
#undef KEYHOLDFOR
