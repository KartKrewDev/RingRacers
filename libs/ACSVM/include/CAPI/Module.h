//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Modules.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__Module_H__
#define ACSVM__CAPI__Module_H__

#include "Types.h"

#ifdef __cplusplus
#include "../ACSVM/Module.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Types                                                                      |
//

//
// ACSVM_ModuleName
//
// ACSVM::ModuleName mirror.
//
struct ACSVM_ModuleName
{
   ACSVM_String *s;
   void         *p;
   size_t        i;
};


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

ACSVM_ModuleName ACSVM_Module_GetName(ACSVM_Module const *module);

// Returns false if reading fails.
bool ACSVM_Module_ReadBytecode(ACSVM_Module *module, ACSVM_Byte const *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Module_H__

