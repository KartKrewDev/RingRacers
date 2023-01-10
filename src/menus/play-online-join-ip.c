/// \file  menus/play-online-join-ip.c
/// \brief MULTIPLAYER JOIN BY IP

#include "../k_menu.h"

menuitem_t PLAY_MP_JoinIP[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "IP: ", "Type the IPv4 address of the server.",
		NULL, {.cvar = &cv_dummyip}, 0, 0},

	{IT_STRING, "CONNECT ", "Attempt to connect to the server you entered the IP for.",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "LAST IPs JOINED:", "Kanade best waifu :)",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip1", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip2", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip3", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

};

menu_t PLAY_MP_JoinIPDef = {
	sizeof (PLAY_MP_JoinIP) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_JoinIP,
	0, 0,
	0, 0,
	-1, 1,	// 1 frame transition.... This is really just because I don't want the black fade when we press esc, hehe
	M_DrawMPJoinIP,
	M_MPOptSelectTick,	// This handles the unfolding options
	NULL,
	M_MPResetOpts,
	M_JoinIPInputs
};
