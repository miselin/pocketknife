#ifndef _POCKETKNIFE_OS_H
#define _POCKETKNIFE_OS_H

#include <stddef.h>
#include <stdint.h>

uint64_t get_ticks_ns(void);

int splitpath(const char* path, char* drive, size_t drivelen, char* dir, size_t dirlen, char* fname,
              size_t namelen, char* ext, size_t extlen);

#endif  // _POCKETKNIFE_OS_H
