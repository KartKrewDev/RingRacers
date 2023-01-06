CPMAddPackage(
	NAME RapidJSON
	VERSION 1.1.0
	URL "https://github.com/Tencent/rapidjson/archive/v1.1.0.tar.gz"
	EXCLUDE_FROM_ALL ON
	DOWNLOAD_ONLY ON
)

if(RapidJSON_ADDED)
	add_library(RapidJSON INTERFACE)
	add_library(RapidJSON::RapidJSON ALIAS RapidJSON)
	target_include_directories(RapidJSON INTERFACE "${RapidJSON_SOURCE_DIR}/include")
endif()
