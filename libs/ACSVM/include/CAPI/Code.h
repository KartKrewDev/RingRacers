//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Codes.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__Code_H__
#define ACSVM__CAPI__Code_H__

#include "Types.h"

#ifdef __cplusplus
#include "../ACSVM/Code.hpp"
#endif


//----------------------------------------------------------------------------|
// Types                                                                      |
//

//
// ACSVM_Code
//
// ACSVM::Code mirror.
//
typedef enum ACSVM_Code
{
   #define ACSVM_CodeList(name, ...) ACSVM_Code_##name,
   #include "../ACSVM/CodeList.hpp"

   ACSVM_Code_None
} ACSVM_Code;

//
// ACSVM_Func
//
// ACSVM::Func mirror.
//
typedef enum ACSVM_Func
{
   #define ACSVM_FuncList(name) ACSVM_Func_##name,
   #include "../ACSVM/CodeList.hpp"

   ACSVM_Func_None
} ACSVM_Func;

//
// ACSVM_KillType
//
// ACSVM::KillType mirror.
//
typedef enum ACSVM_KillType
{
   ACSVM_KillType_None,
   ACSVM_KillType_OutOfBounds,
   ACSVM_KillType_UnknownCode,
   ACSVM_KillType_UnknownFunc,
   ACSVM_KillType_BranchLimit,
} ACSVM_KillType;

#endif//ACSVM__CAPI__Code_H__

