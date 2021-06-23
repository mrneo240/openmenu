/*
 * File: dat_format.h
 * Project: ini_parse
 * File Created: Tuesday, 8th June 2021 8:49:08 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include "uthash.h"

typedef struct bin_item {
  char ID[12];
  uint32_t offset;
  UT_hash_handle hh; /* makes this structure hashable */
} bin_item;

typedef struct bin_header {
  union {
    struct
    {
      char alpha[3];
      char version;
    } rich;
    uint32_t raw;
  } magic;             /* DAT1 : DAT + single digit version */
  uint32_t chunk_size; /* Size of each chunk in the file */
  uint32_t num_chunks; /* How many chunks are present in this bin */
  uint32_t padding0;   /* Unused in ver1 */
} bin_header;

typedef struct dat_file {
  uint32_t chunk_size; /* Size of each chunk in the file */
  uint32_t num_chunks; /* How many chunks are present in this bin */
  void *handle;        /* Open File Handle, commonly FILE* */
  bin_item *items;     /* Holds actual data */
  bin_item *hash;      /* Hash table for above */
} dat_file;

int DAT_init(dat_file *bin);
int DAT_load_parse(dat_file *bin, const char *path);
void DAT_dump(const dat_file *bin);

uint32_t DAT_get_offset_by_ID(const dat_file *bin, const char *ID);
uint32_t DAT_get_index_by_ID(const dat_file *bin, const char *ID);
int DAT_read_file_by_ID(const dat_file *bin, const char *ID, void *buf);
int DAT_read_file_by_num(const dat_file *bin, uint32_t chunk_num, void *buf);
