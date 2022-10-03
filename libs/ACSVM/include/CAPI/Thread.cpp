//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Threads.
//
//-----------------------------------------------------------------------------

#include "Thread.h"

#include "Array.h"
#include "Environment.h"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_ThreadInfo constructor
//
ACSVM_ThreadInfo::ACSVM_ThreadInfo(void *data_) :
   data{data_}
{
}

//
// ACSVM_ThreadInfo destructor
//
ACSVM_ThreadInfo::~ACSVM_ThreadInfo()
{
}

//
// ACSVM_Thread constructor
//
ACSVM_Thread::ACSVM_Thread(ACSVM_Environment *env_, ACSVM_ThreadFuncs const &funcs_, void *data_) :
   ACSVM::Thread{env_}, funcs{funcs_}, info{data_}
{
   if(funcs.ctor)
      funcs.ctor(this);
}

//
// ACSVM_Thread destructor
//
ACSVM_Thread::~ACSVM_Thread()
{
   if(funcs.dtor)
      funcs.dtor(this);
}

//
// ACSVM_Thread::getInfo
//
ACSVM::ThreadInfo const *ACSVM_Thread::getInfo() const
{
   return &info;
}

//
// ACSVM_Thread::loadState
//
void ACSVM_Thread::loadState(ACSVM::Serial &in)
{
   ACSVM::Thread::loadState(in);

   if(funcs.loadState)
      funcs.loadState(this, reinterpret_cast<ACSVM_Serial *>(&in));
}

//
// ACSVM_Thread::lockStrings
//
void ACSVM_Thread::lockStrings() const
{
   ACSVM::Thread::lockStrings();

   if(funcs.lockStrings)
      funcs.lockStrings(this);
}

//
// ACSVM_Thread::refStrings
//
void ACSVM_Thread::refStrings() const
{
   ACSVM::Thread::refStrings();

   if(funcs.refStrings)
      funcs.refStrings(this);
}

//
// ACSVM_Thread::saveState
//
void ACSVM_Thread::saveState(ACSVM::Serial &out) const
{
   ACSVM::Thread::saveState(out);

   if(funcs.saveState)
      funcs.saveState(this, reinterpret_cast<ACSVM_Serial *>(&out));
}

//
// ACSVM_Thread::start
//
void ACSVM_Thread::start(ACSVM::Script *script, ACSVM::MapScope *map,
   ACSVM::ThreadInfo const *infoPtr, ACSVM::Word const *argV, ACSVM::Word argC)
{
   ACSVM::Thread::start(script, map, infoPtr, argV, argC);

   if(funcs.start)
   {
      auto infoDer = dynamic_cast<ACSVM_ThreadInfo const *>(infoPtr);
      funcs.start(this, infoDer ? infoDer->data : nullptr);
   }
}

//
// ACSVM_Thread::stop
//
void ACSVM_Thread::stop()
{
   ACSVM::Thread::stop();

   if(funcs.stop)
      funcs.stop(this);
}

//
// ACSVM_Thread::unlockStrings
//
void ACSVM_Thread::unlockStrings() const
{
   ACSVM::Thread::unlockStrings();

   if(funcs.unlockStrings)
      funcs.unlockStrings(this);
}

//
// ACSVM_AllocThread
//
ACSVM_Thread *ACSVM_AllocThread(ACSVM_Environment *env,
   ACSVM_ThreadFuncs const *funcs, void *data)
{
   return new(std::nothrow) ACSVM_Thread(env, *funcs, data);
}

//
// ACSVM_ThreadFromVoid
//
ACSVM_Thread *ACSVM_ThreadFromVoid(void *thread)
{
   return static_cast<ACSVM_Thread *>(static_cast<ACSVM::Thread *>(thread));
}

//
// ACSVM_Thread_Exec
//
void ACSVM_Thread_Exec(ACSVM_Thread *thread)
{
   try
   {
      thread->exec();
   }
   catch(std::bad_alloc const &e)
   {
      auto env = static_cast<ACSVM_Environment *>(thread->env);
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());
   }
}

//
// ACSVM_Thread_GetCodePtr
//
ACSVM_Word const *ACSVM_Thread_GetCodePtr(ACSVM_Thread const *thread)
{
   return thread->codePtr;
}

//
// ACSVM_Thread_GetDelay
//
ACSVM_Word ACSVM_Thread_GetDelay(ACSVM_Thread const *thread)
{
   return thread->delay;
}

//
// ACSVM_Thread_GetEnv
//
ACSVM_Environment *ACSVM_Thread_GetEnv(ACSVM_Thread const *thread)
{
   return static_cast<ACSVM_Environment *>(thread->env);
}

//
// ACSVM_Thread_GetInfo
//
void *ACSVM_Thread_GetInfo(ACSVM_Thread const *thread)
{
   return thread->info.data;
}

//
// ACSVM_Thread_GetLocalArr
//
ACSVM_Array *ACSVM_Thread_GetLocalArr(ACSVM_Thread *thread, ACSVM_Word idx)
{
   if(idx < thread->localArr.size())
      return reinterpret_cast<ACSVM_Array *>(&thread->localArr[idx]);
   else
      return nullptr;
}

//
// ACSVM_Thread_GetLocalReg
//
ACSVM_Word *ACSVM_Thread_GetLocalReg(ACSVM_Thread *thread, ACSVM_Word idx)
{
   if(idx < thread->localReg.size())
      return &thread->localReg[idx];
   else
      return nullptr;
}

//
// ACSVM_Thread_GetModule
//
ACSVM_Module *ACSVM_Thread_GetModule(ACSVM_Thread const *thread)
{
   return reinterpret_cast<ACSVM_Module *>(thread->module);
}

//
// ACSVM_Thread_GetPrintBuf
//
ACSVM_PrintBuf *ACSVM_Thread_GetPrintBuf(ACSVM_Thread *thread)
{
   return reinterpret_cast<ACSVM_PrintBuf *>(&thread->printBuf);
}

//
// ACSVM_Thread_GetResult
//
ACSVM_Word ACSVM_Thread_GetResult(ACSVM_Thread const *thread)
{
   return thread->result;
}

//
// ACSVM_Thread_GetScopeGbl
//
ACSVM_GlobalScope *ACSVM_Thread_GetScopeGbl(ACSVM_Thread const *thread)
{
   return reinterpret_cast<ACSVM_GlobalScope *>(thread->scopeGbl);
}

//
// ACSVM_Thread_GetScopeHub
//
ACSVM_HubScope *ACSVM_Thread_GetScopeHub(ACSVM_Thread const *thread)
{
   return reinterpret_cast<ACSVM_HubScope *>(thread->scopeHub);
}

//
// ACSVM_Thread_GetScopeMap
//
ACSVM_MapScope *ACSVM_Thread_GetScopeMap(ACSVM_Thread const *thread)
{
   return reinterpret_cast<ACSVM_MapScope *>(thread->scopeMap);
}

//
// ACSVM_Thread_GetScopeMod
//
ACSVM_ModuleScope *ACSVM_Thread_GetScopeMod(ACSVM_Thread const *thread)
{
   return reinterpret_cast<ACSVM_ModuleScope *>(thread->scopeMod);
}

//
// ACSVM_Thread_GetScript
//
ACSVM_Script *ACSVM_Thread_GetScript(ACSVM_Thread const *thread)
{
   return reinterpret_cast<ACSVM_Script *>(thread->script);
}

//
// ACSVM_Thread_GetState
//
ACSVM_ThreadState ACSVM_Thread_GetState(ACSVM_Thread const *thread)
{
   return
   {
      static_cast<ACSVM_ThreadStateEnum>(thread->state.state),
      thread->state.data, thread->state.type
   };
}

//
// ACSVM_Thread_DatakStk_Push
//
void ACSVM_Thread_DataStk_Push(ACSVM_Thread *thread, ACSVM_Word data)
{
   thread->dataStk.push(data);
}

//
// ACSVM_Thread_SetCodePtr
//
void ACSVM_Thread_SetCodePtr(ACSVM_Thread *thread, ACSVM_Word const *codePtr)
{
   thread->codePtr = codePtr;
}

//
// ACSVM_Thread_SetDelay
//
void ACSVM_Thread_SetDelay(ACSVM_Thread *thread, ACSVM_Word delay)
{
   thread->delay = delay;
}

//
// ACSVM_Thread_SetInfo
//
void ACSVM_Thread_SetInfo(ACSVM_Thread *thread, void *info)
{
   thread->info.data = info;
}

//
// ACSVM_Thread_SetResult
//
void ACSVM_Thread_SetResult(ACSVM_Thread *thread, ACSVM_Word result)
{
   thread->result = result;
}

//
// ACSVM_Thread_SetState
//
void ACSVM_Thread_SetState(ACSVM_Thread *thread, ACSVM_ThreadState state)
{
   thread->state =
      {static_cast<ACSVM::ThreadState::State>(state.state), state.data, state.type};
}

}

// EOF

