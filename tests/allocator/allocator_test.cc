#include <pocketknife/allocator/allocator.h>

#include <gtest/gtest.h>
#include <sys/mman.h>

#define TEST_REGION_SIZE (4096 * 128)

static void *region_for_test(void) {
  return mmap(NULL, TEST_REGION_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static void free_region_for_test(void *region) {
  if (region) {
    munmap(region, TEST_REGION_SIZE);
  }
}

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

TEST(AllocatorTest, CreateWithRegion) {
  void *region = region_for_test();
  struct allocator *alloc = allocator_new_with_region(region, TEST_REGION_SIZE);
  ASSERT_NE(alloc, nullptr);
  allocator_destroy(alloc);
  free_region_for_test(region);
}

TEST(AllocatorTest, AllocateAndFree) {
  void *region = region_for_test();
  struct allocator *alloc = allocator_new_with_region(region, TEST_REGION_SIZE);
  ASSERT_NE(alloc, nullptr);

  void *ptr = allocator_alloc(alloc, 16);
  ASSERT_NE(ptr, nullptr);
  allocator_free(alloc, ptr);

  void *ptr2 = allocator_alloc(alloc, 16);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_EQ(ptr, ptr2);
  allocator_free(alloc, ptr2);

  allocator_destroy(alloc);
  free_region_for_test(region);
}

TEST(AllocatorTest, AllocateDifferentArenas) {
  void *region = region_for_test();
  struct allocator *alloc = allocator_new_with_region(region, TEST_REGION_SIZE);
  ASSERT_NE(alloc, nullptr);

  void *ptr = allocator_alloc(alloc, 16);
  ASSERT_NE(ptr, nullptr);

  void *ptr2 = allocator_alloc(alloc, 32);
  ASSERT_NE(ptr2, nullptr);

  void *ptr3 = allocator_alloc(alloc, 64);
  ASSERT_NE(ptr3, nullptr);

  allocator_free(alloc, ptr);
  allocator_free(alloc, ptr2);
  allocator_free(alloc, ptr3);

  allocator_destroy(alloc);
  free_region_for_test(region);
}

TEST(AllocatorTest, AllocateLargeAllocation) {
  void *region = region_for_test();
  struct allocator *alloc = allocator_new_with_region(region, TEST_REGION_SIZE);
  ASSERT_NE(alloc, nullptr);

  void *ptr = allocator_alloc(alloc, 4096);
  ASSERT_NE(ptr, nullptr);

  void *ptr2 = allocator_alloc(alloc, 2049);
  ASSERT_NE(ptr2, nullptr);

  allocator_free(alloc, ptr);
  allocator_free(alloc, ptr2);

  allocator_destroy(alloc);
  free_region_for_test(region);
}
