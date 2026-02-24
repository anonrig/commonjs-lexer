set(MERVE_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
if(MSVC)
  add_compile_options("/Zi" "/EHsc" "/GR")
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

include(${PROJECT_SOURCE_DIR}/cmake/add-cpp-test.cmake)
link_libraries(merve)

if(MSVC AND BUILD_SHARED_LIBS)
  message(STATUS "For some tests we use Google Test and it fails when building a DLL.")
  message(STATUS "Thus the tests are disabled. Sorry.")
else()
  include(GoogleTest)
  add_executable(real_world_tests real_world_tests.cpp)
  target_link_libraries(real_world_tests PRIVATE GTest::gtest_main)
  gtest_discover_tests(real_world_tests)

  add_executable(c_api_tests c_api_tests.cpp)
  target_link_libraries(c_api_tests PRIVATE GTest::gtest_main)
  gtest_discover_tests(c_api_tests)

  # Verify merve_c.h compiles as pure C (compile-only test).
  add_executable(c_api_compile_test c_api_compile_test.c)
  target_include_directories(c_api_compile_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  set_target_properties(c_api_compile_test PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    EXCLUDE_FROM_DEFAULT_BUILD TRUE
    LINKER_LANGUAGE C
  )
  add_test(
    NAME c_api_compile_test
    COMMAND ${CMAKE_COMMAND} --build . --target c_api_compile_test --config $<CONFIGURATION>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
      target_link_libraries(real_world_tests PUBLIC stdc++fs)
    endif()
  endif()

  if(MSVC OR MINGW)
    target_compile_definitions(real_world_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(c_api_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()

endif()
