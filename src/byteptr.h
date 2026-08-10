// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  byteptr.h
/// \brief Macros to read/write from/to a uint8_t *,
///        used for packet creation and such

#ifndef BYTEPTR_H
#define BYTEPTR_H

#if defined (__alpha__) || defined (__arm__) || defined (__mips__) || defined (__ia64__) || defined (__clang__)
#define DEALIGNED
#endif

#include "endian.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SRB2_BIG_ENDIAN
//
// Little-endian machines
//
#ifdef DEALIGNED
#define WRITEUINT8(p,b)     do {   uint8_t *p_tmp = (  uint8_t *)p; const   uint8_t tv = (  uint8_t)(b); memcpy(p, &tv, sizeof(  uint8_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITESINT8(p,b)     do {   int8_t *p_tmp = (  int8_t *)p; const   int8_t tv = (  uint8_t)(b); memcpy(p, &tv, sizeof(  uint8_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEINT16(p,b)     do {   int16_t *p_tmp = (  int16_t *)p; const   int16_t tv = (  int16_t)(b); memcpy(p, &tv, sizeof(  int16_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEUINT16(p,b)    do {  uint16_t *p_tmp = ( uint16_t *)p; const  uint16_t tv = ( uint16_t)(b); memcpy(p, &tv, sizeof( uint16_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEINT32(p,b)     do {   int32_t *p_tmp = (  int32_t *)p; const   int32_t tv = (  int32_t)(b); memcpy(p, &tv, sizeof(  int32_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEUINT32(p,b)    do {  uint32_t *p_tmp = ( uint32_t *)p; const  uint32_t tv = ( uint32_t)(b); memcpy(p, &tv, sizeof( uint32_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITECHAR(p,b)      do {    char *p_tmp = (   char *)p; const    char tv = (   char)(b); memcpy(p, &tv, sizeof(   char)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEFIXED(p,b)     do { fixed_t *p_tmp = (fixed_t *)p; const fixed_t tv = (fixed_t)(b); memcpy(p, &tv, sizeof(fixed_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEANGLE(p,b)     do { angle_t *p_tmp = (angle_t *)p; const angle_t tv = (angle_t)(b); memcpy(p, &tv, sizeof(angle_t)); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#else
#define WRITEUINT8(p,b)     do {   uint8_t *p_tmp = (  uint8_t *)p; *p_tmp = (  uint8_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITESINT8(p,b)     do {   int8_t *p_tmp = (  int8_t *)p; *p_tmp = (  int8_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEINT16(p,b)     do {   int16_t *p_tmp = (  int16_t *)p; *p_tmp = (  int16_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEUINT16(p,b)    do {  uint16_t *p_tmp = ( uint16_t *)p; *p_tmp = ( uint16_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEINT32(p,b)     do {   int32_t *p_tmp = (  int32_t *)p; *p_tmp = (  int32_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEUINT32(p,b)    do {  uint32_t *p_tmp = ( uint32_t *)p; *p_tmp = ( uint32_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITECHAR(p,b)      do {    char *p_tmp = (   char *)p; *p_tmp = (   char)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEFIXED(p,b)     do { fixed_t *p_tmp = (fixed_t *)p; *p_tmp = (fixed_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#define WRITEANGLE(p,b)     do { angle_t *p_tmp = (angle_t *)p; *p_tmp = (angle_t)(b); p_tmp++; *(void**)(&(p)) = (void *)p_tmp; } while (0)
#endif

// what is this?
#if defined (__GNUC__) && defined (DEALIGNED)
#define READUINT8(p)        ({   const uint8_t *p_tmp = (const uint8_t   *)p;   uint8_t b; memcpy(&b, p, sizeof(  uint8_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READSINT8(p)        ({   const int8_t *p_tmp = (const int8_t   *)p;   int8_t b; memcpy(&b, p, sizeof(  int8_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READINT16(p)        ({   const int16_t *p_tmp = (const int16_t   *)p;   int16_t b; memcpy(&b, p, sizeof(  int16_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READUINT16(p)       ({  const uint16_t *p_tmp = (const uint16_t  *)p;  uint16_t b; memcpy(&b, p, sizeof( uint16_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READINT32(p)        ({   const int32_t *p_tmp = (const int32_t   *)p;   int32_t b; memcpy(&b, p, sizeof(  int32_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READUINT32(p)       ({  const uint32_t *p_tmp = (const uint32_t  *)p;  uint32_t b; memcpy(&b, p, sizeof( uint32_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READCHAR(p)         ({    const char *p_tmp = (const char    *)p;    char b; memcpy(&b, p, sizeof(   char)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READFIXED(p)        ({ const fixed_t *p_tmp = (const fixed_t *)p; fixed_t b; memcpy(&b, p, sizeof(fixed_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#define READANGLE(p)        ({ const angle_t *p_tmp = (const angle_t *)p; angle_t b; memcpy(&b, p, sizeof(angle_t)); p_tmp++; *(const void**)(&(p)) = (const void *)p_tmp; b; })
#else
#define READUINT8(p)        ((const uint8_t*)  (*(const void**)(&(p)) = (const void*)&((const uint8_t*)  (p))[1]))[-1]
#define READSINT8(p)        ((const int8_t*)  (*(const void**)(&(p)) = (const void*)&((const int8_t*)  (p))[1]))[-1]
#define READINT16(p)        ((const int16_t*)  (*(const void**)(&(p)) = (const void*)&((const int16_t*)  (p))[1]))[-1]
#define READUINT16(p)       ((const uint16_t*) (*(const void**)(&(p)) = (const void*)&((const uint16_t*) (p))[1]))[-1]
#define READINT32(p)        ((const int32_t*)  (*(const void**)(&(p)) = (const void*)&((const int32_t*)  (p))[1]))[-1]
#define READUINT32(p)       ((const uint32_t*) (*(const void**)(&(p)) = (const void*)&((const uint32_t*) (p))[1]))[-1]
#define READCHAR(p)         ((const char*)   (*(const void**)(&(p)) = (const void*)&((const char*)   (p))[1]))[-1]
#define READFIXED(p)        ((const fixed_t*)(*(const void**)(&(p)) = (const void*)&((const fixed_t*)(p))[1]))[-1]
#define READANGLE(p)        ((const angle_t*)(*(const void**)(&(p)) = (const void*)&((const angle_t*)(p))[1]))[-1]
#endif

#else //SRB2_BIG_ENDIAN
//
// definitions for big-endian machines with alignment constraints.
//
// Write a value to a little-endian, unaligned destination.
//
FUNCINLINE static ATTRINLINE void writeshort(void *ptr, int32_t val)
{
	int8_t *cp = ptr;
	cp[0] = val; val >>= 8;
	cp[1] = val;
}

FUNCINLINE static ATTRINLINE void writelong(void *ptr, int32_t val)
{
	int8_t *cp = ptr;
	cp[0] = val; val >>= 8;
	cp[1] = val; val >>= 8;
	cp[2] = val; val >>= 8;
	cp[3] = val;
}

#define WRITEUINT8(p,b)     do {  uint8_t *p_tmp = (  uint8_t *)p; *p_tmp       = (  uint8_t)(b) ; p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITESINT8(p,b)     do {  int8_t *p_tmp = (  int8_t *)p; *p_tmp       = (  int8_t)(b) ; p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEINT16(p,b)     do {  int16_t *p_tmp = (  int16_t *)p; writeshort (p, (  int16_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEUINT16(p,b)    do { uint16_t *p_tmp = ( uint16_t *)p; writeshort (p, ( uint16_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEINT32(p,b)     do {  int32_t *p_tmp = (  int32_t *)p; writelong  (p, (  int32_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEUINT32(p,b)    do { uint32_t *p_tmp = ( uint32_t *)p; writelong  (p, ( uint32_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITECHAR(p,b)      do {   char *p_tmp = (   char *)p; *p_tmp       = (   char)(b) ; p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEFIXED(p,b)     do {fixed_t *p_tmp = (fixed_t *)p; writelong  (p, (fixed_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)
#define WRITEANGLE(p,b)     do {angle_t *p_tmp = (angle_t *)p; writelong  (p, (angle_t)(b)); p_tmp++; p = (void *)p_tmp;} while (0)

// Read a signed quantity from little-endian, unaligned data.
//
FUNCINLINE static ATTRINLINE int16_t readshort(void *ptr)
{
	int8_t *cp  = ptr;
	uint8_t *ucp = ptr;
	return (cp[1] << 8) | ucp[0];
}

FUNCINLINE static ATTRINLINE uint16_t readushort(void *ptr)
{
	uint8_t *ucp = ptr;
	return (ucp[1] << 8) | ucp[0];
}

FUNCINLINE static ATTRINLINE int32_t readlong(void *ptr)
{
	int8_t *cp = ptr;
	uint8_t *ucp = ptr;
	return (cp[3] << 24) | (ucp[2] << 16) | (ucp[1] << 8) | ucp[0];
}

FUNCINLINE static ATTRINLINE uint32_t readulong(void *ptr)
{
	uint8_t *ucp = ptr;
	return (ucp[3] << 24) | (ucp[2] << 16) | (ucp[1] << 8) | ucp[0];
}

#define READUINT8(p)        ((const uint8_t*)(p = (const void*)&((const uint8_t*)p)[1]))[-1]
#define READSINT8(p)        ((const int8_t*)(p = (const void*)&((const int8_t*)p)[1]))[-1]
#define READINT16(p)        readshort(&((const int16_t*)(p = (const void*)&((const int16_t*)p)[1]))[-1])
#define READUINT16(p)       readushort(&((const uint16_t*)(p = (const void*)&((const uint16_t*)p)[1]))[-1])
#define READINT32(p)        readlong(&((const int32_t*)(p = (const void*)&((const int32_t*)p)[1]))[-1])
#define READUINT32(p)       readulong(&((const uint32_t*)(p = (const void*)&((const uint32_t*)p)[1]))
#define READCHAR(p)         ((const char*)(p = (const void*)&((const char*)p)[1]))[-1]
#define READFIXED(p)        readlong(&((const fixed_t*)(p = (const void*)&((const fixed_t*)p)[1]))[-1])
#define READANGLE(p)        readulong(&((const angle_t*)(p = (const void*)&((const angle_t*)p)[1]))[-1])
#endif //SRB2_BIG_ENDIAN

#undef DEALIGNED

#define WRITESTRINGN(p, s, n) do {                          \
	size_t tmp_i;                                           \
                                                            \
	for (tmp_i = 0; tmp_i < n && s[tmp_i] != '\0'; tmp_i++) \
		WRITECHAR(p, s[tmp_i]);                             \
                                                            \
	if (tmp_i < n)                                          \
		WRITECHAR(p, '\0');                                 \
} while (0)

#define WRITESTRINGL(p, s, n) do {                              \
	size_t tmp_i;                                               \
                                                                \
	for (tmp_i = 0; tmp_i < n - 1 && s[tmp_i] != '\0'; tmp_i++) \
		WRITECHAR(p, s[tmp_i]);                                 \
                                                                \
	WRITECHAR(p, '\0');                                         \
} while (0)

#define WRITESTRING(p, s) do {                 \
	size_t tmp_i;                              \
                                               \
	for (tmp_i = 0; s[tmp_i] != '\0'; tmp_i++) \
		WRITECHAR(p, s[tmp_i]);                \
                                               \
	WRITECHAR(p, '\0');                        \
} while (0)

#define WRITEMEM(p, s, n) do { \
	memcpy(p, s, n);           \
	p += n;                    \
} while (0)

#define SKIPSTRING(p) while (READCHAR(p) != '\0')

#define SKIPSTRINGN(p, n) do {               \
	size_t tmp_i = 0;                        \
                                             \
	while (tmp_i < n && READCHAR(p) != '\0') \
		tmp_i++;                             \
} while (0)

#define SKIPSTRINGL(p, n) SKIPSTRINGN(p, n)

#define READSTRINGN(p, s, n) do {                         \
	size_t tmp_i = 0;                                     \
                                                          \
	while (tmp_i < n && (s[tmp_i] = READCHAR(p)) != '\0') \
		tmp_i++;                                          \
                                                          \
	s[tmp_i] = '\0';                                      \
} while (0)

#define READSTRINGL(p, s, n) do {                             \
	size_t tmp_i = 0;                                         \
                                                              \
	while (tmp_i < n - 1 && (s[tmp_i] = READCHAR(p)) != '\0') \
		tmp_i++;                                              \
                                                              \
	s[tmp_i] = '\0';                                          \
} while (0)

#define READSTRING(p, s) do {                \
	size_t tmp_i = 0;                        \
                                             \
	while ((s[tmp_i] = READCHAR(p)) != '\0') \
		tmp_i++;                             \
                                             \
	s[tmp_i] = '\0';                         \
} while (0)

#define READMEM(p, s, n) do { \
	memcpy(s, p, n);          \
	p += n;                   \
} while (0)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BYTEPTR_H
