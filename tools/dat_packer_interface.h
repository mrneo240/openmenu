/*
 * File: dat_packer_interfacer.h
 * Project: tools
 * File Created: Thursday, 17th June 2021 12:07:37 am
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include "../inc/dat_format.h"

typedef struct bin_item_raw {
  char ID[12];
  uint32_t offset;
} bin_item_raw;

#if defined(WIN32) || defined(WINNT)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

void open_output(const char *path);
void write_bin_file(bin_header *file_header, bin_item_raw *bin_items, void *data_buf);
int iterate_dir(const char *path, int (*file_cb)(const char *, const char *, struct stat *), bin_header *file_header, bin_item_raw **bin_items);
