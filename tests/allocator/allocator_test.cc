#include <pocketknife/allocator/allocator.h>

#include <gtest/gtest.h>

TEST(AllocatorTest, CreateAndDestroy) {
  struct allocator *alloc = allocator_new(4096 * 4);
  ASSERT_NE(alloc, nullptr);

  allocator_destroy(alloc);
  // No assertion here, just checking if it doesn't crash.
}

TEST(AllocatorTest, CreateTooSmall) {
  struct allocator *alloc = allocator_new(4096);
  ASSERT_EQ(alloc, nullptr);
}

TEST(AllocatorTest, CreateInvalidSize) {
  struct allocator *alloc = allocator_new(0);
  ASSERT_EQ(alloc, nullptr);
}