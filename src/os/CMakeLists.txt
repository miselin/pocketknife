if(MSVC)
  set(os_SOURCES win/time.c)
else()
  set(os_SOURCES unix/time.c)
endif()

add_library(os STATIC splitpath.c ${os_SOURCES})
add_library(os_shared SHARED splitpath.c ${os_SOURCES})
target_link_libraries(os INTERFACE cmake_base_compiler_options INTERFACE cmake_public_include_options)
target_link_libraries(os_shared INTERFACE cmake_base_compiler_options INTERFACE cmake_public_include_options)
