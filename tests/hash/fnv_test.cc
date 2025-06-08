#include <pocketknife/hash/fnv.h>

#include <gtest/gtest.h>

TEST(FNVHashTest, FNV1_KnownValues) {
  // Test vectors from http://www.isthe.com/chongo/tech/comp/fnv/
  const uint8_t empty[1] = {0};
  EXPECT_EQ(hash_fnv1(empty, 0), 0xcbf29ce484222325ULL);

  const uint8_t a[] = {'a'};
  EXPECT_EQ(hash_fnv1(a, 1), 0xaf63bd4c8601b7beULL);

  const uint8_t hello[] = {'h', 'e', 'l', 'l', 'o'};
  EXPECT_EQ(hash_fnv1(hello, 5), 0x7b495389bdbdd4c7ULL);
}

TEST(FNVHashTest, FNV1a_KnownValues) {
  // Test vectors from http://www.isthe.com/chongo/tech/comp/fnv/
  const uint8_t empty[1] = {0};
  EXPECT_EQ(hash_fnv1a(empty, 0), 0xcbf29ce484222325ULL);

  const uint8_t a[] = {'a'};
  EXPECT_EQ(hash_fnv1a(a, 1), 0xaf63dc4c8601ec8cULL);

  const uint8_t hello[] = {'h', 'e', 'l', 'l', 'o'};
  EXPECT_EQ(hash_fnv1a(hello, 5), 0xa430d84680aabd0bULL);
}

TEST(FNVHashTest, FNV1_DifferentInputs) {
  const uint8_t foo[] = {'f', 'o', 'o'};
  const uint8_t bar[] = {'b', 'a', 'r'};
  EXPECT_NE(hash_fnv1(foo, 3), hash_fnv1(bar, 3));
}

TEST(FNVHashTest, FNV1a_DifferentInputs) {
  const uint8_t foo[] = {'f', 'o', 'o'};
  const uint8_t bar[] = {'b', 'a', 'r'};
  EXPECT_NE(hash_fnv1a(foo, 3), hash_fnv1a(bar, 3));
}

TEST(FNVHashTest, FNV1_Consistency) {
  const uint8_t test[] = {'t', 'e', 's', 't'};
  EXPECT_EQ(hash_fnv1(test, 4), hash_fnv1(test, 4));
}

TEST(FNVHashTest, FNV1a_Consistency) {
  const uint8_t test[] = {'t', 'e', 's', 't'};
  EXPECT_EQ(hash_fnv1a(test, 4), hash_fnv1a(test, 4));
}
