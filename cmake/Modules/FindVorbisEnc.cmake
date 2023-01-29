include(LibFindMacros)

libfind_pkg_check_modules(VorbisEnc_PKGCONF VorbisEnc)

find_path(VorbisEnc_INCLUDE_DIR
	NAMES vorbis/vorbisenc.h
	PATHS
		${VorbisEnc_PKGCONF_INCLUDE_DIRS}
		"/usr/include"
		"/usr/local/include"
)

find_library(VorbisEnc_LIBRARY
	NAMES vorbisenc
	PATHS
		${VorbisEnc_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

set(VorbisEnc_PROCESS_INCLUDES VorbisEnc_INCLUDE_DIR)
set(VorbisEnc_PROCESS_LIBS VorbisEnc_LIBRARY)
libfind_process(VorbisEnc)

if(VorbisEnc_FOUND AND NOT TARGET Vorbis::vorbisenc)
	add_library(Vorbis::vorbisenc UNKNOWN IMPORTED)
	set_target_properties(
		Vorbis::vorbisenc
		PROPERTIES
		IMPORTED_LOCATION "${VorbisEnc_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${VorbisEnc_INCLUDE_DIR}"
	)
endif()
