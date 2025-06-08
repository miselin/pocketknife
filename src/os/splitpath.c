#include <stddef.h>
#ifndef _MSC_VER
#include <libgen.h>
#endif
#include <pocketknife/os/os.h>

#include <stdlib.h>
#include <string.h>

int splitpath(const char* path, char* drive, size_t drivelen, char* dir, size_t dirlen, char* fname,
              size_t namelen, char* ext, size_t extlen) {
#ifdef _MSC_VER
  return _splitpath_s(path, drive, drivelen, dir, dirlen, fname, namelen, ext, extlen);
#else
  if (drive && drivelen) {
    *drive = 0;
  }

  char* pathcopy = (char*)malloc(256);
  strncpy(pathcopy, path, 256);

  const char* d = dirname(pathcopy);
  strncpy(dir, d, dirlen);

  // dirname can modify its parameter, so make sure we have a stable path
  strncpy(pathcopy, path, 256);
  const char* filename = basename(pathcopy);

  char* extp = strrchr(filename, '.');
  if (extp) {
    if (ext) {
      strncpy(ext, extp, extlen);
      ext[extlen - 1] = '\0';
    }
    *extp = '\0';
  } else {
    if (ext) {
      *ext = '\0';
    }
  }

  if (fname) {
    strncpy(fname, filename, namelen);
    fname[namelen - 1] = '\0';
  }

  free(pathcopy);
#endif

  return 1;
}
