#include <pocketknife/allocator/allocator.h>

#include <assert.h>
#include <stdint.h>

#include <sys/mman.h>

struct arena;
struct block;

struct arena {
  struct allocator *parent;

  size_t block_size;
  size_t blocks_per_page;
  struct block *free_list;
};

struct allocator {
  void *region;
  size_t region_size;

  uint64_t *bitmap;

  // selected based on lg2 of size (<= 2K)
  struct arena arenas[12];
};

struct block {
  void *at;
  struct block *next;
};

static void *arena_alloc(struct arena *arena);
static void arena_free(struct arena *arena, void *ptr);
static void arena_compact(struct arena *arena, AllocatorMoveCallback callback);

static size_t bitwise_log2(size_t sz) {
  size_t log2 = 0;
  while (sz >>= 1) {
    log2++;
  }
  return log2;
}

static int is_within_allocator_region(struct allocator *allocator, void *ptr) {
  return (ptr >= allocator->region &&
          (char *)ptr < (char *)allocator->region + allocator->region_size);
}

static void *get_free_allocator_page(struct allocator *allocator) {
  size_t index = ~0ULL;
  size_t bit = 1;
  for (size_t i = 0; i < allocator->region_size / 4096; ++i) {
    if (!(allocator->bitmap[i / 64] & bit)) {
      index = i;
      break;
    }
    bit <<= 1;
    if (bit == 0) {
      bit = 1;
    }
  }

  if (index == ~0ULL) {
    // no free area found!
    return NULL;
  }

  void *page = (char *)allocator->region + (index * 4096);
  allocator->bitmap[index / 64] |= (1ULL << (index % 64));

  return page;
}

void free_allocator_page(struct allocator *allocator, void *page) {
  ptrdiff_t index = ((char *)page - (char *)allocator->region) / 4096;

  allocator->bitmap[index / 64] &= ~(1ULL << (index % 64));
  madvise(page, 0, MADV_DONTNEED);
}

void *allocator_alloc(struct allocator *allocator, size_t size) {
  (void)allocator;

  if (size >= 4096) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  } else if (size == 0) {
    return NULL;
  }

  return arena_alloc(&allocator->arenas[bitwise_log2(size)]);
}

void allocator_free(struct allocator *allocator, void *ptr) {
  if (!is_within_allocator_region(allocator, ptr)) {
    // Large allocation - just use munmap
    munmap(ptr, allocator->region_size);
  } else {
    // TODO: need either a header on allocations or a mapping of page -> arena...
    arena_free(&allocator->arenas[0], ptr);
  }
}

int allocator_should_compact(struct allocator *allocator) {
  (void)allocator;

  // TODO: check for fragmentation, large free lists in arenas, etc
  return 1;
}

void allocator_compact(struct allocator *allocator, AllocatorMoveCallback callback) {
  for (int i = 0; i < 12; ++i) {
    arena_compact(&allocator->arenas[i], callback);
  }
}

static void *arena_alloc(struct arena *arena) {
  if (!arena->free_list) {
    void *new_block = get_free_allocator_page(arena->parent);
    if (!new_block) {
      return NULL;
    }

    // TODO
    return NULL;
  } else {
    struct block *block = arena->free_list;
    arena->free_list = block->next;
    return block->at;
  }
}

static void arena_free(struct arena *arena, void *ptr) {
  (void)arena;
  (void)ptr;
}

void arena_compact(struct arena *arena, AllocatorMoveCallback callback) {
  (void)arena;
  (void)callback;
  //
}
