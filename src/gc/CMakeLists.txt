add_library(gc STATIC
    gc.c
)
target_link_libraries(gc INTERFACE cmake_base_compiler_options)
