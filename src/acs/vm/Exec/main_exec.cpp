//----------------------------------------------------------------------------
//
// Copyright (C) 2015-2026 David Hill
//
// See COPYING for license information.
//
//----------------------------------------------------------------------------
//
// Program entry point.
//
//----------------------------------------------------------------------------

#include "ACSVM/Action.hpp"
#include "ACSVM/Code.hpp"
#include "ACSVM/CodeData.hpp"
#include "ACSVM/Environment.hpp"
#include "ACSVM/Error.hpp"
#include "ACSVM/Function.hpp"
#include "ACSVM/Module.hpp"
#include "ACSVM/ProfileData.hpp"
#include "ACSVM/Scope.hpp"
#include "ACSVM/Script.hpp"
#include "ACSVM/SerialSTD.hpp"
#include "ACSVM/Thread.hpp"

#include "Util/Floats.hpp"

#include <algorithm>
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

   virtual ACSVM::ProfileTime getProfileTime() const
   {
      return std::chrono::duration_cast<std::chrono::duration<ACSVM::ProfileTime>>(
         std::chrono::steady_clock::now().time_since_epoch()).count() + 1.0;
   }

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

static void PrintProfileData(std::ostream &out, ACSVM::Environment *env,
   ACSVM::Word minCall, ACSVM::ProfileTime minTime);

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

   std::cout << std::flush;

   return false;
}

//
// void ACSVM_DumpProfileData(int minCall, fixed minTime)
//
static bool CF_DumpProfileData(ACSVM::Thread *thread, ACSVM::Word const *argv, ACSVM::Word argc)
{
   ACSVM::Word        minCall = argc > 0 ? argv[0] : 0;
   ACSVM::ProfileTime minTime = argc > 1 ? static_cast<ACSVM::ProfileTime>(argv[1]) / 65536 : 0;

   PrintProfileData(std::cout, thread->env, minCall, minTime);

   return false;
}

//
// CF_DumpStack
//
static bool CF_DumpStack(ACSVM::Thread *thread, ACSVM::Word const *argv, ACSVM::Word argc)
{
   // Stack info.
   std::cout << "Stack=" << thread->dataStk.begin() << '+' << thread->dataStk.size() << "\n";

   // Stack values.
   if(argc >= 1 && argv[0])
      for(std::size_t i = 0, e = thread->dataStk.size(); i != e; ++i)
         std::cout << "  [" << i << "]=" << thread->dataStk[i] << '\n';

   std::cout << std::flush;

   return false;
}

//
// CF_EndPrint
//
static bool CF_EndPrint(ACSVM::Thread *thread, ACSVM::Word const *, ACSVM::Word)
{
   std::cout << thread->printBuf.data() << std::endl;
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
// int ACSVM_ExecuteDelayed(int script, int map, int delay, ...)
//
static bool CF_ExecuteDelayed(ACSVM::Thread *thread, ACSVM::Word const *argV, ACSVM::Word argC)
{
   ACSVM::Word    script = argV[0];
   ACSVM::ScopeID scope  = thread->env->getScopeID(argV[1]);
   ACSVM::Word    delay  = argV[2];

   auto func = [delay](ACSVM::Thread *thread)
   {
      thread->delay = delay;
   };

   thread->dataStk.push(thread->scopeMap->scriptStart(
      script, scope, {argV+3, argC-3, thread->getInfo(), func}));

   return false;
}

//
// void ACSVM_ResetProfileData(void)
//
static bool CF_ResetProfileData(ACSVM::Thread *thread, ACSVM::Word const *, ACSVM::Word)
{
   thread->env->resetProfileData();
   return false;
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

//
// PrintProfileData
//
static void PrintProfileData(std::ostream &out, ACSVM::Environment *env,
   ACSVM::Word minCall, ACSVM::ProfileTime minTime)
{
   struct ProfileDataPrint
   {
      bool operator < (ProfileDataPrint const &pdp) {return pd->timeTot < pdp.pd->timeTot;}

      ACSVM::ProfileData *pd;
      ACSVM::String      *ns;
      ACSVM::Word         ni;
   };

   std::vector<ProfileDataPrint> pdpV;

   for(auto &prof : env->getGlobalScope(0)->getHubScope(0)->getMapScope(0)->profileFunction)
   {
      if(!prof.val.timeNum) continue;
      if(prof.val.callNum < minCall) continue;
      if(prof.val.timeTot < minTime) continue;

      pdpV.push_back({&prof.val, prof.key->name, 0});
   }

   for(auto &prof : env->getGlobalScope(0)->getHubScope(0)->getMapScope(0)->profileScript)
   {
      if(!prof.val.timeNum) continue;
      if(prof.val.callNum < minCall) continue;
      if(prof.val.timeTot < minTime) continue;

      pdpV.push_back({&prof.val, prof.key->name.s, prof.key->name.i});
   }

   std::sort(pdpV.begin(), pdpV.end());

   out << std::fixed;

   for(auto const &pdp : pdpV)
   {
      if(pdp.ns)
         out << "PD: " << pdp.ns->str;
      else
         out << "PD: " << pdp.ni;
      out << " " << pdp.pd->callNum << "/" << pdp.pd->timeNum << "\n";

      if(pdp.pd->callNum)
      {
         out << "   call: "
            << pdp.pd->callMin << " / "
            << (pdp.pd->timeTot / pdp.pd->callNum) << " / "
            << pdp.pd->callMax << "\n";
      }

      out << "   time: "
         << pdp.pd->timeMin << " / "
         << (pdp.pd->timeTot / pdp.pd->timeNum) << " / "
         << pdp.pd->timeMax << " / "
         << pdp.pd->timeTot << "\n";

      out << std::endl;
   }
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
   ACSVM::Word funcEndPrint = addCallFunc(CF_EndPrint);

   addCodeDataACS0( 86, {"", 0, funcEndPrint});
   addCodeDataACS0( 93, {"", 0, addCallFunc(CF_Timer)});
   addCodeDataACS0(270, {"", 0, funcEndPrint});

   addFuncDataACS0(0x10000, addCallFunc(CF_TestSave));
   addFuncDataACS0(0x10001, addCallFunc(CF_CollectStrings));
   addFuncDataACS0(0x10002, addCallFunc(CF_DumpLocals));
   addFuncDataACS0(0x10003, addCallFunc(CF_Exit));
   addFuncDataACS0(0x10004, addCallFunc(CF_DumpStack));
   addFuncDataACS0(0x10005, addCallFunc(CF_ExecuteDelayed));
   addFuncDataACS0(0x10006, addCallFunc(CF_DumpProfileData));
   addFuncDataACS0(0x10007, addCallFunc(CF_ResetProfileData));

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
            ACSVM::SerialSTD out{static_cast<std::ostream &>(buf)};
            out.signs = true;
            out.saveHead();
            env.saveState(out);
            out.saveTail();
         }

         {
            ACSVM::SerialSTD in{static_cast<std::istream &>(buf)};
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

