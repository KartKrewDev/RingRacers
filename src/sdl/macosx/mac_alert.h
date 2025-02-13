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
/// \file
/// \brief Graphical Alerts for MacOSX
///
///	Shows alerts, since we can't just print these to the screen when
///	launched graphically on a mac.

#ifdef __APPLE_CC__

extern int MacShowAlert(const char *title, const char *message, const char *button1, const char *button2, const char *button3);

#endif
