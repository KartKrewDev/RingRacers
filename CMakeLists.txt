cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
	message(FATAL_ERROR "In-source builds are blocked. Please build from a separate directory.")
endif()

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

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

##### PACKAGE CONFIGURATION #####

set(SRB2_CPACK_GENERATOR "" CACHE STRING "Generator to use for making a package. E.g., ZIP, TGZ, DragNDrop (OSX only). Leave blank for default generator.")

if("${SRB2_CPACK_GENERATOR}" STREQUAL "")
	if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
		set(SRB2_CPACK_GENERATOR "ZIP")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
		set(SRB2_CPACK_GENERATOR "TGZ")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
		set(SRB2_CPACK_GENERATOR "TGZ")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
		set(SRB2_CPACK_GENERATOR "TGZ")
	endif()
endif()

set(CPACK_GENERATOR ${SRB2_CPACK_GENERATOR})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Dr. Robotnik's Ring Racers" CACHE STRING "Program name for display purposes")
set(CPACK_PACKAGE_VENDOR "Kart Krew" CACHE STRING "Vendor name for display purposes")
#set(CPACK_PACKAGE_DESCRIPTION_FILE )
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${SRB2_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${SRB2_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${SRB2_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}")
SET(CPACK_OUTPUT_FILE_PREFIX package)
include(CPack)

# Options

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

add_subdirectory(thirdparty)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(CURL REQUIRED)
# Use the one in thirdparty/fmt to guarantee a minimum version
#find_package(FMT CONFIG REQUIRED)

# libgme defaults to "Nuked" YM2612 emulator, which is
# very SLOW. The system library probably uses the
# default so just always build it.
#find_package(GME REQUIRED)

if (SRB2_CONFIG_ENABLE_WEBM_MOVIES)
	find_package(YUV REQUIRED)

	find_package(unofficial-libvpx CONFIG)
	if(NOT unofficial-libvpx_FOUND)
		find_package(VPX REQUIRED)
	endif()

	find_package(Ogg REQUIRED)
	find_package(Vorbis REQUIRED)
	find_package(VorbisEnc REQUIRED)
endif()

if(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})
	message(FATAL_ERROR "In-source builds will bring you a world of pain. Please make a separate directory to invoke CMake from.")
endif()

if ((${SRB2_USE_CCACHE}) AND (${CMAKE_C_COMPILER} MATCHES "clang"))
	message(WARNING "Using clang and CCache: You may want to set environment variable CCACHE_CPP2=yes to prevent include errors during compile.")
endif()

# bitness check
set(SRB2_SYSTEM_BITS 0)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "Target is 64-bit")
	set(SRB2_SYSTEM_BITS 64)
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(STATUS "Target is 32-bit")
	set(SRB2_SYSTEM_BITS 32)
endif()
if(${SRB2_SYSTEM_BITS} EQUAL 0)
	message(STATUS "Target bitness is unknown")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set EXE names so the assets CMakeLists can refer to its target
set(SRB2_SDL2_EXE_NAME "" CACHE STRING "Override executable binary output name")
set(SRB2_SDL2_EXE_SUFFIX "" CACHE STRING "Optional executable suffix, separated by an underscore")

set(GIT_EXECUTABLE "git" CACHE FILEPATH "Path to git binary")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src)
add_subdirectory(assets)

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
