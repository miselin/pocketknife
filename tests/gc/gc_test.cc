#include <pocketknife/gc/gc.h>

#include <gtest/gtest.h>

struct gc_object {
  int value;
  struct gc_slot *child;
};

struct gc_erasable_object {
  int *value;
};

static struct gc_config gc_config = {
    .debug = 1,
    .alloc = malloc,
    .free = free,
    .young_max_cycles = 2,
    .large_threshold = 1024,  // 1K
};

static struct gc *gc_create_for_test() {
  return gc_create_with_config(&gc_config);
}

static void gc_object_marker(struct gc *gc, struct gc_slot *slot) {
  void *locked = gc_lock_slot(slot);
  struct gc_object *obj = (struct gc_object *)locked;

  if (obj->child) {
    gc_mark(gc, obj->child);
  }

  gc_unlock_slot(slot);
}

static void gc_erasable_object_eraser(struct gc *gc, struct gc_slot *slot) {
  (void)gc;

  void *locked = gc_lock_slot(slot);

  struct gc_erasable_object *obj = (struct gc_erasable_object *)locked;
  free(obj->value);

  gc_unlock_slot(slot);
}

TEST(GCTest, CreateAndDestroy) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);
  gc_destroy(gc);
}

TEST(GCTest, DestroyWithRoots) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);

  struct gc_slot *slot = gc_alloc(gc, sizeof(struct gc_object), NULL, NULL);
  ASSERT_TRUE(slot != NULL);
  gc_root(gc, slot);

  gc_destroy(gc);

  // expecting no ASAN crashes
}

TEST(GCTest, AllocAndFree) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);

  struct gc_slot *slot = gc_alloc(gc, sizeof(struct gc_object), NULL, NULL);
  ASSERT_TRUE(slot != NULL);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);

  ASSERT_EQ(stats.total_objects, 1);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 1);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}

TEST(GCTest, AllocAndFreeWithEraser) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);

  struct gc_slot *slot =
      gc_alloc(gc, sizeof(struct gc_erasable_object), NULL, gc_erasable_object_eraser);
  struct gc_erasable_object *ptr = (struct gc_erasable_object *)gc_lock_slot(slot);
  ASSERT_TRUE(ptr != NULL);
  ptr->value = (int *)malloc(sizeof(int));
  *ptr->value = 42;
  gc_unlock_slot(slot);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);

  ASSERT_EQ(stats.total_objects, 1);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 1);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);

  // expecting no ASAN crashes or errors from failing to release the object
}

TEST(GCTest, MarkWithCycles) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);

  // First node - root object
  struct gc_slot *slot1 = gc_alloc(gc, sizeof(struct gc_object), gc_object_marker, NULL);
  struct gc_object *obj1 = (struct gc_object *)gc_lock_slot(slot1);
  ASSERT_TRUE(obj1 != NULL);
  obj1->value = 1;

  // Second node - creates a cycle
  obj1->child = gc_alloc(gc, sizeof(struct gc_object), gc_object_marker, NULL);
  ASSERT_TRUE(obj1->child != NULL);
  struct gc_object *obj2 = (struct gc_object *)gc_lock_slot(obj1->child);
  obj2->value = 2;
  obj2->child = slot1;

  gc_unlock_slot(obj1->child);
  gc_unlock_slot(slot1);

  // Pins the whole object structure
  gc_root(gc, slot1);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);

  ASSERT_EQ(stats.total_objects, 2);

  gc_run(gc, &stats);

  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(stats.total_objects, 2);

  gc_unroot(gc, slot1);

  gc_run(gc, &stats);

  ASSERT_EQ(stats.total_collected, 2);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}

TEST(GCTest, YoungSpacePromotion) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);

  struct gc_space_config old_space_config = {
      .sweep_every = 1,         // sweep old space more aggressively for testing
      .max_size = 1024 * 1024,  // 1MB
  };
  gc_configure_space(gc, GC_SPACE_OLD, &old_space_config);

  struct gc_slot *slot = gc_alloc(gc, sizeof(struct gc_object), NULL, NULL);
  ASSERT_TRUE(slot != NULL);

  struct gc_stats stats;

  gc_mark(gc, slot);
  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_objects, 1);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(gc_get_space(slot), GC_SPACE_YOUNG);

  gc_mark(gc, slot);
  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_objects, 1);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(gc_get_space(slot), GC_SPACE_YOUNG);

  gc_mark(gc, slot);
  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_objects, 1);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(gc_get_space(slot), GC_SPACE_OLD);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 1);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}

TEST(GCTest, YoungSpacePromotionWithRoot) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);

  struct gc_space_config old_space_config = {
      .sweep_every = 1,         // sweep old space more aggressively for testing
      .max_size = 1024 * 1024,  // 1MB
  };
  gc_configure_space(gc, GC_SPACE_OLD, &old_space_config);

  struct gc_slot *slot = gc_alloc(gc, sizeof(struct gc_object), NULL, NULL);
  ASSERT_TRUE(slot != NULL);

  gc_root(gc, slot);

  struct gc_stats stats;

  gc_mark(gc, slot);
  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_objects, 1);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(gc_get_space(slot), GC_SPACE_YOUNG);

  gc_mark(gc, slot);
  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_objects, 1);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(gc_get_space(slot), GC_SPACE_YOUNG);

  gc_mark(gc, slot);
  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_objects, 1);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(gc_get_space(slot), GC_SPACE_OLD);

  gc_unroot(gc, slot);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 1);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}

TEST(GCTest, CannotCollectLocked) {
  struct gc *gc = gc_create_for_test();
  ASSERT_TRUE(gc != NULL);

  struct gc_slot *slot = gc_alloc(gc, sizeof(struct gc_object), NULL, NULL);
  ASSERT_TRUE(slot != NULL);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);
  ASSERT_EQ(stats.total_objects, 1);

  gc_lock_slot(slot);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(stats.total_objects, 1);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(stats.total_objects, 1);

  gc_unlock_slot(slot);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 1);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}
