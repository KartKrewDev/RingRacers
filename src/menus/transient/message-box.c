// \file menus/transient/message-box.c
// \brief MESSAGE BOX (aka: a hacked, cobbled together menu)

#include "../../k_menu.h"
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
	size_t max = 0, maxatstart = 0, start = 0, strlines, i;
	static char *message = NULL;
	Z_Free(message);
	message = Z_StrDup(string);
	DEBFILE(message);

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, etc. but who cares.
	for (i = 0; message[i]; i++)
	{
		if (message[i] == ' ')
		{
			start = i;
			max += 4;
			maxatstart = max;
		}
		else if (message[i] == '\n')
		{
			start = 0;
			max = 0;
			maxatstart = 0;
			continue;
		}
		else if (message[i] & 0x80)
			continue;
		else
			max += 8;

		// Start trying to wrap if presumed length exceeds the screen width.
		if (max >= BASEVIDWIDTH && start > 0)
		{
			message[start] = '\n';
			max -= maxatstart;
			start = 0;
		}
	}

	strncpy(menumessage.message, string, MAXMENUMESSAGE);
	menumessage.header = header;
	menumessage.flags = itemtype;
	menumessage.routine = routine;
	menumessage.answer = MA_NONE;
	menumessage.fadetimer = 1;
	menumessage.timer = 0;
	menumessage.closing = 0;
	menumessage.active = true;

	start = 0;
	max = 0;

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
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0; i < strlen(message+start);i++)
		{
			if (*(message+start+i) == '\n')
			{
				if (i > max)
					max = i;
				start += i;
				i = (size_t)-1; //added : 07-02-98 : damned!
				start++;
				break;
			}
		}

		if (i == strlen(message+start))
		{
			start += i;
			if (i > max)
				max = i;
		}
	}

	menumessage.x = (8 * MAXSTRINGLENGTH) - 1;
	menumessage.y = M_StringHeight(message);

	M_SetMenuDelay(pid);	// Set menu delay to avoid setting off any of the handlers.
}

void M_StopMessage(INT32 choice)
{
	if (!menumessage.active || menumessage.closing)
		return;

	const char pid = 0;

	// Set the answer.
	menumessage.answer = choice;

	// Intended length of time.
	menumessage.closing = (TICRATE/2);

	// This weird operation is necessary so the text flash is consistently timed.
	menumessage.closing |= ((2*MENUMESSAGECLOSE) - 1);

	M_SetMenuDelay(pid);
}

boolean M_MenuMessageTick(void)
{
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
