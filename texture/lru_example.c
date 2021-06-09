
/*
 * File: example.c
 * Project: lru_cache
 * File Created: Thursday, 20th May 2021 5:27:21 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block_pool.h"
#include "lru.h"

#define SLOT_NUM (5)
#define SLOT_SIZE (24)
#define POOL_SIZE (SLOT_NUM * SLOT_SIZE * sizeof(char))

static void pool_debug(block_pool *pool) {
  int i;
  for (i = 0; i < pool->slots; i++) {
    if (pool->state[i]) {
      printf("[%d] %s\n", i, (char *)pool_get_slot_addr(pool, i));
    } else {
      printf("[%d] NULL\n", i);
    }
  }
  printf("\n");
}

unsigned int block_pool_add_cb(const char *key, void *user) {
  printf("\t%s( %s )\n", __func__, key);
  block_pool *pool = (block_pool *)user;
  unsigned int ret;
  char *ptr;
  pool_get_next_free(pool, &ret, (void **)&ptr);
  if (ptr)
    strcpy(ptr, key);
  return ret;
}
unsigned int block_pool_del_cb(const char *key, void *value, void *user) {
  printf("\t%s( %s )\n", __func__, key);
  block_pool *pool = (block_pool *)user;
  unsigned int slot_num = *(unsigned int *)value;
  pool_dealloc_slot(pool, slot_num);
  return 0;
}

int example_main(int argc, char **argv) {
  block_pool test;
  void *buffer = malloc(POOL_SIZE);
  pool_create(buffer, POOL_SIZE, SLOT_NUM, &test);
  cache_set_size(SLOT_NUM);
  cache_callback_userdata(&test);
  cache_callback_add(block_pool_add_cb);
  cache_callback_del(block_pool_del_cb);
  pool_debug(&test);

  int slot_num;
  printf("Fill LRU in order:\n");
  add_to_cache("AAA.pvr", 0);
  add_to_cache("BBB.pvr", 0);
  add_to_cache("CCC.pvr", 0);
  add_to_cache("DDD.pvr", 0);
  add_to_cache("EEE.pvr", 0);
  pool_debug(&test);

  printf("\nAdd FFF which will replace AAA (used last oldest):\n");
  add_to_cache("FFF.pvr", 0);

  printf("\nUse BBB then add GGG which will replace CCC (next last oldest):\n");
  slot_num = find_in_cache("BBB.pvr");
  add_to_cache("GGG.pvr", 0);

  printf("\nFinal Block Pool:\n");
  pool_debug(&test);

  (void)slot_num;
  return 0;
}