// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_system.h
/// \brief System specific interface stuff.

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "d_ticcmd.h"
#include "d_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**	\brief max quit functions
*/
#define MAX_QUIT_FUNCS     16


/**	\brief Graphic system had started up
*/
extern UINT8 graphics_started;

/**	\brief Keyboard system is up and run
*/
extern UINT8 keyboard_started;

/** \brief Set to true when inside a signal handler that will exit the program. */
extern boolean g_in_exiting_signal_handler;

/**	\brief	The I_GetFreeMem function

	\param	total	total memory in the system

	\return	free memory in the system
*/
UINT32 I_GetFreeMem(UINT32 *total);

/**	\brief	Returns precise time value for performance measurement. The precise
            time should be a monotonically increasing counter, and will wrap.
			precise_t is internally represented as an unsigned integer and
			integer arithmetic may be used directly between values of precise_t.
  */
precise_t I_GetPreciseTime(void);

/** \brief  Get the precision of precise_t in units per second. Invocations of
            this function for the program's duration MUST return the same value.
  */
UINT64 I_GetPrecisePrecision(void);

/** \brief  Get the current time in rendering tics, including fractions.
*/
double I_GetFrameTime(void);

/**	\brief	Sleeps for the given duration in milliseconds. Depending on the
            operating system's scheduler, the calling thread may give up its
			time slice for a longer duration. The implementation should give a
			best effort to sleep for the given duration, without spin-locking.
			Calling code should check the current precise time after sleeping
			and not assume the thread has slept for the expected duration.

	\return	void
*/
void I_Sleep(UINT32 ms);

boolean I_CheckFrameCap(precise_t start, precise_t end);

/**	\brief Get events

	Called by D_SRB2Loop,
	called before processing each tic in a frame.
	Quick syncronous operations are performed here.
	Can call D_PostEvent.
*/
void I_GetEvent(void);

/**	\brief Asynchronous interrupt functions should maintain private queues
	that are read by the synchronous functions
	to be converted into events.
*/
void I_OsPolling(void);

/**	\brief Called by M_Responder when quit is selected, return exit code 0
*/
FUNCNORETURN void ATTRNORETURN I_Quit(void);

typedef enum
{
	EvilForce = -1,
	//Constant
	ConstantForce = 0,
	//Ramp
	RampForce,
	//Periodics
	SquareForce,
	SineForce,
	TriangleForce,
	SawtoothUpForce,
	SawtoothDownForce,
	//MAX
	NumberofForces,
} FFType;

struct JoyFF_t
{
	INT32 ForceX; ///< The X of the Force's Vel
	INT32 ForceY; ///< The Y of the Force's Vel
	//All
	UINT32 Duration; ///< The total duration of the effect, in microseconds
	INT32 Gain; //< /The gain to be applied to the effect, in the range from 0 through 10,000.
	//All, CONSTANTFORCE -10,000 to 10,000
	INT32 Magnitude; ///< Magnitude of the effect, in the range from 0 through 10,000.
	//RAMPFORCE
	INT32 Start; ///< Magnitude at the start of the effect, in the range from -10,000 through 10,000.
	INT32 End; ///< Magnitude at the end of the effect, in the range from -10,000 through 10,000.
	//PERIODIC
	INT32 Offset; ///< Offset of the effect.
	UINT32 Phase; ///< Position in the cycle of the periodic effect at which playback begins, in the range from 0 through 35,999
	UINT32 Period; ///< Period of the effect, in microseconds.
};

/**	\brief	Forcefeedback for the first joystick

	\param	Type   what kind of Effect
	\param	Effect Effect Info

	\return	void
*/

void I_Tactile(FFType Type, const JoyFF_t *Effect);

/**	\brief	Forcefeedback for the second joystick

	\param	Type   what kind of Effect
	\param	Effect Effect Info

	\return	void
*/
void I_Tactile2(FFType Type, const JoyFF_t *Effect);

/**	\brief	Forcefeedback for the third joystick

\param	Type   what kind of Effect
\param	Effect Effect Info

\return	void
*/
void I_Tactile3(FFType Type, const JoyFF_t *Effect);

/**	\brief	Forcefeedback for the fourth joystick

\param	Type   what kind of Effect
\param	Effect Effect Info

\return	void
*/
void I_Tactile4(FFType Type, const JoyFF_t *Effect);

/**	\brief to set up the first joystick scale
*/
void I_JoyScale(void);

/**	\brief to set up the second joystick scale
*/
void I_JoyScale2(void);

/**	\brief to set up the third joystick scale
*/
void I_JoyScale3(void);

/**	\brief to set up the fourth joystick scale
*/
void I_JoyScale4(void);

// Called by D_SRB2Main.

/// Startup input subsystems.
void I_StartupInput(void);

/**	\brief to startup the first joystick
*/
void I_InitJoystick1(void);

/**	\brief to startup the second joystick
*/
void I_InitJoystick2(void);

/**	\brief to startup the third joystick
*/
void I_InitJoystick3(void);

/**	\brief to startup the fourth joystick
*/
void I_InitJoystick4(void);

/**	\brief return the number of joystick on the system
*/
INT32 I_NumJoys(void);

/**	\brief	The *I_GetJoyName function

	\param	joyindex	which joystick

	\return	joystick name
*/
const char *I_GetJoyName(INT32 joyindex);

#ifndef NOMUMBLE
#include "p_mobj.h" // mobj_t
#include "s_sound.h" // listener_t
/** \brief to update Mumble of Player Postion
*/
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener);
#endif

/**	\brief Startup the mouse
*/
void I_StartupMouse(void);

/**	\brief  setup timer irq and user timer routine.
*/
void I_StartupTimer(void);

/**	\brief sample quit function
*/
typedef void (*quitfuncptr)();

/**	\brief	add a list of functions to call at program cleanup

	\param	(*func)()	funcction to call at program cleanup

	\return	void
*/
void I_AddExitFunc(void (*func)());

/**	\brief	The I_RemoveExitFunc function

	\param	(*func)()	function to remove from the list

	\return	void
*/
void I_RemoveExitFunc(void (*func)());

/**	\brief Setup signal handler, plus stuff for trapping errors and cleanly exit.
*/
INT32 I_StartupSystem(void);

/**	\brief Shutdown systems
*/
void I_ShutdownSystem(void);

/**	\brief	The I_GetDiskFreeSpace function

	\param	freespace	a INT64 pointer to hold the free space amount

	\return	void
*/
void I_GetDiskFreeSpace(INT64 *freespace);

/**	\brief find out the user's name
*/
char *I_GetUserName(void);

/**	\brief	The I_mkdir function

	\param	dirname	string of mkidr
	\param	unixright	unix right

	\return status of new folder
*/
INT32 I_mkdir(const char *dirname, INT32 unixright);

/**	\brief Find main WAD
		\return path to main WAD
*/
const char *I_LocateWad(void);

/**	\brief First Joystick's events
*/
void I_GetJoystickEvents(UINT8 index);

/**	\brief Checks if the mouse needs to be grabbed
*/
void I_UpdateMouseGrab(void);

char *I_GetEnv(const char *name);

INT32 I_PutEnv(char *variable);

/** \brief Put data in system clipboard
*/
INT32 I_ClipboardCopy(const char *data, size_t size);

/** \brief Retrieve data from system clipboard
*/
const char *I_ClipboardPaste(void);

void I_RegisterSysCommands(void);

void I_CursedWindowMovement(int xd, int yd);

boolean I_HasOpenURL(void);

void I_OpenURL(const char *data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
