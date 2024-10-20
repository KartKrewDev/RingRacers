# Tracy Profiler Client 0.11.1
# BSD 3-clause
# Copyright (c) 2017-2024, Bartosz Taudul <wolf@nereid.pl>

# includes libbacktrace
# BSD 3-clause
# Copyright (c) 2012-2016 Free Software Foundation, Inc.

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)


add_library(TracyClient STATIC "${CMAKE_CURRENT_SOURCE_DIR}/include/tracy/TracyClient.cpp")
target_compile_features(TracyClient PUBLIC cxx_std_11)
target_include_directories(TracyClient PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(WIN32 AND ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    target_link_libraries(TracyClient PUBLIC ws2_32 dbghelp)
endif()

if(SRB2_CONFIG_TRACY)
	target_compile_definitions(TracyClient PUBLIC TRACY_ENABLE)
endif()
target_compile_definitions(TracyClient PUBLIC -DTRACY_ON_DEMAND -DTRACY_DELAYED_INIT)

add_library(Tracy::TracyClient ALIAS TracyClient)
