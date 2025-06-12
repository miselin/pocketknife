#ifndef _POCKETKNIFE_ALLOCATOR_H
#define _POCKETKNIFE_ALLOCATOR_H

#include <stddef.h>

struct allocator;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*AllocatorMoveCallback)(void *old_ptr, void *new_ptr);

struct allocator *allocator_new(size_t size);

/**
 * @brief Create a new allocator using the given memory region.
 *
 * This limits the allocator's ability to release memory but simplifies external testing or
 * integration into environments that have specific memory mapping requirements.
 *
 * @param base The base address of the memory region to use for allocation.
 * @param size The size of the memory region in bytes.
 * @return struct allocator* The newly created allocator instance, or NULL on failure.
 * @note The allocator will not attempt to release the memory region back to the system, even in
 * \ref allocator_destroy.
 * @note The memory region must be aligned to the system's page size (typically 4K).
 */
struct allocator *allocator_new_with_region(void *base, size_t size);
void allocator_destroy(struct allocator *allocator);

void *allocator_alloc(struct allocator *allocator, size_t size);
void allocator_free(struct allocator *allocator, void *ptr);

int allocator_should_compact(struct allocator *allocator);
void allocator_compact(struct allocator *allocator, AllocatorMoveCallback callback);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _POCKETKNIFE_ALLOCATOR_H
