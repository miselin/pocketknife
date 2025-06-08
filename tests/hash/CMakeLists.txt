add_executable(hash_test fnv_test.cc)

target_link_libraries(hash_test hash GTest::gtest GTest::gtest_main)

gtest_discover_tests(hash_test)
