#
# Amalgamation
#
set(SINGLEHEADER_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/lexer.h
)
set_source_files_properties(${SINGLEHEADER_FILES} PROPERTIES GENERATED TRUE)

# In theory, this is unneeded, because the tests module does the same test:
find_package (Python3 COMPONENTS Interpreter)

if (Python3_Interpreter_FOUND)
  MESSAGE( STATUS "Python found, we are going to amalgamate.py." )

  add_custom_command(
    OUTPUT ${SINGLEHEADER_FILES}
    COMMAND ${CMAKE_COMMAND} -E env
      AMALGAMATE_SOURCE_PATH=${PROJECT_SOURCE_DIR}/src
      AMALGAMATE_INPUT_PATH=${PROJECT_SOURCE_DIR}/include
      AMALGAMATE_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate.py
      #
      # This is the best way I could find to make amalgamation trigger whenever source files or
      # header files change: since the "lexer" library has to get rebuilt when that happens, we
      # take a dependency on the generated library file (even though we're not using it). Depending
      # on lexer-source doesn't do the trick because DEPENDS here can only depend on an
      # *artifact*--it won't scan source and include files the way a concrete library or executable
      # will.
      #
      # It sucks that we have to build the actual library to make it happen, but it's better than\
      # nothing!
      #
      DEPENDS amalgamate.py lexer
  )
  add_custom_target(lexer-singleheader-files DEPENDS ${SINGLEHEADER_FILES})

  #
  # Include this if you intend to #include "lexer.cpp" in your own .cpp files.
  #
  add_library(lexer-singleheader-include-source INTERFACE)
  target_include_directories(lexer-singleheader-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
  add_dependencies(lexer-singleheader-include-source lexer-singleheader-files)

  add_library(lexer-singleheader-source INTERFACE)
  target_sources(lexer-singleheader-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp>)
  target_link_libraries(lexer-singleheader-source INTERFACE lexer-singleheader-include-source)
  add_library(lexer-singleheader-lib STATIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp>)
else()
  MESSAGE( STATUS "Python not found, we are unable to test amalgamate.py." )
endif()
