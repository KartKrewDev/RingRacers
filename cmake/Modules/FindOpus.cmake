include(FindPackageHandleStandardArgs)

find_package(PkgConfig)

if(PkgConfig_FOUND)
	pkg_check_modules(Opus_PKGCONF opus)
endif()

find_path(Opus_INCLUDE_DIR
	NAMES opus.h
	PATHS
		${Opus_PKGCONF_INCLUDE_DIRS}
		"/usr/include"
		"/usr/local/include"
)

find_library(Opus_LIBRARY
	NAMES opus
	PATHS
		${Opus_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

if(Opus_LIBRARY)
	set(Opus_FOUND TRUE)
else()
	set(Opus_FOUND FALSE)
endif()

find_package_handle_standard_args(Opus
	REQUIRED_VARS Opus_LIBRARY Opus_INCLUDE_DIR
)

if(Opus_FOUND)
	set(Opus_INCLUDE_DIRS "${Opus_INCLUDE_DIR}")
	set(Opus_LIBRARIES "${Opus_LIBRARY}")
	if(NOT TARGET Opus::opus)
		add_library(Opus::opus UNKNOWN IMPORTED)
		set_target_properties(Opus::opus PROPERTIES
			IMPORTED_LOCATION "${Opus_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${Opus_INCLUDE_DIR}"
		)
	endif()
endif()

mark_as_advanced(
	Opus_INCLUDE_DIR
	Opus_LIBRARY
)
