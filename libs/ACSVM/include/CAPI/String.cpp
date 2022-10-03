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

#include "ACS_String.h"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_StrHash
//
size_t ACSVM_StrHash(char const *str, size_t len)
{
   return ACSVM::StrHash(str, len);
}

//
// ACSVM_String_GetHash
//
size_t ACSVM_String_GetHash(ACSVM_String const *s)
{
   return reinterpret_cast<ACSVM::String const *>(s)->hash;
}

//
// ACSVM_String_GetIdx
//
ACSVM_Word ACSVM_String_GetIdx(ACSVM_String const *s)
{
   return reinterpret_cast<ACSVM::String const *>(s)->idx;
}

//
// ACSVM_String_GetLen
//
size_t ACSVM_String_GetLen(ACSVM_String const *s)
{
   return reinterpret_cast<ACSVM::String const *>(s)->len;
}

//
// ACSVM_String_GetLen0
//
ACSVM_Word ACSVM_String_GetLen0(ACSVM_String const *s)
{
   return reinterpret_cast<ACSVM::String const *>(s)->len0;
}

//
// ACSVM_String_GetLock
//
size_t ACSVM_String_GetLock(ACSVM_String const *s)
{
   return reinterpret_cast<ACSVM::String const *>(s)->lock;
}

//
// ACSVM_String_GetStr
//
char const *ACSVM_String_GetStr(ACSVM_String const *s)
{
   return reinterpret_cast<ACSVM::String const *>(s)->str;
}

//
// ACSVM_String_SetLock
//
void ACSVM_String_SetLock(ACSVM_String *s, size_t lock)
{
   reinterpret_cast<ACSVM::String *>(s)->lock = lock;
}

//
// ACSVM_String_SetRef
//
void ACSVM_String_SetRef(ACSVM_String *s)
{
   reinterpret_cast<ACSVM::String *>(s)->ref = true;
}

//
// ACSVM_AllocStringTable
//
ACSVM_StringTable *ACSVM_AllocStringTable(void)
{
   return reinterpret_cast<ACSVM_StringTable *>(new(std::nothrow) ACSVM::StringTable);
}

//
// ACSVM_FreeStringTable
//
void ACSVM_FreeStringTable(ACSVM_StringTable *table)
{
   delete reinterpret_cast<ACSVM::StringTable *>(table);
}

//
// ACSVM_StringTable_Clear
//
void ACSVM_StringTable_Clear(ACSVM_StringTable *table)
{
   reinterpret_cast<ACSVM::StringTable *>(table)->clear();
}

//
// ACSVM_StringTable_CollectBegin
//
void ACSVM_StringTable_CollectBegin(ACSVM_StringTable *table)
{
   reinterpret_cast<ACSVM::StringTable *>(table)->collectBegin();
}

//
// ACSVM_StringTable_CollectEnd
//
void ACSVM_StringTable_CollectEnd(ACSVM_StringTable *table)
{
   reinterpret_cast<ACSVM::StringTable *>(table)->collectEnd();
}

//
// ACSVM_StringTable_GetNone
//
ACSVM_String *ACSVM_StringTable_GetNone(ACSVM_StringTable *table)
{
   return reinterpret_cast<ACSVM_String *>(
      &reinterpret_cast<ACSVM::StringTable *>(table)->getNone());
}

//
// ACSVM_StringTable_GetStringByData
//
ACSVM_String *ACSVM_StringTable_GetStringByData(ACSVM_StringTable *table,
   char const *str, size_t len, size_t hash)
{
   return reinterpret_cast<ACSVM_String *>(
      &reinterpret_cast<ACSVM::StringTable &>(*table)[{str, len, hash}]);
}

//
// ACSVM_StringTable_GetStringByIdx
//
ACSVM_String *ACSVM_StringTable_GetStringByIdx(ACSVM_StringTable *table, ACSVM::Word idx)
{
   return reinterpret_cast<ACSVM_String *>(
      &reinterpret_cast<ACSVM::StringTable &>(*table)[idx]);
}

//
// ACSVM_StringTable_LoadState
//
void ACSVM_StringTable_LoadState(ACSVM_StringTable *table, ACSVM_IStream *in)
{
   reinterpret_cast<ACSVM::StringTable *>(table)->loadState(
      reinterpret_cast<std::istream &>(*in));
}

//
// ACSVM_StringTable_SaveState
//
void ACSVM_StringTable_SaveState(ACSVM_StringTable *table, ACSVM_OStream *out)
{
   reinterpret_cast<ACSVM::StringTable *>(table)->saveState(
      reinterpret_cast<std::ostream &>(*out));
}

}

// EOF

