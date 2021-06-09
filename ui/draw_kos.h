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

/* defined in pvr_texture.c */
void* pvr_get_internal_buffer(void);
/* Convenience functions */
extern pvr_ptr_t load_pvr(const char* filename, uint32_t* w, uint32_t* h, uint32_t* txrFormat);
extern pvr_ptr_t load_pvr_to_buffer(const char* filename, uint32_t* w, uint32_t* h, uint32_t* txrFormat, void* buffer);
extern pvr_ptr_t load_pvr_from_buffer(const void* input, uint32_t* w, uint32_t* h, uint32_t* txrFormat);

/* base method */
extern pvr_ptr_t load_pvr_from_buffer_to_buffer(const void* input, uint32_t* w, uint32_t* h, uint32_t* txrFormat, void* buffer);
