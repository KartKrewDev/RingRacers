// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#define LOG_TAG "SRB2"

#include "../doomdef.h"
#include "../i_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utils/Log.h>

#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMFREE "MemFree:"

uint8_t graphics_started = 0;

uint8_t keyboard_started = 0;

static int64_t start_time; // as microseconds since the epoch

// I should probably return how much memory is remaining
// for this process, considering Android's process memory limit.
uint64_t I_GetFreeMem(uint64_t *total)
{
  // what the heck?  sysinfo() is partially missing in bionic?
  /* struct sysinfo si; */
  /* if(sysinfo(&si) != 0) { */
  /*   I_Error("Couldn't invoke sysinfo()...?"); */
  /* } */
  /* return si.freeram; */
  char buf[1024];
  char *memTag;
  uint64_t freeKBytes;
  uint64_t totalKBytes;
  int32_t n;
  int32_t meminfo_fd = -1;

  meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
  n = read(meminfo_fd, buf, 1023);
  close(meminfo_fd);

  if (n < 0)
    {
      // Error
      *total = 0;
      return 0;
    }

  buf[n] = '\0';
  if (NULL == (memTag = strstr(buf, MEMTOTAL)))
    {
      // Error
      *total = 0;
      return 0;
    }

  memTag += sizeof (MEMTOTAL);
  totalKBytes = atoi(memTag);

  if (NULL == (memTag = strstr(buf, MEMFREE)))
    {
      // Error
      *total = 0;
      return 0;
    }

  memTag += sizeof (MEMFREE);
  freeKBytes = atoi(memTag);

  if (total)
    *total = totalKBytes << 10;
  return freeKBytes << 10;
}

int64_t current_time_in_ps() {
  struct timeval t;
  gettimeofday(&t, NULL);
  return (t.tv_sec * (int64_t)1000000) + t.tv_usec;
}

void I_Sleep(uint32_t ms){}

precise_t I_GetPreciseTime(void)
{
	return 0;
}

uint64_t I_GetPrecisePrecision(void)
{
	return 1000000;
}

void I_GetEvent(void){}

void I_OsPolling(void){}

ticcmd_t *I_BaseTiccmd(void)
{
  return NULL;
}

ticcmd_t *I_BaseTiccmd2(void)
{
  return NULL;
}

void I_Quit(void)
{
  LOGD("SRB2Kart quitting!");
  exit(0);
}

void I_Error(const char *error, ...)
{
  va_list argptr;
  char logbuf[8192];

  va_start(argptr, error);
  vsnprintf(logbuf, sizeof(logbuf), error, argptr);
  va_end(argptr);

  LOGE(logbuf);
  exit(-1);
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
  (void)Type;
  (void)Effect;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
  (void)Type;
  (void)Effect;
}

void I_JoyScale(void){}

void I_JoyScale2(void){}

void I_InitJoystick(void){}

void I_InitJoystick2(void){}

int32_t I_NumJoys(void)
{
  return 0;
}

const char *I_GetJoyName(int32_t joyindex)
{
  (void)joyindex;
  return NULL;
}

void I_SetupMumble(void)
{
}

#ifndef NOMUMBLE
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
	(void)mobj;
	(void)listener;
}
#endif

void I_OutputMsg(const char *fmt, ...)
{
  va_list argptr;
  char logbuf[8192];

  va_start(argptr, fmt);
  vsnprintf(logbuf, sizeof(logbuf), fmt, argptr);
  va_end(argptr);

  LOGD(logbuf);
}

void I_StartupMouse(void){}

void I_StartupMouse2(void){}

void I_StartupKeyboard(void){}

int32_t I_GetKey(void)
{
  return 0;
}

void I_StartupTimer(void) {
  struct timeval t;
  gettimeofday(&t, NULL);
  start_time = (t.tv_sec * 1000000) + t.tv_usec;
}

void I_AddExitFunc(void (*func)())
{
  (void)func;
}

void I_RemoveExitFunc(void (*func)())
{
  (void)func;
}

int32_t I_StartupSystem(void)
{
  return -1;
}

void I_ShutdownSystem(void){}

void I_GetDiskFreeSpace(int64_t* freespace)
{
  *freespace = 0;
}

char *I_GetUserName(void)
{
  return "Android";
}

int32_t I_mkdir(const char *dirname, int32_t unixright)
{
  (void)dirname;
  (void)unixright;
  return -1;
}

const char *I_LocateWad(void)
{
  return "/sdcard/srb2";
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

void I_UpdateMouseGrab(void){}

char *I_GetEnv(const char *name)
{
  LOGW("I_GetEnv() called?!");
  (void)name;
  return NULL;
}

int32_t I_PutEnv(char *variable)
{
  (void)variable;
  return -1;
}

int32_t I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

char *I_ClipboardPaste(void)
{
	return NULL;
}

void I_RegisterSysCommands(void) {}

#include "../sdl/dosstr.c"
