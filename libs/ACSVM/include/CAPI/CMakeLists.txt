##-----------------------------------------------------------------------------
##
## Copyright (C) 2015 David Hill
##
## See COPYING for license information.
##
##-----------------------------------------------------------------------------
##
## CMake file for acsvm-capi.
##
##-----------------------------------------------------------------------------


##----------------------------------------------------------------------------|
## Environment Configuration                                                  |
##

include_directories(.)


##----------------------------------------------------------------------------|
## Targets                                                                    |
##

##
## acsvm-capi
##
add_library(acsvm-capi ${ACSVM_SHARED_DECL}
   Array.cpp
   Array.h
   BinaryIO.cpp
   BinaryIO.h
   Environment.cpp
   Environment.h
   Floats.cpp
   Floats.h
   Module.cpp
   Module.h
   PrintBuf.cpp
   PrintBuf.h
   Scope.cpp
   Scope.h
   String.cpp
   String.h
   Thread.cpp
   Thread.h
   Types.h
)

target_link_libraries(acsvm-capi acsvm-util)

ACSVM_INSTALL_LIB(acsvm-capi)

## EOF

