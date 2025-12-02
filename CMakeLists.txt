cmake_minimum_required(VERSION 3.15)
project(HLFFI VERSION 3.0.0 LANGUAGES C CXX)

# ========== Project Configuration ==========

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ========== Platform Detection ==========

if(WIN32)
    message(STATUS "Platform: Windows")
    set(HLFFI_PLATFORM "Windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Platform: Linux")
    set(HLFFI_PLATFORM "Linux")
elseif(APPLE)
    message(STATUS "Platform: macOS")
    set(HLFFI_PLATFORM "macOS")
else()
    message(WARNING "Platform: Unknown")
    set(HLFFI_PLATFORM "Unknown")
endif()

# ========== Build Options ==========

option(HLFFI_BUILD_SHARED "Build shared library" OFF)
option(HLFFI_BUILD_EXAMPLES "Build example programs" ON)
option(HLFFI_BUILD_TESTS "Build test programs" ON)
option(HLFFI_ENABLE_HOT_RELOAD "Enable hot reload support (requires HL 1.12+)" ON)
option(HLFFI_HLC_MODE "Build for HLC (HashLink/C) mode instead of JIT" OFF)
option(HLFFI_BUILD_LIBHL "Build libhl from vendor/hashlink" ON)

# ========== Build libhl from vendor ==========

if(HLFFI_BUILD_LIBHL)
    # Build HashLink as a subdirectory - only libhl DLL, no hdll plugins
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
    add_subdirectory(vendor/hashlink)
    message(STATUS "Building libhl.dll from vendor/hashlink")
endif()

# ========== Find HashLink ==========

# Try to find HashLink installation
# User can set HASHLINK_DIR to point to HashLink installation
if(DEFINED ENV{HASHLINK_DIR})
    set(HASHLINK_DIR $ENV{HASHLINK_DIR})
    message(STATUS "Using HASHLINK_DIR from environment: ${HASHLINK_DIR}")
elseif(WIN32)
    # Common HashLink install locations on Windows
    set(HASHLINK_SEARCH_PATHS
        "C:/HashLink"
        "C:/Program Files/HashLink"
        "C:/Program Files (x86)/HashLink"
        "$ENV{ProgramFiles}/HashLink"
        "$ENV{ProgramFiles\(x86\)}/HashLink"
    )
    foreach(SEARCH_PATH ${HASHLINK_SEARCH_PATHS})
        if(EXISTS "${SEARCH_PATH}")
            set(HASHLINK_DIR "${SEARCH_PATH}")
            message(STATUS "Found HashLink at: ${HASHLINK_DIR}")
            break()
        endif()
    endforeach()
endif()

if(NOT HASHLINK_DIR)
    message(WARNING "HashLink not found. Set HASHLINK_DIR environment variable or cmake -DHASHLINK_DIR=<path>")
    message(WARNING "Download from: https://hashlink.haxe.org/")
else()
    # Find HashLink headers
    find_path(HASHLINK_INCLUDE_DIR
        NAMES hl.h hlc.h
        PATHS ${HASHLINK_DIR}/include
        NO_DEFAULT_PATH
    )

    # Find HashLink library
    if(WIN32)
        find_library(HASHLINK_LIBRARY
            NAMES libhl.lib hl.lib
            PATHS ${HASHLINK_DIR} ${HASHLINK_DIR}/lib
            NO_DEFAULT_PATH
        )
    else()
        find_library(HASHLINK_LIBRARY
            NAMES hl libhl
            PATHS ${HASHLINK_DIR} ${HASHLINK_DIR}/lib
            NO_DEFAULT_PATH
        )
    endif()

    if(HASHLINK_INCLUDE_DIR AND HASHLINK_LIBRARY)
        message(STATUS "HashLink include: ${HASHLINK_INCLUDE_DIR}")
        message(STATUS "HashLink library: ${HASHLINK_LIBRARY}")
    else()
        message(WARNING "HashLink headers or library not found in ${HASHLINK_DIR}")
    endif()
endif()

# ========== HLFFI Libraries ==========

set(HLFFI_HEADERS
    include/hlffi.h
)

# Common sources for both JIT and HLC
set(HLFFI_COMMON_SOURCES
    src/hlffi_core.c
    src/hlffi_lifecycle.c
    src/hlffi_integration.c
    src/hlffi_events.c
    src/hlffi_threading.c
    src/hlffi_reload.c
    src/hlffi_types.c
    src/hlffi_values.c
    src/hlffi_objects.c
)

# JIT-specific sources (HashLink module loading)
set(HLFFI_JIT_SOURCES
    ${HLFFI_COMMON_SOURCES}
    vendor/hashlink/src/code.c
    vendor/hashlink/src/module.c
    vendor/hashlink/src/jit.c
)

# HLC-specific sources
set(HLFFI_HLC_SOURCES
    ${HLFFI_COMMON_SOURCES}
    src/hlffi_hlc.c
)

# Disable warnings-as-errors for HashLink vendor sources (they have known warnings)
if(MSVC)
    set_source_files_properties(
        vendor/hashlink/src/code.c
        vendor/hashlink/src/module.c
        vendor/hashlink/src/jit.c
        PROPERTIES COMPILE_FLAGS "/W3"
    )
endif()

# Common include directories
set(HLFFI_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set(HLFFI_PRIVATE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/hashlink/src
)

# ========== hlffi_jit (JIT mode library) ==========
add_library(hlffi_jit STATIC ${HLFFI_HEADERS} ${HLFFI_JIT_SOURCES})
target_include_directories(hlffi_jit PUBLIC ${HLFFI_INCLUDE_DIRS} PRIVATE ${HLFFI_PRIVATE_INCLUDE_DIRS})
if(WIN32)
    target_link_libraries(hlffi_jit PRIVATE ws2_32)
    if(MSVC)
        target_compile_options(hlffi_jit PRIVATE /W4 /WX /MP)
        set_property(TARGET hlffi_jit PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# ========== hlffi_hlc (HLC mode library) ==========
add_library(hlffi_hlc STATIC ${HLFFI_HEADERS} ${HLFFI_HLC_SOURCES})
target_include_directories(hlffi_hlc PUBLIC ${HLFFI_INCLUDE_DIRS} PRIVATE ${HLFFI_PRIVATE_INCLUDE_DIRS})
target_compile_definitions(hlffi_hlc PUBLIC HLFFI_HLC_MODE=1)
if(WIN32)
    target_link_libraries(hlffi_hlc PRIVATE ws2_32)
    if(MSVC)
        target_compile_options(hlffi_hlc PRIVATE /W4 /WX /MP)
        set_property(TARGET hlffi_hlc PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# ========== Alias for backward compatibility ==========
# 'hlffi' target points to JIT by default, or HLC if HLFFI_HLC_MODE is set
if(HLFFI_HLC_MODE)
    add_library(hlffi ALIAS hlffi_hlc)
    message(STATUS "Default hlffi target: hlffi_hlc (HLC mode)")
else()
    add_library(hlffi ALIAS hlffi_jit)
    message(STATUS "Default hlffi target: hlffi_jit (JIT mode)")
endif()

# ========== Examples ==========

if(HLFFI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ========== Tests ==========

if(HLFFI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ========== Install ==========

install(TARGETS hlffi_jit hlffi_hlc
    EXPORT HLFFITargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES ${HLFFI_HEADERS}
    DESTINATION include
)

# ========== Package Config ==========

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/HLFFIConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HLFFIConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/HLFFIConfig.cmake"
    INSTALL_DESTINATION lib/cmake/HLFFI
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HLFFIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HLFFIConfigVersion.cmake"
    DESTINATION lib/cmake/HLFFI
)

install(EXPORT HLFFITargets
    FILE HLFFITargets.cmake
    NAMESPACE HLFFI::
    DESTINATION lib/cmake/HLFFI
)

# ========== Summary ==========

message(STATUS "")
message(STATUS "========== HLFFI v${PROJECT_VERSION} Configuration ==========")
message(STATUS "Platform: ${HLFFI_PLATFORM}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build shared: ${HLFFI_BUILD_SHARED}")
message(STATUS "Build examples: ${HLFFI_BUILD_EXAMPLES}")
message(STATUS "Build tests: ${HLFFI_BUILD_TESTS}")
message(STATUS "Hot reload: ${HLFFI_ENABLE_HOT_RELOAD}")
message(STATUS "HLC mode: ${HLFFI_HLC_MODE}")
message(STATUS "HashLink dir: ${HASHLINK_DIR}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "==============================================")
message(STATUS "")
