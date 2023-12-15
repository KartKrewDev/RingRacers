# Update from https://github.com/g-truc/glm/releases
# glm 0.9.9.8
# License: The Happy Bunny License or MIT License

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(glm::glm ALIAS glm)
