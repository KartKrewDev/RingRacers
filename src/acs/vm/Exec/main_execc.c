//----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//----------------------------------------------------------------------------
//
// Program entry point.
//
//----------------------------------------------------------------------------

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//----------------------------------------------------------------------------|
// Static Objects                                                             |
//

static ACSVM_Word ExecTime = 0;


//----------------------------------------------------------------------------|
// Static Functions                                                           |
//

//
// CF_EndPrint
//
static bool CF_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
   (void)argV; (void)argC;

   ACSVM_PrintBuf *buf = ACSVM_Thread_GetPrintBuf(thread);

   printf("%s\n", ACSVM_PrintBuf_GetData(buf));
   ACSVM_PrintBuf_Drop(buf);

   return false;
}

//
// CF_Timer
//
static bool CF_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
   (void)argV; (void)argC;

   ACSVM_Thread_DataStk_Push(thread, ExecTime);
   return false;
}

//
// Environment_BadAlloc
//
static void Environment_BadAlloc(ACSVM_Environment *env, char const *what)
{
   (void)env;

   fprintf(stderr, "bad_alloc: %s\n", what);
   exit(EXIT_FAILURE);
}

//
// Environment_Construct
//
static void Environment_Construct(ACSVM_Environment *env)
{
   ACSVM_Word funcEndPrint = ACSVM_Environment_AddCallFunc(env, CF_EndPrint);
   ACSVM_Word funcTimer    = ACSVM_Environment_AddCallFunc(env, CF_Timer);

   ACSVM_Environment_AddCodeDataACS0(env,  86, "", ACSVM_Code_CallFunc, 0, funcEndPrint);
   ACSVM_Environment_AddCodeDataACS0(env,  93, "", ACSVM_Code_CallFunc, 0, funcTimer);
   ACSVM_Environment_AddCodeDataACS0(env, 270, "", ACSVM_Code_CallFunc, 0, funcEndPrint);
}

//
// Environment_LoadModule
//
static bool Environment_LoadModule(ACSVM_Environment *env, ACSVM_Module *module)
{
   (void)env;

   ACSVM_ModuleName name = ACSVM_Module_GetName(module);

   FILE *stream = fopen(ACSVM_String_GetStr(name.s), "rb");
   if(!stream)
   {
      fprintf(stderr, "failed to fopen: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   ACSVM_Byte *data  = NULL;
   size_t      size  = 0;
   size_t      alloc = 0;

   for(int c; (c = fgetc(stream)) != EOF;)
   {
      if(size == alloc)
      {
         alloc = alloc + alloc / 2 + 256;
         data = realloc(data, alloc);
         if(!data)
         {
            fprintf(stderr, "failed to allocate %zu\n", alloc);
            exit(EXIT_FAILURE);
         }
      }

      data[size++] = c;
   }

   fclose(stream);

   bool res = ACSVM_Module_ReadBytecode(module, data, size);

   free(data);

   return res;
}

//
// Environment_ReadError
//
static void Environment_ReadError(ACSVM_Environment *env, char const *what)
{
   (void)env;

   fprintf(stderr, "ReadError: %s\n", what);
   exit(EXIT_FAILURE);
}

//
// LoadModules
//
static void LoadModules(ACSVM_Environment *env, char **argv, int argc)
{
   ACSVM_StringTable *strTab = ACSVM_Environment_GetStringTable(env);

   // Create and activate scopes.
   ACSVM_GlobalScope *global = ACSVM_Environment_GetGlobalScope(env, 0);
   ACSVM_GlobalScope_SetActive(global, true);
   ACSVM_HubScope *hub = ACSVM_GlobalScope_GetHubScope(global, 0);
   ACSVM_HubScope_SetActive(hub, true);
   ACSVM_MapScope *map = ACSVM_HubScope_GetMapScope(hub, 0);
   ACSVM_MapScope_SetActive(map, true);

   // Load modules.

   size_t         moduleC = argc - 1;
   ACSVM_Module **moduleV = malloc(sizeof(ACSVM_Module *) * moduleC);
   if(!moduleV)
   {
      fprintf(stderr, "failed to alloc moduleV[%zu]\n", moduleC);
      exit(EXIT_FAILURE);
   }

   for(int argi = 1; argi < argc; ++argi)
   {
      char const      *arg  = argv[argi];
      size_t           len  = strlen(arg);
      size_t           hash = ACSVM_StrHash(arg, len);
      ACSVM_ModuleName name =
         {ACSVM_StringTable_GetStringByData(strTab, arg, len, hash), NULL, 0};

      moduleV[argi - 1] = ACSVM_Environment_GetModule(env, name);
   }

   // Register modules with map scope.
   ACSVM_MapScope_AddModules(map, moduleV, moduleC);

   // Start Open scripts.
   ACSVM_MapScope_ScriptStartType(map, 1, NULL, 0, NULL, NULL);
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// main
//
int main(int argc, char *argv[])
{
   ACSVM_Environment *env = ACSVM_AllocEnvironment(
      &(ACSVM_EnvironmentFuncs)
      {
         .bad_alloc = Environment_BadAlloc,
         .readError = Environment_ReadError,

         .ctor = Environment_Construct,

         .loadModule = Environment_LoadModule,
      },
      NULL);

   if(!env)
   {
      fprintf(stderr, "failed to allocate environment\n");
      return EXIT_FAILURE;
   }

   LoadModules(env, argv, argc);

   while(ACSVM_Environment_HasActiveThread(env))
   {
      ++ExecTime;
      ACSVM_Environment_Exec(env);

      // TODO: Sleep.
   }

   ACSVM_FreeEnvironment(env);

   return EXIT_SUCCESS;
}

// EOF

