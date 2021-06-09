#include "lru.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void cache_set_size(cache_instance *cache, int size) {
  cache->cache_max_size = size;
}

void cache_callback_userdata(cache_instance *cache, void *user) {
  cache->callback_data = user;
}

void cache_callback_add(cache_instance *cache, user_add_cb callback) {
  cache->callback_add = callback;
}

void cache_callback_del(cache_instance *cache, user_del_cb callback) {
  cache->callback_del = callback;
}

int find_in_cache(cache_instance *cache, const char *key) {
  struct CacheEntry *entry;
  HASH_FIND_STR(cache->cache, key, entry);
  if (entry) {
    // remove it (so the subsequent add will throw it on the front of the list)
    HASH_DELETE(hh, cache->cache, entry);
    HASH_ADD_KEYPTR(hh, cache->cache, entry->key, strlen(entry->key), entry);
    return entry->value;
  }
  return -1;
}

void add_to_cache(cache_instance *cache, const char *key, int value) {
  DBG_PRINT("+%s( %s )\n", __func__, key);
  struct CacheEntry *entry, *tmp_entry, *new_entry;
  unsigned int cb_return = 0xFFFFFFFF;

  /* Call user function */
  if (cache->callback_add) {
    cb_return = (*cache->callback_add)(key, cache->callback_data);
  }

  entry = malloc(sizeof(struct CacheEntry));
  entry->key = strdup(key);
  if (cb_return != 0xFFFFFFFF) {
    value = cb_return;
  }
  entry->value = value;
  new_entry = entry;
  HASH_ADD_KEYPTR(hh, cache->cache, entry->key, strlen(entry->key), entry);

  // prune the cache to cache_max_size
  if (HASH_COUNT(cache->cache) > cache->cache_max_size) {
    HASH_ITER(hh, cache->cache, entry, tmp_entry) {
      // prune the first entry (loop is based on insertion order so this deletes the oldest item)
      HASH_DELETE(hh, cache->cache, entry);
      DBG_PRINT("-del_from_cache( %s )\n", key);
      if (cache->callback_del) {
        (*cache->callback_del)(entry->key, &entry->value, cache->callback_data);
        if (cache->callback_add) {
          cb_return = (*cache->callback_add)(key, cache->callback_data);
        }
        new_entry->value = cb_return;
      }

      free(entry->key);
      free(entry);
      break;
    }
  }
}
