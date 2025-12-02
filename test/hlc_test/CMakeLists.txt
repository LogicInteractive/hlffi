# HLC Mode Test
# Build with: cmake -B build -DHLFFI_HLC_MODE=ON && cmake --build build

cmake_minimum_required(VERSION 3.15)
project(hlffi_hlc_test LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# This test requires HLC mode
if(NOT HLFFI_HLC_MODE)
    message(WARNING "HLC test requires HLFFI_HLC_MODE=ON, skipping")
    return()
endif()

# Find HashLink
if(NOT HASHLINK_INCLUDE_DIR)
    message(FATAL_ERROR "HASHLINK_INCLUDE_DIR not set")
endif()

# Collect all HLC-generated C files
file(GLOB_RECURSE HLC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlc_output/*.c"
)

# Create HLC test executable
add_executable(test_hlc
    test_hlc.c
    ${HLC_SOURCES}
)

# Include directories
target_include_directories(test_hlc PRIVATE
    ${HASHLINK_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../hlc_output
)

# Link HLFFI library
target_link_libraries(test_hlc PRIVATE hlffi)

# Define HLC mode
target_compile_definitions(test_hlc PRIVATE HLFFI_HLC_MODE=1)

# Disable warnings for HLC-generated code
if(MSVC)
    target_compile_options(test_hlc PRIVATE /W3)
else()
    target_compile_options(test_hlc PRIVATE -w)
endif()

message(STATUS "HLC test configured with ${CMAKE_CURRENT_SOURCE_DIR}/../hlc_output")
