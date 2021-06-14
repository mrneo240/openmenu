/*
 * File: draw_kos.c
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:33:03 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "draw_kos.h"

#include <stdio.h>

#include "../inc/dat_format.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

//#define KOS_SPRITE (1)

extern int round(float x);

image txr_highlight, txr_bg; /* Highlight square and Background */
image img_empty_boxart;

static float z_depth;
float z_get(void) {
  return z_depth;
}
float z_set(float z) {
  z_depth = z;
  return z_depth;
}
void z_reset(void) {
  z_depth = 1.0f;
}
float z_inc(void) {
  z_depth += 1.0f;

  /* 512 Puts Blit infront of everything*/
  if (z_depth > 512.0f) {
    z_depth = 512.0f;
  }
  return z_depth;
}

/* Called only once at start */
void draw_init(void) {
  draw_load_texture("HIGHLIGHT.PVR", &txr_highlight);
  draw_load_texture("BG_RIGHT.PVR", &txr_bg);

  font_init();

  pvr_ptr_t txr = load_pvr("EMPTY.PVR", &img_empty_boxart.width, &img_empty_boxart.height, &img_empty_boxart.format);
  img_empty_boxart.texture = txr;

  z_reset();
}

/* called at the start of each frame */
void draw_setup(void) {
}

void* draw_load_missing_icon(void* user) {
  image* img = (image*)user;
  img->texture = img_empty_boxart.texture;
  img->width = img_empty_boxart.width;
  img->height = img_empty_boxart.height;
  img->format = img_empty_boxart.format;
  return img;
}

/* Throws ID into id and returns something if needs to*/
void* draw_load_texture(const char* filename, void* user) {
  image* img = (image*)user;

  pvr_ptr_t txr;

  if (!(txr = load_pvr(filename, &img->width, &img->height, &img->format))) {
    img->texture = img_empty_boxart.texture;
    img->width = img_empty_boxart.width;
    img->height = img_empty_boxart.height;
    img->format = img_empty_boxart.format;
    return img;
  }
  img->texture = txr;

  return user;
}

void* draw_load_texture_buffer(const char* filename, void* user, void* buffer) {
  image* img = (image*)user;

  pvr_ptr_t txr;

  if (!(txr = load_pvr_to_buffer(filename, &img->width, &img->height, &img->format, buffer))) {
    img->texture = img_empty_boxart.texture;
    img->width = img_empty_boxart.width;
    img->height = img_empty_boxart.height;
    img->format = img_empty_boxart.format;
    return img;
  }
  img->texture = txr;

  return user;
}

void* draw_load_texture_from_DAT_to_buffer(struct dat_file* bin, const char* ID, void* user, void* buffer) {
  image* img = (image*)user;
  pvr_ptr_t txr;
  int ret = DAT_read_file_by_ID(bin, ID, pvr_get_internal_buffer());
  if (!ret) {
    img->texture = img_empty_boxart.texture;
    img->width = img_empty_boxart.width;
    img->height = img_empty_boxart.height;
    img->format = img_empty_boxart.format;
    return img;
  } else {
    txr = load_pvr_from_buffer_to_buffer(pvr_get_internal_buffer(), &img->width, &img->height, &img->format, buffer);
    img->texture = txr;

    return user;
  }
}

// 0x1c + 0x164 =
/* draws an image at coords of a given size */
void draw_draw_image(int x, int y, float width, float height, float alpha, void* user) {
  image* img = (image*)user;

  if (img->width == 0 || img->height == 0) {
    return;
  }

  /* Upper left */
  const float x1 = round((float)x);
  const float y1 = round((float)y);
  const float u1 = 0.f;
  const float v1 = 0.f;

  /* Lower right */
  const float x2 = round((float)x + width);
  const float y2 = round((float)y + height);
  const float u2 = 1.0f;
  const float v2 = 1.0f;

  const float z = z_inc();

#ifdef KOS_SPRITE
  pvr_sprite_cxt_t context;
  pvr_sprite_hdr_t header;

  pvr_sprite_cxt_txr(&context, PVR_LIST_TR_POLY, img->format, img->width, img->width, img->texture, PVR_FILTER_BILINEAR);
  pvr_sprite_compile(&header, &context);

  pvr_prim(&header, sizeof(header));

  pvr_sprite_txr_t vert = {
      .flags = PVR_CMD_VERTEX_EOL, /* Always? */
      /*  upper left */
      .ax = x1,
      .ay = y1,
      .az = z,
      /* upper right */
      .bx = x2,
      .by = y1,
      .bz = z,
      /* lower left */
      .cx = x2,
      .cy = y2,
      .cz = z,
      /* interpolated */
      .dx = x1,
      .dy = y2,
      .auv = PVR_PACK_16BIT_UV(u1, v1), /* UVS */
      .buv = PVR_PACK_16BIT_UV(u2, v1), /* UVS */
      .cuv = PVR_PACK_16BIT_UV(u2, v2), /* UVS */
  };
  pvr_prim(&vert, sizeof(vert));

#else
  pvr_poly_cxt_t context;
  pvr_poly_hdr_t header;

  pvr_poly_cxt_txr(&context, PVR_LIST_TR_POLY, img->format, img->width, img->width, img->texture, PVR_FILTER_BILINEAR);
  pvr_poly_compile(&header, &context);

  pvr_prim(&header, sizeof(header));

  pvr_vertex_t vert = {
      .argb = PVR_PACK_COLOR(alpha, 1.0f, 1.0f, 1.0f),
      .oargb = 0,
      .flags = PVR_CMD_VERTEX,
      .z = 1};

  vert.x = x1;
  vert.y = y2;
  vert.z = z;
  vert.u = u1;
  vert.v = v2;
  pvr_prim(&vert, sizeof(vert));

  vert.x = x1;
  vert.y = y1;
  vert.u = u1;
  vert.v = v1;
  pvr_prim(&vert, sizeof(vert));

  vert.x = x2;
  vert.y = y2;
  vert.u = u2;
  vert.v = v2;
  pvr_prim(&vert, sizeof(vert));

  vert.flags = PVR_CMD_VERTEX_EOL;
  vert.x = x2;
  vert.y = y1;
  vert.u = u2;
  vert.v = v1;
  pvr_prim(&vert, sizeof(vert));
#endif
}

/* draws an image at coords as a square */
void draw_draw_square(int x, int y, float size, float alpha, void* user) {
  draw_draw_image(x, y, size, size, alpha, user);
}