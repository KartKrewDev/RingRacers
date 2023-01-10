/// \file  menus/transient/pause-replay.c
/// \brief Replay popup menu

#include "../../k_menu.h"

menuitem_t PAUSE_PlaybackMenu[] =
{
	{IT_CALL   | IT_STRING, "Hide Menu (Esc)",			NULL, "M_PHIDE",	{.routine = M_SelectableClearMenus},	  0, 0},

	{IT_CALL   | IT_STRING, "Rewind ([)",				NULL, "M_PREW",		{.routine = M_PlaybackRewind},			 20, 0},
	{IT_CALL   | IT_STRING, "Pause (\\)",				NULL, "M_PPAUSE",	{.routine = M_PlaybackPause},			 36, 0},
	{IT_CALL   | IT_STRING, "Fast-Forward (])",			NULL, "M_PFFWD",	{.routine = M_PlaybackFastForward},		 52, 0},
	{IT_CALL   | IT_STRING, "Backup Frame ([)",			NULL, "M_PSTEPB",	{.routine = M_PlaybackRewind},			 20, 0},
	{IT_CALL   | IT_STRING, "Resume",					NULL, "M_PRESUM",	{.routine = M_PlaybackPause},			 36, 0},
	{IT_CALL   | IT_STRING, "Advance Frame (])",		NULL, "M_PFADV",	{.routine = M_PlaybackAdvance},			 52, 0},

	{IT_ARROWS | IT_STRING, "View Count (- and =)",		NULL, "M_PVIEWS",	{.routine = M_PlaybackSetViews},		 72, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint (1)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		 88, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 2 (2)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		104, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 3 (3)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		120, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 4 (4)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		136, 0},

	{IT_CALL   | IT_STRING, "Toggle Free Camera (')",	NULL, "M_PVIEWS",	{.routine = M_PlaybackToggleFreecam},	156, 0},
	{IT_CALL   | IT_STRING, "Stop Playback",			NULL, "M_PEXIT",	{.routine = M_PlaybackQuit},			172, 0},
};

menu_t PAUSE_PlaybackMenuDef = {
	sizeof (PAUSE_PlaybackMenu) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_PlaybackMenu,
	BASEVIDWIDTH/2 - 88, 2,
	0, 0,
	0, 0,
	M_DrawPlaybackMenu,
	NULL,
	NULL,
	NULL,
	NULL
};
