//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2026 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Scope classes.
//
//-----------------------------------------------------------------------------

#include "Scope.hpp"

#include "Action.hpp"
#include "BinaryIO.hpp"
#include "Environment.hpp"
#include "Error.hpp"
#include "Function.hpp"
#include "HashMap.hpp"
#include "HashMapFixed.hpp"
#include "Init.hpp"
#include "Module.hpp"
#include "ProfileData.hpp"
#include "Script.hpp"
#include "Serial.hpp"
#include "Thread.hpp"

#include <algorithm>
#include <unordered_set>
#include <vector>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // GlobalScope::PrivData
   //
   struct GlobalScope::PrivData
   {
      HashMapKeyMem<Word, HubScope, &HubScope::id, &HubScope::hashLink> scopes;
   };

   //
   // HubScope::PrivData
   //
   struct HubScope::PrivData
   {
      HashMapKeyMem<Word, MapScope, &MapScope::id, &MapScope::hashLink> scopes;
   };

   //
   // MapScope::PrivData
   //
   struct MapScope::PrivData
   {
      HashMapFixed<Module *, ModuleScope> scopes;

      HashMapFixed<Word,     Script *> scriptInt;
      HashMapFixed<String *, Script *> scriptStr;

      HashMapFixed<Script *, Thread *> scriptThread;
   };
}


//----------------------------------------------------------------------------|
// Extern Objects                                                             |
//

namespace ACSVM
{
   constexpr std::size_t ModuleScope::ArrC;
   constexpr std::size_t ModuleScope::RegC;
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // GlobalScope constructor
   //
   GlobalScope::GlobalScope(Environment *env_, Word id_) :
      env{env_},
      id {id_},

      arrV{},
      regV{},

      hashLink{this},

      active{false},

      pd{new PrivData}
   {
   }

   //
   // GlobalScope destructor
   //
   GlobalScope::~GlobalScope()
   {
      reset();
      delete pd;
   }

   //
   // GlobalScope::countActiveThread
   //
   std::size_t GlobalScope::countActiveThread() const
   {
      std::size_t n = 0;

      for(auto &scope : pd->scopes)
      {
         if(scope.active)
            n += scope.countActiveThread();
      }

      return n;
   }

   //
   // GlobalScope::exec
   //
   void GlobalScope::exec()
   {
      // Delegate deferred script actions.
      for(auto itr = scriptAction.begin(), end = scriptAction.end(); itr != end;)
      {
         auto scope = pd->scopes.find(itr->id.hub);
         if(scope && scope->active)
            itr++->link.relink(&scope->scriptAction);
         else
            ++itr;
      }

      for(auto &scope : pd->scopes)
      {
         if(scope.active)
            scope.exec();
      }
   }

   //
   // GlobalScope::freeHubScope
   //
   void GlobalScope::freeHubScope(HubScope *scope)
   {
      pd->scopes.unlink(scope);
      delete scope;
   }

   //
   // GlobalScope::getHubScope
   //
   HubScope *GlobalScope::getHubScope(Word scopeID)
   {
      if(auto *scope = pd->scopes.find(scopeID))
         return scope;

      auto scope = new HubScope(this, scopeID);
      pd->scopes.insert(scope);
      return scope;
   }

   //
   // GlobalScope::hasActiveThread
   //
   bool GlobalScope::hasActiveThread() const
   {
      for(auto &scope : pd->scopes)
      {
         if(scope.active && scope.hasActiveThread())
            return true;
      }

      return false;
   }

   //
   // GlobalScope::loadState
   //
   void GlobalScope::loadState(Serial &in)
   {
      reset();

      in.readSign(Signature::GlobalScope);

      LoadArrV(in, arrV, ArrC);
      LoadRegV(in, regV, RegC);

      env->readScriptActions(in, scriptAction);

      active = in.readByte();

      for(auto n = ReadVLN<std::size_t>(in); n--;)
         getHubScope(ReadVLN<Word>(in))->loadState(in);

      in.readSign(~Signature::GlobalScope);
   }

   //
   // GlobalScope::lockStrings
   //
   void GlobalScope::lockStrings() const
   {
      for(auto &arr : arrV) arr.lockStrings(env);
      for(auto &reg : regV) ++env->getString(reg)->lock;

      for(auto &action : scriptAction)
         action.lockStrings(env);

      for(auto &scope : pd->scopes)
         scope.lockStrings();
   }

   //
   // GlobalScope::refStrings
   //
   void GlobalScope::refStrings() const
   {
      for(auto &arr : arrV) arr.refStrings(env);
      for(auto &reg : regV) env->getString(reg)->ref = true;

      for(auto &action : scriptAction)
         action.refStrings(env);

      for(auto &scope : pd->scopes)
         scope.refStrings();
   }

   //
   // GlobalScope::reset
   //
   void GlobalScope::reset()
   {
      for(auto &arr : arrV) arr.clear();
      for(auto &reg : regV) reg = 0;

      while(scriptAction.next->obj)
         delete scriptAction.next->obj;

      active = false;

      pd->scopes.free();
   }

   //
   // GlobalScope::resetProfileData
   //
   void GlobalScope::resetProfileData()
   {
      for(auto &scope : pd->scopes)
         scope.resetProfileData();
   }

   //
   // GlobalScope::saveState
   //
   void GlobalScope::saveState(Serial &out) const
   {
      out.writeSign(Signature::GlobalScope);

      SaveArrV(out, arrV, ArrC);
      SaveRegV(out, regV, RegC);

      env->writeScriptActions(out, scriptAction);

      out.writeByte(active);

      WriteVLN(out, pd->scopes.size());
      for(auto &scope : pd->scopes)
      {
         WriteVLN(out, scope.id);
         scope.saveState(out);
      }

      out.writeSign(~Signature::GlobalScope);
   }

   //
   // GlobalScope::unlockStrings
   //
   void GlobalScope::unlockStrings() const
   {
      for(auto &arr : arrV) arr.unlockStrings(env);
      for(auto &reg : regV) --env->getString(reg)->lock;

      for(auto &action : scriptAction)
         action.unlockStrings(env);

      for(auto &scope : pd->scopes)
         scope.unlockStrings();
   }

   //
   // GlobalScope::LoadArrV
   //
   void GlobalScope::LoadArrV(Serial &in, Array *arrV, std::size_t arrC)
   {
      std::size_t inC = in.version == 0 ? 256 : ReadVLN<std::size_t>(in);

      if(inC > arrC)
         throw SerialError("too many arrays");

      while(inC--)
         arrV++->loadState(in);
   }

   //
   // GlobalScope::LoadRegV
   //
   void GlobalScope::LoadRegV(Serial &in, Word *regV, std::size_t regC)
   {
      std::size_t inC = in.version == 0 ? 256 : ReadVLN<std::size_t>(in);

      if(inC > regC)
         throw SerialError("too many registers");

      while(inC--)
         *regV++ = ReadVLN<Word>(in);
   }

   //
   // GlobalScope::SaveArrV
   //
   void GlobalScope::SaveArrV(Serial &out, Array const *arrV, std::size_t arrC)
   {
      // Skip trailing empty arrays.
      while(arrC && !arrV[arrC - 1].empty())
         --arrC;

      WriteVLN(out, arrC);
      while(arrC--)
         arrV++->saveState(out);
   }

   //
   // GlobalScope::SaveRegV
   //
   void GlobalScope::SaveRegV(Serial &out, Word const *regV, std::size_t regC)
   {
      // Skip trailing 0 registers.
      while(regC && !regV[regC - 1])
         --regC;

      WriteVLN(out, regC);
      while(regC--)
         WriteVLN(out, *regV++);
   }

   //
   // HubScope constructor
   //
   HubScope::HubScope(GlobalScope *global_, Word id_) :
      env   {global_->env},
      global{global_},
      id    {id_},

      arrV{},
      regV{},

      hashLink{this},

      active{false},

      pd{new PrivData}
   {
   }

   //
   // HubScope destructor
   //
   HubScope::~HubScope()
   {
      reset();
      delete pd;
   }

   //
   // HubScope::countActiveThread
   //
   std::size_t HubScope::countActiveThread() const
   {
      std::size_t n = 0;

      for(auto &scope : pd->scopes)
      {
         if(scope.active)
            n += scope.countActiveThread();
      }

      return n;
   }

   //
   // HubScope::exec
   //
   void HubScope::exec()
   {
      // Delegate deferred script actions.
      for(auto itr = scriptAction.begin(), end = scriptAction.end(); itr != end;)
      {
         auto scope = pd->scopes.find(itr->id.map);
         if(scope && scope->active)
            itr++->link.relink(&scope->scriptAction);
         else
            ++itr;
      }

      for(auto &scope : pd->scopes)
      {
         if(scope.active)
            scope.exec();
      }
   }

   //
   // HubScope::freeMapScope
   //
   void HubScope::freeMapScope(MapScope *scope)
   {
      pd->scopes.unlink(scope);
      delete scope;
   }

   //
   // HubScope::getMapScope
   //
   MapScope *HubScope::getMapScope(Word scopeID)
   {
      if(auto *scope = pd->scopes.find(scopeID))
         return scope;

      auto scope = new MapScope(this, scopeID);
      pd->scopes.insert(scope);
      return scope;
   }

   //
   // HubScope::hasActiveThread
   //
   bool HubScope::hasActiveThread() const
   {
      for(auto &scope : pd->scopes)
      {
         if(scope.active && scope.hasActiveThread())
            return true;
      }

      return false;
   }

   //
   // HubScope::loadState
   //
   void HubScope::loadState(Serial &in)
   {
      reset();

      in.readSign(Signature::HubScope);

      GlobalScope::LoadArrV(in, arrV, ArrC);
      GlobalScope::LoadRegV(in, regV, RegC);

      env->readScriptActions(in, scriptAction);

      active = in.readByte();

      for(auto n = ReadVLN<std::size_t>(in); n--;)
         getMapScope(ReadVLN<Word>(in))->loadState(in);

      in.readSign(~Signature::HubScope);
   }

   //
   // HubScope::lockStrings
   //
   void HubScope::lockStrings() const
   {
      for(auto &arr : arrV) arr.lockStrings(env);
      for(auto &reg : regV) ++env->getString(reg)->lock;

      for(auto &action : scriptAction)
         action.lockStrings(env);

      for(auto &scope : pd->scopes)
         scope.lockStrings();
   }

   //
   // HubScope::refStrings
   //
   void HubScope::refStrings() const
   {
      for(auto &arr : arrV) arr.refStrings(env);
      for(auto &reg : regV) env->getString(reg)->ref = true;

      for(auto &action : scriptAction)
         action.refStrings(env);

      for(auto &scope : pd->scopes)
         scope.refStrings();
   }

   //
   // HubScope::reset
   //
   void HubScope::reset()
   {
      for(auto &arr : arrV) arr.clear();
      for(auto &reg : regV) reg = 0;

      while(scriptAction.next->obj)
         delete scriptAction.next->obj;

      active = false;

      pd->scopes.free();
   }

   //
   // HubScope::resetProfileData
   //
   void HubScope::resetProfileData()
   {
      for(auto &scope : pd->scopes)
         scope.resetProfileData();
   }

   //
   // HubScope::saveState
   //
   void HubScope::saveState(Serial &out) const
   {
      out.writeSign(Signature::HubScope);

      GlobalScope::SaveArrV(out, arrV, ArrC);
      GlobalScope::SaveRegV(out, regV, RegC);

      env->writeScriptActions(out, scriptAction);

      out.writeByte(active);

      WriteVLN(out, pd->scopes.size());
      for(auto &scope : pd->scopes)
      {
         WriteVLN(out, scope.id);
         scope.saveState(out);
      }

      out.writeSign(~Signature::HubScope);
   }

   //
   // HubScope::unlockStrings
   //
   void HubScope::unlockStrings() const
   {
      for(auto &arr : arrV) arr.unlockStrings(env);
      for(auto &reg : regV) --env->getString(reg)->lock;

      for(auto &action : scriptAction)
         action.unlockStrings(env);

      for(auto &scope : pd->scopes)
         scope.unlockStrings();
   }

   //
   // MapScope constructor
   //
   MapScope::MapScope(HubScope *hub_, Word id_) :
      env{hub_->env},
      hub{hub_},
      id {id_},

      hashLink{this},

      module0{nullptr},

      active       {false},
      clampCallSpec{false},

      pd{new PrivData}
   {
   }

   //
   // MapScope destructor
   //
   MapScope::~MapScope()
   {
      reset();
      delete pd;
   }

   //
   // MapScope::addModules
   //
   void MapScope::addModules(Module *const *moduleV, std::size_t moduleC)
   {
      module0 = moduleC ? moduleV[0] : nullptr;

      // Find all associated modules.

      struct
      {
         std::unordered_set<Module *> set;
         std::vector<Module *>        vec;

         void add(Module *module)
         {
            if(!set.insert(module).second) return;

            vec.push_back(module);
            for(auto &import : module->importV)
               add(import);
         }
      } modules;

      for(auto itr = moduleV, end = itr + moduleC; itr != end; ++itr)
         modules.add(*itr);

      // Count functions and scripts.

      std::size_t funcC      = 0;
      std::size_t scriptC    = 0;
      std::size_t scriptIntC = 0;
      std::size_t scriptStrC = 0;

      for(auto &module : modules.vec)
      {
         for(auto &func : module->functionV)
            if(func->module == module)
               ++funcC;

         for(auto &script : module->scriptV)
         {
            ++scriptC;
            if(script.name.s)
               ++scriptStrC;
            else
               ++scriptIntC;
         }
      }

      // Create lookup tables.

      profileFunction.alloc(funcC);
      profileScript.alloc(scriptC);

      pd->scopes.alloc(modules.vec.size());
      pd->scriptInt.alloc(scriptIntC);
      pd->scriptStr.alloc(scriptStrC);
      pd->scriptThread.alloc(scriptC);

      auto profFuncItr = profileFunction.begin();
      auto profScrItr  = profileScript.begin();

      auto scopeItr     = pd->scopes.begin();
      auto scriptIntItr = pd->scriptInt.begin();
      auto scriptStrItr = pd->scriptStr.begin();
      auto scriptThrItr = pd->scriptThread.begin();

      for(auto &module : modules.vec)
      {
         using ElemScope = HashMapFixed<Module *, ModuleScope>::Elem;

         new(scopeItr++) ElemScope{module, {this, module}, nullptr};

         for(auto &func : module->functionV)
         {
            using ElemPF = HashMapFixed<Function *, ProfileData>::Elem;

            if(func->module == module)
               new(profFuncItr++) ElemPF{func, {}, nullptr};
         }

         for(auto &script : module->scriptV)
         {
            using ElemInt = HashMapFixed<Word,     Script *>::Elem;
            using ElemPS  = HashMapFixed<Script *, ProfileData>::Elem;
            using ElemStr = HashMapFixed<String *, Script *>::Elem;
            using ElemThr = HashMapFixed<Script *, Thread *>::Elem;

            new(profScrItr++) ElemPS{&script, {}, nullptr};

            new(scriptThrItr++) ElemThr{&script, nullptr, nullptr};

            if(script.name.s)
               new(scriptStrItr++) ElemStr{script.name.s, &script, nullptr};
            else
               new(scriptIntItr++) ElemInt{script.name.i, &script, nullptr};
         }
      }

      profileFunction.build();
      profileScript.build();

      pd->scopes.build();
      pd->scriptInt.build();
      pd->scriptStr.build();
      pd->scriptThread.build();

      for(auto &scope : pd->scopes)
         scope.val.import();
   }

   //
   // MapScope::addProfileCall Function
   //
   void MapScope::addProfileCall(Function *func, ProfileTime pt)
   {
      if(auto lookup = profileFunction.find(func))
      {
         lookup->addTime(pt);
         lookup->addCall();
      }
   }

   //
   // MapScope::addProfileCall Script
   //
   void MapScope::addProfileCall(Script *script, ProfileTime pt)
   {
      if(auto lookup = profileScript.find(script))
      {
         lookup->addTime(pt);
         lookup->addCall();
      }
   }

   //
   // MapScope::addProfileTime Function
   //
   void MapScope::addProfileTime(Function *func, ProfileTime pt)
   {
      if(auto lookup = profileFunction.find(func))
         lookup->addTime(pt);
   }

   //
   // MapScope::addProfileTime Script
   //
   void MapScope::addProfileTime(Script *script, ProfileTime pt)
   {
      if(auto lookup = profileScript.find(script))
         lookup->addTime(pt);
   }

   //
   // MapScope::countActiveThread
   //
   std::size_t MapScope::countActiveThread() const
   {
      return threadActive.size();
   }

   //
   // MapScope::exec
   //
   void MapScope::exec()
   {
      // Execute deferred script actions.
      while(scriptAction.next->obj)
      {
         ScriptAction *action = scriptAction.next->obj;
         Script       *script = findScript(action->name);

         if(script) switch(action->action)
         {
         case ScriptAction::Start:
            scriptStart(script, {action->argV.data(), action->argV.size(),
               nullptr, env->funcScriptStartDeferred});
            break;

         case ScriptAction::StartForced:
            scriptStartForced(script, {action->argV.data(), action->argV.size(),
               nullptr, env->funcScriptStartForcedDeferred});
            break;

         case ScriptAction::Stop:
            scriptStop(script);
            break;

         case ScriptAction::Pause:
            scriptPause(script);
            break;
         }

         delete action;
      }

      // Execute running threads.
      for(auto itr = threadActive.begin(), end = threadActive.end(); itr != end;)
      {
         itr->exec();
         if(itr->state == ThreadState::Inactive)
            freeThread(&*itr++);
         else
            ++itr;
      }
   }

   //
   // MapScope::findScript
   //
   Script *MapScope::findScript(ScriptName name)
   {
      return name.s ? findScript(name.s) : findScript(name.i);
   }

   //
   // MapScope::findScript
   //
   Script *MapScope::findScript(String *name)
   {
      if(Script **script = pd->scriptStr.find(name))
         return *script;
      else
         return nullptr;
   }

   //
   // MapScope::findScript
   //
   Script *MapScope::findScript(Word name)
   {
      if(Script **script = pd->scriptInt.find(name))
         return *script;
      else
         return nullptr;
   }

   //
   // MapScope::freeThread
   //
   void MapScope::freeThread(Thread *thread)
   {
      auto itr = pd->scriptThread.find(thread->script);
      if(itr  && *itr == thread)
         *itr = nullptr;

      env->freeThread(thread);
   }

   //
   // MapScope::getModuleScope
   //
   ModuleScope *MapScope::getModuleScope(Module *module)
   {
      return pd->scopes.find(module);
   }

   //
   // MapScope::getString
   //
   String *MapScope::getString(Word idx) const
   {
      if(idx & 0x80000000)
         return &env->stringTable[~idx];

      if(!module0 || idx >= module0->stringV.size())
         return &env->stringTable.getNone();

      return module0->stringV[idx];
   }

   //
   // MapScope::hasActiveThread
   //
   bool MapScope::hasActiveThread() const
   {
      for(auto &thread : threadActive)
      {
         if(thread.state != ThreadState::Inactive)
            return true;
      }

      return false;
   }

   //
   // MapScope::hasModules
   //
   bool MapScope::hasModules() const
   {
      return !pd->scopes.empty();
   }

   //
   // MapScope::isScriptActive
   //
   bool MapScope::isScriptActive(Script *script)
   {
      auto itr = pd->scriptThread.find(script);
      return itr && *itr && (*itr)->state != ThreadState::Inactive;
   }

   //
   // MapScope::loadModules
   //
   void MapScope::loadModules(Serial &in)
   {
      auto count = ReadVLN<std::size_t>(in);
      std::vector<Module *> modules;
      modules.reserve(count);

      for(auto n = count; n--;)
         modules.emplace_back(env->getModule(env->readModuleName(in)));

      addModules(modules.data(), modules.size());

      for(auto &module : modules)
         pd->scopes.find(module)->loadState(in);
   }

   //
   // MapScope::loadProfileData
   //
   void MapScope::loadProfileData(Serial &in)
   {
      ProfileData dummy;

      // Load function profiling.
      for(auto c = in.readVLN<std::size_t>(); c--;)
      {
         if(auto lookup = profileFunction.find(env->readFunction(in)))
            lookup->loadState(in);
         else
            dummy.loadState(in);
      }

      // Load script profiling.
      for(auto c = in.readVLN<std::size_t>(); c--;)
      {
         if(auto lookup = profileScript.find(env->readScript(in)))
            lookup->loadState(in);
         else
            dummy.loadState(in);
      }
   }

   //
   // MapScope::loadState
   //
   void MapScope::loadState(Serial &in)
   {
      reset();

      in.readSign(Signature::MapScope);

      env->readScriptActions(in, scriptAction);
      active = in.readByte();
      loadModules(in);
      if(in.version >= 2)
         loadProfileData(in);
      loadThreads(in);

      in.readSign(~Signature::MapScope);
   }

   //
   // MapScope::loadThreads
   //
   void MapScope::loadThreads(Serial &in)
   {
      for(auto n = ReadVLN<std::size_t>(in); n--;)
      {
         Thread *thread = env->getFreeThread();
         thread->link.insert(&threadActive);
         thread->loadState(in);

         if(in.readByte())
         {
            auto scrThread = pd->scriptThread.find(thread->script);
            if(scrThread)
               *scrThread = thread;
         }
      }
   }

   //
   // MapScope::lockStrings
   //
   void MapScope::lockStrings() const
   {
      for(auto &action : scriptAction)
         action.lockStrings(env);

      for(auto &scope : pd->scopes)
         scope.val.lockStrings();

      for(auto &thread : threadActive)
         thread.lockStrings();
   }

   //
   // MapScope::refStrings
   //
   void MapScope::refStrings() const
   {
      for(auto &action : scriptAction)
         action.refStrings(env);

      for(auto &scope : pd->scopes)
         scope.val.refStrings();

      for(auto &thread : threadActive)
         thread.refStrings();
   }

   //
   // MapScope::reset
   //
   void MapScope::reset()
   {
      // Stop any remaining threads and return them to free list.
      while(threadActive.next->obj)
      {
         threadActive.next->obj->stop();
         env->freeThread(threadActive.next->obj);
      }

      while(scriptAction.next->obj)
         delete scriptAction.next->obj;

      active = false;

      profileFunction.free();
      profileScript.free();

      pd->scopes.free();

      pd->scriptInt.free();
      pd->scriptStr.free();
      pd->scriptThread.free();
   }

   //
   // MapScope::resetProfileData
   //
   void MapScope::resetProfileData()
   {
      for(auto &prof : profileFunction)
         prof.val.reset();

      for(auto &prof : profileScript)
         prof.val.reset();
   }

   //
   // MapScope::saveModules
   //
   void MapScope::saveModules(Serial &out) const
   {
      WriteVLN(out, pd->scopes.size());

      for(auto &scope : pd->scopes)
         env->writeModuleName(out, scope.key->name);

      for(auto &scope : pd->scopes)
         scope.val.saveState(out);
   }

   //
   // MapScope::saveProfileData
   //
   void MapScope::saveProfileData(Serial &out) const
   {
      // Save function profiling.
      std::size_t count = 0;
      for(auto &prof : profileFunction)
         if(prof.val) ++count;

      out.writeVLN(count);

      for(auto &prof : profileFunction)
      {
         if(!prof.val) continue;

         env->writeFunction(out, prof.key);
         prof.val.saveState(out);
      }

      // Save script profiling.
      count = 0;
      for(auto &prof : profileScript)
         if(prof.val) ++count;

      out.writeVLN(count);

      for(auto &prof : profileScript)
      {
         if(!prof.val) continue;

         env->writeScript(out, prof.key);
         prof.val.saveState(out);
      }
   }

   //
   // MapScope::saveState
   //
   void MapScope::saveState(Serial &out) const
   {
      out.writeSign(Signature::MapScope);

      env->writeScriptActions(out, scriptAction);
      out.writeByte(active);
      saveModules(out);
      saveProfileData(out);
      saveThreads(out);

      out.writeSign(~Signature::MapScope);
   }

   //
   // MapScope::saveThreads
   //
   void MapScope::saveThreads(Serial &out) const
   {
      WriteVLN(out, threadActive.size());
      for(auto &thread : threadActive)
      {
         thread.saveState(out);

         auto scrThread = pd->scriptThread.find(thread.script);
         out.writeByte(scrThread && *scrThread == &thread);
      }
   }

   //
   // MapScope::scriptPause
   //
   bool MapScope::scriptPause(Script *script)
   {
      auto itr = pd->scriptThread.find(script);
      if(!itr || !*itr)
         return false;

      switch((*itr)->state.state)
      {
      case ThreadState::Inactive:
      case ThreadState::Paused:
      case ThreadState::Stopped:
         return false;

      default:
         (*itr)->state = ThreadState::Paused;
         return true;
      }
   }

   //
   // MapScope::scriptPause
   //
   bool MapScope::scriptPause(ScriptName name, ScopeID scope)
   {
      if(scope != ScopeID{hub->global->id, hub->id, id})
      {
         env->deferAction({scope, name, ScriptAction::Pause, {}});
         return true;
      }

      if(Script *script = findScript(name))
         return scriptPause(script);
      else
         return false;
   }

   //
   // MapScope::scriptStart
   //
   bool MapScope::scriptStart(Script *script, ScriptStartInfo const &info)
   {
      auto itr = pd->scriptThread.find(script);
      if(!itr)
         return false;

      if(Thread *&thread = *itr)
      {
         switch(thread->state.state)
         {
         case ThreadState::Paused:
            thread->state = ThreadState::Running;
            return true;

         default:
            return false;
         }
      }
      else
      {
         thread = env->getFreeThread();
         thread->start(script, this, info.info, info.argV, info.argC);
         if(info.func) info.func(thread);
         return true;
      }
   }

   //
   // MapScope::scriptStart
   //
   bool MapScope::scriptStart(ScriptName name, ScopeID scope, ScriptStartInfo const &info)
   {
      if(scope != ScopeID{hub->global->id, hub->id, id})
      {
         env->deferAction({scope, name, ScriptAction::Start, {info.argV, info.argC}});
         return true;
      }

      if(Script *script = findScript(name))
         return scriptStart(script, info);
      else
         return false;
   }

   //
   // MapScope::scriptStartForced
   //
   bool MapScope::scriptStartForced(Script *script, ScriptStartInfo const &info)
   {
      Thread *thread = env->getFreeThread();

      thread->start(script, this, info.info, info.argV, info.argC);
      if(info.func) info.func(thread);
      return true;
   }

   //
   // MapScope::scriptStartForced
   //
   bool MapScope::scriptStartForced(ScriptName name, ScopeID scope, ScriptStartInfo const &info)
   {
      if(scope != ScopeID{hub->global->id, hub->id, id})
      {
         env->deferAction({scope, name, ScriptAction::StartForced, {info.argV, info.argC}});
         return true;
      }

      if(Script *script = findScript(name))
         return scriptStartForced(script, info);
      else
         return false;
   }

   //
   // MapScope::scriptStartResult
   //
   Word MapScope::scriptStartResult(Script *script, ScriptStartInfo const &info)
   {
      Thread *thread = env->getFreeThread();

      thread->start(script, this, info.info, info.argV, info.argC);
      if(info.func) info.func(thread);
      thread->exec();

      Word result = thread->result;
      if(thread->state == ThreadState::Inactive)
         freeThread(thread);
      return result;
   }

   //
   // MapScope::scriptStartResult
   //
   Word MapScope::scriptStartResult(ScriptName name, ScriptStartInfo const &info)
   {
      if(Script *script = findScript(name))
         return scriptStartResult(script, info);
      else
         return 0;
   }

   //
   // MapScope::scriptStartType
   //
   Word MapScope::scriptStartType(Word type, ScriptStartInfo const &info)
   {
      Word result = 0;

      for(auto &script : pd->scriptThread)
      {
         if(script.key->type == type)
            result += scriptStart(script.key, info);
      }

      return result;
   }

   //
   // MapScope::scriptStartTypeForced
   //
   Word MapScope::scriptStartTypeForced(Word type, ScriptStartInfo const &info)
   {
      Word result = 0;

      for(auto &script : pd->scriptThread)
      {
         if(script.key->type == type)
            result += scriptStartForced(script.key, info);
      }

      return result;
   }

   //
   // MapScope::scriptStop
   //
   bool MapScope::scriptStop(Script *script)
   {
      auto itr = pd->scriptThread.find(script);
      if(!itr || !*itr)
         return false;

      switch((*itr)->state.state)
      {
      case ThreadState::Inactive:
      case ThreadState::Stopped:
         return false;

      default:
         (*itr)->state = ThreadState::Stopped;
         (*itr)        = nullptr;
         return true;
      }
   }

   //
   // MapScope::scriptStop
   //
   bool MapScope::scriptStop(ScriptName name, ScopeID scope)
   {
      if(scope != ScopeID{hub->global->id, hub->id, id})
      {
         env->deferAction({scope, name, ScriptAction::Stop, {}});
         return true;
      }

      if(Script *script = findScript(name))
         return scriptStop(script);
      else
         return false;
   }

   //
   // MapScope::unlockStrings
   //
   void MapScope::unlockStrings() const
   {
      for(auto &action : scriptAction)
         action.unlockStrings(env);

      for(auto &scope : pd->scopes)
         scope.val.unlockStrings();

      for(auto &thread : threadActive)
         thread.unlockStrings();
   }

   //
   // ModuleScope constructor
   //
   ModuleScope::ModuleScope(MapScope *map_, Module *module_) :
      env   {map_->env},
      map   {map_},
      module{module_},

      selfArrV{},
      selfRegV{}
   {
      // Set arrays and registers to refer to this scope's by default.
      for(std::size_t i = 0; i != ArrC; ++i) arrV[i] = &selfArrV[i];
      for(std::size_t i = 0; i != RegC; ++i) regV[i] = &selfRegV[i];

      // Apply initialization data from module.

      for(std::size_t i = 0; i != ArrC; ++i)
      {
         if(i < module->arrInitV.size())
            module->arrInitV[i].apply(selfArrV[i], module);
      }

      for(std::size_t i = 0; i != RegC; ++i)
      {
         if(i < module->regInitV.size())
            selfRegV[i] = module->regInitV[i].getValue(module);
      }
   }

   //
   // ModuleScope destructor
   //
   ModuleScope::~ModuleScope()
   {
   }

   //
   // ModuleScope::import
   //
   void ModuleScope::import()
   {
      for(std::size_t i = 0, e = std::min<std::size_t>(ArrC, module->arrImpV.size()); i != e; ++i)
      {
         String *arrName = module->arrImpV[i];
         if(!arrName) continue;

         for(auto &imp : module->importV)
         {
            for(auto &impName : imp->arrNameV)
            {
               if(impName == arrName)
               {
                  std::size_t impIdx = &impName - imp->arrNameV.data();
                  if(impIdx >= ArrC) continue;
                  arrV[i] = &map->getModuleScope(imp)->selfArrV[impIdx];
                  goto arr_found;
               }
            }
         }

      arr_found:;
      }

      for(std::size_t i = 0, e = std::min<std::size_t>(RegC, module->regImpV.size()); i != e; ++i)
      {
         String *regName = module->regImpV[i];
         if(!regName) continue;

         for(auto &imp : module->importV)
         {
            for(auto &impName : imp->regNameV)
            {
               if(impName == regName)
               {
                  std::size_t impIdx = &impName - imp->regNameV.data();
                  if(impIdx >= RegC) continue;
                  regV[i] = &map->getModuleScope(imp)->selfRegV[impIdx];
                  goto reg_found;
               }
            }
         }

      reg_found:;
      }
   }

   //
   // ModuleScope::loadState
   //
   void ModuleScope::loadState(Serial &in)
   {
      in.readSign(Signature::ModuleScope);

      GlobalScope::LoadArrV(in, selfArrV, ArrC);
      GlobalScope::LoadRegV(in, selfRegV, RegC);

      in.readSign(~Signature::ModuleScope);
   }

   //
   // ModuleScope::lockStrings
   //
   void ModuleScope::lockStrings() const
   {
      for(auto &arr : selfArrV) arr.lockStrings(env);
      for(auto &reg : selfRegV) ++env->getString(reg)->lock;
   }

   //
   // ModuleScope::refStrings
   //
   void ModuleScope::refStrings() const
   {
      for(auto &arr : selfArrV) arr.refStrings(env);
      for(auto &reg : selfRegV) env->getString(reg)->ref = true;
   }

   //
   // ModuleScope::saveState
   //
   void ModuleScope::saveState(Serial &out) const
   {
      out.writeSign(Signature::ModuleScope);

      GlobalScope::SaveArrV(out, selfArrV, ArrC);
      GlobalScope::SaveRegV(out, selfRegV, RegC);

      out.writeSign(~Signature::ModuleScope);
   }

   //
   // ModuleScope::unlockStrings
   //
   void ModuleScope::unlockStrings() const
   {
      for(auto &arr : selfArrV) arr.unlockStrings(env);
      for(auto &reg : selfRegV) --env->getString(reg)->lock;
   }
}

// EOF

