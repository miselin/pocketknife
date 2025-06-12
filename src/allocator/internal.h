#ifndef _POCKETKNIFE_ALLOCATOR_INTERNAL_H
#define _POCKETKNIFE_ALLOCATOR_INTERNAL_H

#include <pocketknife/allocator/allocator.h>

#include <stddef.h>
#include <stdint.h>

#define PAGE_OWNER_INTERNAL 252
#define PAGE_OWNER_LARGE_ALLOCATION 253
#define PAGE_OWNER_INTERNAL_BLOCK 254
#define PAGE_OWNER_FREE 0

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
  size_t which;

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

  // Is the region madvise(2)-able? This is essentially always true, but can be set to false if the
  // allocator is given a region in which it is to operate.
  int is_fully_owned;

  // 0 = free
  // 1-9 = arena index, 1-indexed, where 1 is the 16-byte arena, 2 is the 32-byte arena, etc.
  // 253 = large allocation, fully allocated page (but how do we know how big so we can unmap??)
  // 254 = internal "struct block" arena
  uint8_t *page_owners;

  // Sub-arenas, selected based on lg2 of size (<= 2K)
  // Arenas will pull individual pages from the allocator's region and manage their own free/used
  // lists.
  // We set a minimum allocation size of 16 bytes, so the index is actually log2(sz) - 4.
  struct arena arenas[8];

  // This is a set of "struct block" objects available for arenas to use in their free/used lists.
  struct block *free_blocks;

  struct arena_page *free_arena_pages;
};

void *arena_alloc(struct arena *arena);
void arena_free(struct arena *arena, void *ptr);
void arena_compact(struct arena *arena, AllocatorMoveCallback callback);

int is_within_allocator_region(struct allocator *allocator, void *ptr);

struct block *allocator_alloc_block(struct allocator *allocator);
void allocator_free_block(struct allocator *allocator, struct block *block);

struct arena_page *allocator_alloc_arena_page_meta(struct allocator *allocator);
void allocator_free_arena_page_meta(struct allocator *allocator, struct arena_page *page);

void *get_free_allocator_page(struct allocator *allocator, int owner);

size_t bitwise_log2(size_t sz);

#endif  // _POCKETKNIFE_ALLOCATOR_INTERNAL_H
