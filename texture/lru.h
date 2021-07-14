#pragma once

#include <external/uthash.h>

/* Function callbacks */
typedef unsigned int (*user_add_cb)(const char* key, void* user);
typedef unsigned int (*user_del_cb)(const char* key, void* value, void* user);

struct CacheEntry {
  char* key;
  int value;
  UT_hash_handle hh;
};

typedef struct cache_instance {
  unsigned int cache_max_size;
  void* callback_data;
  user_add_cb callback_add;
  user_del_cb callback_del;
  struct CacheEntry* cache;
} cache_instance;

void cache_set_size(cache_instance* cache, int size);
void cache_callback_userdata(cache_instance* cache, void* user);
void cache_callback_add(cache_instance* cache, user_add_cb callback);
void cache_callback_del(cache_instance* cache, user_del_cb callback);

int find_in_cache(cache_instance* cache, const char* key);
void add_to_cache(cache_instance* cache, const char* key, int value);
void empty_cache(cache_instance* cache);
