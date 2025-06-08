#ifndef _POCKETKNIFE_GC_H
#define _POCKETKNIFE_GC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gc;

struct gc_stats {
  size_t total_allocated;  // Total memory allocated by the GC
  size_t total_freed;      // Total memory freed by the GC
  size_t total_collected;  // Total memory collected by the GC
  size_t total_marked;     // Total memory marked by the GC
  size_t total_objects;    // Total number of objects managed by the GC
};

typedef void *(*GCAllocateFunc)(size_t size);
typedef void (*GCFreeFunc)(void *ptr);

typedef void (*GCEraseFunc)(struct gc *gc, void *ptr);
typedef void (*GCMarkFunc)(struct gc *gc, void *ptr);

/**
 * @brief Create a new garbage collector instance.
 *
 * @return A new garbage collector instance. Must be destroyed with `gc_destroy`.
 */
struct gc *gc_create(void);

/**
 * @brief Create a new garbage collector instance using a custom allocator.
 *
 * @param alloc A function to allocate memory for the garbage collector. This should implement the
 * same semantics as malloc().
 * @param free A function to free memory allocated by the garbage collector. This should implement
 * the same semantics as free().
 * @return A new garbage collector instance. Must be destroyed with `gc_destroy`.
 */
struct gc *gc_create_with_allocator(GCAllocateFunc alloc, GCFreeFunc free);

/**
 * @brief Destroys the given garbage collector instance.
 *
 * @param gc The garbage collector instance to destroy. All objects managed by this GC will be
 * erased.
 */
void gc_destroy(struct gc *gc);

/**
 * @brief Allocates memory for an object in the garbage collector.
 *
 * @param gc The garbage collector instance to use for allocation.
 * @param size The number of bytes to allocate. More bytes than this may be allocated to support
 * garbage collection routines, and for alignment.
 * @param marker An optional function that can be used by the garbage collector when marking the
 * object. This is used when the object is a root object. The implementation of this callback should
 * call gc_mark on any child objects that should be retained.
 * @param eraser An optional function that can be used by the garbage collector when erasing the
 * object. This must not free the object itself, but should free any non-garbage-collected resources
 * within. For example, if the GC object is a structure that includes a pointer allocated by
 * malloc(), free() it in this callback.
 * @return void* A pointer to the allocated memory that is managed by the garbage collector, or NULL
 * if the allocation failed.
 */
void *gc_alloc(struct gc *gc, size_t size, GCMarkFunc marker, GCEraseFunc eraser);

/**
 * @brief Mark the given object as a root object in the garbage collector.
 *
 * This will ensure that the object is not collected by the garbage collector.
 *
 * @param gc The garbage collector instance to use.
 * @param ptr The pointer to the object to mark as a root object.
 */
void gc_root(struct gc *gc, void *ptr);

/**
 * @brief Remove the given object as a root object in the garbage collector.
 *
 * @param gc The garbage collector instance to use.
 * @param ptr The pointer to the object to remove as a root object.
 */
void gc_unroot(struct gc *gc, void *ptr);

/**
 * @brief Set the marked flag on the given object in the garbage collector.
 *
 * This should primarily be used in marker functions to mark child objects of a root object.
 *
 * @param gc The garbage collector instance to use.
 * @param ptr The pointer to the object to mark.
 * @return int 1 if the object was successfully marked, 0 if the object was already marked or
 * invalid. You should not continue marking child objects of an object if gc_mark returns 0.
 */
int gc_mark(struct gc *gc, void *ptr);

/**
 * @brief Runs a garbage collection cycle.
 *
 * @param gc The garbage collector instance to use.
 * @param stats An optional pointer to a gc_stats structure that will be filled with statistics
 * about the garbage collection cycle. If NULL, no statistics will be collected.
 */
void gc_run(struct gc *gc, struct gc_stats *stats);

/**
 * @brief Get statistics about the garbage collector.
 *
 * This will fill the provided stats structure with information about the current state of the
 * garbage collector.
 *
 * @param gc The garbage collector instance to use.
 * @param stats A pointer to a gc_stats structure that will be filled with statistics.
 */
void gc_stats(struct gc *gc, struct gc_stats *stats);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _POCKETKNIFE_GC_H
