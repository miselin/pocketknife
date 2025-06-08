# Pocketknife

[![CI](https://github.com/miselin/pocketknife/actions/workflows/ci.yml/badge.svg)](https://github.com/miselin/pocketknife/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/miselin/pocketknife/graph/badge.svg?token=1ZYQM217XJ)](https://codecov.io/gh/miselin/pocketknife)

All the stuff I keep having to reimplement, or that I might need again in the
future. A pocketknife is a multitasker - this repo is my go-to for all the
little tasks that I can reuse in other projects.

## Not Invented Here

There's stuff here that's already readily available. Many of these libraries
are as much a learning exercise as they are a useful tool. That means the
quality may not be quite production-grade yet, or there may be issues that
stem from learning by doing. You've been warned :smiley:

## Libraries

### gc

`libgc` provides a rudimentary mark-sweep garbage collector.

### hash

`libhash` provides hashing routines like FNV-1a.

### os

`libos` offers an OS-compatibility layer for other projects to use.

### string

`libstring` offers string manipulation primitives like a C dynamic string builder, and a cord data structure.

### trie

`libtrie` offers a trie for storing key/value pairs with string keys. The use of a trie allows for prefix matching in addition to direct lookup.
