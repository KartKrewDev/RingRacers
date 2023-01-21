add_library(glad STATIC src/gles2.c src/gl.c include/glad/gles2.h include/glad/gl.h include/KHR/khrplatform.h)
add_library(glad::glad ALIAS glad)
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
