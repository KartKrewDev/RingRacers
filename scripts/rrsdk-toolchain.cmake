cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	set(EXE_SUFFIX ".exe")
else()
	set(EXE_SUFFIX "")
endif()

get_property(AM_I_IN_TRY_COMPILE GLOBAL PROPERTY IN_TRY_COMPILE)

if(NOT AM_I_IN_TRY_COMPILE)
	include(FetchContent)

	if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
		set(LLVM_MINGW_URL
			"https://github.com/mstorsjo/llvm-mingw/releases/download/20260616/llvm-mingw-20260616-ucrt-x86_64.zip")
		set(LLVM_MINGW_HASH
			SHA256=b9b68a4d276e16fa25802aaba458e4638f64b3884c290aaccdc2d87083b6ca35)
		set(NINJA_URL "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip")
		set(NINJA_HASH "SHA256=07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65")
		set(CCACHE_URL "https://github.com/ccache/ccache/releases/download/v4.13.6/ccache-4.13.6-windows-x86_64.zip")
		set(CCACHE_HASH "SHA256=3d7cebb05850ad704e197b3f1d3f0f924ab6c9fdfc561578e146184fe9d89380")

	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
		if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
			set(LLVM_MINGW_URL
				"https://github.com/mstorsjo/llvm-mingw/releases/download/20260616/llvm-mingw-20260616-ucrt-ubuntu-22.04-x86_64.tar.xz")
			set(LLVM_MINGW_HASH
				SHA256=534b92e067b22a6b4441f48ae9240a3341b17825d04d577eab0cf85c44b4deda)
			set(NINJA_URL "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-linux.zip")
			set(NINJA_HASH "SHA256=5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6")
			set(CCACHE_URL "https://github.com/ccache/ccache/releases/download/v4.13.6/ccache-4.13.6-linux-x86_64-musl-static.tar.xz")
			set(CCACHE_HASH "SHA256=156ec57c5198cc849d92834023d09910b83dc5504c6cf405d09e6ae7b208a3e5")
		elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
			set(LLVM_MINGW_URL
				"https://github.com/mstorsjo/llvm-mingw/releases/download/20260616/llvm-mingw-20260616-ucrt-ubuntu-22.04-aarch64.tar.xz")
			set(LLVM_MINGW_HASH
				SHA256=e7e5d135d93d3f2a3beaaea633a5b0e66ac75391a53feae654391913dd76102b)
			set(NINJA_URL "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-linux-aarch64.zip")
			set(NINJA_HASH "SHA256=fd2cacc8050a7f12a16a2e48f9e06fca5c14fc4c2bee2babb67b58be17a607fc")
			set(CCACHE_URL "https://github.com/ccache/ccache/releases/download/v4.13.6/ccache-4.13.6-linux-aarch64-musl-static.tar.xz")
			set(CCACHE_HASH "SHA256=2098d561e4a8e36bd06a29aedce53ea90c7e365f9573a93d91c230efbf96a958")
		endif()

	elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
		set(LLVM_MINGW_URL
			"https://github.com/mstorsjo/llvm-mingw/releases/download/20260616/llvm-mingw-20260616-ucrt-macos-universal.tar.xz")
		set(LLVM_MINGW_HASH
			SHA256=2cab02a2e964bd4aae981150a45985d07c657cfa8d244959eb9e2dcc5eedd7b1)
		set(NINJA_URL "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-mac.zip")
		set(NINJA_HASH "SHA256=c99048673aa765960a99cf10c6ddb9f1fad506099ff0a0e137ad8960a88f321b")
		set(CCACHE_URL "https://github.com/ccache/ccache/releases/download/v4.13.6/ccache-4.13.6-darwin.tar.gz")
		set(CCACHE_HASH "SHA256=0274210ec9c9936ed5711d59b0de3167a51216a588ddde35f6bc828f366fe6d9")
	endif()

	FetchContent_Declare(llvm_mingw64
		URL     "${LLVM_MINGW_URL}"
		URL_HASH "${LLVM_MINGW_HASH}"
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		EXCLUDE_FROM_ALL
	)

	FetchContent_Declare(rrsdk
		URL     "https://gitlab.com/api/v4/projects/83528690/packages/generic/ring-racers-sdk/1/rrsdk-msys2-clang64.zip"
		URL_HASH "SHA256=1d816b61ce1936240ab067df98e5badd98fa642da0ae9afa87e85b61e30a4fd3"
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		EXCLUDE_FROM_ALL
	)

	FetchContent_Declare(downloadedninja
		URL      "${NINJA_URL}"
		URL_HASH "${NINJA_HASH}"
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		EXCLUDE_FROM_ALL
	)

	FetchContent_Declare(downloadedccache
		URL      "${CCACHE_URL}"
		URL_HASH "${CCACHE_HASH}"
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		EXCLUDE_FROM_ALL
	)

	FetchContent_MakeAvailable(llvm_mingw64 rrsdk downloadedninja downloadedccache)
	if(CMAKE_GENERATOR MATCHES "Ninja")
		set(CMAKE_MAKE_PROGRAM "${downloadedninja_SOURCE_DIR}/ninja${EXE_SUFFIX}" CACHE INTERNAL "Path to build tool")
	endif()
	if(NOT CMAKE_C_COMPILER_LAUNCHER)
		set(CMAKE_C_COMPILER_LAUNCHER "${downloadedccache_SOURCE_DIR}/ccache${EXE_SUFFIX}" CACHE INTERNAL "Path to compiler launcher")
	endif()
	if(NOT CMAKE_CXX_COMPILER_LAUNCHER)
		set(CMAKE_CXX_COMPILER_LAUNCHER "${downloadedccache_SOURCE_DIR}/ccache${EXE_SUFFIX}" CACHE INTERNAL "Path to compiler launcher")
	endif()
endif()

set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   "${llvm_mingw64_SOURCE_DIR}/bin/x86_64-w64-mingw32-clang${EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${llvm_mingw64_SOURCE_DIR}/bin/x86_64-w64-mingw32-clang++${EXE_SUFFIX}")
set(CMAKE_RC_COMPILER  "${llvm_mingw64_SOURCE_DIR}/bin/x86_64-w64-mingw32-windres${EXE_SUFFIX}")
set(CMAKE_AR           "${llvm_mingw64_SOURCE_DIR}/bin/x86_64-w64-mingw32-ar${EXE_SUFFIX}")
set(CMAKE_RANLIB       "${llvm_mingw64_SOURCE_DIR}/bin/x86_64-w64-mingw32-ranlib${EXE_SUFFIX}")

set(CMAKE_SYSROOT "${llvm_mingw64_SOURCE_DIR}")
set(CMAKE_FIND_ROOT_PATH "${llvm_mingw64_SOURCE_DIR}")
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
list(APPEND CMAKE_FIND_ROOT_PATH "${rrsdk_SOURCE_DIR}")
list(APPEND CMAKE_FIND_ROOT_PATH "${downloadedninja_SOURCE_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_PREFIX_PATH "${rrsdk_SOURCE_DIR}")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()
