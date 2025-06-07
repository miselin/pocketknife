#ifndef _POCKETKNIFE_GC_H
#define _POCKETKNIFE_GC_H

#ifdef __cplusplus
extern "C" {
#endif

struct gc;

struct gc *gc_create(void);
void gc_destroy(struct gc *gc);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _POCKETKNIFE_GC_H
