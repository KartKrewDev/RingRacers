##-----------------------------------------------------------------------------
##
## Copyright (C) 2015-2026 David Hill
##
## See COPYING for license information.
##
##-----------------------------------------------------------------------------
##
## CMake file for acsvm.
##
##-----------------------------------------------------------------------------


##----------------------------------------------------------------------------|
## Environment Configuration                                                  |
##

include_directories(.)


##----------------------------------------------------------------------------|
## Variables                                                                  |
##

set(ACSVM_ACSVM_H
   Action.hpp
   Array.hpp
   BinaryIO.hpp
   CallFunc.hpp
   Code.hpp
   CodeData.hpp
   CodeList.hpp
   Environment.hpp
   Error.hpp
   Function.hpp
   HashMap.hpp
   HashMapFixed.hpp
   ID.hpp
   Init.hpp
   Jump.hpp
   List.hpp
   Module.hpp
   PrintBuf.hpp
   ProfileData.hpp
   Scope.hpp
   Script.hpp
   Serial.hpp
   SerialSTD.hpp
   Stack.hpp
   Store.hpp
   String.hpp
   Thread.hpp
   Tracer.hpp
   Types.hpp
   Vector.hpp
)

##----------------------------------------------------------------------------|
## Targets                                                                    |
##

##
## acsvm
##
add_library(acsvm ${ACSVM_SHARED_DECL}
   ${ACSVM_ACSM_H}
   Action.cpp
   Array.cpp
   CallFunc.cpp
   CodeData.cpp
   Environment.cpp
   Error.cpp
   Function.cpp
   Init.cpp
   Jump.cpp
   Module.cpp
   ModuleACS0.cpp
   ModuleACSE.cpp
   PrintBuf.cpp
   ProfileData.cpp
   Scope.cpp
   Script.cpp
   Serial.cpp
   SerialSTD.cpp
   String.cpp
   Thread.cpp
   ThreadExec.cpp
   Tracer.cpp
)
target_compile_features(acsvm PRIVATE cxx_std_17)

ACSVM_INSTALL_LIB(acsvm ACSVM ACSVM)

## EOF

