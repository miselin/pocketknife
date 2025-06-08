#include <assert.h>
#include <pocketknife/gc/gc.h>
#include <stdlib.h>

struct gcroot;
struct gcnode;
struct gclist_node;

struct gc {
  struct gcroot *roots;
  struct gclist_node *nodes;

  struct gc_stats stats;

  GCAllocateFunc alloc;
  GCFreeFunc free;
};

struct gcroot {
  struct gcnode *node;
  struct gcroot *next;
};

// Used as a header on the resulting allocation
struct gcnode {
  GCMarkFunc marker;
  GCEraseFunc eraser;
  size_t size;
  size_t marked;  // wastefully large, but conveniently pushes this struct to 32 bytes instead of 24
} __attribute__((packed)) __attribute__((aligned(8)));

struct gclist_node {
  struct gcnode *node;
  struct gclist_node *next;
};

static struct gcnode *gcnode_from_ptr(void *ptr) {
  assert(ptr != NULL);
  return (struct gcnode *)((char *)ptr - sizeof(struct gcnode));
}

static void *gcnode_to_ptr(struct gcnode *node) {
  assert(node != NULL);
  return (void *)((char *)node + sizeof(struct gcnode));
}

struct gc *gc_create(void) {
  struct gc *gc = calloc(1, sizeof(struct gc));
  if (!gc) {
    return NULL;
  }

  gc->alloc = malloc;
  gc->free = free;

  return gc;
}

struct gc *gc_create_with_allocator(GCAllocateFunc alloc, GCFreeFunc free) {
  assert(alloc != NULL);
  assert(free != NULL);

  struct gc *gc = alloc(sizeof(struct gc));
  if (!gc) {
    return NULL;
  }

  gc->alloc = alloc;
  gc->free = free;

  return gc;
}

void gc_destroy(struct gc *gc) {
  assert(gc != NULL);

  // Clean up all roots so we can perform a full collection
  while (gc->roots) {
    struct gcroot *next = gc->roots->next;
    free(gc->roots);
    gc->roots = next;
  }

  gc_run(gc, NULL);
  assert(gc->stats.total_objects == 0);

  GCFreeFunc free_func = gc->free;

  free_func(gc);
}

void *gc_alloc(struct gc *gc, size_t size, GCMarkFunc marker, GCEraseFunc eraser) {
  assert(gc != NULL);

  if (size == 0) {
    return NULL;
  }

  void *ptr = gc->alloc(size + sizeof(struct gcnode));
  if (!ptr) {
    return NULL;
  }

  struct gclist_node *list_node = gc->alloc(sizeof(struct gclist_node));
  if (!list_node) {
    free(ptr);
    return NULL;
  }

  struct gcnode *node = (struct gcnode *)ptr;
  node->marker = marker;
  node->eraser = eraser;
  node->size = size;
  node->marked = 0;

  gc->stats.total_allocated += size;
  gc->stats.total_objects++;

  list_node->node = node;
  list_node->next = gc->nodes;
  gc->nodes = list_node;

  return gcnode_to_ptr(node);
}

void gc_root(struct gc *gc, void *ptr) {
  assert(gc != NULL);
  assert(ptr != NULL);

  struct gcnode *node = gcnode_from_ptr(ptr);

  struct gcroot *root = malloc(sizeof(struct gcroot));
  if (!root) {
    return;
  }

  root->node = node;
  root->next = gc->roots;
  gc->roots = root;
}

void gc_unroot(struct gc *gc, void *ptr) {
  assert(gc != NULL);
  assert(ptr != NULL);

  struct gcnode *node = gcnode_from_ptr(ptr);

  struct gcroot *current = gc->roots;
  struct gcroot *prev = NULL;
  while (current) {
    if (current->node == node) {
      if (prev) {
        prev->next = current->next;
      } else {
        gc->roots = current->next;
      }
      free(current);
      return;
    }

    prev = current;
    current = current->next;
  }
}

int gc_mark(struct gc *gc, void *ptr) {
  assert(gc != NULL);
  assert(ptr != NULL);

  struct gcnode *node = gcnode_from_ptr(ptr);

  if (node->marked) {
    return 0;
  }

  node->marked = 1;
  if (node->marker) {
    node->marker(gc, gcnode_to_ptr(node));
  }

  gc->stats.total_marked++;

  return 1;
}

void gc_run(struct gc *gc, struct gc_stats *stats) {
  assert(gc != NULL);

  gc->stats.total_collected = 0;
  gc->stats.total_freed = 0;
  gc->stats.total_marked = 0;

  // Phase 1: Mark
  struct gcroot *current = gc->roots;
  while (current) {
    struct gcnode *node = current->node;
    if (!node->marked) {
      node->marked = 1;
      gc->stats.total_marked++;

      if (node->marker) {
        node->marker(gc, gcnode_to_ptr(node));
      }
    }

    current = current->next;
  }

  // Phase 2: Sweep
  struct gclist_node *nodelist = gc->nodes;
  struct gclist_node *prev = NULL;
  while (nodelist) {
    struct gclist_node *next = nodelist->next;

    struct gcnode *node = nodelist->node;
    if (!node->marked) {
      if (node->eraser) {
        node->eraser(gc, gcnode_to_ptr(node));
      }
      gc->free(node);

      gc->stats.total_objects--;
      gc->stats.total_collected++;
      gc->stats.total_freed += node->size;
      gc->stats.total_allocated -= node->size;

      if (prev) {
        prev->next = next;
      } else {
        gc->nodes = next;
      }

      gc->free(nodelist);
    } else {
      node->marked = 0;
    }

    nodelist = next;
  }

  if (stats) {
    *stats = gc->stats;
  }
}

void gc_get_stats(struct gc *gc, struct gc_stats *stats) {
  assert(gc != NULL);
  assert(stats != NULL);

  *stats = gc->stats;
}
