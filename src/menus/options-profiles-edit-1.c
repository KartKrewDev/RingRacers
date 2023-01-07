/// \file  menus/options-profiles-edit-1.c
/// \brief Profile Editor

#include "../k_menu.h"

menuitem_t OPTIONS_EditProfile[] = {
	{IT_STRING | IT_CVAR | IT_CV_STRING, "Profile Name", "6-character long name to identify this Profile.",
		NULL, {.cvar = &cv_dummyprofilename}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Player Name", "Name displayed online when using this Profile.",
	NULL, {.cvar = &cv_dummyprofileplayername}, 0, 0},

	{IT_STRING | IT_CALL, "Character", "Default character and color for this Profile.",
		NULL, {.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "Controls", "Select the button mappings for this Profile.",
	NULL, {.routine = M_ProfileDeviceSelect}, 0, 0},

	{IT_STRING | IT_CALL, "Confirm", "Confirm changes.",
	NULL, {.routine = M_ConfirmProfile}, 0, 0},

};

menu_t OPTIONS_EditProfileDef = {
	sizeof (OPTIONS_EditProfile) / sizeof (menuitem_t),
	&OPTIONS_ProfilesDef,
	0,
	OPTIONS_EditProfile,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	2, 5,
	M_DrawEditProfile,
	M_HandleProfileEdit,
	NULL,
	NULL,
	M_ProfileEditInputs,
};
