cmake_minimum_required(VERSION 3.16)

project(merve
  DESCRIPTION "Fast lexer to extract named exports via analysis from CommonJS modules"
  LANGUAGES C CXX
  VERSION 1.1.0 # x-release-please-version
)

set(MERVE_LIB_VERSION "1.1.0" CACHE STRING "lexer library version") # x-release-please-version
set(MERVE_LIB_SOVERSION "1" CACHE STRING "lexer library soversion") # x-release-please-major

include(GNUInstallDirs)

# Optional simdutf support for optimized string operations
if(MERVE_USE_SIMDUTF)
  # Try to find simdutf system package first
  find_package(simdutf QUIET)
  if(NOT simdutf_FOUND)
    include(cmake/CPM.cmake)
    find_package(Git QUIET)
    if(Git_FOUND)
      CPMAddPackage(
        NAME simdutf
        GITHUB_REPOSITORY simdutf/simdutf
        VERSION 8.0.0
        OPTIONS "SIMDUTF_TESTS OFF" "SIMDUTF_BENCHMARKS OFF" "SIMDUTF_TOOLS OFF"
      )
      set(simdutf_FOUND TRUE)
    endif()
  endif()
  if(simdutf_FOUND)
    message(STATUS "simdutf enabled for optimized string operations")
  else()
    message(WARNING "simdutf requested but not found, building without it")
    set(MERVE_USE_SIMDUTF OFF)
  endif()
endif()

include(CTest)
include(cmake/lexer-flags.cmake)

set(MERVE_SOURCE_DIR src)

add_subdirectory(src)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake)

option(MERVE_BENCHMARKS "Build benchmarks" OFF)
option(MERVE_TESTING "Build tests" ${BUILD_TESTING})

# There are cases where when embedding lexer as a dependency for other CMake
# projects as submodules or subdirectories (via FetchContent) can lead to
# errors due to CPM, so this is here to support disabling all the testing
# for lexer if one only wishes to use the lexer library.
if(MERVE_TESTING OR MERVE_BENCHMARKS)
  # Try to find GTest system package first
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(cmake/CPM.cmake)
    # CPM requires git as an implicit dependency
    find_package(Git QUIET)
    # We use googletest in the tests
    if(Git_FOUND AND MERVE_TESTING)
      CPMAddPackage(
        NAME GTest
        GITHUB_REPOSITORY google/googletest
        VERSION 1.14.0
        OPTIONS  "BUILD_GMOCK OFF" "INSTALL_GTEST OFF"
      )
    endif()
  endif()
  # We use Google Benchmark, but it does not build under several 32-bit systems.
  if(Git_FOUND AND MERVE_BENCHMARKS AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
    CPMAddPackage(
      NAME benchmark
      GITHUB_REPOSITORY google/benchmark
      GIT_TAG f91b6b4
      OPTIONS "BENCHMARK_ENABLE_TESTING OFF"
              "BENCHMARK_ENABLE_INSTALL OFF"
              "BENCHMARK_ENABLE_WERROR OFF"

    )
  endif()

  if (MERVE_TESTING AND NOT EMSCRIPTEN)
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
  endif(MERVE_TESTING AND NOT EMSCRIPTEN)

  If(MERVE_BENCHMARKS AND NOT EMSCRIPTEN)
    if(Git_FOUND)
      message(STATUS "Lexer benchmarks enabled.")
      add_subdirectory(benchmarks)
    else()
      message(STATUS "The benchmarks are disabled because git was not found.")
    endif()
  else(MERVE_BENCHMARKS AND NOT EMSCRIPTEN)
    if(is_top_project)
      message(STATUS "Lexer benchmarks disabled. Set MERVE_BENCHMARKS=ON to enable them.")
    endif()
  endif(MERVE_BENCHMARKS AND NOT EMSCRIPTEN)

  if (MERVE_TESTING AND EMSCRIPTEN)
    add_subdirectory(tests/wasm)
  endif(MERVE_TESTING AND EMSCRIPTEN)
endif()


add_library(merve::merve ALIAS merve)

set_target_properties(
  merve PROPERTIES
  VERSION "${MERVE_LIB_VERSION}"
  SOVERSION "${MERVE_LIB_SOVERSION}"
  WINDOWS_EXPORT_ALL_SYMBOLS YES
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(NOT MERVE_COVERAGE AND NOT EMSCRIPTEN)
  add_subdirectory(singleheader)
endif()

install(
  FILES include/merve.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT merve_development
)

install(
  DIRECTORY include/merve
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT merve_development
)

install(
  TARGETS merve
  EXPORT merve_targets
  RUNTIME COMPONENT merve_runtime
  LIBRARY COMPONENT merve_runtime
  NAMELINK_COMPONENT merve_development
  ARCHIVE COMPONENT merve_development
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

configure_file(cmake/merve-config.cmake.in merve-config.cmake @ONLY)

write_basic_package_version_file(
  merve-config-version.cmake
  COMPATIBILITY SameMinorVersion
)

set(
  MERVE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/merve"
  CACHE STRING "CMake package config location relative to the install prefix"
)
mark_as_advanced(MERVE_INSTALL_CMAKEDIR)

configure_file(
  "${PROJECT_SOURCE_DIR}/merve.pc.in"
  "${PROJECT_BINARY_DIR}/merve.pc"
  @ONLY
)
install(
  FILES "${PROJECT_BINARY_DIR}/merve.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
  COMPONENT nbytes_development
)

install(
  FILES
  "${PROJECT_BINARY_DIR}/merve-config.cmake"
  "${PROJECT_BINARY_DIR}/merve-config-version.cmake"
  DESTINATION "${MERVE_INSTALL_CMAKEDIR}"
  COMPONENT merve_development
)

install(
  EXPORT merve_targets
  NAMESPACE merve::
  DESTINATION "${MERVE_INSTALL_CMAKEDIR}"
  COMPONENT merve_development
)

install(
  EXPORT merve_targets
  NAMESPACE merve::
  DESTINATION "${MERVE_INSTALL_CMAKEDIR}"
  COMPONENT example_development
)

if(is_top_project)
  set(CPACK_PACKAGE_VENDOR "Merve Authors")
  set(CPACK_PACKAGE_CONTACT "yagiz@nizipli.com")
  set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE-MIT")
  set(CPACK_RPM_PACKAGE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE-MIT")
  set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
  set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
  include(CPack)
endif()
