include(LibFindMacros)

libfind_pkg_check_modules(Ogg_PKGCONF ogg)

find_path(Ogg_INCLUDE_DIR
	NAMES ogg/ogg.h
	PATHS
		${Ogg_PKGCONF_INCLUDE_DIRS}
		"/usr/include"
		"/usr/local/include"
)

find_library(Ogg_LIBRARY
	NAMES ogg
	PATHS
		${Ogg_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ogg
    REQUIRED_VARS Ogg_LIBRARY Ogg_INCLUDE_DIR)

if(Ogg_FOUND AND NOT TARGET Ogg::ogg)
	add_library(Ogg::ogg UNKNOWN IMPORTED)
	set_target_properties(
		Ogg::ogg
		PROPERTIES
		IMPORTED_LOCATION "${Ogg_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${Ogg_INCLUDE_DIR}"
	)
endif()

mark_as_advanced(Ogg_LIBRARY Ogg_INCLUDE_DIR)
