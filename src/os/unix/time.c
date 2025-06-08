#include <pocketknife/os/os.h>

#include <stdint.h>
#include <time.h>

uint64_t get_ticks_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}
