#include <stddef.h>

#include "internal.h"

size_t bitwise_log2(size_t sz) {
  size_t log2 = 0;
  while (sz >>= 1) {
    log2++;
  }
  return log2;
}
