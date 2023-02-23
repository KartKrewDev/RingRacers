CPMAddPackage(
	NAME ogg
	VERSION 1.3.5
	URL "https://github.com/xiph/ogg/releases/download/v1.3.5/libogg-1.3.5.zip"
	EXCLUDE_FROM_ALL ON
)

if(ogg_ADDED)
	# Fixes bug with find_package not being able to find
	# ogg when cross-building.
	set(OGG_INCLUDE_DIR "${ogg_SOURCE_DIR}/include")
	set(OGG_LIBRARY Ogg:ogg)
endif()
