/// \file  menus/transient/virtual-keyboard.c
/// \brief Keyboard input

#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../console.h" // CON_ShiftChar
#include "../../i_system.h" // I_Clipboard funcs

// Typing "sub"-menu
struct menutyping_s menutyping;

// keyboard layouts
INT16 virtualKeyboard[5][NUMVIRTUALKEYSINROW] = {

	{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  KEY_BACKSPACE, 1},
	{'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '-', '='},
	{'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '/',  '[', ']'},
	{'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '\\', ';', '\''},
	{KEY_RSHIFT, 1, 1, KEY_SPACE, 1, 1, 1, 1, KEY_ENTER, 1, 1, 1}
};

INT16 shift_virtualKeyboard[5][NUMVIRTUALKEYSINROW] = {

	{'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', KEY_BACKSPACE, 1},
	{'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '_', '+'},
	{'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '?', '{', '}'},
	{'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '|', ':', '\"'},
	{KEY_RSHIFT, 1, 1, KEY_SPACE, 1, 1, 1, 1, KEY_ENTER, 1, 1, 1}
};

typedef enum
{
	CVCPM_NONE,
	CVCPM_COPY,
	CVCPM_CUT,
	CVCPM_PASTE
} cvarcopypastemode_t;

boolean M_ChangeStringCvar(INT32 choice)
{
	size_t len;
	cvarcopypastemode_t copypastemode = CVCPM_NONE;

	if (menutyping.keyboardtyping == true)
	{
		// We can only use global modifiers in key mode.

		if (ctrldown)
		{
			if (choice == 'c' || choice == 'C' || choice == KEY_INS)
			{
				// ctrl+c, ctrl+insert, copying
				copypastemode = CVCPM_COPY;
			}
			else if (choice == 'x' || choice == 'X')
			{
				// ctrl+x, cutting
				copypastemode = CVCPM_CUT;
			}
			else if (choice == 'v' || choice == 'V')
			{
				// ctrl+v, pasting
				copypastemode = CVCPM_PASTE;
			}
			else
			{
				// not a known ctrl code
				return false;
			}
		}
		else if (shiftdown)
		{
			if (choice == KEY_INS)
			{
				// shift+insert, pasting
				copypastemode = CVCPM_PASTE;
			}
			else if (choice == KEY_DEL)
			{
				// shift+delete, cutting
				copypastemode = CVCPM_CUT;
			}
		}

		if (copypastemode != CVCPM_NONE)
		{
			len = strlen(menutyping.cache);

			if (copypastemode == CVCPM_PASTE)
			{
				const char *paste = I_ClipboardPaste();
				if (paste == NULL || paste[0] == '\0')
					;
				else if (len < MAXSTRINGLENGTH - 1)
				{
					strlcat(menutyping.cache, paste, MAXSTRINGLENGTH);

					S_StartSound(NULL, sfx_tmxbdn); // Tails
				}
			}
			else if (len > 0 /*&& (copypastemode == CVCPM_COPY
				|| copypastemode == CVCPM_CUT)*/
				)
			{
				I_ClipboardCopy(menutyping.cache, len);

				if (copypastemode == CVCPM_CUT)
				{
					// A cut should wipe.
					strcpy(menutyping.cache, "");
					S_StartSound(NULL, sfx_tmxbup); // Tails
				}
				else
				{
					S_StartSound(NULL, sfx_tmxbdn); // Tails
				}
			}

			return true;
		}

		// Okay, now we can auto-modify the character.
		choice = CON_ShiftChar(choice);
	}

	switch (choice)
	{
		case KEY_BACKSPACE:
			if (menutyping.cache[0])
			{
				len = strlen(menutyping.cache);
				menutyping.cache[len - 1] = 0;

				S_StartSound(NULL, sfx_tmxbup); // Tails
			}
			return true;
		case KEY_DEL:
			if (menutyping.cache[0])
			{
				strcpy(menutyping.cache, "");

				S_StartSound(NULL, sfx_tmxbup); // Tails
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(menutyping.cache);
				if (len < MAXSTRINGLENGTH - 1)
				{
					menutyping.cache[len++] = (char)choice;
					menutyping.cache[len] = 0;

					S_StartSound(NULL, sfx_tmxbdn); // Tails
				}
				return true;
			}
			break;
	}

	return false;
}

static void M_ToggleVirtualShift(void)
{
	if (menutyping.keyboardcapslock == true)
	{
		menutyping.keyboardcapslock = false;
	}
	else
	{
		menutyping.keyboardshift ^= true;
		if (menutyping.keyboardshift == false)
		{
			menutyping.keyboardcapslock = true;
		}
	}
}

static void M_CloseVirtualKeyboard(void)
{
	menutyping.menutypingclose = true;	// close menu.
	CV_Set(currentMenu->menuitems[itemOn].itemaction.cvar, menutyping.cache);
}

static boolean M_IsTypingKey(INT32 key)
{
	return key == KEY_BACKSPACE || key == KEY_ENTER
		|| key == KEY_ESCAPE || key == KEY_DEL
		|| key == KEY_LCTRL || key == KEY_RCTRL
		|| isprint(key);
}

void M_MenuTypingInput(INT32 key)
{
	const UINT8 pid = 0;

	// Fade-in

	if (menutyping.menutypingclose)
	{
		// Closing
		menutyping.menutypingfade--;
		if (!menutyping.menutypingfade)
			menutyping.active = false;

		return;	// prevent inputs while closing the menu.
	}
	else
	{
		// Opening
		const UINT8 destination = (menutyping.keyboardtyping ? 9 : 18);

		if (menutyping.menutypingfade > destination)
		{
			menutyping.menutypingfade--;
		}
		else if (menutyping.menutypingfade < destination)
		{
			menutyping.menutypingfade++;
		}

		if (menutyping.menutypingfade != destination)
		{
			// Don't allow typing until it's fully opened.
			return;
		}
	}

	// Determine when to check for keyboard inputs or controller inputs using menuKey, which is the key passed here as argument.
	if (!menutyping.keyboardtyping)	// controller inputs
	{
		// we pressed a keyboard input that's not any of our buttons
		if (key >= 0 && M_IsTypingKey(key) && menucmd[pid].dpad_lr == 0 && menucmd[pid].dpad_ud == 0
			&& !(menucmd[pid].buttons & MBT_A)
			&& !(menucmd[pid].buttons & MBT_B)
			&& !(menucmd[pid].buttons & MBT_C)
			&& !(menucmd[pid].buttons & MBT_X)
			&& !(menucmd[pid].buttons & MBT_Y)
			&& !(menucmd[pid].buttons & MBT_Z)
			&& !(menucmd[pid].buttons & MBT_START))
		{
			menutyping.keyboardtyping = true;
		}
	}
	else	// Keyboard inputs.
	{
		// On the flipside, if we're pressing any keyboard input, switch to controller inputs.
		if (key >= 0 && !M_IsTypingKey(key) && (
			M_MenuButtonPressed(pid, MBT_A)
			|| M_MenuButtonPressed(pid, MBT_B)
			|| M_MenuButtonPressed(pid, MBT_C)
			|| M_MenuButtonPressed(pid, MBT_X)
			|| M_MenuButtonPressed(pid, MBT_Y)
			|| M_MenuButtonPressed(pid, MBT_Z)
			|| M_MenuButtonPressed(pid, MBT_START)
			|| (menucmd[pid].dpad_lr != 0 && menucmd[pid].prev_dpad_lr == 0)
			|| (menucmd[pid].dpad_ud != 0 && menucmd[pid].prev_dpad_ud != 0)
		))
		{
			/*CONS_Printf("key is %d, \
				%c%c%c-%c%c%c-%c-%c%c%c%c\n",
				key,
				M_MenuButtonPressed(pid, MBT_A) ? 'A' : ' ',
				M_MenuButtonPressed(pid, MBT_B) ? 'B' : ' ',
				M_MenuButtonPressed(pid, MBT_C) ? 'C' : ' ',
				M_MenuButtonPressed(pid, MBT_X) ? 'X' : ' ',
				M_MenuButtonPressed(pid, MBT_Y) ? 'Y' : ' ',
				M_MenuButtonPressed(pid, MBT_Z) ? 'Z' : ' ',
				M_MenuButtonPressed(pid, MBT_START) ? '+' : ' ',
				menucmd[pid].dpad_lr < 0 ? '<' : ' ',
				menucmd[pid].dpad_ud > 0 ? '^' : ' ',
				menucmd[pid].dpad_ud < 0 ? 'v' : ' ',
				menucmd[pid].dpad_lr > 0 ? '>' : ' '
				);*/
			menutyping.keyboardtyping = false;
			return;
		}

		// OTHERWISE, process keyboard inputs for typing!
		if (key == KEY_ENTER || key == KEY_ESCAPE)
		{
			M_CloseVirtualKeyboard();

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);

			return;
		}
	}

	if (menucmd[pid].delay == 0 && !menutyping.keyboardtyping)	// We must check for this here because we bypass the normal delay check to allow for normal keyboard inputs
	{
		if (menucmd[pid].dpad_ud > 0)	// down
		{
			menutyping.keyboardy++;
			if (menutyping.keyboardy > 4)
				menutyping.keyboardy = 0;

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_ud < 0) // up
		{
			menutyping.keyboardy--;
			if (menutyping.keyboardy < 0)
				menutyping.keyboardy = 4;

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_lr > 0)	// right
		{
			do
			{
				menutyping.keyboardx++;
				if (menutyping.keyboardx > NUMVIRTUALKEYSINROW-1)
				{
					menutyping.keyboardx = 0;
					break;
				}
			}
			while (virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx] == 1);

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_lr < 0)	// left
		{
			while (virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx] == 1)
			{
				menutyping.keyboardx--;
				if (menutyping.keyboardx < 0)
				{
					menutyping.keyboardx = NUMVIRTUALKEYSINROW-1;
					break;
				}
			}

			menutyping.keyboardx--;
			if (menutyping.keyboardx < 0)
			{
				menutyping.keyboardx = NUMVIRTUALKEYSINROW-1;
			}

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_MenuButtonPressed(pid, MBT_START))
		{
			// Shortcut for close menu.
			M_CloseVirtualKeyboard();

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_MenuBackPressed(pid))
		{
			// Shortcut for backspace.
			M_ChangeStringCvar(KEY_BACKSPACE);

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_MenuExtraPressed(pid))
		{
			// Shortcut for shift/caps lock.
			M_ToggleVirtualShift();

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_MenuConfirmPressed(pid))
		{
			// Add the character. First though, check what we're pressing....
			INT32 tempkeyboardx = menutyping.keyboardx;
			INT16 c = 0;
			while ((c = virtualKeyboard[menutyping.keyboardy][tempkeyboardx]) == 1
			&& tempkeyboardx > 0)
				tempkeyboardx--;

			if (c > 1)
			{
				if (menutyping.keyboardshift ^ menutyping.keyboardcapslock)
					c = shift_virtualKeyboard[menutyping.keyboardy][tempkeyboardx];

				if (c == KEY_RSHIFT)
				{
					M_ToggleVirtualShift();
				}
				else if (c == KEY_ENTER)
				{
					M_CloseVirtualKeyboard();
				}
				else
				{
					M_ChangeStringCvar((INT32)c);	// Write!
					menutyping.keyboardshift = false;			// undo shift if it had been pressed
				}

				M_SetMenuDelay(pid);
				S_StartSound(NULL, sfx_s3k5b);
			}
		}
	}
}

void M_OpenVirtualKeyboard(boolean gamepad)
{
	menutyping.keyboardtyping = !gamepad;
	menutyping.active = true;
	menutyping.menutypingclose = false;

	strlcpy(menutyping.cache, currentMenu->menuitems[itemOn].itemaction.cvar->string, MAXSTRINGLENGTH);
}
