#include <gtest/gtest.h>
#include <pocketknife/gc/gc.h>

struct gc_object {
  int value;
  struct gc_object *child;
};

struct gc_erasable_object {
  int *value;
};

static void gc_object_marker(struct gc *gc, void *ptr) {
  struct gc_object *obj = (struct gc_object *)ptr;

  if (obj->child) {
    gc_mark(gc, obj->child);
  }
}

static void gc_erasable_object_eraser(struct gc *gc, void *ptr) {
  (void)gc;

  struct gc_erasable_object *obj = (struct gc_erasable_object *)ptr;
  free(obj->value);
}

TEST(GCTest, CreateAndDestroy) {
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);
  gc_destroy(gc);
}

TEST(GCTest, DestroyWithRoots) {
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);

  struct gc_object *obj = (struct gc_object *)gc_alloc(gc, sizeof(struct gc_object), NULL, NULL);
  ASSERT_TRUE(obj != NULL);
  gc_root(gc, obj);

  gc_destroy(gc);

  // expecting no ASAN crashes
}

TEST(GCTest, AllocAndFree) {
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);

  void *ptr = gc_alloc(gc, sizeof(struct gc_object), NULL, NULL);
  ASSERT_TRUE(ptr != NULL);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);

  ASSERT_EQ(stats.total_objects, 1);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 1);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}

TEST(GCTest, AllocAndFreeWithEraser) {
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);

  struct gc_erasable_object *ptr = (struct gc_erasable_object *)gc_alloc(
      gc, sizeof(struct gc_erasable_object), NULL, gc_erasable_object_eraser);
  ASSERT_TRUE(ptr != NULL);
  ptr->value = (int *)malloc(sizeof(int));
  *ptr->value = 42;

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
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);

  // First node - root object
  struct gc_object *obj1 =
      (struct gc_object *)gc_alloc(gc, sizeof(struct gc_object), gc_object_marker, NULL);
  ASSERT_TRUE(obj1 != NULL);
  obj1->value = 1;

  // Second node - creates a cycle
  obj1->child = (struct gc_object *)gc_alloc(gc, sizeof(struct gc_object), gc_object_marker, NULL);
  ASSERT_TRUE(obj1->child != NULL);
  obj1->child->value = 2;
  obj1->child->child = obj1;

  // Pins the whole object structure
  gc_root(gc, obj1);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);

  ASSERT_EQ(stats.total_objects, 2);

  gc_run(gc, &stats);

  ASSERT_EQ(stats.total_collected, 0);
  ASSERT_EQ(stats.total_objects, 2);

  gc_unroot(gc, obj1);

  gc_run(gc, &stats);

  ASSERT_EQ(stats.total_collected, 2);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}
