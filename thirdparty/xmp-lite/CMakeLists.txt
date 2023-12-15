# Update from https://github.com/libxmp/libxmp/releases
# xmp-lite 4.6.0
# License: MIT

# Always statically linked because xmp-lite is fairly small

set(xmp_sources
	control.c
	dataio.c
	effects.c
	filetype.c
	filter.c
	format.c
	hio.c
	lfo.c
	load.c
	load_helpers.c
	md5.c
	memio.c
	misc.c
	mix_all.c
	mixer.c
	period.c
	player.c
	read_event.c
	scan.c
	smix.c
	virtual.c
	win32.c

	loaders/common.c
	loaders/itsex.c
	loaders/sample.c
	loaders/xm_load.c
	loaders/mod_load.c
	loaders/s3m_load.c
	loaders/it_load.c
)
list(TRANSFORM xmp_sources PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_library(xmp-lite STATIC ${xmp_sources})
target_compile_definitions(xmp-lite PRIVATE -D_REENTRANT -DLIBXMP_CORE_PLAYER -DLIBXMP_NO_PROWIZARD -DLIBXMP_NO_DEPACKERS)
if(WIN32)
	# BUILDING_STATIC has to be public to work around a bug in xmp.h
	# which adds __declspec(dllimport) even when statically linking
	target_compile_definitions(xmp-lite PUBLIC -DBUILDING_STATIC)
else()
	target_compile_definitions(xmp-lite PRIVATE -DBUILDING_STATIC)
endif()
target_compile_definitions(xmp-lite PUBLIC -DLIBXMP_STATIC)

target_include_directories(xmp-lite PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(xmp-lite PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/libxmp-lite")
add_library(xmp-lite::xmp-lite ALIAS xmp-lite)
