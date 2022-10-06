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

#include "Module.h"

#include "Environment.h"

#include "ACSVM/Error.hpp"


extern "C"
{

//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// ACSVM_Module_GetName
//
ACSVM_ModuleName ACSVM_Module_GetName(ACSVM_Module const *module_)
{
   auto module = reinterpret_cast<ACSVM::Module const *>(module_);

   return {reinterpret_cast<ACSVM_String *>(module->name.s), module->name.p, module->name.i};
}

//
// ACSVM_Module_ReadBytecode
//
bool ACSVM_Module_ReadBytecode(ACSVM_Module *module_, ACSVM_Byte const *data, size_t size)
{
   auto module = reinterpret_cast<ACSVM::Module *>(module_);

   try
   {
      module->readBytecode(data, size);
      return true;
   }
   catch(ACSVM::ReadError const &e)
   {
      auto env = static_cast<ACSVM_Environment *>(module->env);
      if(env->funcs.readError)
         env->funcs.readError(env, e.what());
      return false;
   }
   catch(std::bad_alloc const &e)
   {
      auto env = static_cast<ACSVM_Environment *>(module->env);
      if(env->funcs.bad_alloc)
         env->funcs.bad_alloc(env, e.what());
      return false;
   }
}

}

// EOF

