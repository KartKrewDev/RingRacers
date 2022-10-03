//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Scopes.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__Scope_H__
#define ACSVM__CAPI__Scope_H__

#include "Types.h"

#ifdef __cplusplus
#include "../ACSVM/Scope.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Types                                                                      |
//

//
// ACSVM_ScopeID
//
struct ACSVM_ScopeID
{
   ACSVM_Word global;
   ACSVM_Word hub;
   ACSVM_Word map;
};


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

void ACSVM_GlobalScope_FreeHubScope(ACSVM_GlobalScope *scope, ACSVM_HubScope *scopeHub);

ACSVM_Array    *ACSVM_GlobalScope_GetGblArr  (ACSVM_GlobalScope       *scope, ACSVM_Word idx);
ACSVM_Word      ACSVM_GlobalScope_GetGblArrC (ACSVM_GlobalScope const *scope);
ACSVM_HubScope *ACSVM_GlobalScope_GetHubScope(ACSVM_GlobalScope       *scope, ACSVM_Word id);
ACSVM_Word      ACSVM_GlobalScope_GetGblReg  (ACSVM_GlobalScope const *scope, ACSVM_Word idx);
ACSVM_Word      ACSVM_GlobalScope_GetGblRegC (ACSVM_GlobalScope const *scope);

void ACSVM_GlobalScope_SetActive(ACSVM_GlobalScope *scope, bool active);
void ACSVM_GlobalScope_SetGblReg(ACSVM_GlobalScope *scope, ACSVM_Word idx, ACSVM_Word reg);

void ACSVM_HubScope_FreeMapScope(ACSVM_HubScope *scope, ACSVM_MapScope *scopeMap);

ACSVM_Array    *ACSVM_HubScope_GetHubArr  (ACSVM_HubScope       *scope, ACSVM_Word idx);
ACSVM_Word      ACSVM_HubScope_GetHubArrC (ACSVM_HubScope const *scope);
ACSVM_MapScope *ACSVM_HubScope_GetMapScope(ACSVM_HubScope       *scope, ACSVM_Word id);
ACSVM_Word      ACSVM_HubScope_GetHubReg  (ACSVM_HubScope const *scope, ACSVM_Word idx);
ACSVM_Word      ACSVM_HubScope_GetHubRegC (ACSVM_HubScope const *scope);

void ACSVM_HubScope_SetActive(ACSVM_HubScope *scope, bool active);
void ACSVM_HubScope_SetHubReg(ACSVM_HubScope *scope, ACSVM_Word idx, ACSVM_Word reg);

void ACSVM_MapScope_AddModules(ACSVM_MapScope *scope,
   ACSVM_Module *const *moduleV, size_t moduleC);

ACSVM_ModuleScope *ACSVM_MapScope_GetModuleScope(ACSVM_MapScope *scope, ACSVM_Module *module);

bool ACSVM_MapScope_HasModules(ACSVM_MapScope const *scope);

bool ACSVM_MapScope_ScriptPause(ACSVM_MapScope *scope,
   ACSVM_ScriptName name, ACSVM_ScopeID id);

bool ACSVM_MapScope_ScriptStart(ACSVM_MapScope *scope,
   ACSVM_ScriptName name, ACSVM_ScopeID id, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread));

bool ACSVM_MapScope_ScriptStartForced(ACSVM_MapScope *scope,
   ACSVM_ScriptName name, ACSVM_ScopeID id, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread));

ACSVM_Word ACSVM_MapScope_ScriptStartResult(ACSVM_MapScope *scope,
   ACSVM_ScriptName name, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread));

ACSVM_Word ACSVM_MapScope_ScriptStartType(ACSVM_MapScope *scope,
   ACSVM_Word type, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread));

ACSVM_Word ACSVM_MapScope_ScriptStartTypeForced(ACSVM_MapScope *scope,
   ACSVM_Word type, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread));

bool ACSVM_MapScope_ScriptStop(ACSVM_MapScope *scope,
   ACSVM_ScriptName name, ACSVM_ScopeID id);

void ACSVM_MapScope_SetActive(ACSVM_MapScope *scope, bool active);

ACSVM_Array *ACSVM_ModuleScope_GetModArr (ACSVM_ModuleScope       *scope, ACSVM_Word idx);
ACSVM_Word   ACSVM_ModuleScope_GetModArrC(ACSVM_ModuleScope const *scope);
ACSVM_Word   ACSVM_ModuleScope_GetModReg (ACSVM_ModuleScope const *scope, ACSVM_Word idx);
ACSVM_Word   ACSVM_ModuleScope_GetModRegC(ACSVM_ModuleScope const *scope);

void ACSVM_ModuleScope_SetModReg(ACSVM_ModuleScope *scope, ACSVM_Word idx, ACSVM_Word reg);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Scope_H__

