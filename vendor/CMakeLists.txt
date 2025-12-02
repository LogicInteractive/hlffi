# Build libhl from vendor/hashlink
#
# This is a standalone CMake project for building libhl.dll separately from HLFFI.
#
# Usage:
#   cd vendor
#   mkdir build && cd build
#   cmake .. -G "Visual Studio 17 2022" -A x64
#   cmake --build . --config Release
#
# Output: build/bin/libhl.dll, build/lib/libhl.lib

cmake_minimum_required(VERSION 3.15)
project(libhl_build)

# Build HashLink - only libhl DLL, no VM executable or hdll plugins
set(WITH_VM OFF CACHE BOOL "Don't build HL VM executable" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build libhl as DLL" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Don't build HashLink tests" FORCE)

# Disable all optional hdll libraries (we only need libhl core)
set(WITH_FMT OFF CACHE BOOL "" FORCE)
set(WITH_OPENAL OFF CACHE BOOL "" FORCE)
set(WITH_SDL OFF CACHE BOOL "" FORCE)
set(WITH_SQLITE OFF CACHE BOOL "" FORCE)
set(WITH_SSL OFF CACHE BOOL "" FORCE)
set(WITH_UI OFF CACHE BOOL "" FORCE)
set(WITH_UV OFF CACHE BOOL "" FORCE)
set(WITH_VIDEO OFF CACHE BOOL "" FORCE)
set(WITH_HEAPS OFF CACHE BOOL "" FORCE)

add_subdirectory(hashlink)

message(STATUS "Building libhl.dll from hashlink submodule")
