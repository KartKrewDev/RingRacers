/// \file  menus/extras-challenges.c
/// \brief Challenges.

#include "../k_menu.h"

menuitem_t MISC_ChallengesStatsDummyMenu[] =
{
	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t MISC_ChallengesDef = {
	sizeof (MISC_ChallengesStatsDummyMenu)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_ChallengesStatsDummyMenu,
	BASEVIDWIDTH/2, 32,
	0, 0,
	98, 0,
	M_DrawChallenges,
	M_ChallengesTick,
	NULL,
	NULL,
	M_ChallengesInputs,
};

menu_t MISC_StatisticsDef = {
	sizeof (MISC_ChallengesStatsDummyMenu)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_ChallengesStatsDummyMenu,
	280, 185,
	0, 0,
	98, 0,
	M_DrawStatistics,
	NULL,
	NULL,
	NULL,
	M_StatisticsInputs,
};
