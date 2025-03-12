# Update from https://github.com/KhronosGroup/Vulkan-Headers
# Vulkan-Headers version 1.4.304
# License: MIT OR Apache-2.0

add_library(VulkanHeaders INTERFACE)
target_include_directories(VulkanHeaders INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
