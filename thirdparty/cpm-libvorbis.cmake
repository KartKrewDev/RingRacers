CPMAddPackage(
	NAME vorbis
	VERSION 1.3.7
	URL "https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.zip"
	EXCLUDE_FROM_ALL ON
)

if(vorbis_ADDED)
	add_library(Vorbis::vorbis ALIAS vorbis)
	add_library(Vorbis::vorbisenc ALIAS vorbisenc)
endif()
