/*
 * File: simple_texture_allocator.h
 * Project: texture
 * File Created: Friday, 18th June 2021 3:28:27 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once
#define TEX_ALIGNMENT (32)

#include <stdint.h>

/* 1mb buffer */
#define TEXMAN_BUFFER_SIZE (2 * 1024 * 1024)

struct Simple_Texture {
  unsigned char *location;
  int width, height;
};

/* used for initialization */
int texman_inited(void);
void texman_reset(void *buf, uint32_t size);
void texman_set_buffer(void *buf, uint32_t size);

/* management funcs for clients
Steps:
1. create
2. reserve memory & upload by pointer OR upload_swizzle

note: texture will be bound
*/
uint32_t texman_create(void);
void texman_clear(void);
int texman_get_space_available(void);
int texman_is_space_available(void);
unsigned char *texman_get_tex_data(uint32_t num);
struct Simple_Texture *texman_reserve_memory(uint32_t width, uint32_t height, int bpp);
void texman_upload(uint32_t width, uint32_t height, int bpp, const void *buffer);
void texman_bind_tex(uint32_t num);
