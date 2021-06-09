#include "lru.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../uthash.h"

#if DEBUG
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

/* Missing on sh-elf-gcc 9.1 ? */
char *strdup(const char *s);

// this is an example of how to do a LRU cache in C using uthash
// http://uthash.sourceforge.net/
// by Jehiah Czebotar 2011 - jehiah@gmail.com
// this code is in the public domain http://unlicense.org/

static int cache_max_size = 0;
void cache_set_size(int size) {
  cache_max_size = size;
}

struct CacheEntry {
  char *key;
  int value;
  UT_hash_handle hh;
};
struct CacheEntry *cache = NULL;

/* Callbacks that can be set */
static void *_callback_data = NULL;
static user_add_cb _callback_add = NULL;
static user_del_cb _callback_del = NULL;

void cache_callback_userdata(void *user) {
  _callback_data = user;
}
void cache_callback_add(user_add_cb callback) {
  _callback_add = callback;
}
void cache_callback_del(user_del_cb callback) {
  _callback_del = callback;
}

int find_in_cache(const char *key) {
  struct CacheEntry *entry;
  HASH_FIND_STR(cache, key, entry);
  if (entry) {
    // remove it (so the subsequent add will throw it on the front of the list)
    HASH_DELETE(hh, cache, entry);
    HASH_ADD_KEYPTR(hh, cache, entry->key, strlen(entry->key), entry);
    return entry->value;
  }
  return -1;
}

void add_to_cache(const char *key, int value) {
  DBG_PRINT("+%s( %s )\n", __func__, key);
  struct CacheEntry *entry, *tmp_entry, *new_entry;
  unsigned int cb_return = 0xFFFFFFFF;

  /* Call user function */
  if (_callback_add) {
    cb_return = (*_callback_add)(key, _callback_data);
  }

  entry = malloc(sizeof(struct CacheEntry));
  entry->key = strdup(key);
  if (cb_return != 0xFFFFFFFF) {
    value = cb_return;
  }
  entry->value = value;
  new_entry = entry;
  HASH_ADD_KEYPTR(hh, cache, entry->key, strlen(entry->key), entry);

  // prune the cache to cache_max_size
  if (HASH_COUNT(cache) > cache_max_size) {
    HASH_ITER(hh, cache, entry, tmp_entry) {
      // prune the first entry (loop is based on insertion order so this deletes the oldest item)
      HASH_DELETE(hh, cache, entry);
      DBG_PRINT("-del_from_cache( %s )\n", key);
      if (_callback_del) {
        (*_callback_del)(entry->key, &entry->value, _callback_data);
        if (_callback_add) {
          cb_return = (*_callback_add)(key, _callback_data);
        }
        new_entry->value = cb_return;
      }

      free(entry->key);
      free(entry);
      break;
    }
  }
}
