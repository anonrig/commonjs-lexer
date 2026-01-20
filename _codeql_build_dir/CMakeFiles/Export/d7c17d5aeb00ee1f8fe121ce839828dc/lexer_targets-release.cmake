#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "lexer::lexer" for configuration "Release"
set_property(TARGET lexer::lexer APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(lexer::lexer PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/liblexer.a"
  )

list(APPEND _cmake_import_check_targets lexer::lexer )
list(APPEND _cmake_import_check_files_for_lexer::lexer "${_IMPORT_PREFIX}/lib/liblexer.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
