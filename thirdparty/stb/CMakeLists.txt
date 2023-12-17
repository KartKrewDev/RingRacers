# Update from https://github.com/nothings/stb
add_library(Stb STATIC)
target_sources(Stb PRIVATE stb_rect_pack.c stb_vorbis.c)
target_sources(Stb PUBLIC include/stb_rect_pack.h include/stb_vorbis.h)
target_include_directories(Stb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(Stb::Stb ALIAS Stb)
