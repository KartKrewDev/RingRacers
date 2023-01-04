CPMAddPackage(
	NAME xmp-lite
	VERSION 4.5.0
	URL "https://github.com/libxmp/libxmp/releases/download/libxmp-4.5.0/libxmp-lite-4.5.0.tar.gz"
	EXCLUDE_FROM_ALL ON
	DOWNLOAD_ONLY ON
)

if(xmp-lite_ADDED)
	set(xmp_sources
		virtual.c
		format.c
		period.c
		player.c
		read_event.c
		misc.c
		dataio.c
		lfo.c
		scan.c
		control.c
		filter.c
		effects.c
		mixer.c
		mix_all.c
		load_helpers.c
		load.c
		hio.c
		smix.c
		memio.c
		win32.c

		loaders/common.c
		loaders/itsex.c
		loaders/sample.c
		loaders/xm_load.c
		loaders/mod_load.c
		loaders/s3m_load.c
		loaders/it_load.c
	)
	list(TRANSFORM xmp_sources PREPEND "${xmp-lite_SOURCE_DIR}/src/")

	add_library(xmp-lite "${SRB2_INTERNAL_LIBRARY_TYPE}" ${xmp_sources})

	target_compile_definitions(xmp-lite PRIVATE -D_REENTRANT -DLIBXMP_CORE_PLAYER -DLIBXMP_NO_PROWIZARD -DLIBXMP_NO_DEPACKERS)
	if("${SRB2_INTERNAL_LIBRARY_TYPE}" STREQUAL "STATIC")
		if(WIN32)
			# BUILDING_STATIC has to be public to work around a bug in xmp.h
			# which adds __declspec(dllimport) even when statically linking
			target_compile_definitions(xmp-lite PUBLIC -DBUILDING_STATIC)
		else()
			target_compile_definitions(xmp-lite PRIVATE -DBUILDING_STATIC)
		endif()
	else()
		target_compile_definitions(xmp-lite PRIVATE -DBUILDING_DLL)
	endif()
	target_include_directories(xmp-lite PRIVATE "${xmp-lite_SOURCE_DIR}/src")
	target_include_directories(xmp-lite PUBLIC "${xmp-lite_SOURCE_DIR}/include/libxmp-lite")

	add_library(xmp-lite::xmp-lite ALIAS xmp-lite)
endif()
