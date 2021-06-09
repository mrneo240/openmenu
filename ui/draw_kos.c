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

#include "../dat_format.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

image txr_highlight, txr_bg; /* Highlight square and Background */
image img_empty_boxart;

/* Called only once at start */
void draw_init(void) {

  draw_load_texture("/cd/highlight.pvr", &txr_highlight);
  draw_load_texture("/cd/bg_right.pvr", &txr_bg);

  font_init();

  pvr_ptr_t txr = load_pvr("/cd/empty.pvr", &img_empty_boxart.width, &img_empty_boxart.height, &img_empty_boxart.format);
  img_empty_boxart.texture = txr;
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

/* draws an image at coords of a given size */
void draw_draw_image(unsigned int sx, unsigned int sy, float width, float height, float alpha, void* user) {
  /*

	This Function creates & Displays a rectangle polygon(useing a triangle strip) with textures at any position
	you would like.

	sx, sy is the position that you would like to display the rectangle polygon on the
	screen at.

	Size is the total texture size e.g 256x256 is just 256 this assumes that your textures are square.

	Sizew, Sizeh is the size of the polygon  * Note: This does not have to be
	the same as the above in that case the texture will be stretched to fit the poly.

	x, y start a 1,1 NOT 0, 0  x2, y2 start at 64, 64 not 63, 63
	x, y / x2, y2 is the start/end position of the texture postion given in REAL
	Bitmap pixel format this creates a box and grabs that data from the texture map to display.

*/
  image* img = (image*)user;
  const float z = 512;

  /* Create command for sending textured triangle strips with alpha masking. */
  pvr_poly_cxt_t context; /* This is just a convenience function for creating the following. */
  pvr_poly_hdr_t header;  /* This is sent to the PVR before geometry is sent. */

  if (img->width == 0 && img->height == 0) {
    return;
  }

  pvr_poly_cxt_txr(&context, PVR_LIST_TR_POLY, img->format, img->width, img->width, img->texture, PVR_FILTER_BILINEAR);
  pvr_poly_compile(&header, &context);

  pvr_prim(&header, sizeof(header));

  pvr_vertex_t vert = {
      .argb = PVR_PACK_COLOR(alpha, 1.0f, 1.0f, 1.0f),
      .oargb = 0,
      .flags = PVR_CMD_VERTEX,
      .z = 1};

  vert.x = sx;
  vert.y = height + sy;
  vert.z = z; /* 512 Puts Blit infront of everything*/
  vert.u = 0.0f;
  vert.v = 1.0f;
  pvr_prim(&vert, sizeof(vert));

  vert.x = sx;
  vert.y = sy;
  vert.u = 0.0f;
  vert.v = 0.0f;
  pvr_prim(&vert, sizeof(vert));

  vert.x = width + sx;
  vert.y = height + sy;
  vert.u = 1.0f;
  vert.v = 1.0f;
  pvr_prim(&vert, sizeof(vert));

  vert.flags = PVR_CMD_VERTEX_EOL;
  vert.x = width + sx;
  vert.y = sy;
  vert.u = 1.0f;
  vert.v = 0.0f;
  pvr_prim(&vert, sizeof(vert));
}

/* draws an image at coords as a square */
void draw_draw_square(unsigned int sx, unsigned int sy, float size, float alpha, void* user) {
  /*

	This Function creates & Displays a rectangle polygon(useing a triangle strip) with textures at any position
	you would like.

	sx, sy is the position that you would like to display the rectangle polygon on the
	screen at.

	Size is the total texture size e.g 256x256 is just 256 this assumes that your textures are square.

	Sizew, Sizeh is the size of the polygon  * Note: This does not have to be
	the same as the above in that case the texture will be stretched to fit the poly.

	x, y start a 1,1 NOT 0, 0  x2, y2 start at 64, 64 not 63, 63
	x, y / x2, y2 is the start/end position of the texture postion given in REAL
	Bitmap pixel format this creates a box and grabs that data from the texture map to display.

*/
  image* img = (image*)user;
  const float z = 512;

  if (img->width == 0 && img->height == 0) {
    return;
  }

  /* Create command for sending textured triangle strips with alpha masking. */
  pvr_poly_cxt_t context; /* This is just a convenience function for creating the following. */
  pvr_poly_hdr_t header;  /* This is sent to the PVR before geometry is sent. */

  /* Define which palette, texture and polyon list are to be used (punchthru for alpha masking here).
	 * Filtering is disabled because sprites are displayed 1:1.
	 */
  pvr_poly_cxt_txr(&context, PVR_LIST_TR_POLY, img->format, img->width, img->width, img->texture, PVR_FILTER_BILINEAR);
  pvr_poly_compile(&header, &context);

  pvr_prim(&header, sizeof(header));

  pvr_vertex_t vert = {
      .argb = PVR_PACK_COLOR(alpha, 1.0f, 1.0f, 1.0f),
      .oargb = 0,
      .flags = PVR_CMD_VERTEX,
      .z = 1};

  vert.x = sx;
  vert.y = size + sy;
  vert.z = z; /* 512 Puts Blit infront of everything*/
  vert.u = 0.0f;
  vert.v = 1.0f;
  pvr_prim(&vert, sizeof(vert));

  vert.x = sx;
  vert.y = sy;
  vert.u = 0.0f;
  vert.v = 0.0f;
  pvr_prim(&vert, sizeof(vert));

  vert.x = size + sx;
  vert.y = size + sy;
  vert.u = 1.0f;
  vert.v = 1.0f;
  pvr_prim(&vert, sizeof(vert));

  vert.flags = PVR_CMD_VERTEX_EOL;
  vert.x = size + sx;
  vert.y = sy;
  vert.u = 1.0f;
  vert.v = 0.0f;
  pvr_prim(&vert, sizeof(vert));
}