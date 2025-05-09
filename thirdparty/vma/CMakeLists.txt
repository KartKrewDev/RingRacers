# Update from https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
# VulkanMemoryAllocator version 3.2.1
# License: MIT

add_library(VulkanMemoryAllocator STATIC src/vk_mem_alloc.cpp)
target_link_libraries(VulkanMemoryAllocator PUBLIC VulkanHeaders)
target_include_directories(VulkanMemoryAllocator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(VulkanMemoryAllocator PUBLIC cxx_std_17)
set_target_properties(VulkanMemoryAllocator PROPERTIES CXX_STANDARD 17)
add_library(GPUOpen::VulkanMemoryAllocator ALIAS VulkanMemoryAllocator)
