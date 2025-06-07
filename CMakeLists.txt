cmake_minimum_required(VERSION 3.16)
project(pocketknife C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

include(config)
include(CMakeDependentOption)
include(compiler)

if (NOT MSVC)
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif ()

add_subdirectory(src)

option(BUILD_TESTS "Build tests" ON)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTS)
  add_subdirectory(third_party)

  enable_testing()
  add_subdirectory(tests)
  add_subdirectory(benchmarks)
endif()
