# Update from https://github.com/fmtlib/fmt
# fmt 11.1.3
# License: MIT with object code exception

add_library(fmt INTERFACE)
target_include_directories(fmt INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(fmt INTERFACE cxx_std_11)
target_compile_definitions(fmt INTERFACE -DFMT_HEADER_ONLY)
target_compile_definitions(fmt INTERFACE "-DFMT_UNICODE=0")
add_library(fmt::fmt-header-only ALIAS fmt)
