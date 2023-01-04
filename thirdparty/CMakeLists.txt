macro(export)
endmacro()

if(SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES)
	set(SRB2_INTERNAL_LIBRARY_TYPE SHARED)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES OFF)
else()
	set(SRB2_INTERNAL_LIBRARY_TYPE STATIC)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES ON)
endif()


if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME SDL2
		VERSION 2.24.2
		URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.24.2.zip"
		EXCLUDE_FROM_ALL ON
		OPTIONS
			"BUILD_SHARED_LIBS ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"SDL_SHARED ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"SDL_STATIC ${NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"SDL_TEST OFF"
			"SDL2_DISABLE_SDL2MAIN ON"
			"SDL2_DISABLE_INSTALL ON"
	)
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME ZLIB
		VERSION 1.2.13
		URL "https://github.com/madler/zlib/archive/refs/tags/v1.2.13.zip"
		EXCLUDE_FROM_ALL
		DOWNLOAD_ONLY YES
	)
	if(ZLIB_ADDED)
		set(ZLIB_SRCS
			crc32.h
			deflate.h
			gzguts.h
			inffast.h
			inffixed.h
			inflate.h
			inftrees.h
			trees.h
			zutil.h

			adler32.c
			compress.c
			crc32.c
			deflate.c
			gzclose.c
			gzlib.c
			gzread.c
			gzwrite.c
			inflate.c
			infback.c
			inftrees.c
			inffast.c
			trees.c
			uncompr.c
			zutil.c
		)
		list(TRANSFORM ZLIB_SRCS PREPEND "${ZLIB_SOURCE_DIR}/")

		configure_file("${ZLIB_SOURCE_DIR}/zlib.pc.cmakein" "${ZLIB_BINARY_DIR}/zlib.pc" @ONLY)
		configure_file("${ZLIB_SOURCE_DIR}/zconf.h.cmakein" "${ZLIB_BINARY_DIR}/include/zconf.h" @ONLY)
		configure_file("${ZLIB_SOURCE_DIR}/zlib.h" "${ZLIB_BINARY_DIR}/include/zlib.h" @ONLY)

		add_library(ZLIB ${SRB2_INTERNAL_LIBRARY_TYPE} ${ZLIB_SRCS})
		set_target_properties(ZLIB PROPERTIES
			VERSION 1.2.13
			OUTPUT_NAME "z"
		)
		target_include_directories(ZLIB PRIVATE "${ZLIB_SOURCE_DIR}")
		target_include_directories(ZLIB PUBLIC "${ZLIB_BINARY_DIR}/include")
		if(MSVC)
			target_compile_definitions(ZLIB PRIVATE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
		endif()
		add_library(ZLIB::ZLIB ALIAS ZLIB)
	endif()
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME png
		VERSION 1.6.38
		URL "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.38.zip"
		# png cmake build is broken on msys/mingw32
		DOWNLOAD_ONLY YES
	)

	if(png_ADDED)
		# Since png's cmake build is broken, we're going to create a target manually
		set(
			PNG_SOURCES

			png.h
			pngconf.h

			pngpriv.h
			pngdebug.h
			pnginfo.h
			pngstruct.h

			png.c
			pngerror.c
			pngget.c
			pngmem.c
			pngpread.c
			pngread.c
			pngrio.c
			pngrtran.c
			pngrutil.c
			pngset.c
			pngtrans.c
			pngwio.c
			pngwrite.c
			pngwtran.c
			pngwutil.c
		)
		list(TRANSFORM PNG_SOURCES PREPEND "${png_SOURCE_DIR}/")

		add_custom_command(
			OUTPUT "${png_BINARY_DIR}/include/png.h" "${png_BINARY_DIR}/include/pngconf.h"
			COMMAND ${CMAKE_COMMAND} -E copy "${png_SOURCE_DIR}/png.h" "${png_SOURCE_DIR}/pngconf.h" "${png_BINARY_DIR}/include"
			DEPENDS "${png_SOURCE_DIR}/png.h" "${png_SOURCE_DIR}/pngconf.h"
			VERBATIM
		)
		add_custom_command(
			OUTPUT "${png_BINARY_DIR}/include/pnglibconf.h"
			COMMAND ${CMAKE_COMMAND} -E copy "${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt" "${png_BINARY_DIR}/include/pnglibconf.h"
			DEPENDS "${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt"
			VERBATIM
		)
		list(
			APPEND PNG_SOURCES
			"${png_BINARY_DIR}/include/png.h"
			"${png_BINARY_DIR}/include/pngconf.h"
			"${png_BINARY_DIR}/include/pnglibconf.h"
		)
		add_library(png "${SRB2_INTERNAL_LIBRARY_TYPE}" ${PNG_SOURCES})

		# Disable ARM NEON since having it automatic breaks libpng external build on clang for some reason
		target_compile_definitions(png PRIVATE -DPNG_ARM_NEON_OPT=0)

		# The png includes need to be available to consumers
		target_include_directories(png PUBLIC "${png_BINARY_DIR}/include")

		# ... and these also need to be present only for png build
		target_include_directories(png PRIVATE "${ZLIB_SOURCE_DIR}")
		target_include_directories(png PRIVATE "${ZLIB_BINARY_DIR}")
		target_include_directories(png PRIVATE "${png_BINARY_DIR}")

		target_link_libraries(png PRIVATE ZLIB::ZLIB)
		add_library(PNG::PNG ALIAS png)
	endif()
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	set(
		internal_curl_options

		"BUILD_CURL_EXE OFF"
		"BUILD_SHARED_LIBS ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
		"CURL_DISABLE_TESTS ON"
		"HTTP_ONLY ON"
		"CURL_DISABLE_CRYPTO_AUTH ON"
		"CURL_DISABLE_NTLM ON"
		"ENABLE_MANUAL OFF"
		"ENABLE_THREADED_RESOLVER OFF"
		"CURL_USE_LIBPSL OFF"
		"CURL_USE_LIBSSH2 OFF"
		"USE_LIBIDN2 OFF"
		"CURL_ENABLE_EXPORT_TARGET OFF"
	)
	if(${CMAKE_SYSTEM} MATCHES Windows)
		list(APPEND internal_curl_options "CURL_USE_OPENSSL OFF")
		list(APPEND internal_curl_options "CURL_USE_SCHANNEL ON")
	endif()
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		list(APPEND internal_curl_options "CURL_USE_OPENSSL OFF")
		list(APPEND internal_curl_options "CURL_USE_SECTRANSP ON")
	endif()
	if(${CMAKE_SYSTEM} MATCHES Linux)
		list(APPEND internal_curl_options "CURL_USE_OPENSSL ON")
	endif()

	CPMAddPackage(
		NAME curl
		VERSION 7.86.0
		URL "https://github.com/curl/curl/archive/refs/tags/curl-7_86_0.zip"
		EXCLUDE_FROM_ALL ON
		OPTIONS ${internal_curl_options}
	)
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME libgme
		VERSION 0.6.3
		URL "https://bitbucket.org/mpyne/game-music-emu/get/e76bdc0cb916e79aa540290e6edd0c445879d3ba.zip"
		EXCLUDE_FROM_ALL ON
		OPTIONS
			"BUILD_SHARED_LIBS ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"ENABLE_UBSAN OFF"
			"GME_YM2612_EMU MAME"
	)
	target_compile_features(gme PRIVATE cxx_std_11)
	target_link_libraries(gme PRIVATE ZLIB::ZLIB)

endif()

include("cpm-rapidjson.cmake")
include("cpm-discordrpc.cmake")
include("cpm-xmp-lite.cmake")

add_subdirectory(tcbrindle_span)
add_subdirectory(stb_vorbis)
