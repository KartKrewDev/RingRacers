/// \file  menus/options-gameplay-item-toggles.c
/// \brief Random Item Toggles

#include "../k_menu.h"

menuitem_t OPTIONS_GameplayItems[] =
{
	// Mostly handled by the drawing function.
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Super Rings",			NULL, {.routine = M_HandleItemToggles}, KITEM_SUPERRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Self-Propelled Bombs",	NULL, {.routine = M_HandleItemToggles}, KITEM_SPB, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Eggman Marks",			NULL, {.routine = M_HandleItemToggles}, KITEM_EGGMAN, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Toggle All",			NULL, {.routine = M_HandleItemToggles}, 0, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers",				NULL, {.routine = M_HandleItemToggles}, KITEM_SNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers x2",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALSNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLESNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Rocket Sneakers",		NULL, {.routine = M_HandleItemToggles}, KITEM_ROCKETSNEAKER, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bananas",				NULL, {.routine = M_HandleItemToggles}, KITEM_BANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bananas x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Proximity Mines",		NULL, {.routine = M_HandleItemToggles}, KITEM_MINE, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts",				NULL, {.routine = M_HandleItemToggles}, KITEM_ORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts x4",			NULL, {.routine = M_HandleItemToggles}, KRITEM_QUADORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Land Mines",			NULL, {.routine = M_HandleItemToggles}, KITEM_LANDMINE, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz",					NULL, {.routine = M_HandleItemToggles}, KITEM_JAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz x2",				NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ballhogs",				NULL, {.routine = M_HandleItemToggles}, KITEM_BALLHOG, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Drop Targets",			NULL, {.routine = M_HandleItemToggles}, KITEM_DROPTARGET, sfx_s258},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Lightning Shields",		NULL, {.routine = M_HandleItemToggles}, KITEM_LIGHTNINGSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bubble Shields",		NULL, {.routine = M_HandleItemToggles}, KITEM_BUBBLESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Flame Shields",			NULL, {.routine = M_HandleItemToggles}, KITEM_FLAMESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Hyudoros",				NULL, {.routine = M_HandleItemToggles}, KITEM_HYUDORO, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Invinciblity",			NULL, {.routine = M_HandleItemToggles}, KITEM_INVINCIBILITY, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Grow",					NULL, {.routine = M_HandleItemToggles}, KITEM_GROW, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Shrink",				NULL, {.routine = M_HandleItemToggles}, KITEM_SHRINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pogo Springs",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_POGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Kitchen Sinks",			NULL, {.routine = M_HandleItemToggles}, KITEM_KITCHENSINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0}
};

menu_t OPTIONS_GameplayItemsDef = {
	sizeof (OPTIONS_GameplayItems) / sizeof (menuitem_t),
	&OPTIONS_GameplayDef,
	0,
	OPTIONS_GameplayItems,
	14, 40,
	SKINCOLOR_SCARLET, 0,
	2, 5,
	M_DrawItemToggles,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
