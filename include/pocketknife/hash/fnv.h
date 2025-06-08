#ifndef _POCKETKNIFE_HASH_H
#define _POCKETKNIFE_HASH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t hash_fnv1(const uint8_t *data, size_t length);
uint64_t hash_fnv1a(const uint8_t *data, size_t length);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _POCKETKNIFE_HASH_H