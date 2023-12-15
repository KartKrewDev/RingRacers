# Update from https://github.com/webmproject/libwebm
# libwebm version 1.0.0.31
# License: BSD-3

add_library(libwebm_mkvmuxer STATIC
	common/webmids.h

	common/file_util.cc
	common/file_util.h
	common/hdr_util.cc
	common/hdr_util.h

	mkvmuxer/mkvmuxer.h
	mkvmuxer/mkvmuxertypes.h
	mkvmuxer/mkvmuxerutil.h
	mkvmuxer/mkvwriter.h
	mkvmuxer/mkvmuxer.cc
	mkvmuxer/mkvmuxerutil.cc
	mkvmuxer/mkvwriter.cc
)
target_include_directories(libwebm_mkvmuxer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(libwebm_mkvmuxer PUBLIC cxx_std_11)
target_compile_definitions(libwebm_mkvmuxer PRIVATE
	-D__STDC_CONSTANT_MACROS
	-D__STDC_FORMAT_MACROS
	-D__STDC_LIMIT_MACROS

)
set_target_properties(libwebm_mkvmuxer PROPERTIES CXX_STANDARD 11)
add_library(webm::libwebm_mkvmuxer ALIAS libwebm_mkvmuxer)
