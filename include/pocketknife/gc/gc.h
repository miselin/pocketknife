#ifndef _POCKETKNIFE_GC_H
#define _POCKETKNIFE_GC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gc;

struct gc_slot;

typedef void *(*GCAllocateFunc)(size_t size);
typedef void (*GCFreeFunc)(void *ptr);

typedef void (*GCEraseFunc)(struct gc *gc, struct gc_slot *slot);
typedef void (*GCMarkFunc)(struct gc *gc, struct gc_slot *slot);

/**
 * @brief Spaces in the garbage collector.
 *
 * Several options for custom spaces are available which can be used for various purposes, such as
 * storing interned data or code.
 *
 * The default spaces (young, old, and large) always exist and are part of the core generational GC.
 */
enum GCSpace {
  GC_SPACE_YOUNG = 0,
  GC_SPACE_OLD = 1,
  GC_SPACE_LARGE = 2,
  GC_SPACE_CUSTOM1 = 3,
  GC_SPACE_CUSTOM2 = 4,
  GC_SPACE_CUSTOM3 = 5,
  GC_SPACE_CUSTOM4 = 6,
  GC_SPACE_CUSTOM5 = 7,
};

struct gc_config {
  // Enable debug mode for the garbage collector. This will print debug information to stderr.
  int debug;

  // Function used to allocate memory within the garbage collector. Defaults to malloc().
  GCAllocateFunc alloc;
  // Function used to free memory allocated by the garbage collector. Defaults to free().
  GCFreeFunc free;

  // Number of cycles an allocation can survive in the young space before a promotion.
  size_t young_max_cycles;
  // Number of bytes above which to directly allocate in the large space.
  size_t large_threshold;
};

struct gc_space_config {
  // Number of GC cycles to run before sweeping this space. Higher numbers result in fewer
  // mark+sweep cycles, saving CPU time but increasing memory usage.
  size_t sweep_every;

  // The desired maximum size of this space in bytes. If an allocation would exceed this size,
  // an immediate garbage collection cycle will be triggered. That immediate cycle would attempt
  // to collect in this space, and if that is insufficient, it will run on the entire garbage
  // collector.
  size_t max_size;
};

struct gc_stats {
  size_t total_allocated;  // Total memory allocated by the GC
  size_t total_freed;      // Total memory freed by the GC
  size_t total_collected;  // Total memory collected by the GC
  size_t total_marked;     // Total memory marked by the GC
  size_t total_objects;    // Total number of objects managed by the GC
};

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
 * @brief Create a new garbage collector instance with a custom configuration.
 *
 * @param config Configuration for the garbage collector.
 * @return struct gc* A new garbage collector instance. Must be destroyed with `gc_destroy`.
 */
struct gc *gc_create_with_config(struct gc_config *config);

/**
 * @brief Configure the given space in the given garbage collector instance.
 *
 * @param gc The garbage collector instance to configure.
 * @param space A space to configure.
 * @param config Configuration for the space.
 */
void gc_configure_space(struct gc *gc, enum GCSpace space, struct gc_space_config *config);

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
 * call gc_mark on any child objects that should be retained. Don't call gc_mark on the object
 * itself, as it is already marked by the garbage collector.
 * @param eraser An optional function that can be used by the garbage collector when erasing the
 * object. This must not free the object itself, but should free any non-garbage-collected resources
 * within. For example, if the GC object is a structure that includes a pointer allocated by
 * malloc(), free() it in this callback.
 * @return void* A pointer to the allocated memory that is managed by the garbage collector, or NULL
 * if the allocation failed.
 */
struct gc_slot *gc_alloc(struct gc *gc, size_t size, GCMarkFunc marker, GCEraseFunc eraser);

/**
 * @brief Allocate memory for an object in the garbage collector with a specific space.
 *
 * This is useful when you know you want to allocate in a specific space, such as the old space, or
 * a custom space that you have configured.
 *
 * Works identically to \ref gc_alloc, but with a custom space instead of defaulting to the young
 * space.
 */
struct gc_slot *gc_alloc_with_space(struct gc *gc, size_t size, GCMarkFunc marker,
                                    GCEraseFunc eraser, enum GCSpace space);

/**
 * @brief Retrieve the underlying pointer from a gc_slot.
 *
 * While the pointer is live, it can be used as a normal pointer. The garbage collector will not be
 * allowed to relocate the memory. Use \ref gc_unlock_slot when you are done with the pointer to
 * allow for garbage collection to continue.
 *
 * @param slot The gc_slot from which to retrieve the pointer.
 * @return void* The pointer to the allocated memory. This pointer is valid until you call
 * gc_unlock_slot on the slot.
 */
void *gc_lock_slot(struct gc_slot *slot);

/**
 * @brief Mark the given slot as unlocked, invalidating any pointers retrieved from it.
 *
 * This allows the garbage collector to move the underlying memory once again.
 *
 * @param slot The gc_slot to unlock. This should be the same slot that was previously locked with
 * gc_lock_slot.
 */
void gc_unlock_slot(struct gc_slot *slot);

/**
 * @brief Mark the given object as a root object in the garbage collector.
 *
 * This will ensure that the object is not collected by the garbage collector, but the underlying
 * memory may still be moved. Use \ref gc_lock_slot if you need to guarantee that the memory itself
 * will not be moved during garbage collection.
 *
 * @param gc The garbage collector instance to use.
 * @param ptr The pointer to the object to mark as a root object.
 */
void gc_root(struct gc *gc, struct gc_slot *slot);

/**
 * @brief Remove the given object as a root object in the garbage collector.
 *
 * @param gc The garbage collector instance to use.
 * @param ptr The pointer to the object to remove as a root object.
 * @return int 1 if the object was previously a root object, 0 if it was not.
 */
int gc_unroot(struct gc *gc, struct gc_slot *slot);

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
int gc_mark(struct gc *gc, struct gc_slot *slot);

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
void gc_get_stats(struct gc *gc, struct gc_stats *stats);

/**
 * @brief Get the current space in which a slot is located.
 *
 * @param slot The slot to check.
 * @return enum GCSpace The space in which the slot is located.
 */
enum GCSpace gc_get_space(struct gc_slot *slot);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _POCKETKNIFE_GC_H
