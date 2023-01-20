/// \file  menus/transient/virtual-keyboard.c
/// \brief Keyboard input

#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../hu_stuff.h" // shiftxform

// Typing "sub"-menu
struct menutyping_s menutyping;

// keyboard layouts
INT16 virtualKeyboard[5][13] = {

	{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0},
	{'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0},
	{'a', 's', 'd', 'f', 'g', 'h', 'i', 'j', 'k', 'l', ';', '\'', '\\'},
	{'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0},
	{KEY_SPACE, KEY_RSHIFT, KEY_BACKSPACE, KEY_CAPSLOCK, KEY_ENTER, 0, 0, 0, 0, 0, 0, 0, 0}
};

INT16 shift_virtualKeyboard[5][13] = {

	{'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0},
	{'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0},
	{'A', 'S', 'D', 'F', 'G', 'H', 'I', 'J', 'K', 'L', ':', '\"', '|'},
	{'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0},
	{KEY_SPACE, KEY_RSHIFT, KEY_BACKSPACE, KEY_CAPSLOCK, KEY_ENTER, 0, 0, 0, 0, 0, 0, 0, 0}
};

boolean M_ChangeStringCvar(INT32 choice)
{
	consvar_t *cv = currentMenu->menuitems[itemOn].itemaction.cvar;
	char buf[MAXSTRINGLENGTH];
	size_t len;

	if (shiftdown && choice >= 32 && choice <= 127)
		choice = shiftxform[choice];

	switch (choice)
	{
		case KEY_BACKSPACE:
			len = strlen(cv->string);
			if (len > 0)
			{
				S_StartSound(NULL, sfx_s3k5b); // Tails
				M_Memcpy(buf, cv->string, len);
				buf[len-1] = 0;
				CV_Set(cv, buf);
			}
			return true;
		case KEY_DEL:
			if (cv->string[0])
			{
				S_StartSound(NULL, sfx_s3k5b); // Tails
				CV_Set(cv, "");
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(cv->string);
				if (len < MAXSTRINGLENGTH - 1)
				{
					S_StartSound(NULL, sfx_s3k5b); // Tails
					M_Memcpy(buf, cv->string, len);
					buf[len++] = (char)choice;
					buf[len] = 0;
					CV_Set(cv, buf);
				}
				return true;
			}
			break;
	}

	return false;
}

// Updates the x coordinate of the keybord so prevent it from going in weird places
static void M_UpdateKeyboardX(void)
{
	// 0s are only at the rightmost edges of the keyboard table, so just go backwards until we get something.
	while (!virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx])
		menutyping.keyboardx--;
}

static boolean M_IsTypingKey(INT32 key)
{
	return key == KEY_BACKSPACE || key == KEY_ENTER ||
		key == KEY_ESCAPE || key == KEY_DEL || isprint(key);
}

void M_MenuTypingInput(INT32 key)
{

	const UINT8 pid = 0;

	// Fade-in

	if (menutyping.menutypingclose)	// closing
	{
		menutyping.menutypingfade--;
		if (!menutyping.menutypingfade)
			menutyping.active = false;

		return;	// prevent inputs while closing the menu.
	}
	else					// opening
	{
		menutyping.menutypingfade++;
		if (menutyping.menutypingfade > 9)	// Don't fade all the way, but have it VERY strong to be readable
			menutyping.menutypingfade = 9;
		else if (menutyping.menutypingfade < 9)
			return;	// Don't allow typing until it's fully opened.
	}

	// Determine when to check for keyboard inputs or controller inputs using menuKey, which is the key passed here as argument.
	if (!menutyping.keyboardtyping)	// controller inputs
	{
		// we pressed a keyboard input that's not any of our buttons
		if (M_IsTypingKey(key) && menucmd[pid].dpad_lr == 0 && menucmd[pid].dpad_ud == 0
			&& !(menucmd[pid].buttons & MBT_A)
			&& !(menucmd[pid].buttons & MBT_B)
			&& !(menucmd[pid].buttons & MBT_C)
			&& !(menucmd[pid].buttons & MBT_X)
			&& !(menucmd[pid].buttons & MBT_Y)
			&& !(menucmd[pid].buttons & MBT_Z))
		{
			menutyping.keyboardtyping = true;
		}
	}
	else	// Keyboard inputs.
	{
		// On the flipside, if we're pressing any keyboard input, switch to controller inputs.
		if (!M_IsTypingKey(key) && (
			M_MenuButtonPressed(pid, MBT_A)
			|| M_MenuButtonPressed(pid, MBT_B)
			|| M_MenuButtonPressed(pid, MBT_C)
			|| M_MenuButtonPressed(pid, MBT_X)
			|| M_MenuButtonPressed(pid, MBT_Y)
			|| M_MenuButtonPressed(pid, MBT_Z)
			|| menucmd[pid].dpad_lr != 0
			|| menucmd[pid].dpad_ud != 0
		))
		{
			menutyping.keyboardtyping = false;
			return;
		}

		// OTHERWISE, process keyboard inputs for typing!
		if (key == KEY_ENTER || key == KEY_ESCAPE)
		{
			menutyping.menutypingclose = true;	// close menu.
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

			M_UpdateKeyboardX();
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_ud < 0) // up
		{
			menutyping.keyboardy--;
			if (menutyping.keyboardy < 0)
				menutyping.keyboardy = 4;

			M_UpdateKeyboardX();
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_lr > 0)	// right
		{
			menutyping.keyboardx++;
			if (!virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx])
				menutyping.keyboardx = 0;

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_lr < 0)	// left
		{
			menutyping.keyboardx--;
			if (menutyping.keyboardx < 0)
			{
				menutyping.keyboardx = 12;
				M_UpdateKeyboardX();
			}
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_MenuConfirmPressed(pid))
		{
			// Add the character. First though, check what we're pressing....
			INT16 c = virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx];
			if (menutyping.keyboardshift ^ menutyping.keyboardcapslock)
				c = shift_virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx];

			if (c == KEY_RSHIFT)
				menutyping.keyboardshift = !menutyping.keyboardshift;
			else if (c == KEY_CAPSLOCK)
				menutyping.keyboardcapslock = !menutyping.keyboardcapslock;
			else if (c == KEY_ENTER)
			{
				menutyping.menutypingclose = true;	// close menu.
				return;
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
