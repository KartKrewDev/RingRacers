# Update from https://github.com/fmtlib/fmt
# fmt 11.1.0
# License: MIT with object code exception

add_library(fmt INTERFACE)
target_include_directories(fmt INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(fmt INTERFACE cxx_std_11)
target_compile_definitions(fmt INTERFACE -DFMT_HEADER_ONLY)
add_library(fmt::fmt-header-only ALIAS fmt)
