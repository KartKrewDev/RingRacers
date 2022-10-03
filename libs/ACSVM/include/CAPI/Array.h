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

#ifndef ACSVM__CAPI__Array_H__
#define ACSVM__CAPI__Array_H__

#include "Types.h"

#ifdef __cplusplus
#include "../ACSVM/Array.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

ACSVM_Array *ACSVM_AllocArray(void);
void ACSVM_FreeArray(ACSVM_Array *arr);

void ACSVM_Array_Clear(ACSVM_Array *arr);

ACSVM_Word ACSVM_Array_Find(ACSVM_Array const *arr, ACSVM_Word idx);

ACSVM_Word *ACSVM_Array_Get(ACSVM_Array *arr, ACSVM_Word idx);

bool ACSVM_Array_LoadState(ACSVM_Array *arr, ACSVM_Serial *in);

void ACSVM_Array_LockStrings(ACSVM_Array const *arr, ACSVM_Environment *env);

void ACSVM_Array_RefStrings(ACSVM_Array const *arr, ACSVM_Environment *env);

void ACSVM_Array_SaveState(ACSVM_Array const *arr, ACSVM_Serial *out);

void ACSVM_Array_UnlockStrings(ACSVM_Array const *arr, ACSVM_Environment *env);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Array_H__

