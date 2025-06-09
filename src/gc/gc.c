#include <pocketknife/gc/gc.h>

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct gcroot;
struct gcnode;
struct gc_slot;
struct gc_space;

struct gc_space {
  struct gc_stats stats;

  size_t cycles_since_sweep;

  struct gc_space_config config;

  struct gcroot *roots;
  struct gc_slot *nodes;
};

struct gc {
  struct gc_config config;

  struct gc_space spaces[8];
};

struct gcroot {
  struct gc_slot *slot;
  struct gcroot *next;
};

// Used as a header on the resulting allocation
// Currently a 32-byte header - if adding fields, try to ensure at least 16-byte alignment
struct gcnode {
  GCMarkFunc marker;
  GCEraseFunc eraser;

  size_t size;

  union {
    struct {
      // Mark bit for mark-sweep.
      uint64_t marked : 1;
      // If set, the object is locked and cannot be moved.
      uint64_t locked : 1;
      // GCSpace the object is currently in
      uint64_t space : 3;
      // # of cycles the object has lived in its current space (will not wrap around to zero)
      uint64_t survived : 8;
    } flags;
    uint64_t value;
  };
} __attribute__((packed)) __attribute__((aligned(8)));

struct gc_slot {
  struct gcnode *node;
  struct gc_slot *next;
};

static void gc_debugf(struct gc *gc, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void gc_debugf(struct gc *gc, const char *fmt, ...) {
  if (!gc->config.debug) {
    return;
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

static void sanity_check_space_config(struct gc_space_config *config) {
  assert(config != NULL);
  assert(config->sweep_every > 0);
  assert(config->max_size > 0);

  if (config->alloc == NULL) {
    assert(config->free == NULL);  // If alloc is default, free must also be default
  } else {
    assert(config->free != NULL);  // If alloc is custom, free must also be custom
  }
}

static void gc_default_config(struct gc *gc) {
  gc->config.alloc = malloc;
  gc->config.free = free;
  gc->config.young_max_cycles = 10;
  gc->config.large_threshold = 1024 * 1024;  // 1 MB

  gc->spaces[GC_SPACE_YOUNG].config.sweep_every = 1;
  gc->spaces[GC_SPACE_YOUNG].config.max_size = 256 * 1024 * 1024;  // 256 MiB young space
  gc->spaces[GC_SPACE_OLD].config.sweep_every = 10;
  gc->spaces[GC_SPACE_OLD].config.max_size = 1024 * 1024 * 1024;  // 1 GiB old space
  gc->spaces[GC_SPACE_LARGE].config.sweep_every = 100;
  gc->spaces[GC_SPACE_LARGE].config.max_size = 1024 * 1024 * 1024;  // 1 GiB large space

  for (int i = GC_SPACE_CUSTOM1; i < 8; ++i) {
    gc->spaces[i].config.alloc = gc->config.alloc;
    gc->spaces[i].config.free = gc->config.free;

    if (i >= GC_SPACE_CUSTOM1) {
      gc->spaces[i].config.sweep_every = 1;
      gc->spaces[i].config.max_size = 256 * 1024 * 1024;  // 256 MiB custom space
    }

    sanity_check_space_config(&gc->spaces[i].config);
  }
}

struct gc *gc_create(void) {
  struct gc *gc = calloc(1, sizeof(struct gc));
  if (!gc) {
    return NULL;
  }

  gc_default_config(gc);

  return gc;
}

struct gc *gc_create_with_allocator(GCAllocateFunc alloc, GCFreeFunc free) {
  assert(alloc != NULL);
  assert(free != NULL);

  struct gc *gc = alloc(sizeof(struct gc));
  if (!gc) {
    return NULL;
  }

  gc_default_config(gc);

  gc->config.alloc = alloc;
  gc->config.free = free;

  for (int i = 0; i < 8; ++i) {
    gc->spaces[i].config.alloc = alloc;
    gc->spaces[i].config.free = free;

    sanity_check_space_config(&gc->spaces[i].config);
  }

  return gc;
}

struct gc *gc_create_with_config(struct gc_config *config) {
  struct gc *gc = gc_create();
  if (!gc) {
    return NULL;
  }

  if (config) {
    gc->config = *config;
  }

  for (int i = 0; i < 8; ++i) {
    gc->spaces[i].config.alloc = gc->config.alloc;
    gc->spaces[i].config.free = gc->config.free;

    sanity_check_space_config(&gc->spaces[i].config);
  }

  return gc;
}

void gc_configure_space(struct gc *gc, enum GCSpace space, struct gc_space_config *config) {
  assert(gc != NULL);
  struct gc_space *gc_space = &gc->spaces[space];
  assert(gc_space != NULL);
  assert(config != NULL);

  gc_space->config = *config;

  if (gc_space->config.alloc == NULL) {
    gc_space->config.alloc = gc->config.alloc;
    gc_space->config.free = gc->config.free;
  }

  sanity_check_space_config(&gc_space->config);
}

static void gc_space_destroy(struct gc *gc, struct gc_space *space) {
  assert(space != NULL);

  GCFreeFunc free_func = gc->config.free;

  while (space->roots) {
    struct gcroot *next = space->roots->next;
    free_func(space->roots);
    space->roots = next;
  }

  // Force an immediate sweep of the space in the next cycle
  space->config.sweep_every = 0;
}

void gc_destroy(struct gc *gc) {
  assert(gc != NULL);

  // Clean up all spaces of their roots so we can perform a full collection
  for (int i = 0; i < 8; ++i) {
    gc_space_destroy(gc, &gc->spaces[i]);
  }

  gc_run(gc, NULL);

  GCFreeFunc free_func = gc->config.free;

  free_func(gc);
}

struct gc_slot *gc_alloc(struct gc *gc, size_t size, GCMarkFunc marker, GCEraseFunc eraser) {
  enum GCSpace space = GC_SPACE_YOUNG;
  if (size >= gc->config.large_threshold) {
    space = GC_SPACE_LARGE;
  }

  return gc_alloc_with_space(gc, size, marker, eraser, space);
}

struct gc_slot *gc_alloc_with_space(struct gc *gc, size_t size, GCMarkFunc marker,
                                    GCEraseFunc eraser, enum GCSpace space) {
  assert(gc != NULL);

  if (size == 0) {
    return NULL;
  }

  struct gc_space *gc_space = &gc->spaces[space];

  void *ptr = gc_space->config.alloc(size + sizeof(struct gcnode));
  if (!ptr) {
    return NULL;
  }

  struct gc_slot *list_node = gc_space->config.alloc(sizeof(struct gc_slot));
  if (!list_node) {
    gc_space->config.free(ptr);
    return NULL;
  }

  struct gcnode *node = (struct gcnode *)ptr;
  node->marker = marker;
  node->eraser = eraser;
  node->size = size;
  node->value = 0;
  node->flags.space = space;

  gc_space->stats.total_allocated += size;
  gc_space->stats.total_objects++;

  list_node->node = node;
  list_node->next = gc_space->nodes;
  gc_space->nodes = list_node;

  return list_node;
}

void *gc_lock_slot(struct gc_slot *slot) {
  assert(slot != NULL);

  struct gcnode *node = slot->node;

  if (node->flags.locked) {
    return NULL;
  }

  node->flags.locked = 1;
  return (void *)(node + 1);
}

void gc_unlock_slot(struct gc_slot *slot) {
  assert(slot != NULL);

  struct gcnode *node = slot->node;

  node->flags.locked = 0;
}

void gc_root(struct gc *gc, struct gc_slot *slot) {
  assert(gc != NULL);
  assert(slot != NULL);

  struct gc_space *space = &gc->spaces[slot->node->flags.space];

  struct gcroot *root = space->config.alloc(sizeof(struct gcroot));
  if (!root) {
    return;
  }

  root->slot = slot;
  root->next = space->roots;
  space->roots = root;
}

int gc_unroot(struct gc *gc, struct gc_slot *slot) {
  assert(gc != NULL);
  assert(slot != NULL);

  struct gc_space *space = &gc->spaces[slot->node->flags.space];

  struct gcroot *current = space->roots;
  struct gcroot *prev = NULL;
  while (current) {
    if (current->slot == slot) {
      if (prev) {
        prev->next = current->next;
      } else {
        space->roots = current->next;
      }
      space->config.free(current);
      return 1;
    }

    prev = current;
    current = current->next;
  }

  return 0;
}

int gc_mark(struct gc *gc, struct gc_slot *slot) {
  assert(gc != NULL);
  assert(slot != NULL);

  enum GCSpace space = slot->node->flags.space;

  int marked = slot->node->flags.marked;
  slot->node->flags.marked = 1;

  if (marked) {
    return 0;
  }

  if (slot->node->marker) {
    slot->node->marker(gc, slot);
  }

  gc->spaces[space].stats.total_marked++;

  return 1;
}

static void gc_space_run(struct gc *gc, struct gc_space *space) {
  assert(gc != NULL);
  assert(space != NULL);

  if ((++space->cycles_since_sweep) < space->config.sweep_every) {
    gc_debugf(gc, "gc: skipping sweep, cycles since last sweep: %zu, needed at least %zd\n",
              space->cycles_since_sweep, space->config.sweep_every);
    return;
  }

  GCFreeFunc free_func = space->config.free;

  space->stats.total_collected = 0;
  space->stats.total_freed = 0;
  space->stats.total_marked = 0;

  // Phase 1: Mark
  struct gcroot *current = space->roots;
  while (current) {
    struct gc_slot *slot = current->slot;
    struct gcnode *node = slot->node;
    if (!node->flags.marked) {
      gc_debugf(gc, "gc: marking root %p (%zd bytes)\n", (void *)node, node->size);

      node->flags.marked = 1;
      space->stats.total_marked++;

      if (node->marker) {
        node->marker(gc, slot);
      }
    }

    current = current->next;
  }

  // Phase 2: Sweep
  struct gc_slot *nodelist = space->nodes;
  struct gc_slot *prev = NULL;
  while (nodelist) {
    struct gc_slot *next = nodelist->next;

    struct gcnode *node = nodelist->node;
    if (node->flags.locked) {
      // We can't do anything with this node.
      // We won't increment the survived count, as it technically hasn't survived a cycle, it's
      // just currently locked.
      gc_debugf(gc, "gc: skipping locked object %p (%zd bytes)\n", (void *)node, node->size);
    } else if (node->flags.marked) {
      node->flags.marked = 0;
      if (node->flags.survived < 255) {
        node->flags.survived++;
      }
      gc_debugf(gc, "gc: skipping marked object %p (%zd bytes, survived %d cycles)\n", (void *)node,
                node->size, node->flags.survived);
    } else {
      gc_debugf(gc, "gc: collecting unmarked object %p (%zd bytes)\n", (void *)node, node->size);

      if (node->eraser) {
        node->eraser(gc, nodelist);
      }

      space->stats.total_objects--;
      space->stats.total_collected++;
      space->stats.total_freed += node->size;
      space->stats.total_allocated -= node->size;

      free_func(node);

      if (prev) {
        prev->next = next;
      } else {
        space->nodes = next;
      }

      free_func(nodelist);
    }

    nodelist = next;
  }

  space->cycles_since_sweep = 0;
}

void gc_run(struct gc *gc, struct gc_stats *stats) {
  assert(gc != NULL);

  for (int i = 0; i < 8; ++i) {
    gc_debugf(gc, "gc: running space %d\n", i);
    gc_space_run(gc, &gc->spaces[i]);
  }

  // Can we promote any objects from young to old space?
  struct gc_space *young_space = &gc->spaces[GC_SPACE_YOUNG];
  struct gc_space *old_space = &gc->spaces[GC_SPACE_OLD];

  struct gc_slot *nodelist = young_space->nodes;
  struct gc_slot *prev = NULL;
  while (nodelist) {
    struct gc_slot *next = nodelist->next;

    struct gcnode *node = nodelist->node;
    if (node->flags.survived > gc->config.young_max_cycles) {
      gc_debugf(gc,
                "gc: promoting slot %p (object %p of %zd bytes) from young space to old space\n",
                (void *)nodelist, (void *)node, node->size);

      int needs_root = gc_unroot(gc, nodelist);

      // TODO: actually move the node object + its data to the old space

      if (prev) {
        prev->next = next;
      } else {
        young_space->nodes = next;
      }

      nodelist->next = old_space->nodes;
      old_space->nodes = nodelist;

      old_space->stats.total_objects++;
      old_space->stats.total_allocated += node->size;
      young_space->stats.total_objects--;
      young_space->stats.total_allocated -= node->size;
      young_space->stats.total_freed += node->size;

      node->flags.survived = 0;
      node->flags.space = GC_SPACE_OLD;

      if (needs_root) {
        gc_debugf(gc, "gc: re-rooting object %p in old space\n", (void *)node);
        gc_root(gc, nodelist);
      }
    }

    nodelist = next;
  }

  if (stats) {
    gc_get_stats(gc, stats);
  }
}

void gc_get_stats(struct gc *gc, struct gc_stats *stats) {
  assert(gc != NULL);
  assert(stats != NULL);

  memset(stats, 0, sizeof(struct gc_stats));
  for (int i = 0; i < 8; ++i) {
    stats->total_allocated += gc->spaces[i].stats.total_allocated;
    stats->total_freed += gc->spaces[i].stats.total_freed;
    stats->total_collected += gc->spaces[i].stats.total_collected;
    stats->total_marked += gc->spaces[i].stats.total_marked;
    stats->total_objects += gc->spaces[i].stats.total_objects;
  }
}

enum GCSpace gc_get_space(struct gc_slot *slot) {
  assert(slot != NULL);
  return slot->node->flags.space;
}
