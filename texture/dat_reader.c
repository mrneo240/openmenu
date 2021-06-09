/*
 * File: reader.c
 * Project: dat_builder
 * File Created: Tuesday, 8th June 2021 2:47:35 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "../dat_format.h"
#include "../uthash.h"

/* Called:
./reader input.bin

Dumps all info about the container
*/

/* Define configure constants */
/* only defined when building the binary tool */
//#define STANDALONE_BINARY (1)

#if DEBUG
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
  FILE *bin_fd;
  bin_header file_header;

  bin_fd = fopen(path, "rb");
  if (!bin_fd) {
    DBG_PRINT("Err: Cant read input %s!\n", path);
    return 1;
  }

  fread(&file_header, sizeof(bin_header), 1, bin_fd);
  if (file_header.magic.rich.version != 1) {
    DBG_PRINT("Err: Incorrect input file format!\n");
    return 1;
  }

  /* setup basic bin file info */
  bin->chunk_size = file_header.chunk_size;
  bin->num_chunks = file_header.num_chunks;
  bin->handle = bin_fd;
  bin->items = malloc(bin->num_chunks * sizeof(bin_item));
  bin->hash = NULL;

  /* Parse file table to Hash table */
  for (int i = 0; i < file_header.num_chunks; i++) {
    fread(&bin->items[i], sizeof(bin_item_raw), 1, bin->handle);
    HASH_ADD_STR(bin->hash, ID, &bin->items[i]);
  }

  return 0;
}

void DAT_dump(const dat_file *bin) {
  DBG_PRINT("BIN Stats:\nChunk Size: %d\nNum Chunks: %d\n\n", bin->chunk_size, bin->num_chunks);
  for (int i = 0; i < bin->num_chunks; i++) {
    DBG_PRINT("Record[%d] %s at 0x%X\n", bin->items[i].offset, bin->items[i].ID, bin->items[i].offset * bin->chunk_size);
  }
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

int DAT_read_file_by_ID(const dat_file *bin, const char *ID, void *buf) {
  uint32_t offset = DAT_get_offset_by_ID(bin, ID);
  if (offset) {
    fseek(bin->handle, offset, SEEK_SET);
    fread(buf, bin->chunk_size, 1, bin->handle);
    return 1;
  } else {
    return 0;
  }
}

int DAT_read_file_by_num(const dat_file *bin, uint32_t chunk_num, void *buf) {
  uint32_t offset = chunk_num * bin->chunk_size;
  if (chunk_num <= bin->num_chunks) {
    fseek(bin->handle, offset, SEEK_SET);
    fread(buf, bin->chunk_size, 1, bin->handle);
    return 1;
  } else {
    return 0;
  }
  return 1;
}

#ifdef STANDALONE_BINARY
#define NUM_ARGS (1)
int main(int argc, char **argv) {
  if (argc < NUM_ARGS + 1 /*binary itself*/) {
    printf("Incorrect usage!\n\t./reader input.bin\n");
    return 1;
  }

  /* Basic Usage */
  dat_file input_bin;
  DAT_init(&input_bin);
  DAT_load_parse(&input_bin, argv[1]);

  /* Dump info and files */
  DAT_dump(&input_bin);

  /* Hashmap test */
  printf("\nSearching known:\n");
  const char *search = "T40502N";
  printf("Found %s at %X\n", search, DAT_get_offset_by_ID(&input_bin, search));

  printf("\nSearching missing:\n");
  const char *missing = "MISSING";
  printf("Found %s at %X\n", missing, DAT_get_offset_by_ID(&input_bin, missing));
}
#endif
