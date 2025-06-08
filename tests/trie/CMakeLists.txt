add_executable(trie_test trie_test.cc)

target_link_libraries(trie_test trie GTest::gtest GTest::gtest_main)

gtest_discover_tests(trie_test)
