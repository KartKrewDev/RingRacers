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

#include "Environment.h"

#include "Code.h"
#include "Module.h"
#include "Thread.h"

#include "ACSVM/CodeData.hpp"
#include "ACSVM/Error.hpp"
#include "ACSVM/Serial.hpp"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_Environment constructor
//
ACSVM_Environment::ACSVM_Environment(ACSVM_EnvironmentFuncs const &funcs_, void *data_) :
   funcs{funcs_}, data{data_}
{
   if(funcs.ctor)
      funcs.ctor(this);
}

//
// ACSVM_Environment destructor
//
ACSVM_Environment::~ACSVM_Environment()
{
   if(funcs.dtor)
      funcs.dtor(this);
}

//
// ACSVM_Environment::allocThread
//
ACSVM::Thread *ACSVM_Environment::allocThread()
{
   if(!funcs.allocThread)
      return new ACSVM_Thread(this, {}, nullptr);

   if(ACSVM_Thread *thread = funcs.allocThread(this))
      return thread;

   throw std::bad_alloc();
}

//
// ACSVM_Environment::callFunc
//
bool ACSVM_Environment::callFunc(ACSVM::Thread *thread, ACSVM::Word func,
   ACSVM::Word const *argV, ACSVM::Word argC)
{
   if(func <= static_cast<ACSVM::Word>(ACSVM::Func::None))
      return ACSVM::Environment::callFunc(thread, func, argV, argC);

   auto callFunc = callFuncV[func - static_cast<ACSVM::Word>(ACSVM::Func::None) - 1];
   return callFunc(static_cast<ACSVM_Thread *>(thread), argV, argC);
}

//
// ACSVM_Environment::callSpecImpl
//
ACSVM::Word ACSVM_Environment::callSpecImpl(ACSVM::Thread *thread,
   ACSVM::Word spec, ACSVM::Word const *argV, ACSVM::Word argC)
{
   if(!funcs.callSpecImpl)
      return ACSVM::Environment::callSpecImpl(thread, spec, argV, argC);

   return funcs.callSpecImpl(this, static_cast<ACSVM_Thread *>(thread), spec, argV, argC);
}

//
// ACSVM_Environment::checkLock
//
bool ACSVM_Environment::checkLock(ACSVM::Thread *thread, ACSVM::Word lock, bool door)
{
   if(!funcs.checkLock)
      return ACSVM::Environment::checkLock(thread, lock, door);

   return funcs.checkLock(this, static_cast<ACSVM_Thread *>(thread), lock, door);
}

//
// ACSVM_Environment::checkTag
//
bool ACSVM_Environment::checkTag(ACSVM::Word type, ACSVM::Word tag)
{
   if(!funcs.checkTag)
      return ACSVM::Environment::checkTag(type, tag);

   return funcs.checkTag(this, type, tag);
}

//
// ACSVM_Environment::getModuleName
//
ACSVM::ModuleName ACSVM_Environment::getModuleName(char const *str, std::size_t len)
{
   if(!funcs.getModuleName)
      return ACSVM::Environment::getModuleName(str, len);

   auto name = funcs.getModuleName(this, str, len);
   return {reinterpret_cast<ACSVM::String *>(name.s), name.p, name.i};
}

//
// ACSVM_Environment::loadModule
//
void ACSVM_Environment::loadModule(ACSVM::Module *module)
{
   if(!funcs.loadModule)
      throw ACSVM::ReadError();

   if(!funcs.loadModule(this, reinterpret_cast<ACSVM_Module *>(module)))
      throw ACSVM::ReadError();
}

//
// ACSVM_Environment::loadState
//
void ACSVM_Environment::loadState(ACSVM::Serial &in)
{
   ACSVM::Environment::loadState(in);

   if(funcs.loadState)
      funcs.loadState(this, reinterpret_cast<ACSVM_Serial *>(&in));
}

//
// ACSVM_Environment::printArray
//
void ACSVM_Environment::printArray(ACSVM::PrintBuf &buf, ACSVM::Array const &array,
   ACSVM::Word index, ACSVM::Word limit)
{
   if(!funcs.printArray)
      return ACSVM::Environment::printArray(buf, array, index, limit);

   funcs.printArray(this, reinterpret_cast<ACSVM_PrintBuf *>(&buf),
      reinterpret_cast<ACSVM_Array const *>(&array), index, limit);
}

//
// ACSVM_Environment::printKill
//
void ACSVM_Environment::printKill(ACSVM::Thread *thread, ACSVM::Word type, ACSVM::Word killData)
{
   if(!funcs.printKill)
      return ACSVM::Environment::printKill(thread, type, killData);

   funcs.printKill(this, static_cast<ACSVM_Thread *>(thread), type, killData);
}

//
// ACSVM_Environment::readModuleName
//
ACSVM::ModuleName ACSVM_Environment::readModuleName(ACSVM::Serial &in) const
{
   if(!funcs.readModuleName)
      return ACSVM::Environment::readModuleName(in);

   auto name = funcs.readModuleName(this, reinterpret_cast<ACSVM_Serial *>(&in));
   return {reinterpret_cast<ACSVM::String *>(name.s), name.p, name.i};
}

//
// ACSVM_Environment::refStrings
//
void ACSVM_Environment::refStrings()
{
   ACSVM::Environment::refStrings();

   if(funcs.refStrings)
      funcs.refStrings(this);
}

//
// ACSVM_Environment::resetStrings
//
void ACSVM_Environment::resetStrings()
{
   ACSVM::Environment::resetStrings();

   if(funcs.resetStrings)
      funcs.resetStrings(this);
}

//
// ACSVM_Environment::saveState
//
void ACSVM_Environment::saveState(ACSVM::Serial &out) const
{
   ACSVM::Environment::saveState(out);

   if(funcs.saveState)
      funcs.saveState(this, reinterpret_cast<ACSVM_Serial *>(&out));
}

//
// ACSVM_Environment::writeModuleName
//
void ACSVM_Environment::writeModuleName(ACSVM::Serial &out, ACSVM::ModuleName const &in) const
{
   if(!funcs.writeModuleName)
      return ACSVM::Environment::writeModuleName(out, in);

   funcs.writeModuleName(this, reinterpret_cast<ACSVM_Serial *>(&out),
      {reinterpret_cast<ACSVM_String *>(in.s), in.p, in.i});
}

//
// ACSVM_AllocEnvironment
//
ACSVM_Environment *ACSVM_AllocEnvironment(ACSVM_EnvironmentFuncs const *funcs, void *data)
{
   return new(std::nothrow) ACSVM_Environment(*funcs, data);
}

//
// ACSVM_FreeEnvironment
//
void ACSVM_FreeEnvironment(ACSVM_Environment *env)
{
   delete env;
}

//
// ACSVM_Environment_AddCallFunc
//
ACSVM_Word ACSVM_Environment_AddCallFunc(ACSVM_Environment *env, ACSVM_CallFunc func)
{
   env->callFuncV.push_back(func);
   return env->callFuncV.size() + static_cast<ACSVM_Word>(ACSVM::Func::None);
}

//
// ACSVM_Environment_AddCodeDataACS0
//
void ACSVM_Environment_AddCodeDataACS0(ACSVM_Environment *env, ACSVM_Word code,
   char const *args, ACSVM_Code transCode_, ACSVM_Word stackArgC, ACSVM_Word transFunc)
{
   try
   {
      auto transCode = static_cast<ACSVM::Code>(transCode_);
      env->addCodeDataACS0(code, {args, transCode, stackArgC, transFunc});
   }
   catch(std::bad_alloc const &e)
   {
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());
   }
}

//
// ACSVM_Environment_AddFuncDataACS0
//
void ACSVM_Environment_AddFuncDataACS0(ACSVM_Environment *env, ACSVM_Word func,
   ACSVM_Word transFunc, ACSVM_Word const *transCodeArgCV,
   ACSVM_Code const *transCodeV, size_t transCodeC)
{
   try
   {
      if(transCodeC)
      {
         std::unique_ptr<ACSVM::FuncDataACS0::TransCode[]> transCodes
            {new ACSVM::FuncDataACS0::TransCode[transCodeC]};

         for(std::size_t i = 0; i != transCodeC; ++i)
            transCodes[i] = {transCodeArgCV[i], static_cast<ACSVM::Code>(transCodeV[i])};

         env->addFuncDataACS0(func, {transFunc, std::move(transCodes), transCodeC});
      }
      else
         env->addFuncDataACS0(func, transFunc);
   }
   catch(std::bad_alloc const &e)
   {
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());
   }
}

//
// ACSVM_Environment_CollectStrings
//
void ACSVM_Environment_CollectStrings(ACSVM_Environment *env)
{
   env->collectStrings();
}

//
// ACSVM_Environment_Exec
//
void ACSVM_Environment_Exec(ACSVM_Environment *env)
{
   try
   {
      env->exec();
   }
   catch(std::bad_alloc const &e)
   {
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());
   }
}

//
// ACSVM_Environment_FreeGlobalScope
//
void ACSVM_Environment_FreeGlobalScope(ACSVM_Environment *env, ACSVM_GlobalScope *scope)
{
   env->freeGlobalScope(reinterpret_cast<ACSVM::GlobalScope *>(scope));
}

//
// ACSVM_Environment_FreeModule
//
void ACSVM_Environment_FreeModule(ACSVM_Environment *env, ACSVM_Module *module)
{
   env->freeModule(reinterpret_cast<ACSVM::Module *>(module));
}

//
// ACSVM_Environment_GetBranchLimit
//
ACSVM_Word ACSVM_Environment_GetBranchLimit(ACSVM_Environment const *env)
{
   return env->branchLimit;
}

//
// ACSVM_Environment_GetData
//
void *ACSVM_Environment_GetData(ACSVM_Environment const *env)
{
   return env->data;
}

//
// ACSVM_Environment_GetGlobalScope
//
ACSVM_GlobalScope *ACSVM_Environment_GetGlobalScope(ACSVM_Environment *env, ACSVM_Word id)
{
   try
   {
      return reinterpret_cast<ACSVM_GlobalScope *>(env->getGlobalScope(id));
   }
   catch(std::bad_alloc const &e)
   {
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());

      return nullptr;
   }
}

//
// ACSVM_Environment_GetModule
//
ACSVM_Module *ACSVM_Environment_GetModule(ACSVM_Environment *env, ACSVM_ModuleName name)
{
   try
   {
      return reinterpret_cast<ACSVM_Module *>(env->getModule(
         {reinterpret_cast<ACSVM::String *>(name.s), name.p, name.i}));
   }
   catch(ACSVM::ReadError const &e)
   {
      if(env->funcs.readError)
         env->funcs.readError(env, e.what());

      return nullptr;
   }
   catch(std::bad_alloc const &e)
   {
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());

      return nullptr;
   }
}

//
// ACSVM_Environment_GetScriptLocRegC
//
ACSVM_Word ACSVM_Environment_GetScriptLocRegC(ACSVM_Environment const *env)
{
   return env->scriptLocRegC;
}

//
// ACSVM_Environment_GetStringTable
//
ACSVM_StringTable *ACSVM_Environment_GetStringTable(ACSVM_Environment *env)
{
   return reinterpret_cast<ACSVM_StringTable *>(&env->stringTable);
}

//
// ACSVM_Environment_HasActiveThread
//
bool ACSVM_Environment_HasActiveThread(ACSVM_Environment const *env)
{
   return env->hasActiveThread();
}

//
// ACSVM_Environment_LoadState
//
bool ACSVM_Environment_LoadState(ACSVM_Environment *env, ACSVM_Serial *in)
{
   try
   {
      env->loadState(*reinterpret_cast<ACSVM::Serial *>(in));
      return true;
   }
   catch(ACSVM::SerialError const &e)
   {
      if(env->funcs.serialError)
         env->funcs.serialError(env, e.what());

      return false;
   }
   catch(std::bad_alloc const &e)
   {
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());

      return false;
   }
}

//
// ACSVM_Environment_SaveState
//
void ACSVM_Environment_SaveState(ACSVM_Environment *env, ACSVM_Serial *out)
{
   env->saveState(*reinterpret_cast<ACSVM::Serial *>(out));
}

//
// ACSVM_Environment_SetBranchLimit
//
void ACSVM_Environment_SetBranchLimit(ACSVM_Environment *env, ACSVM_Word branchLimit)
{
   env->branchLimit = branchLimit;
}

//
// ACSVM_Environment_SetData
//
void ACSVM_Environment_SetData(ACSVM_Environment *env, void *data)
{
   env->data = data;
}

//
// ACSVM_Environment_SetScriptLocRegC
//
void ACSVM_Environment_SetScriptLocReg(ACSVM_Environment *env, ACSVM_Word scriptLocRegC)
{
   env->scriptLocRegC = scriptLocRegC;
}

}

// EOF

