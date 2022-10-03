//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Scripts.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__Script_H__
#define ACSVM__CAPI__Script_H__

#include "Types.h"

#ifdef __cplusplus
#include "ACSVM/Script.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Types                                                                      |
//

//
// ACSVM_ScriptName
//
// ACSVM::ScriptName mirror.
//
struct ACSVM_ScriptName
{
   ACSVM_String *s;
   ACSVM_Word    i;
};


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Script_H__

