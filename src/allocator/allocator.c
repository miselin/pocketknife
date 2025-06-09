#include <pocketknife/allocator/allocator.h>

#include <assert.h>
#include <stdint.h>

#include <sys/mman.h>

struct block {
  void *at;
  struct block *next;
};

struct arena_page {
  uint64_t bitmap;
  size_t used_count;
  size_t free_count;
  struct arena_page *next;
};

struct arena {
  struct allocator *parent;

  size_t block_size;
  size_t blocks_per_page;

  struct block *free_list;

  struct arena_page *pages;

  // Note: if free_count * block_size >= 4096, we know a compaction is possible.
  size_t free_count;
  size_t used_count;
};

struct allocator {
  // The region of memory assigned to this allocator. It's broken up into 4K pages for use in
  // different allocator routines. Generally, every page is readable and writable, and the allocator
  // uses madvise() or other signals to indicate that memory can be released to the system.
  void *region;

  // The size of the region in bytes. This will be a multiple of 4K.
  size_t region_size;

  // 0-11 = arena index
  // 253 = large allocation, fully allocated page (but how do we know how big so we can unmap??)
  // 254 = internal "struct block" arena
  // 255 = free
  // TODO: compact this to 4 bits and halve the storage needed (albeit with some extra bit math)
  uint8_t *page_owners;

  // Sub-arenas, selected based on lg2 of size (<= 2K)
  // Arenas will pull individual pages from the allocator's region and manage their own free/used
  // lists.
  struct arena arenas[12];

  // This is a set of "struct block" objects available for arenas to use in their free/used lists.
  struct block *free_blocks;

  uint64_t *bitmap;
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

static struct block *allocator_alloc_block(struct allocator *allocator) {
  if (allocator->free_blocks) {
    struct block *block = allocator->free_blocks;
    allocator->free_blocks = block->next;
    return block;
  }

  void *page = get_free_allocator_page(allocator);
  if (!page) {
    return NULL;
  }

  struct block *block = (struct block *)page;
  block->at = page;
  block->next = NULL;
  size_t block_size = 4096 / sizeof(struct block);

  // add the rest of the blocks
  for (size_t i = 1; i < block_size; ++i) {
    struct block *next_block = (struct block *)((char *)page + (i * sizeof(struct block)));
    next_block->at = (char *)page + (i * sizeof(struct block));
    next_block->next = allocator->free_blocks;
    allocator->free_blocks = next_block;
  }

  return block;
}

static void allocator_free_block(struct allocator *allocator, struct block *block) {
  block->next = allocator->free_blocks;
  allocator->free_blocks = block;
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
  // try and see if we can free full pages from our struct block free list...

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
