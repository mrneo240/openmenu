/*
 * File: item.h
 * Project: ini_parse
 * File Created: Wednesday, 19th May 2021 3:00:52 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

typedef struct gd_item {
  char name[128];
  char date[12];
  char product[12];
  char disc[8];
  char version[8];
  char region[4];
  unsigned int slot_num;
  char vga[1];
} gd_item;

