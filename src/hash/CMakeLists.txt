add_library(hash STATIC fnv.c)
add_library(hash_shared SHARED fnv.c)
target_link_libraries(hash INTERFACE cmake_base_compiler_options)
target_link_libraries(hash_shared INTERFACE cmake_base_compiler_options)
