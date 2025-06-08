add_executable(gc_test gc_test.cc)

# Force ASAN for GC tests to catch memory errors
target_link_libraries(gc_test gc cmake_asan_options GTest::gtest GTest::gtest_main)

gtest_discover_tests(gc_test)
