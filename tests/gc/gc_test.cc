#include <gtest/gtest.h>
#include <pocketknife/gc/gc.h>

struct gc_object {
  int value;
  struct gc_object *child;
};

static void gc_object_marker(struct gc *gc, void *ptr) {
  struct gc_object *obj = reinterpret_cast<struct gc_object *>(ptr);
  if (gc_mark(gc, ptr)) {
    return;
  }

  if (obj->child) {
    gc_mark(gc, obj->child);
  }
}

TEST(GCTest, CreateAndDestroy) {
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);
  gc_destroy(gc);
}

TEST(GCTest, AllocAndFree) {
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);

  void *ptr = gc_alloc(gc, 128, NULL, NULL);
  ASSERT_TRUE(ptr != NULL);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);

  ASSERT_EQ(stats.total_objects, 1);

  gc_run(gc, &stats);
  ASSERT_EQ(stats.total_collected, 1);
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}

TEST(GCTest, MarkWithCycles) {
  struct gc *gc = gc_create();
  ASSERT_TRUE(gc != NULL);

  struct gc_object *obj1 = reinterpret_cast<struct gc_object *>(
      gc_alloc(gc, sizeof(struct gc_object), gc_object_marker, NULL));
  ASSERT_TRUE(obj1 != NULL);
  obj1->value = 1;
  obj1->child = reinterpret_cast<struct gc_object *>(
      gc_alloc(gc, sizeof(struct gc_object), gc_object_marker, NULL));
  ASSERT_TRUE(obj1->child != NULL);
  obj1->child->value = 2;
  obj1->child->child = obj1;

  gc_root(gc, obj1);

  struct gc_stats stats;
  gc_get_stats(gc, &stats);

  ASSERT_EQ(stats.total_objects, 2);

  gc_run(gc, &stats);

  ASSERT_EQ(stats.total_collected, 0);  // Should not collect because of the root
  ASSERT_EQ(stats.total_objects, 2);

  gc_unroot(gc, obj1);

  gc_run(gc, &stats);

  ASSERT_EQ(stats.total_collected, 2);  // Should collect both objects now
  ASSERT_EQ(stats.total_objects, 0);

  gc_destroy(gc);
}
