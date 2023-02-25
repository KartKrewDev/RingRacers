CPMAddPackage(
	NAME libwebm
	VERSION 1.0.0.29
	URL "https://chromium.googlesource.com/webm/libwebm/+archive/2f9fc054ab9547ca06071ec68dab9d54960abb2e.tar.gz"
	EXCLUDE_FROM_ALL ON
	DOWNLOAD_ONLY YES
)

if(libwebm_ADDED)
	set(libwebm_SOURCES

		common/file_util.cc
		common/file_util.h
		common/hdr_util.cc
		common/hdr_util.h
		common/webmids.h

		mkvmuxer/mkvmuxer.cc
		mkvmuxer/mkvmuxer.h
		mkvmuxer/mkvmuxertypes.h
		mkvmuxer/mkvmuxerutil.cc
		mkvmuxer/mkvmuxerutil.h
		mkvmuxer/mkvwriter.cc
		mkvmuxer/mkvwriter.h
	)
	list(TRANSFORM libwebm_SOURCES PREPEND "${libwebm_SOURCE_DIR}/")
	add_library(webm STATIC ${libwebm_SOURCES})
	target_include_directories(webm PUBLIC "${libwebm_SOURCE_DIR}")
	target_compile_features(webm PRIVATE cxx_std_11)
	add_library(webm::libwebm ALIAS webm)
endif()
