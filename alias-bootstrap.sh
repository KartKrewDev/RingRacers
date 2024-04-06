#!/usr/bin/env sh

# All these commands can be run from anywhere in the git
# tree, not just the top level.

# Usage: git cmake
#
# Same usage as standard CMake command.
#
git config 'alias.cmake' '!cmake'

# Usage: git build <build preset> [options]
# Usage: git build [options]
#
# In the second usage, when no preset is given, the
# "default" build preset is used.
#
# Available options can be found by running:
#
#     git cmake --build
#
git config 'alias.build' '!./custom-git-commands/git-build'

# Usage: git crossmake
#
# Shortcut to i686-w64-mingw32-cmake-static (CMake cross
# compiler)
#
git config 'alias.crossmake' '!i686-w64-mingw32-cmake-static'
