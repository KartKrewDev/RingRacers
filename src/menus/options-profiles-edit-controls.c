/// \file  menus/options-profiles-edit-controls.c
/// \brief Profile Controls Editor

#include "../k_menu.h"

menuitem_t OPTIONS_ProfileControls[] = {

	{IT_HEADER, "MAIN CONTROLS", "That's the stuff on the controller!!",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL, "A", "Accelerate / Confirm",
		"PR_BTA", {.routine = M_ProfileSetControl}, gc_a, 0},

	{IT_CONTROL, "B", "Look backwards / Back",
		"PR_BTB", {.routine = M_ProfileSetControl}, gc_b, 0},

	{IT_CONTROL, "C", "Spindash / Extra",
		"PR_BTC", {.routine = M_ProfileSetControl}, gc_c, 0},

	{IT_CONTROL, "X", "Brake / Back",
		"PR_BTX", {.routine = M_ProfileSetControl}, gc_x, 0},

	// @TODO What does this do???
	{IT_CONTROL, "Y", "N/A ?",
		"PR_BTY", {.routine = M_ProfileSetControl}, gc_y, 0},

	{IT_CONTROL, "Z", "N/A ?",
		"PR_BTZ", {.routine = M_ProfileSetControl}, gc_z, 0},

	{IT_CONTROL, "L", "Use item",
		"PR_BTL", {.routine = M_ProfileSetControl}, gc_l, 0},

	{IT_CONTROL, "R", "Drift",
		"PR_BTR", {.routine = M_ProfileSetControl}, gc_r, 0},

	{IT_CONTROL, "Turn Left", "Turn left",
		"PR_PADL", {.routine = M_ProfileSetControl}, gc_left, 0},

	{IT_CONTROL, "Turn Right", "Turn right",
		"PR_PADR", {.routine = M_ProfileSetControl}, gc_right, 0},

	{IT_CONTROL, "Aim Forward", "Aim forwards",
		"PR_PADU", {.routine = M_ProfileSetControl}, gc_up, 0},

	{IT_CONTROL, "Aim Backwards", "Aim backwards",
		"PR_PADD", {.routine = M_ProfileSetControl}, gc_down, 0},

	{IT_CONTROL, "Start", "Open pause menu",
		"PR_BTS", {.routine = M_ProfileSetControl}, gc_start, 0},

	{IT_HEADER, "OPTIONAL CONTROLS", "Take a screenshot, chat...",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL, "SCREENSHOT", "Also usable with F8 on Keyboard.",
		NULL, {.routine = M_ProfileSetControl}, gc_screenshot, 0},

	{IT_CONTROL, "GIF CAPTURE", "Also usable with F9 on Keyboard.",
		NULL, {.routine = M_ProfileSetControl}, gc_recordgif, 0},

	{IT_CONTROL, "OPEN CHAT", "Opens chatbox in online games.",
		NULL, {.routine = M_ProfileSetControl}, gc_talk, 0},

	{IT_CONTROL, "OPEN TEAM CHAT", "Do we even have team gamemodes?",
		NULL, {.routine = M_ProfileSetControl}, gc_teamtalk, 0},

	{IT_CONTROL, "SHOW RANKINGS", "Show mid-game rankings.",
		NULL, {.routine = M_ProfileSetControl}, gc_rankings, 0},

	{IT_CONTROL, "OPEN CONSOLE", "Opens the developer options console.",
		NULL, {.routine = M_ProfileSetControl}, gc_console, 0},

	{IT_CONTROL, "LUA/A", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luaa, 0},

	{IT_CONTROL, "LUA/B", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luab, 0},

	{IT_CONTROL, "LUA/C", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luac, 0},

	{IT_HEADER, "TOGGLES", "For per-player commands",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL | IT_CVAR, "KICKSTART ACCEL", "Hold A to auto-accel. Tap it to cancel.",
		NULL, {.cvar = &cv_dummyprofilekickstart}, 0, 0},

	{IT_HEADER, "EXTRA", "",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "TRY MAPPINGS", "Test your controls.",
		NULL, {.routine = M_ProfileTryController}, 0, 0},

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
	3, 5,
	M_DrawProfileControls,
	M_HandleProfileControls,
	NULL,
	NULL,
	M_ProfileControlsInputs,
};
