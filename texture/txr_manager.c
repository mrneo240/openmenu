/*
 * File: txr_manager.c
 * Project: texture
 * File Created: Friday, 21st May 2021 2:30:13 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "txr_manager.h"

#include <dc/pvr.h>
#include <stdio.h>
#include <string.h>

#include "../dat_format.h"
#include "../ui/draw_kos.h"
#include "../ui/draw_prototypes.h"
#include "block_pool.h"
#include "lru.h"

/* CFG for small pvr pool (128x128 16bit, 32 spaces) */
#define SM_SLOT_NUM (16)
#define SM_SLOT_SIZE (128 * 128 * 2)
#define SM_POOL_SIZE (SM_SLOT_NUM * SM_SLOT_SIZE * sizeof(char))

static block_pool pvr_small;
static dat_file dat_icon;

extern const char *serial_santize(const char *id);
extern int serial_sanitizer_init(void);

unsigned int block_pool_add_cb(const char *key, void *user) {
  block_pool *pool = (block_pool *)user;
  unsigned int ret;
  char *ptr;
  pool_get_next_free(pool, &ret, (void **)&ptr);
  return ret;
}

unsigned int block_pool_del_cb(const char *key, void *value, void *user) {
  block_pool *pool = (block_pool *)user;
  unsigned int slot_num = *(unsigned int *)value;
  pool_dealloc_slot(pool, slot_num);
  return 0;
}

int txr_load_DATs(void) {
  serial_sanitizer_init();

  DAT_init(&dat_icon);
  DAT_load_parse(&dat_icon, "/cd/ICON.DAT");

  return 0;
}

int txr_create_small_pool(void) {
  void *buffer = pvr_mem_malloc(SM_POOL_SIZE);
  pool_create(buffer, SM_POOL_SIZE, SM_SLOT_NUM, &pvr_small);
  cache_set_size(SM_SLOT_NUM);
  cache_callback_userdata(&pvr_small);
  cache_callback_add(block_pool_add_cb);
  cache_callback_del(block_pool_del_cb);

  return 0;

  /* pool_destroy_user(&pvr_small, pvr_mem_free); */
}
/* unused for now */
int txr_create_large_pool(void) {
  return 0;
}

/*
called with "T1121.pvr" and a pointer to pointer to vram
returns pointer to use for texture upload/reference
 */
int txr_get_small(const char *id, struct image *img) {
  void *txr_ptr;
  int slot_num;
  const char *id_santized = serial_santize(id);

#ifdef LOOSE_FILES
  /* construct full filename */
  char buffer[64] = {0};
  strcat(buffer, "/cd/icon/");
  strcat(buffer, id_santized);
  strcat(buffer, ".pvr");

  /* check if exists and if not, return missing image */
  if (!file_exists(buffer)) {
    draw_load_missing_icon(img);
  } else
#else
  /* check if exists in DAT and if not, return missing image */
  if (!DAT_get_offset_by_ID(&dat_icon, id_santized)) {
    draw_load_missing_icon(img);
  } else
#endif
  {
    slot_num = find_in_cache(id_santized);
    if (slot_num == -1) {
      add_to_cache(id_santized, 0);
      slot_num = find_in_cache(id_santized);
      txr_ptr = pool_get_slot_addr(&pvr_small, slot_num);

      /* now load the texture into vram */
#ifdef LOOSE_FILES
      draw_load_texture_buffer(buffer, img, txr_ptr);
#else
      draw_load_texture_from_DAT_to_buffer(&dat_icon, id_santized, img, txr_ptr);
#endif
    }

    img->texture = pool_get_slot_addr(&pvr_small, slot_num);
  }
  return 0;
}
/* unused for now */
int txr_get_large(const char *id, struct image *img) {
  return 0;
}
