#ifndef _POCKETKNIFE_TRIE_H
#define _POCKETKNIFE_TRIE_H

struct trie;

struct trieiter;

#ifdef __cplusplus
extern "C" {
#endif

struct trie *new_trie(void);
void trie_insert(struct trie *trie, const char *key, void *value);
void *trie_lookup(struct trie *trie, const char *key);
void trie_remove(struct trie *trie, const char *key);
void dump_trie(struct trie *trie);
void destroy_trie(struct trie *trie);

#ifdef __cplusplus
};
#endif

#endif