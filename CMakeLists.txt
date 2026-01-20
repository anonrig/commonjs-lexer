cmake_minimum_required(VERSION 3.16)

project(lexer
  DESCRIPTION "Fast lexer to extract named exports via analysis from CommonJS modules"
  LANGUAGES C CXX
  VERSION 1.0.0
)

set(LEXER_LIB_VERSION "1.0.0" CACHE STRING "lexer library version")
set(LEXER_LIB_SOVERSION "1" CACHE STRING "lexer library soversion")

include(GNUInstallDirs)

include(CTest)
include(cmake/lexer-flags.cmake)

set(LEXER_SOURCE_DIR src)

add_subdirectory(src)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake)

option(LEXER_BENCHMARKS "Build benchmarks" OFF)
option(LEXER_TESTING "Build tests" ${BUILD_TESTING})

# There are cases where when embedding lexer as a dependency for other CMake
# projects as submodules or subdirectories (via FetchContent) can lead to
# errors due to CPM, so this is here to support disabling all the testing
# for lexer if one only wishes to use the lexer library.
if(LEXER_TESTING OR LEXER_BENCHMARKS)
  # Try to find GTest system package first
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(cmake/CPM.cmake)
    # CPM requires git as an implicit dependency
    find_package(Git QUIET)
    # We use googletest in the tests
    if(Git_FOUND AND LEXER_TESTING)
      CPMAddPackage(
        NAME GTest
        GITHUB_REPOSITORY google/googletest
        VERSION 1.14.0
        OPTIONS  "BUILD_GMOCK OFF" "INSTALL_GTEST OFF"
      )
    endif()
  endif()
  # We use Google Benchmark, but it does not build under several 32-bit systems.
  if(Git_FOUND AND LEXER_BENCHMARKS AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
    CPMAddPackage(
      NAME benchmark
      GITHUB_REPOSITORY google/benchmark
      GIT_TAG f91b6b4
      OPTIONS "BENCHMARK_ENABLE_TESTING OFF"
              "BENCHMARK_ENABLE_INSTALL OFF"
              "BENCHMARK_ENABLE_WERROR OFF"

    )
  endif()

  if (LEXER_TESTING AND NOT EMSCRIPTEN)
    if(Git_FOUND)
      message(STATUS "The tests are enabled.")
      add_subdirectory(tests)
    else()
      message(STATUS "The tests are disabled because git was not found.")
    endif()
  else()
    if(is_top_project)
      message(STATUS "The tests are disabled.")
    endif()
  endif(LEXER_TESTING AND NOT EMSCRIPTEN)

  If(LEXER_BENCHMARKS AND NOT EMSCRIPTEN)
    if(Git_FOUND)
      message(STATUS "Lexer benchmarks enabled.")
      add_subdirectory(benchmarks)
    else()
      message(STATUS "The benchmarks are disabled because git was not found.")
    endif()
  else(LEXER_BENCHMARKS AND NOT EMSCRIPTEN)
    if(is_top_project)
      message(STATUS "Lexer benchmarks disabled. Set LEXER_BENCHMARKS=ON to enable them.")
    endif()
  endif(LEXER_BENCHMARKS AND NOT EMSCRIPTEN)

  if (LEXER_TESTING AND EMSCRIPTEN)
    add_subdirectory(tests/wasm)
  endif(LEXER_TESTING AND EMSCRIPTEN)
endif()


add_library(lexer::lexer ALIAS lexer)

set_target_properties(
  lexer PROPERTIES
  VERSION "${LEXER_LIB_VERSION}"
  SOVERSION "${LEXER_LIB_SOVERSION}"
  WINDOWS_EXPORT_ALL_SYMBOLS YES
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(NOT LEXER_COVERAGE AND NOT EMSCRIPTEN)
  add_subdirectory(singleheader)
endif()

install(
  FILES include/lexer.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT lexer_development
)

install(
  DIRECTORY include/lexer
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT lexer_development
)

install(
  TARGETS lexer
  EXPORT lexer_targets
  RUNTIME COMPONENT lexer_runtime
  LIBRARY COMPONENT lexer_runtime
  NAMELINK_COMPONENT lexer_development
  ARCHIVE COMPONENT lexer_development
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

configure_file(cmake/lexer-config.cmake.in lexer-config.cmake @ONLY)

write_basic_package_version_file(
  lexer-config-version.cmake
  COMPATIBILITY SameMinorVersion
)

set(
  LEXER_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/lexer"
  CACHE STRING "CMake package config location relative to the install prefix"
)
mark_as_advanced(LEXER_INSTALL_CMAKEDIR)

install(
  FILES
  "${PROJECT_BINARY_DIR}/lexer-config.cmake"
  "${PROJECT_BINARY_DIR}/lexer-config-version.cmake"
  DESTINATION "${LEXER_INSTALL_CMAKEDIR}"
  COMPONENT lexer_development
)

install(
  EXPORT lexer_targets
  NAMESPACE lexer::
  DESTINATION "${LEXER_INSTALL_CMAKEDIR}"
  COMPONENT lexer_development
)

install(
  EXPORT lexer_targets
  NAMESPACE lexer::
  DESTINATION "${LEXER_INSTALL_CMAKEDIR}"
  COMPONENT example_development
)

if(is_top_project)
  set(CPACK_PACKAGE_VENDOR "Lexer Authors")
  set(CPACK_PACKAGE_CONTACT "yagiz@nizipli.com")
  set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE-MIT")
  set(CPACK_RPM_PACKAGE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE-MIT")
  set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
  set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
  include(CPack)
endif()
