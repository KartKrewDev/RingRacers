include(LibFindMacros)

libfind_pkg_check_modules(Vorbis_PKGCONF Vorbis)

find_path(Vorbis_INCLUDE_DIR
	NAMES vorbis/codec.h
	PATHS
		${Vorbis_PKGCONF_INCLUDE_DIRS}
		"/usr/include"
		"/usr/local/include"
)

find_library(Vorbis_LIBRARY
	NAMES vorbis
	PATHS
		${Vorbis_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

set(Vorbis_PROCESS_INCLUDES Vorbis_INCLUDE_DIR)
set(Vorbis_PROCESS_LIBS Vorbis_LIBRARY)
libfind_process(Vorbis)

if(Vorbis_FOUND AND NOT TARGET Vorbis::vorbis)
	add_library(Vorbis::vorbis UNKNOWN IMPORTED)
	set_target_properties(
		Vorbis::vorbis
		PROPERTIES
		IMPORTED_LOCATION "${Vorbis_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIR}"
	)
endif()
