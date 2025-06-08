if(compiler_included)
  return()
endif()

set(compiler_included true)

add_library(cmake_base_compiler_options INTERFACE)
add_library(cmake_cpp_boilerplate::compiler_options ALIAS cmake_base_compiler_options)

option(WARNING_AS_ERROR "Treat compiler warnings as errors" ON)
if(MSVC)
  target_compile_options(cmake_base_compiler_options INTERFACE /W4 "/permissive-")
  if(WARNING_AS_ERROR)
    target_compile_options(cmake_base_compiler_options INTERFACE /WX)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_definitions(cmake_base_compiler_options
                             INTERFACE -D_XOPEN_SOURCE=600
                                       -D_POSIX_C_SOURCE=200809L
                                       -D_DEFAULT_SOURCE=1
                                       $<$<BOOL:APPLE>:_DARWIN_C_SOURCE=200809L>)
  target_compile_options(cmake_base_compiler_options
                         INTERFACE -Wall
                                   -Wextra
                                   -Wshadow
                                   $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
                                   $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
                                   -Wcast-align
                                   -Wunused
                                   $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
                                   -Wpedantic
                                   -Wconversion
                                   -Wsign-conversion
                                   -Wnull-dereference
                                   -Wdouble-promotion
                                   -Wformat=2
                                   $<$<C_COMPILER_ID:GNU>:-Wmisleading-indentation>
                                   $<$<C_COMPILER_ID:GNU>:-Wduplicated-cond>
                                   $<$<C_COMPILER_ID:GNU>:-Wduplicated-branches>
                                   $<$<C_COMPILER_ID:GNU>:-Wlogical-op>
                                   $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wuseless-cast>
                        )

  if(WARNING_AS_ERROR)
    target_compile_options(cmake_base_compiler_options INTERFACE -Werror)
  endif()
endif()

option(ENABLE_PCH "Enable Precompiled Headers" OFF)
if (ENABLE_PCH)
  target_precompile_headers(cmake_base_compiler_options INTERFACE
          <algorithm>
          <array>
          <vector>
          <string>
          <utility>
          <functional>
          <memory>
          <memory_resource>
          <string_view>
          <cmath>
          <cstddef>
          <type_traits>
          )
endif ()

add_library(cmake_asan_options INTERFACE)
add_library(cmake_tsan_options INTERFACE)
add_library(cmake_msan_options INTERFACE)
add_library(cmake_ubsan_options INTERFACE)

target_compile_options(cmake_asan_options INTERFACE -fsanitize=address -fno-omit-frame-pointer)
target_link_libraries(cmake_asan_options INTERFACE -fsanitize=address)

target_compile_options(cmake_tsan_options INTERFACE -fsanitize=thread)
target_link_libraries(cmake_tsan_options INTERFACE -fsanitize=thread)

target_compile_options(cmake_msan_options INTERFACE -fsanitize=memory -fno-omit-frame-pointer)
target_link_libraries(cmake_msan_options INTERFACE -fsanitize=memory)

target_compile_options(cmake_ubsan_options INTERFACE -fsanitize=undefined)
target_link_libraries(cmake_ubsan_options INTERFACE -fsanitize=undefined)

option(USE_ASAN "Enable the Address Sanitizers" OFF)
option(USE_TSAN "Enable the Thread Sanitizers" OFF)
option(USE_MSAN "Enable the Memory Sanitizers" OFF)
option(USE_UBSAN "Enable the Undefined Behavior Sanitizers" OFF)

if(USE_ASAN)
  message("Enable Address Sanitizer")
  target_link_libraries(cmake_base_compiler_options INTERFACE cmake_asan_options)
endif ()

if(USE_TSAN)
  message("Enable Thread Sanitizer")
  target_link_libraries(cmake_base_compiler_options INTERFACE cmake_tsan_options)
endif()

if(USE_MSAN)
  message("Enable Memory Sanitizer")
  target_link_libraries(cmake_base_compiler_options INTERFACE cmake_msan_options)
endif()

  if(USE_UBSAN)
  message("Enable Undefined Behavior Sanitizer")
  target_link_libraries(cmake_base_compiler_options INTERFACE cmake_ubsan_options)
endif()
