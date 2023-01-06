# Update from https://github.com/nothings/stb
# This doesn't use CPM because stb_vorbis.c has a weird header setup
add_library(stb_vorbis STATIC stb_vorbis.c include/stb_vorbis.h)
target_include_directories(stb_vorbis PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
