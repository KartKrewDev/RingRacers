include(LibFindMacros)

libfind_pkg_check_modules(VPX_PKGCONF VPX)

find_path(VPX_INCLUDE_DIR
	NAMES vpx/vp8.h
	PATHS
		${VPX_PKGCONF_INCLUDE_DIRS}
		"/usr/include"
		"/usr/local/include"
)

find_library(VPX_LIBRARY
	NAMES vpx
	PATHS
		${VPX_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

set(VPX_PROCESS_INCLUDES VPX_INCLUDE_DIR)
set(VPX_PROCESS_LIBS VPX_LIBRARY)
libfind_process(VPX)

if(VPX_FOUND AND NOT TARGET libvpx::libvpx)
	add_library(libvpx::libvpx UNKNOWN IMPORTED)
	set_target_properties(
		libvpx::libvpx
		PROPERTIES
		IMPORTED_LOCATION "${VPX_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${VPX_INCLUDE_DIR}"
	)
endif()
