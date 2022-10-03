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

#ifndef ACSVM__CAPI__Thread_H__
#define ACSVM__CAPI__Thread_H__

#include "Types.h"

#ifdef __cplusplus
#include "../ACSVM/Thread.hpp"
#endif


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Types                                                                      |
//

//
// ACSVM_ThreadStateEnum
//
typedef enum ACSVM_ThreadStateEnum
{
   ACSVM_ThreadState_Inactive, // Inactive thread.
   ACSVM_ThreadState_Running,  // Running.
   ACSVM_ThreadState_Stopped,  // Will go inactive on next exec.
   ACSVM_ThreadState_Paused,   // Paused by instruction.
   ACSVM_ThreadState_WaitScrI, // Waiting on a numbered script.
   ACSVM_ThreadState_WaitScrS, // Waiting on a named script.
   ACSVM_ThreadState_WaitTag,  // Waiting on tagged object.
} ACSVM_ThreadStateEnum;

//
// ACSVM_ThreadFuncs
//
// ACSVM_Thread functions. If non-null, these get called after the base class.
//
typedef struct ACSVM_ThreadFuncs
{
   // public

   void (*ctor)(ACSVM_Thread *env);
   void (*dtor)(ACSVM_Thread *env);

   void (*loadState)(ACSVM_Thread *thread, ACSVM_Serial *in);

   void (*lockStrings)(ACSVM_Thread const *thread);

   void (*refStrings)(ACSVM_Thread const *thread);

   void (*saveState)(ACSVM_Thread const *thread, ACSVM_Serial *out);

   void (*start)(ACSVM_Thread *thread, void *data);

   void (*stop)(ACSVM_Thread *thread);

   void (*unlockStrings)(ACSVM_Thread const *thread);
} ACSVM_ThreadFuncs;

//
// ACSVM_ThreadState
//
// ACSVM::ThreadState mirror.
//
struct ACSVM_ThreadState
{
   ACSVM_ThreadStateEnum state;
   ACSVM_Word            data;
   ACSVM_Word            type;
};

#ifdef __cplusplus
//
// ACSVM_ThreadInfo
//
struct ACSVM_ThreadInfo : ACSVM::ThreadInfo
{
public:
   explicit ACSVM_ThreadInfo(void *data);
   virtual ~ACSVM_ThreadInfo();

   void *data;
};

//
// ACSVM_Thread
//
struct ACSVM_Thread : ACSVM::Thread
{
public:
   ACSVM_Thread(ACSVM_Environment *env, ACSVM_ThreadFuncs const &funcs, void *data);
   virtual ~ACSVM_Thread();

   virtual ACSVM::ThreadInfo const *getInfo() const;

   virtual void loadState(ACSVM::Serial &in);

   virtual void lockStrings() const;

   virtual void refStrings() const;

   virtual void saveState(ACSVM::Serial &out) const;

   virtual void start(ACSVM::Script *script, ACSVM::MapScope *map,
      ACSVM::ThreadInfo const *info, ACSVM::Word const *argV, ACSVM::Word argC);

   virtual void stop();

   virtual void unlockStrings() const;

   ACSVM_ThreadFuncs funcs;
   ACSVM_ThreadInfo  info;
};
#endif


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

ACSVM_Thread *ACSVM_AllocThread(ACSVM_Environment *env,
   ACSVM_ThreadFuncs const *funcs, void *data);

ACSVM_Thread *ACSVM_ThreadFromVoid(void *thread);

void ACSVM_Thread_Exec(ACSVM_Thread *thread);

ACSVM_Word const  *ACSVM_Thread_GetCodePtr (ACSVM_Thread const *thread);
ACSVM_Word         ACSVM_Thread_GetDelay   (ACSVM_Thread const *thread);
ACSVM_Environment *ACSVM_Thread_GetEnv     (ACSVM_Thread const *thread);
void              *ACSVM_Thread_GetInfo    (ACSVM_Thread const *thread);
ACSVM_Array       *ACSVM_Thread_GetLocalArr(ACSVM_Thread       *thread, ACSVM_Word idx);
ACSVM_Word        *ACSVM_Thread_GetLocalReg(ACSVM_Thread       *thread, ACSVM_Word idx);
ACSVM_Module      *ACSVM_Thread_GetModule  (ACSVM_Thread const *thread);
ACSVM_PrintBuf    *ACSVM_Thread_GetPrintBuf(ACSVM_Thread       *thread);
ACSVM_Word         ACSVM_Thread_GetResult  (ACSVM_Thread const *thread);
ACSVM_GlobalScope *ACSVM_Thread_GetScopeGbl(ACSVM_Thread const *thread);
ACSVM_HubScope    *ACSVM_Thread_GetScopeHub(ACSVM_Thread const *thread);
ACSVM_MapScope    *ACSVM_Thread_GetScopeMap(ACSVM_Thread const *thread);
ACSVM_ModuleScope *ACSVM_Thread_GetScopeMod(ACSVM_Thread const *thread);
ACSVM_Script      *ACSVM_Thread_GetScript  (ACSVM_Thread const *thread);
ACSVM_ThreadState  ACSVM_Thread_GetState   (ACSVM_Thread const *thread);

void ACSVM_Thread_DataStk_Push(ACSVM_Thread *thread, ACSVM_Word data);

void ACSVM_Thread_SetCodePtr(ACSVM_Thread *thread, ACSVM_Word const *codePtr);
void ACSVM_Thread_SetDelay  (ACSVM_Thread *thread, ACSVM_Word        delay);
void ACSVM_Thread_SetInfo   (ACSVM_Thread *thread, void             *info);
void ACSVM_Thread_SetResult (ACSVM_Thread *thread, ACSVM_Word        result);
void ACSVM_Thread_SetState  (ACSVM_Thread *thread, ACSVM_ThreadState state);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Thread_H__

