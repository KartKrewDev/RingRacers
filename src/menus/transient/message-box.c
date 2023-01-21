// \file menus/transient/message-box.c
// \brief MESSAGE BOX (aka: a hacked, cobbled together menu)

#include "../../k_menu.h"
#include "../../z_zone.h"

static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0, NULL, NULL, NULL, {NULL}, 0, 0}
};

menu_t MessageDef =
{
	1,					// # of menu items
	NULL,				// previous menu       (TO HACK)
	0,					// lastOn, flags       (TO HACK)
	MessageMenu,		// menuitem_t ->
	0, 0,				// x, y                (TO HACK)
	0, 0,				// extra1, extra2
	0, 0,				// transition tics
	NULL,				// drawing routine ->
	NULL,				// ticker routine
	NULL,				// init routine
	NULL,				// quit routine
	NULL				// input routine
};

// message prompt struct
struct menumessage_s menumessage;

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 8, i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] == '\n')
			h += 8;

	return h;
}

// default message handler
void M_StartMessage(const char *string, void *routine, menumessagetype_t itemtype)
{
	const UINT8 pid = 0;
	size_t max = 0, start = 0, strlines = 0, i;
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
		}
		else if (message[i] == '\n')
		{
			strlines = i;
			start = 0;
			max = 0;
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
			max -= (start-strlines)*8;
			strlines = start;
			start = 0;
		}
	}

	strncpy(menumessage.message, string, MAXMENUMESSAGE);
	menumessage.flags = itemtype;
	*(void**)&menumessage.routine = routine;
	menumessage.fadetimer = (gamestate == GS_WAITINGPLAYERS) ? 9 : 1;
	menumessage.active = true;

	start = 0;
	max = 0;

	if (!routine || menumessage.flags == MM_NOTHING)
	{
		menumessage.flags = MM_NOTHING;
		menumessage.routine = M_StopMessage;
	}

	// event routine
	if (menumessage.flags == MM_EVENTHANDLER)
	{
		*(void**)&menumessage.eroutine = routine;
		menumessage.routine = NULL;
	}

	//added : 06-02-98: now draw a textbox around the message
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0;i < strlen(message+start);i++)
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

	menumessage.x = (INT16)((BASEVIDWIDTH  - 8*max-16)/2);
	menumessage.y = (INT16)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	menumessage.m = (INT16)((strlines<<8)+max);

	M_SetMenuDelay(pid);	// Set menu delay to avoid setting off any of the handlers.
}

void M_StopMessage(INT32 choice)
{
	const char pid = 0;
	(void) choice;

	menumessage.active = false;
	M_SetMenuDelay(pid);
}

// regular handler for MM_NOTHING and MM_YESNO
void M_HandleMenuMessage(void)
{
	const UINT8 pid = 0;
	boolean btok = M_MenuConfirmPressed(pid);
	boolean btnok = M_MenuBackPressed(pid);

	if (menumessage.fadetimer < 9)
		menumessage.fadetimer++;

	switch (menumessage.flags)
	{
		// Send 1 to the routine if we're pressing A/B/X/Y
		case MM_NOTHING:
		{
			// send 1 if any button is pressed, 0 otherwise.
			if (btok || btnok)
				menumessage.routine(0);

			break;
		}
		// Send 1 to the routine if we're pressing A/X, 2 if B/Y, 0 otherwise.
		case MM_YESNO:
		{
			INT32 answer = MA_NONE;
			if (btok)
				answer = MA_YES;
			else if (btnok)
				answer = MA_NO;

			// send 1 if btok is pressed, 2 if nok is pressed, 0 otherwise.
			if (answer)
			{
				menumessage.routine(answer);
				M_StopMessage(0);
			}

			break;
		}
		// MM_EVENTHANDLER: In M_Responder to allow full event compat.
		default:
			break;
	}

	// if we detect any keypress, don't forget to set the menu delay regardless.
	if (btok || btnok)
		M_SetMenuDelay(pid);
}
