/*
 * File: draw_kos.h
 * Project: ui
 * File Created: Wednesday, 19th May 2021 11:54:58 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <dc/fmath.h>
#include <dc/pvr.h>
#include <stdint.h>

typedef struct image {
  char name[16];
  uint32_t width, height;
  uint32_t format;
  pvr_ptr_t texture;
} image;

pvr_ptr_t load_pvr(char const* filename, uint32* w, uint32* h, uint32* txrFormat);
