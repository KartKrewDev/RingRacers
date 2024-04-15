#!/usr/bin/env bash

# Makes a fused macOS Universal app bundle in the arm64 release preset dir
# Only works if in master branch or in source tarball

set -e

rm -rf "build/ninja-x64_osx_vcpkg-release"
rm -rf "build/ninja-arm64_osx_vcpkg-release"
cmake --preset ninja-x64_osx_vcpkg-release -DVCPKG_OVERLAY_TRIPLETS=scripts/ -DVCPKG_TARGET_TRIPLET=x64-osx-1015
cmake --build --preset ninja-x64_osx_vcpkg-release
cmake --preset ninja-arm64_osx_vcpkg-release -DVCPKG_OVERLAY_TRIPLETS=scripts/ -DVCPKG_TARGET_TRIPLET=arm64-osx-1015
cmake --build --preset ninja-arm64_osx_vcpkg-release

mkdir -p build/dist
rm -rf "build/dist/Dr. Robotnik's Ring Racers.app" "build/dist/ringracers.app"

cp -r build/ninja-arm64_osx_vcpkg-release/bin/ringracers.app build/dist/

lipo -create \
	-output "build/dist/ringracers.app/Contents/MacOS/ringracers" \
	build/ninja-x64_osx_vcpkg-release/bin/ringracers.app/Contents/MacOS/ringracers \
	build/ninja-arm64_osx_vcpkg-release/bin/ringracers.app/Contents/MacOS/ringracers

mv build/dist/ringracers.app "build/dist/Dr. Robotnik's Ring Racers.app"
