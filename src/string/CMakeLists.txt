add_library(string STATIC string_builder.c cord.c)
add_library(string_shared SHARED string_builder.c cord.c)
target_link_libraries(string INTERFACE cmake_base_compiler_options)
target_link_libraries(string_shared INTERFACE cmake_base_compiler_options)
