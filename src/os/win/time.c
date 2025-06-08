
#include <pocketknife/os/os.h>

#define WIN32_LEAN_AND_MEAN
#include <direct.h>
#include <stdint.h>
#include <windows.h>

static LARGE_INTEGER qpcfreq;
static int freqset = 0;

uint64_t get_ticks_ns() {
  if (!freqset) {
    QueryPerformanceFrequency(&qpcfreq);
  }

  LARGE_INTEGER curr = {0};
  QueryPerformanceCounter(&curr);

  // qpcfreq = ticks per second
  // we want to convert to ns
  // e.g. freq of 500 ticks per second = 2000000 ns per tick
  uint64_t divisor = 1000000000ULL / qpcfreq.QuadPart;
  return curr.QuadPart * divisor;
}
