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

#include "Scope.h"

#include "Environment.h"
#include "Script.h"
#include "Thread.h"

#include "ACSVM/Action.hpp"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_GlobalScope_FreeHubScope
//
void ACSVM_GlobalScope_FreeHubScope(ACSVM_GlobalScope *scope, ACSVM_HubScope *scopeHub)
{
   reinterpret_cast<ACSVM::GlobalScope *>(scope)
      ->freeHubScope(reinterpret_cast<ACSVM::HubScope *>(scopeHub));
}

//
// ACSVM_GlobalScope_GetGblArr
//
ACSVM_Array *ACSVM_GlobalScope_GetGblArr(ACSVM_GlobalScope *scope, ACSVM_Word idx)
{
   return reinterpret_cast<ACSVM_Array *>(&reinterpret_cast<ACSVM::GlobalScope *>(scope)->arrV[idx]);
}

//
// ACSVM_GlobalScope_GetGblArrC
//
ACSVM_Word ACSVM_GlobalScope_GetGblArrC(ACSVM_GlobalScope const *)
{
   return ACSVM::GlobalScope::ArrC;
}

//
// ACSVM_GlobalScope_GetHubScope
//
ACSVM_HubScope *ACSVM_GlobalScope_GetHubScope(ACSVM_GlobalScope *scope_, ACSVM_Word id)
{
   auto scope = reinterpret_cast<ACSVM::GlobalScope *>(scope_);

   try
   {
      return reinterpret_cast<ACSVM_HubScope *>(scope->getHubScope(id));
   }
   catch(std::bad_alloc const &e)
   {
      auto env = static_cast<ACSVM_Environment *>(scope->env);
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());

      return nullptr;
   }
}

//
// ACSVM_GlobalScope_GetGblReg
//
ACSVM_Word ACSVM_GlobalScope_GetGblReg(ACSVM_GlobalScope const *scope, ACSVM_Word idx)
{
   return reinterpret_cast<ACSVM::GlobalScope const *>(scope)->regV[idx];
}

//
// ACSVM_GlobalScope_GetGblRegC
//
ACSVM_Word ACSVM_GlobalScope_GetGblRegC(ACSVM_GlobalScope const *)
{
   return ACSVM::GlobalScope::RegC;
}

//
// ACSVM_GlobalScope_SetActive
//
void ACSVM_GlobalScope_SetActive(ACSVM_GlobalScope *scope, bool active)
{
   reinterpret_cast<ACSVM::GlobalScope *>(scope)->active = active;
}

//
// ACSVM_GlobalScope_SetGblReg
//
void ACSVM_GlobalScope_SetGblReg(ACSVM_GlobalScope *scope, ACSVM_Word idx, ACSVM_Word reg)
{
   reinterpret_cast<ACSVM::GlobalScope *>(scope)->regV[idx] = reg;
}

//
// ACSVM_HubScope_FreeMapScope
//
void ACSVM_HubScope_FreeMapScope(ACSVM_HubScope *scope, ACSVM_MapScope *scopeMap)
{
   reinterpret_cast<ACSVM::HubScope *>(scope)
      ->freeMapScope(reinterpret_cast<ACSVM::MapScope *>(scopeMap));
}

//
// ACSVM_HubScope_GetHubArr
//
ACSVM_Array *ACSVM_HubScope_GetHubArr(ACSVM_HubScope *scope, ACSVM_Word idx)
{
   return reinterpret_cast<ACSVM_Array *>(&reinterpret_cast<ACSVM::HubScope *>(scope)->arrV[idx]);
}

//
// ACSVM_HubScope_GetHubArrC
//
ACSVM_Word ACSVM_HubScope_GetHubArrC(ACSVM_HubScope const *)
{
   return ACSVM::HubScope::ArrC;
}

//
// ACSVM_HubScope_GetMapScope
//
ACSVM_MapScope *ACSVM_HubScope_GetMapScope(ACSVM_HubScope *scope_, ACSVM_Word id)
{
   auto scope = reinterpret_cast<ACSVM::HubScope *>(scope_);

   try
   {
      return reinterpret_cast<ACSVM_MapScope *>(scope->getMapScope(id));
   }
   catch(std::bad_alloc const &e)
   {
      auto env = static_cast<ACSVM_Environment *>(scope->env);
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());

      return nullptr;
   }
}

//
// ACSVM_HubScope_GetHubReg
//
ACSVM_Word ACSVM_HubScope_GetHubReg(ACSVM_HubScope const *scope, ACSVM_Word idx)
{
   return reinterpret_cast<ACSVM::HubScope const *>(scope)->regV[idx];
}

//
// ACSVM_HubScope_GetHubRegC
//
ACSVM_Word ACSVM_HubScope_GetHubRegC(ACSVM_HubScope const *)
{
   return ACSVM::HubScope::RegC;
}

//
// ACSVM_HubScope_SetActive
//
void ACSVM_HubScope_SetActive(ACSVM_HubScope *scope, bool active)
{
   reinterpret_cast<ACSVM::HubScope *>(scope)->active = active;
}

//
// ACSVM_HubScope_SetHubReg
//
void ACSVM_HubScope_SetHubReg(ACSVM_HubScope *scope, ACSVM_Word idx, ACSVM_Word reg)
{
   reinterpret_cast<ACSVM::HubScope *>(scope)->regV[idx] = reg;
}

//
// ACSVM_MapScope_AddModule
//
void ACSVM_MapScope_AddModules(ACSVM_MapScope *scope_,
   ACSVM_Module *const *moduleV, size_t moduleC)
{
   auto scope = reinterpret_cast<ACSVM::MapScope *>(scope_);

   try
   {
      scope->addModules(reinterpret_cast<ACSVM::Module *const *>(moduleV), moduleC);
   }
   catch(std::bad_alloc const &e)
   {
      auto env = static_cast<ACSVM_Environment *>(scope->env);
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());
   }
}

//
// ACSVM_MapScope_GetModuleScope
//
ACSVM_ModuleScope *ACSVM_MapScope_GetModuleScope(ACSVM_MapScope *scope, ACSVM_Module *module)
{
   return reinterpret_cast<ACSVM_ModuleScope *>(
      reinterpret_cast<ACSVM::MapScope *>(scope)->getModuleScope(
         reinterpret_cast<ACSVM::Module *>(module)));
}

//
// ACSVM_MapScope_HasModules
//
bool ACSVM_MapScope_HasModules(ACSVM_MapScope const *scope)
{
   return reinterpret_cast<ACSVM::MapScope const *>(scope)->hasModules();
}

//
// ACSVM_MapScope_ScriptPause
//
bool ACSVM_MapScope_ScriptPause(ACSVM_MapScope *scope,
   ACSVM_ScriptName name_, ACSVM_ScopeID id)
{
   ACSVM::ScriptName name{reinterpret_cast<ACSVM::String *>(name_.s), name_.i};
   return reinterpret_cast<ACSVM::MapScope *>(scope)
      ->scriptPause(name, {id.global, id.hub, id.map});
}

//
// ACSVM_MapScope_ScriptStart
//
bool ACSVM_MapScope_ScriptStart(ACSVM_MapScope *scope,
   ACSVM_ScriptName name_, ACSVM_ScopeID id, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread))
{
   ACSVM::ScriptName name{reinterpret_cast<ACSVM::String *>(name_.s), name_.i};
   return reinterpret_cast<ACSVM::MapScope *>(scope)
      ->scriptStart(name, {id.global, id.hub, id.map}, {argV, argC, info, func});
}

//
// ACSVM_MapScope_ScriptStartForced
//
bool ACSVM_MapScope_ScriptStartForced(ACSVM_MapScope *scope,
   ACSVM_ScriptName name_, ACSVM_ScopeID id, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread))
{
   ACSVM::ScriptName name{reinterpret_cast<ACSVM::String *>(name_.s), name_.i};
   return reinterpret_cast<ACSVM::MapScope *>(scope)
      ->scriptStartForced(name, {id.global, id.hub, id.map}, {argV, argC, info, func});
}

//
// ACSVM_MapScope_ScriptStartResult
//
ACSVM_Word ACSVM_MapScope_ScriptStartResult(ACSVM_MapScope *scope,
   ACSVM_ScriptName name_, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread))
{
   ACSVM::ScriptName name{reinterpret_cast<ACSVM::String *>(name_.s), name_.i};
   return reinterpret_cast<ACSVM::MapScope *>(scope)
      ->scriptStartResult(name, {argV, argC, info, func});
}

//
// ACSVM_MapScope_ScriptStartType
//
ACSVM_Word ACSVM_MapScope_ScriptStartType(ACSVM_MapScope *scope,
   ACSVM_Word type, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread))
{
   return reinterpret_cast<ACSVM::MapScope *>(scope)
      ->scriptStartType(type, {argV, argC, info, func});
}

//
// ACSVM_MapScope_ScriptStartTypeForced
//
ACSVM_Word ACSVM_MapScope_ScriptStartTypeForced(ACSVM_MapScope *scope,
   ACSVM_Word type, ACSVM_Word const *argV, ACSVM_Word argC,
   ACSVM_ThreadInfo const *info, void (*func)(void *thread))
{
   return reinterpret_cast<ACSVM::MapScope *>(scope)
      ->scriptStartTypeForced(type, {argV, argC, info, func});
}

//
// ACSVM_MapScope_ScriptStop
//
bool ACSVM_MapScope_ScriptStop(ACSVM_MapScope *scope,
   ACSVM_ScriptName name_, ACSVM_ScopeID id)
{
   ACSVM::ScriptName name{reinterpret_cast<ACSVM::String *>(name_.s), name_.i};
   return reinterpret_cast<ACSVM::MapScope *>(scope)
      ->scriptStop(name, {id.global, id.hub, id.map});
}

//
// ACSVM_MapScope_SetActive
//
void ACSVM_MapScope_SetActive(ACSVM_MapScope *scope, bool active)
{
   reinterpret_cast<ACSVM::MapScope *>(scope)->active = active;
}

//
// ACSVM_ModuleScope_GetModArr
//
ACSVM_Array *ACSVM_ModuleScope_GetModArr(ACSVM_ModuleScope *scope, ACSVM_Word idx)
{
   return reinterpret_cast<ACSVM_Array *>(reinterpret_cast<ACSVM::ModuleScope *>(scope)->arrV[idx]);
}

//
// ACSVM_ModuleScope_GetModArrC
//
ACSVM_Word ACSVM_ModuleScope_GetModArrC(ACSVM_ModuleScope const *)
{
   return ACSVM::ModuleScope::ArrC;
}

//
// ACSVM_ModuleScope_SetModReg
//
void ACSVM_ModuleScope_SetModReg(ACSVM_ModuleScope *scope, ACSVM_Word idx, ACSVM_Word reg)
{
   *reinterpret_cast<ACSVM::ModuleScope *>(scope)->regV[idx] = reg;
}

//
// ACSVM_ModuleScope_GetModReg
//
ACSVM_Word ACSVM_ModuleScope_GetModReg(ACSVM_ModuleScope const *scope, ACSVM_Word idx)
{
   return *reinterpret_cast<ACSVM::ModuleScope const *>(scope)->regV[idx];
}

//
// ACSVM_ModuleScope_GetModRegC
//
ACSVM_Word ACSVM_ModuleScope_GetModRegC(ACSVM_ModuleScope const *)
{
   return ACSVM::ModuleScope::RegC;
}

}

// EOF

