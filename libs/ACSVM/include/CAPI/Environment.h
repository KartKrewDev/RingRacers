//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Environment.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__Environment_H__
#define ACSVM__CAPI__Environment_H__

#include "Code.h"

#ifdef __cplusplus
#include "../ACSVM/Environment.hpp"

#include <vector>
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Types                                                                      |
//

//
// ACSVM_EnvironmentFuncs
//
// Overridable virtual functions in ACSVM_Environment. If null, the base
// function is called. If the base function is pure virtual, then the function
// in this structure must not be null.
//
typedef struct ACSVM_EnvironmentFuncs
{
   // Called in the event of a memory allocation failure. This function may
   // return normally, but the ACSVM_Environment object must not be used
   // afterward, except to be passed to ACSVM_FreeEnvironment.
   void (*bad_alloc)(ACSVM_Environment *env, char const *what);

   // Called if an ACSVM::ReadError is caught. This function may return
   // normally.
   void (*readError)(ACSVM_Environment *env, char const *what);

   // Called if an ACSVM::SerialError is caught. This function may return
   // normally, but the VM state is indeterminate.
   void (*serialError)(ACSVM_Environment *env, char const *what);

   // public

   void (*ctor)(ACSVM_Environment *env);
   void (*dtor)(ACSVM_Environment *env);

   bool (*checkLock)(ACSVM_Environment const *env, ACSVM_Thread *thread,
      ACSVM_Word lock, bool door);

   bool (*checkTag)(ACSVM_Environment const *env, ACSVM_Word type, ACSVM_Word tag);

   ACSVM_ModuleName (*getModuleName)(ACSVM_Environment *env,
      char const *str, size_t len);

   // Called after base class's.
   void (*loadState)(ACSVM_Environment *env, ACSVM_Serial *in);

   void (*printArray)(ACSVM_Environment const *env, ACSVM_PrintBuf *buf,
      ACSVM_Array const *array, ACSVM_Word index, ACSVM_Word limit);

   void (*printKill)(ACSVM_Environment const *env, ACSVM_Thread *thread,
      ACSVM_Word type, ACSVM_Word data);

   ACSVM_ModuleName (*readModuleName)(ACSVM_Environment const *env, ACSVM_Serial *in);

   // Called after base class's.
   void (*refStrings)(ACSVM_Environment *env);

   // Called after base class's.
   void (*resetStrings)(ACSVM_Environment *env);

   // Called after base class's.
   void (*saveState)(ACSVM_Environment const *env, ACSVM_Serial *out);

   void (*writeModuleName)(ACSVM_Environment const *env, ACSVM_Serial *out,
      ACSVM_ModuleName in);

   // protected

   ACSVM_Thread *(*allocThread)(ACSVM_Environment *env);

   ACSVM_Word (*callSpecImpl)(ACSVM_Environment *env, ACSVM_Thread *thread,
      ACSVM_Word spec, ACSVM_Word const *argV, ACSVM_Word argC);

   // Return false if load fails.
   bool (*loadModule)(ACSVM_Environment *env, ACSVM_Module *module);
} ACSVM_EnvironmentFuncs;

#ifdef __cplusplus
//
// ACSVM_Environment
//
struct ACSVM_Environment : ACSVM::Environment
{
public:
   ACSVM_Environment(ACSVM_EnvironmentFuncs const &funcs, void *data);
   virtual ~ACSVM_Environment();

   virtual bool callFunc(ACSVM::Thread *thread, ACSVM::Word func,
      ACSVM::Word const *argV, ACSVM::Word argC);

   virtual bool checkLock(ACSVM::Thread *thread, ACSVM::Word lock, bool door);

   virtual bool checkTag(ACSVM::Word type, ACSVM::Word tag);

   virtual ACSVM::ModuleName getModuleName(char const *str, size_t len);

   virtual void loadState(ACSVM::Serial &in);

   virtual void printArray(ACSVM::PrintBuf &buf, ACSVM::Array const &array,
      ACSVM::Word index, ACSVM::Word limit);

   virtual void printKill(ACSVM::Thread *thread, ACSVM::Word type, ACSVM::Word data);

   virtual ACSVM::ModuleName readModuleName(ACSVM::Serial &in) const;

   virtual void refStrings();

   virtual void resetStrings();

   virtual void saveState(ACSVM::Serial &out) const;

   virtual void writeModuleName(ACSVM::Serial &out, ACSVM::ModuleName const &in) const;

   std::vector<ACSVM_CallFunc> callFuncV;
   ACSVM_EnvironmentFuncs      funcs;
   void                       *data;

protected:
   virtual ACSVM::Thread *allocThread();

   virtual ACSVM::Word callSpecImpl(ACSVM::Thread *thread, ACSVM::Word spec,
      ACSVM::Word const *argV, ACSVM::Word argC);

   virtual void loadModule(ACSVM::Module *module);
};
#endif


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

ACSVM_Environment *ACSVM_AllocEnvironment(ACSVM_EnvironmentFuncs const *funcs, void *data);
void ACSVM_FreeEnvironment(ACSVM_Environment *env);

ACSVM_Word ACSVM_Environment_AddCallFunc(ACSVM_Environment *env, ACSVM_CallFunc func);

void ACSVM_Environment_AddCodeDataACS0(ACSVM_Environment *env, ACSVM_Word code,
   char const *args, ACSVM_Code transCode, ACSVM_Word stackArgC, ACSVM_Word transFunc);

void ACSVM_Environment_AddFuncDataACS0(ACSVM_Environment *env, ACSVM_Word func,
   ACSVM_Word transFunc, ACSVM_Word const *transCodeArgCV,
   ACSVM_Code const *transCodeV, size_t transCodeC);

void ACSVM_Environment_CollectStrings(ACSVM_Environment *env);

void ACSVM_Environment_Exec(ACSVM_Environment *env);

void ACSVM_Environment_FreeGlobalScope(ACSVM_Environment *env, ACSVM_GlobalScope *scope);
void ACSVM_Environment_FreeModule(ACSVM_Environment *env, ACSVM_Module *module);

ACSVM_Word         ACSVM_Environment_GetBranchLimit(ACSVM_Environment const *env);
void              *ACSVM_Environment_GetData(ACSVM_Environment const *env);
ACSVM_GlobalScope *ACSVM_Environment_GetGlobalScope(ACSVM_Environment *env, ACSVM_Word id);
ACSVM_Module      *ACSVM_Environment_GetModule(ACSVM_Environment *env, ACSVM_ModuleName name);
ACSVM_Word         ACSVM_Environment_GetScriptLocRegC(ACSVM_Environment const *env);
ACSVM_StringTable *ACSVM_Environment_GetStringTable(ACSVM_Environment *env);

bool ACSVM_Environment_HasActiveThread(ACSVM_Environment const *env);

bool ACSVM_Environment_LoadState(ACSVM_Environment *env, ACSVM_Serial *in);

void ACSVM_Environment_SaveState(ACSVM_Environment *env, ACSVM_Serial *out);

void ACSVM_Environment_SetBranchLimit(ACSVM_Environment *env, ACSVM_Word branchLimit);
void ACSVM_Environment_SetData(ACSVM_Environment *env, void *data);
void ACSVM_Environment_SetScriptLocRegC(ACSVM_Environment *env, ACSVM_Word scriptLocRegC);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Environment_H__

