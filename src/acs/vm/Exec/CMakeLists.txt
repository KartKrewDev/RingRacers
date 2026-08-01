##-----------------------------------------------------------------------------
##
## Copyright (C) 2015-2025 David Hill
##
## See COPYING for license information.
##
##-----------------------------------------------------------------------------
##
## CMake file for acsvm-exec.
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
## acsvm-exec
##
add_executable(acsvm-exec
   main_exec.cpp
)

target_link_libraries(acsvm-exec acsvm-util)

ACSVM_INSTALL_EXE(acsvm-exec)

## EOF

