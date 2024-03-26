/*
 * File: simple_texture_allocator.c
 * Project: texture
 * File Created: Friday, 18th June 2021 3:28:27 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "simple_texture_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../texture/txr_manager.h"

static struct Simple_Texture textures[32];
static void *tex_buffer = NULL;
static void *tex_buffer_start = NULL;
static void *tex_buffer_max = NULL;
static uint32_t tex_number = 0;

static inline size_t getMemorySize(int width, int height, int bpp) {
  return (size_t)(bpp * width * height);
}

int texman_inited(void) {
  return tex_buffer != 0;
}

void texman_reset(void *buf, uint32_t size) {
  //txr_empty_small_pool();
  //txr_empty_large_pool();
  memset(textures, 0, sizeof(textures));
  tex_number = 0;
  tex_buffer = tex_buffer_start = buf;
  tex_buffer_max = buf + size;
#ifdef DEBUG
  char msg[64];
  sprintf(msg, "TEXMAN: reset @ %p size %d bytes\n", buf, size);
  //sceIoWrite(1, msg, strlen(msg));
  printf(msg);
#endif
}

void texman_clear(void) {
  memset(textures, 0, sizeof(textures));
  tex_number = 0;
  tex_buffer = tex_buffer_start;
#ifdef DEBUG
  char msg[64];
  sprintf(msg, "TEXMAN: clear %p size %d bytes!\n", tex_buffer, TEXMAN_BUFFER_SIZE);
  //sceIoWrite(1, msg, strlen(msg));
  printf(msg);
#endif
}

void texman_set_buffer(void *buf, uint32_t size) {
  tex_buffer = buf;
  tex_buffer_max = buf + size;
}

int texman_get_space_available(void) {
  return (tex_buffer_max - tex_buffer);
}

int texman_is_space_available(void) {
  return (tex_buffer_max - tex_buffer) > (32 * 1024);
}

unsigned char *texman_get_tex_data(uint32_t num) {
  return textures[num].location;
}

struct Simple_Texture *texman_reserve_memory(uint32_t width, uint32_t height, int bpp) {
  if(texman_is_space_available()){
  size_t tex_size = getMemorySize(width, height, bpp);
  tex_buffer =
      (void *)((((size_t)tex_buffer + tex_size + TEX_ALIGNMENT - 1) / TEX_ALIGNMENT) * TEX_ALIGNMENT);
#ifdef DEBUG
  printf("TEX_MAN: tex [%d] reserved %d bytes @ %x left: %d kb\n", tex_number, tex_size,
         (uint32_t)textures[tex_number].location,
         (tex_buffer_max - tex_buffer) / 1024);
#endif
  } else {
    printf("TEX_MAN: potential memory overrun! free space: %d bytes\n", texman_get_space_available());
  }
  return &textures[tex_number];
}

uint32_t texman_create(void) {
  tex_number++;
  textures[tex_number] = (struct Simple_Texture){
    location : tex_buffer,
    width : 0,
    height : 0
  };

#ifdef DEBUG
  printf("TEX_MAN: new tex [%d] @ %x\n", tex_number, tex_buffer);
#endif
  return tex_number;
}

void texman_upload(uint32_t width, uint32_t height, int bpp, const void *buffer) {
  struct Simple_Texture *current = texman_reserve_memory(width, height, bpp);
  current->width = width;
  current->height = height;
  memcpy(current->location, buffer, getMemorySize(width, height, bpp));
#ifdef DEBUG
  // printf("TEX_MAN upload plain [%d]\n", tex_number);
#endif
}
