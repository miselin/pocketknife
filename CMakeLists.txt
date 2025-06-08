cmake_minimum_required(VERSION 3.16)
project(pocketknife C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

include(config)
include(CMakeDependentOption)
include(CTest)
include(compiler)

find_package(Doxygen)

option(COVERAGE "Generate code coverage data" OFF)
if (COVERAGE AND NOT MSVC)
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif ()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

add_subdirectory(src)

if (DOXYGEN_FOUND)
    doxygen_add_docs(doxygen src include)
endif ()

option(BUILD_TESTS "Build tests" ON)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTS)
  add_subdirectory(third_party)

  enable_testing()
  add_subdirectory(tests)
  add_subdirectory(benchmarks)
endif()
