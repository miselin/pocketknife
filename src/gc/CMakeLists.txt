add_library(gc STATIC gc.c)
add_library(gc_shared SHARED gc.c)
target_link_libraries(gc INTERFACE cmake_base_compiler_options)
target_link_libraries(gc_shared INTERFACE cmake_base_compiler_options)
