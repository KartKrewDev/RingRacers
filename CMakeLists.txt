cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
	message(FATAL_ERROR "In-source builds are blocked. Please build from a separate directory.")
endif()

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)

file(STRINGS src/version.h SRB2_VERSION)
string(REGEX MATCH "[0-9]+\\.[0-9.]+" SRB2_VERSION ${SRB2_VERSION})

# DO NOT CHANGE THIS SRB2 STRING! Some variable names depend on this string.
# Version change is fine.
project(SRB2
	VERSION ${SRB2_VERSION}
	LANGUAGES C CXX)

if(APPLE)
	# DiscordRPC needs but does not properly specify ObjC
    enable_language(OBJC)
endif()

# We need some portion of C++20 support
# Don't need/should not use: parallel algorithms, coroutines, modules, std::format
if(CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.1")
	# Released May 6, 2022
	message(WARNING "GCC version 12.1 or higher is recommended. (Currently ${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_CXX_COMPILER_ID MATCHES Clang AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "17.0.1")
	# Released September 9, 2023
	message(WARNING "Clang version 17.0.1 or higher is recommended. (Currently ${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_CXX_COMPILER_ID MATCHES AppleClang AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
	# Released September 12, 2022
	# Last version that can run on Monterey; lacks C++20 libc++ support but full language feature support
	message(WARNING "AppleClang/XCode version 14.0 or higher is recommended. (Currently ${CMAKE_CXX_COMPILER_VERSION})")
endif()

# Options
if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	if(DEFINED VCPKG_TARGET_TRIPLET)
		set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT ON)
	else()
		set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT OFF)
	endif()
else()
	set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT ON)
endif()

# Clang tidy options will be ignored if CMAKE_<LANG>_CLANG_TIDY are set.
option(SRB2_CONFIG_ENABLE_CLANG_TIDY_C "Enable default clang-tidy check configuration for C" OFF)
option(SRB2_CONFIG_ENABLE_CLANG_TIDY_CXX "Enable default clang-tidy check configuration for C++" OFF)
option(
	SRB2_CONFIG_STATIC_STDLIB
	"Link static version of standard library. All dependencies must also be static"
	ON
)
option(SRB2_CONFIG_ENABLE_WEBM_MOVIES "Enable WebM recording support" ON)
option(SRB2_CONFIG_ENABLE_DISCORDRPC "Enable Discord RPC features" ON)
option(SRB2_CONFIG_HWRENDER "Enable hardware render (OpenGL) support" ON)
option(SRB2_CONFIG_STATIC_OPENGL "Enable static linking GL (do not do this)" OFF)
option(SRB2_CONFIG_ERRORMODE "Compile C code with warnings treated as errors." OFF)
option(SRB2_CONFIG_DEBUGMODE "Compile with PARANOIA, ZDEBUG, RANGECHECK and PACKETDROP defined." OFF)
option(SRB2_CONFIG_DEV_BUILD "Compile a development build." OFF)
option(SRB2_CONFIG_ALWAYS_MAKE_DEBUGLINK "Always make a debuglink .debug." OFF)
option(SRB2_CONFIG_TESTERS "Compile a build for testers." OFF)
option(SRB2_CONFIG_MOBJCONSISTANCY "Compile with MOBJCONSISTANCY defined." OFF)
option(SRB2_CONFIG_PACKETDROP "Compile with PACKETDROP defined." OFF)
option(SRB2_CONFIG_EXECINFO "Enable stack trace dump support." ON)
option(SRB2_CONFIG_ZDEBUG "Compile with ZDEBUG defined." OFF)
option(SRB2_CONFIG_SKIP_COMPTIME "Skip regenerating comptime. To speed up iterative debug builds in IDEs." OFF)
# SRB2_CONFIG_PROFILEMODE is probably superceded by some CMake setting.
option(SRB2_CONFIG_PROFILEMODE "Compile for profiling (GCC only)." OFF)
option(SRB2_CONFIG_TRACY "Compile with Tracy profiling enabled" OFF)
option(SRB2_CONFIG_ASAN "Compile with AddressSanitizer (libasan)." OFF)
set(SRB2_CONFIG_ASSET_DIRECTORY "" CACHE PATH "Path to directory that contains all asset files for the installer. If set, assets will be part of installation and cpack.")

# Enable CCache
# (Set USE_CCACHE=ON to use, CCACHE_OPTIONS for options)
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL Windows)
	option(USE_CCACHE "Enable ccache support" OFF)

	if(USE_CCACHE)
		find_program(CCACHE_TOOL_PATH ccache)
		if(CCACHE_TOOL_PATH)
			set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_TOOL_PATH} CACHE STRING "" FORCE)
			set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_TOOL_PATH} CACHE STRING "" FORCE)
		else()
			message(WARNING "USE_CCACHE was set but ccache is not found (set CCACHE_TOOL_PATH)")
		endif()
	endif()
endif()

# Dependencies
add_subdirectory(thirdparty)

if(SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES)
	set(SRB2_INTERNAL_LIBRARY_TYPE SHARED)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES OFF)
else()
	set(SRB2_INTERNAL_LIBRARY_TYPE STATIC)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES ON)
endif()


find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(Opus REQUIRED)
if(WIN32 AND NOT MINGW)
	find_package(cpptrace CONFIG REQUIRED)
endif()

if (SRB2_CONFIG_ENABLE_WEBM_MOVIES)
	find_package(YUV REQUIRED)

	find_package(unofficial-libvpx CONFIG)
	if(NOT unofficial-libvpx_FOUND)
		find_package(VPX REQUIRED)
	endif()

	find_package(Ogg REQUIRED)
	find_package(Vorbis REQUIRED OPTIONAL_COMPONENTS vorbisenc)
	if(NOT TARGET Vorbis::vorbisenc)
		message(FATAL_ERROR "vorbisenc not found; despite being in OPTIONAL_COMPONENTS, it is actually required")
	endif()
endif()

if ((${SRB2_USE_CCACHE}) AND (${CMAKE_C_COMPILER} MATCHES "clang"))
	message(WARNING "Using clang and CCache: You may want to set environment variable CCACHE_CPP2=yes to prevent include errors during compile.")
endif()

# bitness check
set(SRB2_SYSTEM_BITS 0)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(SRB2_SYSTEM_BITS 64)
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(SRB2_SYSTEM_BITS 32)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set EXE names so the assets CMakeLists can refer to its target
set(SRB2_SDL2_EXE_NAME "" CACHE STRING "Override executable binary output name")
set(SRB2_SDL2_EXE_SUFFIX "" CACHE STRING "Optional executable suffix, separated by an underscore")

set(GIT_EXECUTABLE "git" CACHE FILEPATH "Path to git binary")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src)

include(GitUtilities)

get_git_dir(SRB2_GIT_DIR)

if(NOT "${SRB2_GIT_DIR}" STREQUAL "" AND "${SRB2_SDL2_EXE_NAME}" STREQUAL "")
	# cause a reconfigure if the branch changes
	configure_file("${SRB2_GIT_DIR}/HEAD" HEAD COPYONLY)

	git_current_branch(SRB2_GIT_REVISION)

	if("${SRB2_GIT_REVISION}" STREQUAL "")
		# use abbreviated commit hash if on detached HEAD
		git_latest_commit(SRB2_GIT_REVISION)
	endif()

	list(APPEND EXE_NAME_PARTS "ringracers")

	if(NOT "${SRB2_GIT_REVISION}" STREQUAL "master")
		# substitute path-unsafe characters
		string(REGEX REPLACE "[\\\/\^\$]" "_" SAFE_REVISION "${SRB2_GIT_REVISION}")
		list(APPEND EXE_NAME_PARTS ${SAFE_REVISION})
	endif()

	if (SRB2_CONFIG_TESTERS)
		list(APPEND EXE_NAME_PARTS "TESTERS")
	endif()
elseif(NOT "${SRB2_SDL2_EXE_NAME}" STREQUAL "")
	list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_NAME})
else()
	list(APPEND EXE_NAME_PARTS "ringracers")
endif()

list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_SUFFIX})

list(JOIN EXE_NAME_PARTS "_" EXE_NAME)
set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME ${EXE_NAME})

# Test for functions not consistently available
check_symbol_exists(strupr "string.h" HAVE_STRUPR)
check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
check_symbol_exists(strcasestr "string.h" HAVE_STRCASESTR)
check_symbol_exists(strcasecmp "string.h" HAVE_STRCASECMP)
check_symbol_exists(strncasecmp "string.h" HAVE_STRNCASECMP)
check_symbol_exists(stricmp "string.h" HAVE_STRICMP)
check_symbol_exists(strnicmp "string.h" HAVE_STRNICMP)
check_symbol_exists(strlcat "string.h" HAVE_STRLCAT)
check_symbol_exists(strlcpy "string.h" HAVE_STRLCPY)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/src/config.h")
