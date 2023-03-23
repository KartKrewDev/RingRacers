CPMAddPackage(
	NAME glm
	VERSION 0.9.9.8
	URL "https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip"
	EXCLUDE_FROM_ALL ON
	DOWNLOAD_ONLY ON
)

if(glm_ADDED)
	add_library(glm INTERFACE)
	add_library(glm::glm ALIAS glm)
	target_include_directories(glm INTERFACE "${glm_SOURCE_DIR}")
endif()
