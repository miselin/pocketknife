#include <pocketknife/allocator/allocator.h>

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "internal.h"
#include <sys/mman.h>

static int check_allocator_size(size_t size) {
  size_t minimum_required = sizeof(struct allocator) + (sizeof(uint8_t) * (size / 4096));
  if (size < minimum_required) {
    return 0;
  } else if (size % 4096 != 0) {
    return 0;
  }

  return 1;
}

struct allocator *allocator_new(size_t size) {
  if (!check_allocator_size(size)) {
    return NULL;
  }

  void *region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (region == MAP_FAILED) {
    return NULL;
  }

  struct allocator *allocator = allocator_new_with_region(region, size);
  if (!allocator) {
    munmap(region, size);
    return NULL;
  }

  allocator->is_fully_owned = 1;
  return allocator;
}

struct allocator *allocator_new_with_region(void *base, size_t size) {
  assert(base != NULL);

  if (!check_allocator_size(size)) {
    return NULL;
  } else if ((uintptr_t)base % 4096 != 0) {
    return NULL;
  } else if (!base || size == 0) {
    return NULL;
  }

  struct allocator *allocator = (struct allocator *)base;
  memset(allocator, 0, sizeof(struct allocator));
  allocator->region = base;
  allocator->region_size = size;
  allocator->is_fully_owned = 0;
  allocator->free_blocks = NULL;
  allocator->page_owners = (uint8_t *)((char *)base + sizeof(struct allocator));

  size_t struct_sizes = sizeof(struct allocator) + (sizeof(uint8_t) * (size / 4096));

  // Pin the internal pages for the allocator and owners field
  for (size_t i = 0; i < (struct_sizes + 4095) / 4096; ++i) {
    allocator->page_owners[i] = PAGE_OWNER_INTERNAL;
  }

  // Configure arenas
  for (size_t i = 0; i < 8; ++i) {
    allocator->arenas[i].which = i;
    allocator->arenas[i].parent = allocator;
    allocator->arenas[i].block_size = 1 << (i + 4);  // 16, 32, 64, ..., 2048
    allocator->arenas[i].blocks_per_page = 4096 / allocator->arenas[i].block_size;
    // TODO: consider pre-allocating some pages for each arena
    allocator->arenas[i].free_list = NULL;
    allocator->arenas[i].pages = NULL;
    allocator->arenas[i].free_count = 0;
    allocator->arenas[i].used_count = 0;
  }

  // Set up the page structs
  size_t num_pages = size / 4096;
  void *struct_page_base = get_free_allocator_page(allocator, PAGE_OWNER_INTERNAL);
  if (!struct_page_base) {
    return NULL;
  }
  for (size_t i = 0; i < num_pages; ++i) {
    struct arena_page *page =
        (struct arena_page *)((char *)struct_page_base + ((i % 4096) * sizeof(struct arena_page)));
    memset(page, 0, sizeof(struct arena_page));

    page->next = allocator->free_arena_pages;
    allocator->free_arena_pages = page;

    if (i % 4096 == 0) {
      struct_page_base = get_free_allocator_page(allocator, PAGE_OWNER_INTERNAL);
      if (!struct_page_base) {
        return NULL;
      }
    }
  }

  return allocator;
}

void allocator_destroy(struct allocator *allocator) {
  if (!allocator->is_fully_owned) {
    // no action needed
    return;
  }

  munmap(allocator->region, allocator->region_size);
}

int is_within_allocator_region(struct allocator *allocator, void *ptr) {
  return (ptr >= allocator->region &&
          (char *)ptr < (char *)allocator->region + allocator->region_size);
}

void *get_free_allocator_page(struct allocator *allocator, int owner) {
  for (size_t i = 0; i < allocator->region_size / 4096; ++i) {
    if (allocator->page_owners[i] == PAGE_OWNER_FREE) {
      allocator->page_owners[i] = owner;
      void *page = (char *)allocator->region + (i * 4096);
      return page;
    }
  }

  return NULL;
}

struct block *allocator_alloc_block(struct allocator *allocator) {
  if (allocator->free_blocks) {
    struct block *block = allocator->free_blocks;
    allocator->free_blocks = block->next;
    return block;
  }

  void *page = get_free_allocator_page(allocator, PAGE_OWNER_INTERNAL_BLOCK);
  if (!page) {
    return NULL;
  }

  struct block *block = (struct block *)page;
  block->at = page;
  block->next = NULL;
  size_t block_size = 4096 / sizeof(struct block);

  // add the rest of the blocks to our free list now
  for (size_t i = 1; i < block_size; ++i) {
    struct block *next_block = (struct block *)((char *)page + (i * sizeof(struct block)));
    next_block->at = (char *)page + (i * sizeof(struct block));
    next_block->next = allocator->free_blocks;
    allocator->free_blocks = next_block;
  }

  return block;
}

void allocator_free_block(struct allocator *allocator, struct block *block) {
  block->next = allocator->free_blocks;
  allocator->free_blocks = block;
}

struct arena_page *allocator_alloc_arena_page_meta(struct allocator *allocator) {
  struct arena_page *page = allocator->free_arena_pages;
  allocator->free_arena_pages = page->next;
  page->next = NULL;
  return page;
}

void allocator_free_arena_page_meta(struct allocator *allocator, struct arena_page *page) {
  page->next = allocator->free_arena_pages;
  allocator->free_arena_pages = page;
}

void free_allocator_page(struct allocator *allocator, void *page) {
  ptrdiff_t index = ((char *)page - (char *)allocator->region) / 4096;

  allocator->page_owners[index] = 255;
  madvise(page, 0, MADV_DONTNEED);
}

void *allocator_alloc(struct allocator *allocator, size_t size) {
  (void)allocator;

  // We don't use arenas for anything >2K - doesn't make sense to have the arena overhead
  if (size > 2048) {
    return get_free_allocator_page(allocator, PAGE_OWNER_LARGE_ALLOCATION);
  } else if (size == 0) {
    return NULL;
  }

  if (size < 16) {
    size = 16;  // minimum allocation size
  }

  return arena_alloc(&allocator->arenas[bitwise_log2(size) - 4]);
}

void allocator_free(struct allocator *allocator, void *ptr) {
  assert(is_within_allocator_region(allocator, ptr));

  size_t nth_page = ((char *)ptr - (char *)allocator->region) / 4096;
  if (allocator->page_owners[nth_page] == PAGE_OWNER_LARGE_ALLOCATION) {
    // Large allocation - just use munmap
    free_allocator_page(allocator, ptr);
  } else if (allocator->page_owners[nth_page] == PAGE_OWNER_INTERNAL_BLOCK) {
    // Free a block
    struct block *block = (struct block *)ptr;
    allocator_free_block(allocator, block);
  } else {
    // Free an arena allocation - arenas are 1-indexed (free is zero)
    arena_free(&allocator->arenas[allocator->page_owners[nth_page] - 1], ptr);
  }
}

int allocator_should_compact(struct allocator *allocator) {
  (void)allocator;

  // TODO: check for fragmentation, large free lists in arenas, etc
  return 0;
}

void allocator_compact(struct allocator *allocator, AllocatorMoveCallback callback) {
  // try and see if we can free full pages from our struct block free list...

  for (int i = 0; i < 12; ++i) {
    arena_compact(&allocator->arenas[i], callback);
  }
}
