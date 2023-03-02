# Update from https://github.com/nothings/stb
add_library(stb_rect_pack STATIC stb_rect_pack.c include/stb_rect_pack.h)
target_include_directories(stb_rect_pack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
