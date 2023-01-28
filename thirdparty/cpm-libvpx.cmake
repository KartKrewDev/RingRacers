CPMAddPackage(
	NAME libvpx
	VERSION 1.12.0
	URL "https://chromium.googlesource.com/webm/libvpx/+archive/03265cd42b3783532de72f2ded5436652e6f5ce3.tar.gz"
	EXCLUDE_FROM_ALL ON
	DOWNLOAD_ONLY YES
)

if(libvpx_ADDED)
	include(ExternalProject)

	# libvpx configure script does CPU detection. So lets just
	# call it instead of trying to do all that in CMake.
	ExternalProject_Add(libvpx
		PREFIX "${libvpx_BINARY_DIR}"
		SOURCE_DIR "${libvpx_SOURCE_DIR}"
		BINARY_DIR "${libvpx_BINARY_DIR}"
		CONFIGURE_COMMAND sh "${libvpx_SOURCE_DIR}/configure"
		--enable-vp8 --disable-vp9 --disable-vp8-decoder
		--disable-examples --disable-tools --disable-docs
		--disable-webm-io --disable-libyuv --disable-unit-tests
		BUILD_COMMAND "make"
		BUILD_BYPRODUCTS "${libvpx_BINARY_DIR}/libvpx.a"
		INSTALL_COMMAND ""
		USES_TERMINAL_CONFIGURE ON
		USES_TERMINAL_BUILD ON
	)

	add_library(webm::libvpx STATIC IMPORTED GLOBAL)
	add_dependencies(webm::libvpx libvpx)
	set_target_properties(
		webm::libvpx
		PROPERTIES
		IMPORTED_LOCATION "${libvpx_BINARY_DIR}/libvpx.a"
		INTERFACE_INCLUDE_DIRECTORIES "${libvpx_SOURCE_DIR}"
	)
endif()
