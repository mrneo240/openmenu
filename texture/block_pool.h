/*
 * File: block_pool.h
 * Project: lru_cache
 * File Created: Thursday, 20th May 2021 5:27:47 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <stdint.h>

typedef struct block_pool {
  void *base;
  unsigned int size;
  unsigned int slots;
  unsigned int slot_size;
  unsigned char *state;
} block_pool;

void pool_create(void *buffer, unsigned int size, unsigned int slots, block_pool *pool);
void pool_destroy(block_pool *pool);
void pool_destroy_user(block_pool *pool, void (*user_free)(void *ptr));
void pool_get_next_free(block_pool *pool, unsigned int *slot_num, void **ptr);
void pool_dealloc_slot(block_pool *pool, unsigned int slot_num);

/* inline funcs */
static inline void *pool_get_slot_addr(block_pool *pool, unsigned int slot_num) {
  return (void *)((uintptr_t)pool->base + slot_num * pool->slot_size);
}
