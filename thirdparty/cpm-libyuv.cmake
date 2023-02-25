CPMAddPackage(
	NAME libyuv
	VERSION 0
	URL "https://chromium.googlesource.com/libyuv/libyuv/+archive/b2528b0be934de1918e20c85fc170d809eeb49ab.tar.gz"
	EXCLUDE_FROM_ALL ON
	DOWNLOAD_ONLY YES
)

if(libyuv_ADDED)
	set(libyuv_SOURCES

    # Headers
    include/libyuv.h
    include/libyuv/basic_types.h
    include/libyuv/compare.h
    include/libyuv/convert.h
    include/libyuv/convert_argb.h
    include/libyuv/convert_from.h
    include/libyuv/convert_from_argb.h
    include/libyuv/cpu_id.h
    include/libyuv/mjpeg_decoder.h
    include/libyuv/planar_functions.h
    include/libyuv/rotate.h
    include/libyuv/rotate_argb.h
    include/libyuv/rotate_row.h
    include/libyuv/row.h
    include/libyuv/scale.h
    include/libyuv/scale_argb.h
    include/libyuv/scale_rgb.h
    include/libyuv/scale_row.h
    include/libyuv/scale_uv.h
    include/libyuv/version.h
    include/libyuv/video_common.h

    # Source Files
    source/compare.cc
    source/compare_common.cc
    source/compare_gcc.cc
    source/compare_win.cc
    source/convert.cc
    source/convert_argb.cc
    source/convert_from.cc
    source/convert_from_argb.cc
    source/convert_jpeg.cc
    source/convert_to_argb.cc
    source/convert_to_i420.cc
    source/cpu_id.cc
    source/mjpeg_decoder.cc
    source/mjpeg_validate.cc
    source/planar_functions.cc
    source/rotate.cc
    source/rotate_any.cc
    source/rotate_argb.cc
    source/rotate_common.cc
    source/rotate_gcc.cc
    source/rotate_win.cc
    source/row_any.cc
    source/row_common.cc
    source/row_gcc.cc
    source/row_win.cc
    source/scale.cc
    source/scale_any.cc
    source/scale_argb.cc
    source/scale_common.cc
    source/scale_gcc.cc
    source/scale_rgb.cc
    source/scale_uv.cc
    source/scale_win.cc
    source/video_common.cc
	)
	list(TRANSFORM libyuv_SOURCES PREPEND "${libyuv_SOURCE_DIR}/")
	add_library(yuv STATIC ${libyuv_SOURCES})

	target_include_directories(yuv PUBLIC "${libyuv_SOURCE_DIR}/include")

	add_library(libyuv::libyuv ALIAS yuv)
endif()
