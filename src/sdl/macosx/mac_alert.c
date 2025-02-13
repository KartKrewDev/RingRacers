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

#include "mac_alert.h"
#include <CoreFoundation/CoreFoundation.h>

#define CFSTRINGIFY(x) CFStringCreateWithCString(NULL, x, kCFStringEncodingASCII)

int MacShowAlert(const char *title, const char *message, const char *button1, const char *button2, const char *button3)
{
	CFOptionFlags results;

        CFStringRef cf_title   = CFSTRINGIFY(title);
        CFStringRef cf_message = CFSTRINGIFY(message);
        CFStringRef cf_button1 = NULL;
        CFStringRef cf_button2 = NULL;
        CFStringRef cf_button3 = NULL;

        if (button1 != NULL)
            cf_button1 = CFSTRINGIFY(button1);
        if (button2 != NULL)
            cf_button2 = CFSTRINGIFY(button2);
        if (button3 != NULL)
            cf_button3 = CFSTRINGIFY(button3);

        CFOptionFlags alert_flags = kCFUserNotificationStopAlertLevel | kCFUserNotificationNoDefaultButtonFlag;

	CFUserNotificationDisplayAlert(0, alert_flags, NULL, NULL, NULL, cf_title, cf_message,
                                       cf_button1, cf_button2, cf_button3, &results);

        if (cf_button1 != NULL)
           CFRelease(cf_button1);
        if (cf_button2 != NULL)
           CFRelease(cf_button2);
        if (cf_button3 != NULL)
           CFRelease(cf_button3);
        CFRelease(cf_message);
        CFRelease(cf_title);

	return (int)results;
}

#endif
