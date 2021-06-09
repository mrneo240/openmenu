/*
 * File: block_pool.c
 * Project: lru_cache
 * File Created: Thursday, 20th May 2021 5:27:40 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "block_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void _pool_mark_used(block_pool *pool, unsigned int slot_num) {
  pool->state[slot_num] = 1;
}

static inline void _pool_mark_open(block_pool *pool, unsigned int slot_num) {
  pool->state[slot_num] = 0;
}

void pool_create(block_pool *pool, void *buffer, unsigned int size, unsigned int slots) {
  const unsigned int state_size = sizeof(unsigned char) * slots;

  pool->base = buffer;
  pool->size = size;
  pool->slots = slots;
  pool->slot_size = size / slots;
  pool->state = malloc(state_size);
  memset(pool->state, '\0', state_size);
}

void pool_get_next_free(block_pool *pool, unsigned int *slot_num, void **ptr) {
  int i;
  for (i = 0; i < pool->slots; i++) {
    if (!pool->state[i]) {
      _pool_mark_used(pool, i);

      if (slot_num)
        *slot_num = i;
      if (ptr)
        *ptr = (void *)((uintptr_t)pool->base + i * pool->slot_size);

      return;
    }
  }

  /* Error condition */
  *slot_num = 0xFFFFFFFF;
  *ptr = NULL;
}

void pool_dealloc_slot(block_pool *pool, unsigned int slot_num) {
  if (slot_num < pool->slots)
    _pool_mark_open(pool, slot_num);
}

void pool_destroy_user(block_pool *pool, void (*user_free)(void *ptr)){
  pool->base = NULL;
  pool->size = 0;
  pool->slots = 0;
  (*user_free)(pool->state);
}

void pool_destroy(block_pool *pool) {
  pool->base = NULL;
  pool->size = 0;
  pool->slots = 0;
  free(pool->state);
}