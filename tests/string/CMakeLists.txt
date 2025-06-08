add_executable(string_test string_builder_test.cc)

target_link_libraries(string_test string GTest::gtest GTest::gtest_main)

gtest_discover_tests(string_test)
