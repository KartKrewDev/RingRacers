# https://github.com/tcbrindle/span/
# Portable implementation of C++20 std::span
# Boost License 1.0

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

add_library(tcbrindle_span INTERFACE)
target_sources(tcbrindle_span INTERFACE include/tcb/span.hpp)
target_include_directories(tcbrindle_span INTERFACE include)

add_library(tcbrindle::span ALIAS tcbrindle_span)
