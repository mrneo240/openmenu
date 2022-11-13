/*
 * File: font_bios.c
 * Project: dc
 * File Created: Thursday, 13th October 2022 5:53:53 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2022 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-Clause "New" or "Revised" License, https://opensource.org/licenses/BSD-3-Clause
 */
#include <dc/pvr.h>

#include "../../inc/dbgprint.h"
#include "../draw_prototypes.h"

#define BUFFER_MAX_CHARS (128)

#ifdef KOS_SPRITE
static pvr_sprite_hdr_t font_header;
#define VERT_PER_CHAR (1)
static pvr_sprite_txr_t charbuf[BUFFER_MAX_CHARS * VERT_PER_CHAR] __attribute__((aligned(32)));
#else
static pvr_poly_hdr_t font_header;
#define VERT_PER_CHAR (4)
static pvr_vertex_t charbuf[BUFFER_MAX_CHARS * VERT_PER_CHAR] __attribute__((aligned(32)));
#endif

static int charbuffered;
static uint32_t font_color;
static pvr_ptr_t texture;

void font_bios_set_color(uint32_t color) {
  /*@Note: Either lxdream-nitro weirdness or something is wrong in how we draw, set both to 0xFFFFFFFF */
  font_color = color;
#ifdef KOS_SPRITE
  font_header.argb = color;
#endif
  /* Start a textured polygon set (with the font texture and color) */
  pvr_prim(&font_header, sizeof(font_header));
}

void font_bios_set_color_default(void) {
  font_bios_set_color(0xFFFFFFFF);
}

void font_bios_set_color_components(int r, int g, int b, int a) {
  font_color = PVR_PACK_ARGB(a, r, g, b);
}

extern void *get_romfont_address();
__asm__(
    "\
			\n\
.globl _romfont_address \n\
_get_romfont_address:	\n\
    mov.l 1f,r0		\n\
    mov.l @r0,r0	\n\
    jmp @r0		\n\
    mov #0,r1		\n\
    .align 2		\n\
1:  .long 0x8c0000b4	\n\
			\n\
");

static void _bfont_draw(void *buffer, int bufwidth, int opaque, int c) {
  int i, j;
  uint8 *fa = (uint8 *)get_romfont_address();

  /* By default, map to a space */
  uint32 index = 72 << 2;

  /* 33-126 in ASCII are 1-94 in the font */
  if (c >= 33 && c <= 126)
    index = c - 32;

  /* 160-255 in ASCII are 96-161 in the font */
  else if (c >= 160 && c <= 255)
    index = c - (160 - 96);

  unsigned char *s = fa + index * (12 * 24 / 8);

  const int yalign = bufwidth;
  uint16 *dst = (uint16 *)buffer;
  int bits;
  for (i = 0; i < 12; i++) {
    bits = *s++ << 16;
    bits |= *s++ << 8;
    bits |= *s++ << 0;
    for (j = 0; j < 12; j++, bits <<= 1)
      if (bits & 0x800000) {
        dst[j] = 0xffff;
        dst[j + 1] = 0xffff;
        dst[j + 2] = 0xa108;
        dst[j + yalign] = 0xa108;
        dst[j + yalign + 1] = 0xa108;
      }
    dst += yalign;
    for (j = 0; j < 12; j++, bits <<= 1)
      if (bits & 0x800000) {
        dst[j] = 0xffff;
        dst[j + 1] = 0xffff;
        dst[j + 2] = 0xa108;
        dst[j + yalign] = 0xa108;
        dst[j + yalign + 1] = 0xa108;
      }

    dst += yalign;
  }
}

void font_bios_init() {
  int x, y;
  unsigned int temp = texman_create();
  unsigned short *tex = (unsigned short *)texman_get_tex_data(temp);
  texture = tex;
  texman_reserve_memory(256, 256, 2 /* 16Bit */);
  unsigned short *vram = (unsigned short *)tex;

  for (y = 0; y < 8; y++) {
    for (x = 0; x < 16; x++) {
      _bfont_draw(vram, 256, 0, y * 16 + x);
      vram += 16;
    }
    vram += 24 * 256;
  }
}

void font_bios_begin_draw() {
  /* Make a polygon header */
#ifdef KOS_SPRITE
  pvr_sprite_cxt_t tmp;
  pvr_sprite_cxt_txr(&tmp, draw_get_list(), PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED, 256, 256, texture, PVR_FILTER_NONE);
  pvr_sprite_compile(&font_header, &tmp);
#else
  pvr_poly_cxt_t tmp;
  pvr_poly_cxt_txr(&tmp, draw_get_list(), PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED, 256, 256, texture, PVR_FILTER_NONE);
  tmp.gen.culling = PVR_CULLING_NONE;
  tmp.txr.env = PVR_TXRENV_MODULATE;
  pvr_poly_compile(&font_header, &tmp);
#endif
}

static void draw_bios_poly_char(float x1, float y1, int c) {
  pvr_vertex_t *vert1, *vert2, *vert3, *vert4;
  vert1 = &charbuf[charbuffered + 0];
  vert2 = &charbuf[charbuffered + 1];
  vert3 = &charbuf[charbuffered + 2];
  vert4 = &charbuf[charbuffered + 3];

  int ix = (c % 16) * 16;
  int iy = (c / 16) * 25;
  float u1 = ix * 1.0f / 256.0f;
  float v1 = iy * 1.0f / 256.0f;
  float u2 = (ix + 14) * 1.0f / 256.0f;
  float v2 = (iy + 25) * 1.0f / 256.0f;

  const float x2 = x1 + 14;
  const float y2 = y1 + 25;
  const float z = z_get();

  vert1->flags = PVR_CMD_VERTEX;
  vert1->x = x1;
  vert1->y = y2;
  vert1->z = z;
  vert1->u = u1;
  vert1->v = v2;
  vert1->argb = font_color;
  vert1->oargb = 0;

  vert2->flags = PVR_CMD_VERTEX;
  vert2->x = x1;
  vert2->y = y1;
  vert2->z = z;
  vert2->u = u1;
  vert2->v = v1;
  vert2->argb = font_color;
  vert2->oargb = 0;

  vert3->flags = PVR_CMD_VERTEX;
  vert3->x = x2;
  vert3->y = y2;
  vert3->z = z;
  vert3->u = u2;
  vert3->v = v2;
  vert3->argb = font_color;
  vert3->oargb = 0;

  vert4->flags = PVR_CMD_VERTEX_EOL;
  vert4->x = x2;
  vert4->y = y1;
  vert4->z = z;
  vert4->u = u2;
  vert4->v = v1;
  vert4->argb = font_color;
  vert4->oargb = 0;

  charbuffered += VERT_PER_CHAR;
}

static void _font_bios_draw_string(int x1, int y1, const char *str) {
  const char *s;

  charbuffered = 0;
  z_inc();

  if (x1 == 0.0f) {
    int l = 0;
    int space = 0;
    size_t i;
    for (i = 0; i < strlen(str); i++) {
      if (str[i] != ' ')
        l++;
      else
        space++;
    }

    float len = l * 14.0f + space * 10.0f;
    x1 = (640.0f - len) / 2;
  }

  s = str;
  while (*s) {
    if (*s == ' ') {
      x1 += 10.0f;
    } else {
      draw_bios_poly_char(x1 += 14.0f, y1, *s);
    }
    s++;
  }

  pvr_prim(charbuf, charbuffered * sizeof(charbuf[0]));
}

void font_bios_draw_main(int x1, int y1, const char *str) {
  _font_bios_draw_string(x1, y1, str);
}
