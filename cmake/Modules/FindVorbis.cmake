include(FindPackageHandleStandardArgs)

find_package(PkgConfig)

if(PkgConfig_FOUND)
	pkg_check_modules(Vorbis_PKGCONF vorbis)
	pkg_check_modules(Vorbis_vorbisenc_PKGCONF vorbisenc)
	pkg_check_modules(Vorbis_vorbisfile_PKGCONF vorbisfile)
endif()

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

find_library(Vorbis_vorbisenc_LIBRARY
	NAMES vorbisenc
	PATHS
		${Vorbis_vorbisenc_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

find_library(Vorbis_vorbisfile_LIBRARY
	NAMES vorbisfile
	PATHS
		${Vorbis_vorbisfile_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

if(Vorbis_vorbisenc_LIBRARY)
	set(Vorbis_vorbisenc_FOUND TRUE)
else()
	set(Vorbis_vorbisenc_FOUND FALSE)
endif()
if(Vorbis_vorbisfile_LIBRARY)
	set(Vorbis_vorbisfile_FOUND TRUE)
else()
	set(Vorbis_vorbisfile_FOUND FALSE)
endif()

find_package_handle_standard_args(Vorbis
	REQUIRED_VARS
		Vorbis_LIBRARY
		Vorbis_INCLUDE_DIR
	HANDLE_COMPONENTS)

if(Vorbis_FOUND)
	set(Vorbis_INCLUDE_DIRS ${Vorbis_INCLUDE_DIR})
	set(Vorbis_LIBRARIES ${Vorbis_LIBRARY})
	if(NOT TARGET Vorbis::vorbis)
		add_library(Vorbis::vorbis UNKNOWN IMPORTED)
		set_target_properties(
			Vorbis::vorbis
			PROPERTIES
			IMPORTED_LOCATION "${Vorbis_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIR}"
		)
	endif()
	if(NOT TARGET Vorbis::vorbisenc AND Vorbis_vorbisenc_FOUND)
		add_library(Vorbis::vorbisenc UNKNOWN IMPORTED)
		set_target_properties(
			Vorbis::vorbisenc
			PROPERTIES
			IMPORTED_LOCATION "${Vorbis_vorbisenc_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIR}"
		)
	endif()
	if(NOT TARGET Vorbis::vorbisfile AND Vorbis_vorbisfile_FOUND)
		add_library(Vorbis::vorbisfile UNKNOWN IMPORTED)
		set_target_properties(
			Vorbis::vorbisfile
			PROPERTIES
			IMPORTED_LOCATION "${Vorbis_vorbisfile_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIR}"
		)
	endif()
endif()

mark_as_advanced(
	Vorbis_INCLUDE_DIR
	Vorbis_LIBRARY
	Vorbis_vorbisenc_LIBRARY
	Vorbis_vorbisfile_LIBRARY
)
