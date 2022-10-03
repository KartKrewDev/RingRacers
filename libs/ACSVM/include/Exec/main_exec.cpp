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

#include "ACSVM/Code.hpp"
#include "ACSVM/CodeData.hpp"
#include "ACSVM/Environment.hpp"
#include "ACSVM/Error.hpp"
#include "ACSVM/Module.hpp"
#include "ACSVM/Scope.hpp"
#include "ACSVM/Script.hpp"
#include "ACSVM/Serial.hpp"
#include "ACSVM/Thread.hpp"

#include "Util/Floats.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

//
// Environment
//
class Environment : public ACSVM::Environment
{
public:
   Environment();

   virtual void exec() {++timer; ACSVM::Environment::exec();}

   ACSVM::Word timer;

protected:
   virtual void loadModule(ACSVM::Module *module);
};


//----------------------------------------------------------------------------|
// Static Objects                                                             |
//

static bool NeedExit        = false;
static bool NeedTestSaveEnv = false;


//----------------------------------------------------------------------------|
// Static Functions                                                           |
//

//
// CF_CollectStrings
//
static bool CF_CollectStrings(ACSVM::Thread *thread, ACSVM::Word const *, ACSVM::Word)
{
   std::size_t countOld = thread->env->stringTable.size();
   thread->env->collectStrings();
   std::size_t countNew = thread->env->stringTable.size();
   thread->dataStk.push(countOld - countNew);
   return false;
}

//
// CF_DumpLocals
//
static bool CF_DumpLocals(ACSVM::Thread *thread, ACSVM::Word const *, ACSVM::Word)
{
   // LocReg store info.
   std::cout << "LocReg="
      << thread->localReg.begin()     << '+' << thread->localReg.size()     << " / "
      << thread->localReg.beginFull() << '+' << thread->localReg.sizeFull() << "\n";

   // LocReg values for current function.
   for(std::size_t i = 0, e = thread->localReg.size(); i != e; ++i)
      std::cout << "  [" << i << "]=" << thread->localReg[i] << '\n';

   return false;
}

//
// CF_EndPrint
//
static bool CF_EndPrint(ACSVM::Thread *thread, ACSVM::Word const *, ACSVM::Word)
{
   std::cout << thread->printBuf.data() << '\n';
   thread->printBuf.drop();
   return false;
}

//
// CF_Exit
//
static bool CF_Exit(ACSVM::Thread *thread, ACSVM::Word const *, ACSVM::Word)
{
   NeedExit = true;
   thread->state = ACSVM::ThreadState::Stopped;
   return true;
}

//
// CF_TestSave
//
static bool CF_TestSave(ACSVM::Thread *, ACSVM::Word const *, ACSVM::Word)
{
   NeedTestSaveEnv = true;
   return false;
}

//
// CF_Timer
//
static bool CF_Timer(ACSVM::Thread *thread, ACSVM::Word const *, ACSVM::Word)
{
   thread->dataStk.push(static_cast<Environment *>(thread->env)->timer);
   return false;
}

//
// LoadModules
//
static void LoadModules(Environment &env, char const *const *argv, std::size_t argc)
{
   // Load modules.
   std::vector<ACSVM::Module *> modules;
   for(std::size_t i = 1; i < argc; ++i)
      modules.push_back(env.getModule(env.getModuleName(argv[i])));

   // Create and activate scopes.
   ACSVM::GlobalScope *global = env.getGlobalScope(0);  global->active = true;
   ACSVM::HubScope    *hub    = global->getHubScope(0); hub   ->active = true;
   ACSVM::MapScope    *map    = hub->getMapScope(0);    map   ->active = true;

   // Register modules with map scope.
   map->addModules(modules.data(), modules.size());

   // Start Open scripts.
   map->scriptStartType(1, {});
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

//
// Environment constructor
//
Environment::Environment() :
   timer{0}
{
   ACSVM::Word funcCollectStrings = addCallFunc(CF_CollectStrings);
   ACSVM::Word funcDumpLocals     = addCallFunc(CF_DumpLocals);
   ACSVM::Word funcEndPrint       = addCallFunc(CF_EndPrint);
   ACSVM::Word funcExit           = addCallFunc(CF_Exit);
   ACSVM::Word funcTestSave       = addCallFunc(CF_TestSave);
   ACSVM::Word funcTimer          = addCallFunc(CF_Timer);

   addCodeDataACS0( 86, {"", 0, funcEndPrint});
   addCodeDataACS0( 93, {"", 0, funcTimer});
   addCodeDataACS0(270, {"", 0, funcEndPrint});

   addFuncDataACS0(0x10000, funcTestSave);
   addFuncDataACS0(0x10001, funcCollectStrings);
   addFuncDataACS0(0x10002, funcDumpLocals);
   addFuncDataACS0(0x10003, funcExit);

   addFuncDataACS0(0x10100, addCallFunc(ACSVM::CF_AddF_W1));
   addFuncDataACS0(0x10101, addCallFunc(ACSVM::CF_DivF_W1));
   addFuncDataACS0(0x10102, addCallFunc(ACSVM::CF_MulF_W1));
   addFuncDataACS0(0x10103, addCallFunc(ACSVM::CF_SubF_W1));
   addFuncDataACS0(0x10104, addCallFunc(ACSVM::CF_AddF_W2));
   addFuncDataACS0(0x10105, addCallFunc(ACSVM::CF_DivF_W2));
   addFuncDataACS0(0x10106, addCallFunc(ACSVM::CF_MulF_W2));
   addFuncDataACS0(0x10107, addCallFunc(ACSVM::CF_SubF_W2));
   addFuncDataACS0(0x10108, addCallFunc(ACSVM::CF_PrintFloat));
   addFuncDataACS0(0x10109, addCallFunc(ACSVM::CF_PrintDouble));
}

//
// Environment::loadModule
//
void Environment::loadModule(ACSVM::Module *module)
{
   std::ifstream in{module->name.s->str, std::ios_base::in | std::ios_base::binary};

   if(!in) throw ACSVM::ReadError("file open failure");

   std::vector<ACSVM::Byte> data;

   for(int c; c = in.get(), in;)
      data.push_back(c);

   module->readBytecode(data.data(), data.size());
}

//
// main
//
int main(int argc, char *argv[])
{
   Environment env;

   // Load modules.
   try
   {
      LoadModules(env, argv, argc);
   }
   catch(ACSVM::ReadError &e)
   {
      std::cerr << "Error loading modules: " << e.what() << std::endl;
      return EXIT_FAILURE;
   }

   // Execute until all threads terminate.
   while(!NeedExit && env.hasActiveThread())
   {
      std::chrono::duration<double> rate{1.0 / 35};
      auto time = std::chrono::steady_clock::now() + rate;

      env.exec();

      if(NeedTestSaveEnv)
      {
         std::stringstream buf;

         {
            ACSVM::Serial out{static_cast<std::ostream &>(buf)};
            out.signs = true;
            out.saveHead();
            env.saveState(out);
            out.saveTail();
         }

         {
            ACSVM::Serial in{static_cast<std::istream &>(buf)};
            in.loadHead();
            env.loadState(in);
            in.loadTail();
         }

         NeedTestSaveEnv = false;
      }

      std::this_thread::sleep_until(time);
   }
}

// EOF

