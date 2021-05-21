/*
 * File: font.c
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:00:17 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */

#include "font.h"

#include "text_widths.h"

int font_init() {
  pvr_ptr_t txr;
  pvr_poly_cxt_t tmp;

  if (!(txr = load_pvr("/cd/text_black.pvr", &font_texture.width, &font_texture.height, &font_texture.format)))
    return 1;
  font_texture.texture = txr;

  /* Make a polygon header */
  pvr_poly_cxt_txr(&tmp, PVR_LIST_TR_POLY, font_texture.format, font_texture.width, font_texture.height, font_texture.texture, PVR_FILTER_BILINEAR);
  pvr_poly_compile(&font_header, &tmp);

  return 0;
}

void font_begin_draw() {
  /* Start a textured polygon set (with the font texture) */
  pvr_prim(&font_header, sizeof(font_header));
}

/* Draws a font letter using two triangle strips */
void font_draw_char(int x1, int y1, float color, unsigned char ch, char width) {
  int index, ix, iy;
  float font_size = FONT_SIZE_MAIN;
  float u1, v1, u2, v2;
  pvr_vertex_t vert;

  index = ch - 32;
  ix = (index % FONT_PERROW) * FONT_WIDTH;
  iy = (index / FONT_PERROW) * FONT_HEIGHT;
  u1 = ix * 1.0f / FONT_PIC_WIDTH;
  v1 = iy * 1.0f / FONT_PIC_HEIGHT;
  u2 = (ix + width) * 1.0f / FONT_PIC_WIDTH;
  v2 = (iy + FONT_HEIGHT) * 1.0f / FONT_PIC_HEIGHT;

  if (index == -1)
    return;

  if (index >= 128) {
    font_size = FONT_SIZE_SUB;
  }

  vert.flags = PVR_CMD_VERTEX;
  vert.x = x1;
  vert.y = y1 + FONT_HEIGHT * font_size;
  vert.z = 512.0f;
  vert.u = u1;
  vert.v = v2;
  vert.argb = PVR_PACK_COLOR(color, 0.0f, 0.0f, 0.0f);
  vert.oargb = 0;
  pvr_prim(&vert, sizeof(vert));

  vert.x = x1;
  vert.y = y1;
  vert.u = u1;
  vert.v = v1;
  pvr_prim(&vert, sizeof(vert));

  vert.x = x1 + width * font_size;
  vert.y = y1 + FONT_HEIGHT * font_size;
  vert.u = u2;
  vert.v = v2;
  pvr_prim(&vert, sizeof(vert));

  vert.flags = PVR_CMD_VERTEX_EOL;
  vert.x = x1 + width * font_size;
  vert.y = y1;
  vert.u = u2;
  vert.v = v1;
  pvr_prim(&vert, sizeof(vert));
}

void _font_draw_string(int x1, int y1, float color, const char *str, char font) {
  while (*str) {
    unsigned char chr = (*str) + (font * 128);
    font_draw_char(x1, y1, color, chr, char_widths[chr - 32]);
    x1 += (int)(char_widths[chr - 32] * FONT_SIZE_MAIN + 1);
    (void)*str++;
  }
}

void font_draw_sub_wrap(int x1, int y1, float color, int width, const char *str) {
  int x_start = x1;
  while (*str) {
    unsigned char chr = (*str) + (1 * 128);
    font_draw_char(x1, y1, color, chr, char_widths[chr - 32]);
    x1 += (int)(char_widths[chr - 32] * FONT_SIZE_SUB + 1);
    if ((x1 - x_start >= width) && (*str == ' ')) {
      y1 += (FONT_HEIGHT * FONT_SIZE_SUB) + 4;
      x1 = x_start;
    }
    (void)*str++;
  }
}

void font_draw_main(int x1, int y1, float color, const char *str) {
  _font_draw_string(x1, y1, color, str, 0);
}

void font_draw_sub(int x1, int y1, float color, const char *str) {
  _font_draw_string(x1, y1, color, str, 1);
}