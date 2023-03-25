/// \file  menus/play-online-room-select.c
/// \brief MULTIPLAYER ROOM SELECT MENU

#include "../k_menu.h"
#include "../s_sound.h"
#include "../m_cond.h"

menuitem_t PLAY_MP_RoomSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_MPRoomSelect}, 0, 0},
};

menu_t PLAY_MP_RoomSelectDef = {
	sizeof (PLAY_MP_RoomSelect) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_RoomSelect,
	0, 0,
	0, 0,
	0,
	"NETMD2",
	0, 0,
	M_DrawMPRoomSelect,
	M_MPRoomSelectTick,
	NULL,
	NULL,
	NULL
};

void M_MPRoomSelect(INT32 choice)
{
	const UINT8 pid = 0;
	(void) choice;

	if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);
	}
	else if (M_MenuConfirmPressed(pid))
	{
		M_ServersMenu(0);
		M_SetMenuDelay(pid);
	}
	else if (mpmenu.roomforced == false
		&& menucmd[pid].dpad_lr != 0)
	{
		mpmenu.room ^= 1;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
}

void M_MPRoomSelectTick(void)
{
	mpmenu.ticker++;
}

void M_MPRoomSelectInit(INT32 choice)
{
	(void)choice;
	mpmenu.room = (modifiedgame == true) ? 1 : 0;
	mpmenu.roomforced = ((modifiedgame == true) || (!M_SecretUnlocked(SECRET_ADDONS, true)));
	mpmenu.ticker = 0;
	mpmenu.servernum = 0;
	mpmenu.scrolln = 0;
	mpmenu.slide = 0;

	M_SetupNextMenu(&PLAY_MP_RoomSelectDef, false);
}
