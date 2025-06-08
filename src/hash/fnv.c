#include <pocketknife/hash/fnv.h>

#include <stddef.h>
#include <stdint.h>

static const uint64_t FNV_offset_basis = 0xcbf29ce484222325ULL;
static const uint64_t FNV_prime = 0x100000001b3ULL;

uint64_t hash_fnv1(const uint8_t *data, size_t length) {
  uint64_t hash = FNV_offset_basis;

  for (size_t i = 0; i < length; i++) {
    hash *= FNV_prime;
    hash ^= (uint64_t)data[i];
  }

  return hash;
}

uint64_t hash_fnv1a(const uint8_t *data, size_t length) {
  uint64_t hash = FNV_offset_basis;

  for (size_t i = 0; i < length; i++) {
    hash ^= (uint64_t)data[i];
    hash *= FNV_prime;
  }

  return hash;
}
