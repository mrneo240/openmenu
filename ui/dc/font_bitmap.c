/*
 * File: font.c
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:00:17 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */
#include <dc/pvr.h>

#include "../../gdrom/gdrom_fs.h"
#include "../../inc/dbgprint.h"
#include "../draw_prototypes.h"

typedef struct bitmap_font {
  int char_width;
  int char_height;
  image texture;
} bitmap_font;

static bitmap_font font;
static uint32_t font_color;

#define FONT_PERROW(font) (font.texture.width / font.char_width)

static pvr_poly_hdr_t font_header;

int font_bmp_init(const char *filename, int char_width, int char_height) {
  pvr_ptr_t txr;
  pvr_poly_cxt_t tmp;

  if (!(txr = load_pvr(filename, &font.texture.width, &font.texture.height, &font.texture.format)))
    return 1;
  font.texture.texture = txr;

  /* Make a polygon header */
  pvr_poly_cxt_txr(&tmp, PVR_LIST_TR_POLY, font.texture.format, font.texture.width, font.texture.height, font.texture.texture, PVR_FILTER_NONE);
  pvr_poly_compile(&font_header, &tmp);

  font.char_height = char_height;
  font.char_width = char_width;

  font_color = 0xFFFFFFFF;  //White

  return 0;
}

void font_bmp_begin_draw() {
  /* Start a textured polygon set (with the font texture) */
  pvr_prim(&font_header, sizeof(font_header));
}

void font_bmp_set_color(uint32_t color) {
  font_color = color;
}

void font_bmp_set_color_components(int r, int g, int b, int a) {
  font_color = PVR_PACK_ARGB(a, r, g, b);
}

/* Draws a font letter using two triangle strips */
static void font_bmp_draw_char(int x, int y, float color, unsigned char ch, char width) {
  (void)width;

  pvr_vertex_t vert;

  const int index = ch - 32;
  const int ix = (index % FONT_PERROW(font)) * font.char_width;
  const int iy = (index / FONT_PERROW(font)) * font.char_height;

  /* Upper left */
  const float x1 = x;
  const float y1 = y;
  const float u1 = ix * 1.0f / font.texture.width;
  const float v1 = iy * 1.0f / font.texture.height;

  /* Lower right */
  const float x2 = x1 + font.char_width;
  const float y2 = y1 + font.char_height;
  const float u2 = (ix + font.char_width) * 1.0f / font.texture.width;
  const float v2 = (iy + font.char_height) * 1.0f / font.texture.height;

  if (index == -1)
    return;

  vert.flags = PVR_CMD_VERTEX;
  vert.x = x1;
  vert.y = y2;
  vert.z = z_get();
  vert.u = u1;
  vert.v = v2;
  vert.argb = font_color;
  vert.oargb = 0;
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
}

static void _font_bmp_draw_string(int x1, int y1, float color, const char *str, char unused) {
  (void)unused;

  z_inc();
  while (*str) {
    //unsigned char chr = (*str) + (font * 128);
    unsigned char chr = (*str);
    font_bmp_draw_char(x1, y1, color, chr, 0);
    x1 += (int)(font.char_width);
    (void)*str++;
  }
}

void font_bmp_draw_sub_wrap(int x1, int y1, float color, int width, const char *str) {
#if 0
  int x_start = x1;
  while (*str) {
    unsigned char chr = (*str) + (1 * 128);
    font_draw_char(x1, y1, color, chr, 0);
    x1 += (int)(char_widths[chr - 32] * FONT_SIZE_SUB + 1);
    if ((x1 - x_start >= width) && (*str == ' ')) {
      y1 += (FONT_HEIGHT * FONT_SIZE_SUB) + 4;
      x1 = x_start;
    }
    (void)*str++;
  }
#endif
  _font_bmp_draw_string(x1, y1, color, str, 0);
}

void font_bmp_draw_main(int x1, int y1, float color, const char *str) {
  _font_bmp_draw_string(x1, y1, color, str, 0);
}

void font_draw_sub(int x1, int y1, float color, const char *str) {
  _font_bmp_draw_string(x1, y1, color, str, 1);
}