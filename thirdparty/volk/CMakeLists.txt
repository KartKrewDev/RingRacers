# Update from https://github.com/zeux/volk
# volk 1.4.304
# License: MIT

add_library(volk STATIC src/volk.c)
target_link_libraries(volk PUBLIC VulkanHeaders)
target_include_directories(volk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(volk PUBLIC -DVK_NO_PROTOTYPES -DVK_ENABLE_BETA_EXTENSIONS)
target_compile_features(volk INTERFACE cxx_std_17)
target_compile_features(volk INTERFACE c_std_11)
if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
	target_compile_options(volk PRIVATE -w)
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(volk PRIVATE /W0)
endif()
add_library(volk::volk ALIAS volk)
