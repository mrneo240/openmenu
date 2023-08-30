/*
 * File: reader.c
 * Project: dat_builder
 * File Created: Tuesday, 8th June 2021 2:47:35 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#ifdef COSMO
#include "../tools/cosmo/cosmopolitan.h"
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include <external/uthash.h>

#include "../gdrom/gdrom_fs.h"
#include "../inc/dat_format.h"

/* Define configure constants */
/* only defined when building the binary tool */
#ifdef STANDALONE_BINARY
#define DEBUG
#endif

#ifdef DEBUG
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

typedef struct bin_item_raw {
  char ID[12];
  uint32_t offset;
} bin_item_raw;

int DAT_init(dat_file *bin) {
  memset(bin, '\0', sizeof(dat_file));
  return 0;
}

int DAT_load_parse(dat_file *bin, const char *path) {
  file_t bin_fd;
  bin_header file_header;

#ifdef STANDALONE_BINARY
  bin_fd = fs_open(path, O_RDONLY);
  const char *filename_safe = path;
#else
  char filename_safe[128];
  memcpy(filename_safe, DISC_PREFIX, strlen(DISC_PREFIX) + 1);
  strcat(filename_safe, path);

  bin_fd = fs_open(filename_safe, O_RDONLY);
#endif

  if (bin_fd == -1) {
    printf("DAT:Error Cant read input %s!\n", filename_safe);
    return 1;
  }

  printf("DAT:Open %s (%s)\n", filename_safe, path);

  fs_read(bin_fd, &file_header, sizeof(bin_header));
  if (file_header.magic.rich.version != 1) {
    printf("DAT:Error Incorrect input file format!\n");
    return 1;
  }

  /* setup basic bin file info */
  bin->chunk_size = file_header.chunk_size;
  bin->num_chunks = file_header.num_chunks;
  bin->handle = bin_fd;
  bin->items = malloc(bin->num_chunks * sizeof(bin_item));
  if (!bin->items) {
	  printf("%s no free memory\n", __func__);
	  return 1;
  }
  bin->hash = NULL;

  /* Parse file table to Hash table */
  for (unsigned int i = 0; i < file_header.num_chunks; i++) {
    fs_read(bin->handle, &bin->items[i], sizeof(bin_item_raw));
    HASH_ADD_STR(bin->hash, ID, &bin->items[i]);
  }

  /* Leave our handle in a handy place in case we need to read after */
  fs_seek(bin->handle, bin->items[0].offset * bin->chunk_size, SEEK_SET);

  return 0;
}

void DAT_info(const dat_file *bin) {
  DBG_PRINT("DAT:Stats\nChunk Size: %u\nNum Chunks: %u\n\n", bin->chunk_size, bin->num_chunks);
  for (unsigned int i = 0; i < bin->num_chunks; i++) {
    DBG_PRINT("Record[%u] %s at 0x%X\n", bin->items[i].offset, bin->items[i].ID, (unsigned int)(bin->items[i].offset * bin->chunk_size));
  }
  DBG_PRINT("\n");
}

uint32_t DAT_get_offset_by_ID(const dat_file *bin, const char *ID) {
  const bin_item *item;
  uint32_t ret;

  HASH_FIND_STR(bin->hash, ID, item);
  if (item) {
    ret = item->offset * bin->chunk_size;
  } else {
    ret = 0;
  }

  return ret;
}

uint32_t DAT_get_index_by_ID(const dat_file *bin, const char *ID) {
  const bin_item *item;
  uint32_t ret;

  HASH_FIND_STR(bin->hash, ID, item);
  if (item) {
    ret = item->offset;
  } else {
    ret = 0xFFFFFFFF;
  }

  return ret;
}

int DAT_read_file_by_ID(const dat_file *bin, const char *ID, void *buf) {
  uint32_t offset = DAT_get_offset_by_ID(bin, ID);
  if (offset) {
    fs_seek(bin->handle, offset, SEEK_SET);
    fs_read(bin->handle, buf, bin->chunk_size);
    return 1;
  } else {
    return 0;
  }
}

int DAT_read_file_by_num(const dat_file *bin, uint32_t chunk_num, void *buf) {
  uint32_t offset = chunk_num * bin->chunk_size;
  if (chunk_num <= bin->num_chunks) {
    fs_seek(bin->handle, offset, SEEK_SET);
    fs_read(bin->handle, buf, bin->chunk_size);
    return 1;
  } else {
    return 0;
  }
}
