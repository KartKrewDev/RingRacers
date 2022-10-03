//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Arrays.
//
//-----------------------------------------------------------------------------

#include "Array.h"

#include "Environment.h"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_AllocArray
//
ACSVM_Array *ACSVM_AllocArray(void)
{
   return reinterpret_cast<ACSVM_Array *>(new(std::nothrow) ACSVM::Array);
}

//
// ACSVM_FreeArray
//
void ACSVM_FreeArray(ACSVM_Array *arr)
{
   delete reinterpret_cast<ACSVM::Array *>(arr);
}

//
// ACSVM_Array_Clear
//
void ACSVM_Array_Clear(ACSVM_Array *arr)
{
   reinterpret_cast<ACSVM::Array *>(arr)->clear();
}

//
// ACSVM_Array_Find
//
ACSVM_Word ACSVM_Array_Find(ACSVM_Array const *arr, ACSVM_Word idx)
{
   return reinterpret_cast<ACSVM::Array const *>(arr)->find(idx);
}

//
// ACSVM_Array_Get
//
ACSVM_Word *ACSVM_Array_Get(ACSVM_Array *arr, ACSVM_Word idx)
{
   try
   {
      return &reinterpret_cast<ACSVM::Array &>(*arr)[idx];
   }
   catch(std::bad_alloc const &)
   {
      return nullptr;
   }
}

//
// ACSVM_Array_LoadState
//
bool ACSVM_Array_LoadState(ACSVM_Array *arr, ACSVM_Serial *in)
{
   try
   {
      reinterpret_cast<ACSVM::Array *>(arr)->loadState(
         *reinterpret_cast<ACSVM::Serial *>(in));

      return true;
   }
   catch(std::bad_alloc const &)
   {
      return false;
   }
}

//
// ACSVM_Array_LockStrings
//
void ACSVM_Array_LockStrings(ACSVM_Array const *arr, ACSVM_Environment *env)
{
   reinterpret_cast<ACSVM::Array const *>(arr)->lockStrings(env);
}

//
// ACSVM_Array_RefStrings
//
void ACSVM_Array_RefStrings(ACSVM_Array const *arr, ACSVM_Environment *env)
{
   reinterpret_cast<ACSVM::Array const *>(arr)->refStrings(env);
}

//
// ACSVM_Array_SaveState
//
void ACSVM_Array_SaveState(ACSVM_Array const *arr, ACSVM_Serial *out)
{
   reinterpret_cast<ACSVM::Array const *>(arr)->saveState(
      *reinterpret_cast<ACSVM::Serial *>(out));
}

//
// ACSVM_Array_UnlockStrings
//
void ACSVM_Array_UnlockStrings(ACSVM_Array const *arr, ACSVM_Environment *env)
{
   reinterpret_cast<ACSVM::Array const *>(arr)->unlockStrings(env);
}

}

// EOF

