#ifndef _POCKETKNIFE_ALLOCATOR_H
#define _POCKETKNIFE_ALLOCATOR_H

#include <stddef.h>

struct allocator;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*AllocatorMoveCallback)(void *old_ptr, void *new_ptr);

struct allocator *allocator_new(size_t size);
void allocator_destroy(struct allocator *allocator);

void *allocator_alloc(struct allocator *allocator, size_t size);
void allocator_free(struct allocator *allocator, void *ptr);

int allocator_should_compact(struct allocator *allocator);
void allocator_compact(struct allocator *allocator, AllocatorMoveCallback callback);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _POCKETKNIFE_ALLOCATOR_H
