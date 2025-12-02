#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HLFFI::hlffi" for configuration "Debug"
set_property(TARGET HLFFI::hlffi APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(HLFFI::hlffi PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/hlffi.lib"
  )

list(APPEND _cmake_import_check_targets HLFFI::hlffi )
list(APPEND _cmake_import_check_files_for_HLFFI::hlffi "${_IMPORT_PREFIX}/lib/hlffi.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
