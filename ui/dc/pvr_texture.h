/*
 * File: common.h
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:01:31 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */

#pragma once

#include <dc/pvr.h>
#include <stdint.h>

/* Offset and dimensions of each sprite within a spritesheet (romdisk/foo.txt file) */
typedef struct image {
  char name[16];
  uint32_t width, height;
  uint32_t format;
  pvr_ptr_t texture;
} image;

void* pvr_get_internal_buffer(void);
/* Convenience functions */
extern pvr_ptr_t load_pvr(const char* filename, uint32_t* w, uint32_t* h, uint32_t* txrFormat);
extern pvr_ptr_t load_pvr_to_buffer(const char* filename, uint32_t* w, uint32_t* h, uint32_t* txrFormat, void* buffer);
extern pvr_ptr_t load_pvr_from_buffer(const void* input, uint32_t* w, uint32_t* h, uint32_t* txrFormat);

/* base method */
extern pvr_ptr_t load_pvr_from_buffer_to_buffer(const void* input, uint32_t* w, uint32_t* h, uint32_t* txrFormat, void* buffer);
