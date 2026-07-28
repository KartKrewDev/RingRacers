cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(CMAKE_BINARY_DIR "${BINARY_DIR}")
set(CMAKE_CURRENT_BINARY_DIR "${BINARY_DIR}")

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

include(GitUtilities)

git_current_branch(SRB2_COMP_BRANCH)
git_working_tree_dirty(SRB2_COMP_UNCOMMITTED)

git_latest_commit(SRB2_COMP_REVISION)
git_subject(subject)
string(REGEX REPLACE "([\"\\])" "\\\\\\1" SRB2_COMP_NOTE "${subject}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE None)
endif()

configure_file("${CMAKE_SOURCE_DIR}/src/config_comptime.h.in" "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/src/config_comptime.h")
