#include <pocketknife/allocator/allocator.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "internal.h"

void *arena_alloc(struct arena *arena) {
  if (!arena->free_list) {
    void *new_block = get_free_allocator_page(arena->parent, arena->which + 1);
    if (!new_block) {
      return NULL;
    }

    struct arena_page *new_page_meta = allocator_alloc_arena_page_meta(arena->parent);
    if (!new_page_meta) {
      return NULL;
    }

    // generate the free list for this new block now
    for (size_t i = 0; i < arena->blocks_per_page; ++i) {
      struct block *new_block_struct = allocator_alloc_block(arena->parent);
      new_block_struct->at = (char *)new_block + (i * arena->block_size);
      new_block_struct->next = arena->free_list;
      arena->free_list = new_block_struct;
    }

    new_page_meta->base = new_block;
    new_page_meta->bitmap = 0;
    new_page_meta->used_count = 0;
    new_page_meta->free_count = arena->blocks_per_page;
    new_page_meta->next = arena->pages;
    arena->pages = new_page_meta;
    arena->free_count += arena->blocks_per_page;
  }

  struct block *block = arena->free_list;
  arena->free_list = block->next;

  void *result = block->at;

  arena->free_count--;
  arena->used_count++;

  allocator_free_block(arena->parent, block);

  // TODO: set the bitmap in struct arena_page for this block
  return block->at;
}

void arena_free(struct arena *arena, void *ptr) {
  (void)arena;
  (void)ptr;

  struct arena_page *page = arena->pages;
  while (page) {
    if ((char *)ptr >= (char *)page->base && (char *)ptr < (char *)page->base + 4096) {
      // Found the page containing the pointer
      break;
    }
    page = page->next;
  }

  assert(page != NULL);

  struct block *block = allocator_alloc_block(arena->parent);
  if (!block) {
    return;
  }

  block->at = ptr;
  block->next = arena->free_list;
  arena->free_list = block;

  arena->free_count++;
  arena->used_count--;
}

void arena_compact(struct arena *arena, AllocatorMoveCallback callback) {
  (void)arena;
  (void)callback;
  //
}
