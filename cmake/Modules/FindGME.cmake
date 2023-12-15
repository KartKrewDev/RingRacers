include(LibFindMacros)

libfind_pkg_check_modules(GME_PKGCONF QUIET gme libgme)

find_path(GME_INCLUDE_DIR
	NAMES gme.h
	PATHS
		${GME_PKGCONF_INCLUDE_DIRS}
		/usr/include
		/usr/local/include
	PATH_SUFFIXES
		gme
)

find_library(GME_LIBRARY
	NAMES gme
	PATHS
		${GME_PKGCONF_LIBRARY_DIRS}
		/usr/lib
		/usr/local/lib
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GME
    REQUIRED_VARS GME_LIBRARY GME_INCLUDE_DIR)

if(GME_FOUND AND NOT TARGET gme)
	add_library(gme UNKNOWN IMPORTED)
	set_target_properties(
		gme
		PROPERTIES
		IMPORTED_LOCATION "${GME_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${GME_INCLUDE_DIR}"
	)
	add_library(gme::gme ALIAS gme)
endif()

mark_as_advanced(GME_LIBRARY GME_INCLUDE_DIR)
