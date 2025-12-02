#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "HLFFI::hlffi" for configuration "RelWithDebInfo"
set_property(TARGET HLFFI::hlffi APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(HLFFI::hlffi PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/hlffi.lib"
  )

list(APPEND _cmake_import_check_targets HLFFI::hlffi )
list(APPEND _cmake_import_check_files_for_HLFFI::hlffi "${_IMPORT_PREFIX}/lib/hlffi.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
