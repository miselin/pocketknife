add_library(trie STATIC trie.c)
add_library(trie_shared SHARED trie.c)
target_link_libraries(trie INTERFACE cmake_base_compiler_options)
target_link_libraries(trie_shared INTERFACE cmake_base_compiler_options)
