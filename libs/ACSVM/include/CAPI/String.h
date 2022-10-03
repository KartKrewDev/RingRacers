//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Strings.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__String_H__
#define ACSVM__CAPI__String_H__

#include "Types.h"

#ifdef __cplusplus
#include "../ACSVM/String.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

size_t ACSVM_StrHash(char const *str, size_t len);

size_t      ACSVM_String_GetHash(ACSVM_String const *s);
ACSVM_Word  ACSVM_String_GetIdx (ACSVM_String const *s);
size_t      ACSVM_String_GetLen (ACSVM_String const *s);
ACSVM_Word  ACSVM_String_GetLen0(ACSVM_String const *s);
size_t      ACSVM_String_GetLock(ACSVM_String const *s);
char const *ACSVM_String_GetStr (ACSVM_String const *s);

void ACSVM_String_SetLock(ACSVM_String *s, size_t lock);
void ACSVM_String_SetRef (ACSVM_String *s);

ACSVM_StringTable *ACSVM_AllocStringTable(void);
void ACSVM_FreeStringTable(ACSVM_StringTable *table);

void ACSVM_StringTable_Clear(ACSVM_StringTable *table);

void ACSVM_StringTable_CollectBegin(ACSVM_StringTable *table);
void ACSVM_StringTable_CollectEnd(ACSVM_StringTable *table);

ACSVM_String *ACSVM_StringTable_GetNone(ACSVM_StringTable *table);
ACSVM_String *ACSVM_StringTable_GetStringByData(ACSVM_StringTable *table,
   char const *str, size_t len, size_t hash);
ACSVM_String *ACSVM_StringTable_GetStringByIdx(ACSVM_StringTable *table, ACSVM_Word idx);

void ACSVM_StringTable_LoadState(ACSVM_StringTable *table, ACSVM_IStream *in);

void ACSVM_StringTable_SaveState(ACSVM_StringTable *table, ACSVM_OStream *out);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__String_H__

