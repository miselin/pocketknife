add_executable(allocator_test allocator_test.cc)
target_link_libraries(allocator_test alloc_asan cmake_asan_options GTest::gtest GTest::gtest_main)
gtest_discover_tests(allocator_test)
