//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Common typedefs and struct declarations.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CAPI__Types_H__
#define ACSVM__CAPI__Types_H__

#ifdef __cplusplus
#include "../ACSVM/Types.hpp"
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------|
// Types                                                                      |
//

typedef unsigned char ACSVM_Byte;
typedef uint64_t      ACSVM_DWord;
typedef int64_t       ACSVM_SDWord;
typedef int32_t       ACSVM_SWord;
typedef uint32_t      ACSVM_Word;

typedef struct ACSVM_Array        ACSVM_Array;
typedef struct ACSVM_Environment  ACSVM_Environment;
typedef struct ACSVM_GlobalScope  ACSVM_GlobalScope;
typedef struct ACSVM_HubScope     ACSVM_HubScope;
typedef struct ACSVM_IStream      ACSVM_IStream;
typedef struct ACSVM_MapScope     ACSVM_MapScope;
typedef struct ACSVM_Module       ACSVM_Module;
typedef struct ACSVM_ModuleName   ACSVM_ModuleName;
typedef struct ACSVM_ModuleScope  ACSVM_ModuleScope;
typedef struct ACSVM_OStream      ACSVM_OStream;
typedef struct ACSVM_PrintBuf     ACSVM_PrintBuf;
typedef struct ACSVM_ScopeID      ACSVM_ScopeID;
typedef struct ACSVM_Script       ACSVM_Script;
typedef struct ACSVM_ScriptName   ACSVM_ScriptName;
typedef struct ACSVM_Serial       ACSVM_Serial;
typedef struct ACSVM_String       ACSVM_String;
typedef struct ACSVM_StringTable  ACSVM_StringTable;
typedef struct ACSVM_Thread       ACSVM_Thread;
typedef struct ACSVM_ThreadInfo   ACSVM_ThreadInfo;
typedef struct ACSVM_ThreadState  ACSVM_ThreadState;

typedef bool (*ACSVM_CallFunc)(ACSVM_Thread *, ACSVM_Word const *, ACSVM_Word);

#ifdef __cplusplus
}
#endif

#endif//ACSVM__CAPI__Types_H__

