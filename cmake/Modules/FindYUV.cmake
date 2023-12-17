include(LibFindMacros)

libfind_pkg_check_modules(YUV_PKGCONF YUV)

find_path(YUV_INCLUDE_DIR
	NAMES libyuv.h
	PATHS
		${YUV_PKGCONF_INCLUDE_DIRS}
		"/usr/include"
		"/usr/local/include"
)

find_library(YUV_LIBRARY
	NAMES yuv
	PATHS
		${YUV_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

set(YUV_PROCESS_INCLUDES YUV_INCLUDE_DIR)
set(YUV_PROCESS_LIBS YUV_LIBRARY)
libfind_process(YUV)

if(YUV_FOUND AND NOT TARGET libyuv::libyuv)
	add_library(libyuv::libyuv UNKNOWN IMPORTED)
	set_target_properties(
		libyuv::libyuv
		PROPERTIES
		IMPORTED_LOCATION "${YUV_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${YUV_INCLUDE_DIR}"
	)
endif()
