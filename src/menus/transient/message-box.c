// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
// \file menus/transient/message-box.c
// \brief MESSAGE BOX (aka: a hacked, cobbled together menu)

#include "../../k_menu.h"
#include "../../v_video.h" // V_ScaledWordWrap, HU_FONT
#include "../../z_zone.h"

// message prompt struct
struct menumessage_s menumessage;

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 16, i, len = strlen(string);

	for (i = 0; i < len-1; i++)
	{
		if (string[i] != '\n')
			continue;
		h += 8;
	}

	return h;
}

// default message handler
void M_StartMessage(const char *header, const char *string, void (*routine)(INT32), menumessagetype_t itemtype, const char *confirmstr, const char *defaultstr)
{
	const UINT8 pid = 0;
	DEBFILE(string);

	char *message = V_ScaledWordWrap(
		(BASEVIDWIDTH - 8) << FRACBITS,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0,
		HU_FONT,
		string
	);

	strncpy(menumessage.message, message, MAXMENUMESSAGE);

	Z_Free(message);

	menumessage.header = header;
	menumessage.flags = itemtype;
	menumessage.routine = routine;
	menumessage.answer = MA_NONE;
	menumessage.fadetimer = 1;
	menumessage.timer = 0;
	menumessage.closing = 0;
	menumessage.active = true;

	if (!routine)
	{
		menumessage.flags = MM_NOTHING;
	}

	// Set action strings
	switch (menumessage.flags)
	{
		// Send 1 to the routine if we're pressing A, 2 if B/X, 0 otherwise.
		case MM_YESNO:
			menumessage.defaultstr = defaultstr ? defaultstr : "No";
			menumessage.confirmstr = confirmstr ? confirmstr : "Yes";
			break;

		default:
			menumessage.defaultstr = defaultstr ? defaultstr : "OK";
			menumessage.confirmstr = NULL;
			break;
	}

	// event routine
	/*if (menumessage.flags == MM_EVENTHANDLER)
	{
		*(void**)&menumessage.eroutine = routine;
		menumessage.routine = NULL;
	}*/

	//added : 06-02-98: now draw a textbox around the message
	// oogh my god this was replaced in 2023

	menumessage.x = (8 * MAXSTRINGLENGTH) - 1;
	menumessage.y = M_StringHeight(menumessage.message);

	M_SetMenuDelay(pid);	// Set menu delay to avoid setting off any of the handlers.
}

void M_StopMessage(INT32 choice)
{
	if (!menumessage.active || menumessage.closing)
		return;

	const char pid = 0;

	// Set the answer.
	menumessage.answer = choice;

#if 1
	// The below was cool, but it felt annoyingly unresponsive.
	menumessage.closing = MENUMESSAGECLOSE+1;
#else
	// Intended length of time.
	menumessage.closing = (TICRATE/2);

	// This weird operation is necessary so the text flash is consistently timed.
	menumessage.closing |= ((2*MENUMESSAGECLOSE) - 1);
#endif

	M_SetMenuDelay(pid);
}

boolean M_MenuMessageTick(void)
{
	if (menuwipe)
		return false;

	if (menumessage.closing)
	{
		if (menumessage.closing > MENUMESSAGECLOSE)
		{
			menumessage.closing--;
		}
		else
		{
			if (menumessage.fadetimer > 0)
			{
				menumessage.fadetimer--;
			}

			if (menumessage.fadetimer == 0)
			{
				menumessage.active = false;

				if (menumessage.routine)
				{
					menumessage.routine(menumessage.answer);
				}
			}
		}

		return false;
	}
	else if (menumessage.fadetimer < 9)
	{
		menumessage.fadetimer++;
		return false;
	}

	menumessage.timer++;

	return true;
}

// regular handler for MM_NOTHING and MM_YESNO
void M_HandleMenuMessage(void)
{
	if (!M_MenuMessageTick())
		return;

	const UINT8 pid = 0;
	boolean btok = M_MenuConfirmPressed(pid);
	boolean btnok = M_MenuBackPressed(pid);

	switch (menumessage.flags)
	{
		// Send 1 to the routine if we're pressing A, 2 if B/X, 0 otherwise.
		case MM_YESNO:
		{
			if (btok)
				M_StopMessage(MA_YES);
			else if (btnok)
				M_StopMessage(MA_NO);

			break;
		}
		default:
		{
			if (btok || btnok)
				M_StopMessage(MA_NONE);

			break;
		}
	}
}
