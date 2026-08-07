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

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

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

option(SRB2_CONFIG_ENABLE_WEBM_MOVIES "Enable WebM recording support" ON)
option(SRB2_CONFIG_ENABLE_DISCORDRPC "Enable Discord RPC features" ON)
option(SRB2_CONFIG_HWRENDER "Enable hardware render (OpenGL) support" ON)
option(SRB2_CONFIG_LINK_OPENGL "Enable directly linking GL (needed for some platforms)" OFF)
option(SRB2_CONFIG_ERRORMODE "Compile C code with warnings treated as errors." OFF)
option(SRB2_CONFIG_DEBUGMODE "Compile with PARANOIA, ZDEBUG, RANGECHECK and PACKETDROP defined." OFF)
option(SRB2_CONFIG_DEV_BUILD "Compile a development build." OFF)
option(SRB2_CONFIG_DISABLE_DEBUGLINK "Disable debuglink generation" OFF)
option(SRB2_CONFIG_INSTALL_DEBUGLINK "Install .debug debuglink with cmake installation (Windows only)" OFF)
option(SRB2_CONFIG_MOBJCONSISTANCY "Compile with MOBJCONSISTANCY defined." OFF)
option(SRB2_CONFIG_PACKETDROP "Compile with PACKETDROP defined." OFF)
option(SRB2_CONFIG_ZDEBUG "Compile with ZDEBUG defined." OFF)
option(SRB2_CONFIG_SKIP_COMPTIME "Skip regenerating comptime. To speed up iterative debug builds in IDEs." OFF)
option(SRB2_CONFIG_PROFILEMODE "Compile for profiling (GCC only)." OFF)
option(SRB2_CONFIG_TRACY "Compile with Tracy profiling enabled" OFF)
option(SRB2_CONFIG_ASAN "Compile with AddressSanitizer (libasan)." OFF)

# Dependencies
add_subdirectory(thirdparty)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(Opus REQUIRED)
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

# Set EXE names so the assets CMakeLists can refer to its target
set(SRB2_SDL2_EXE_NAME "" CACHE STRING "Override executable binary output name")
set(SRB2_SDL2_EXE_SUFFIX "" CACHE STRING "Optional executable suffix, separated by an underscore")

set(GIT_EXECUTABLE "git" CACHE FILEPATH "Path to git binary")

add_executable(SRB2SDL2 MACOSX_BUNDLE WIN32)
target_compile_features(SRB2SDL2 PRIVATE c_std_17 cxx_std_20)
set_target_properties(SRB2SDL2 PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED ON
	C_STANDARD_EXTENSIONS OFF
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	CXX_SCAN_FOR_MODULES OFF
)

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
elseif(NOT "${SRB2_SDL2_EXE_NAME}" STREQUAL "")
	list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_NAME})
else()
	list(APPEND EXE_NAME_PARTS "ringracers")
endif()

list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_SUFFIX})

list(JOIN EXE_NAME_PARTS "_" EXE_NAME)
set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME "${EXE_NAME}")

if(NOT SRB2_CONFIG_SKIP_COMPTIME)
	# This updates the modification time for comptime.c at the
	# end of building so when the build system is ran next time,
	# that file gets flagged. comptime.c will always be rebuilt.
	#
	# This begs the question, why always rebuild comptime.c?
	# Some things like the git commit must be checked each time
	# the program is built. But the build system determines which
	# files should be rebuilt before anything else. So
	# comptime.c, which only needs to be rebuilt based on
	# information known at build time, must be told to rebuild
	# before that information can be ascertained.
	add_custom_command(
		TARGET SRB2SDL2
		POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E touch_nocreate "${CMAKE_CURRENT_SOURCE_DIR}/src/comptime.c"
	)
endif()

set_target_properties(SRB2SDL2 PROPERTIES
	MACOSX_BUNDLE_INFO_PLIST
	"${CMAKE_CURRENT_SOURCE_DIR}/src/sdl/macosx/MacOSXBundleInfo.plist.in"
)

# config_comptime.h is generated by this command. It should be done at
# build time for accurate git information and before anything
# that needs it, obviously.
add_custom_target(_SRB2_reconf ALL
	COMMAND ${CMAKE_COMMAND}
		"-DGIT_EXECUTABLE=${GIT_EXECUTABLE}"
		"-DCMAKE_BUILD_TYPE=$<CONFIG>"
		"-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Comptime.cmake"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/config_comptime.h.in"
	BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/src/config_comptime.h"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
add_dependencies(SRB2SDL2 _SRB2_reconf)

# Test for functions not consistently available
check_symbol_exists(strupr "string.h" HAVE_STRUPR)
check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
check_symbol_exists(strcasestr "string.h" HAVE_STRCASESTR)
check_symbol_exists(strcasecmp "strings.h" HAVE_STRCASECMP)
check_symbol_exists(strncasecmp "strings.h" HAVE_STRNCASECMP)
check_symbol_exists(stricmp "string.h" HAVE_STRICMP)
check_symbol_exists(strnicmp "string.h" HAVE_STRNICMP)
check_symbol_exists(strlcat "string.h" HAVE_STRLCAT)
check_symbol_exists(strlcpy "string.h" HAVE_STRLCPY)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/src/config.h")

if(MSVC OR CMAKE_CXX_LINK_EXECUTABLE MATCHES "lld-link\\.exe")
	target_link_options(SRB2SDL2 PRIVATE /MANIFEST:NO)
endif()

# Strip debug sections
find_program(CMAKE_OBJCOPY objcopy)
if(CMAKE_OBJCOPY AND NOT SRB2_CONFIG_DISABLE_DEBUGLINK)
	add_custom_command(TARGET SRB2SDL2 POST_BUILD
		COMMAND ${CMAKE_COMMAND}
			-D "OBJCOPY=${CMAKE_OBJCOPY}"
			-D "TARGET_FILE=$<TARGET_FILE:SRB2SDL2>"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/strip_debug.cmake"
	)
endif()

#### Installation ####

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	# Mac bundle fixup
	# Must be installed before the actual target.
	install(CODE "
		include(BundleUtilities)
		fixup_bundle(\"$<TARGET_FILE:SRB2SDL2>\" \"\" /Library/Frameworks)
		"
	)
	install(TARGETS SRB2SDL2 BUNDLE DESTINATION .)
	set_property(TARGET SRB2SDL2 PROPERTY INSTALL_RPATH_USE_LINK_PATH ON)
else()
	if(CMAKE_SYSTEM_NAME MATCHES "Windows")
		install(TARGETS SRB2SDL2 RUNTIME DESTINATION .)
		if(SRB2_CONFIG_INSTALL_DEBUGLINK)
			install(FILES $<TARGET_FILE:SRB2SDL2>.debug
				DESTINATION .
				OPTIONAL
			)
		endif()
	else()
		install(TARGETS SRB2SDL2)
	endif()
endif()
