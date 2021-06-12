/*
 * File: font.c
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:00:17 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */

#include <dc/pvr.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../draw_prototypes.h"

//#define DEBUG (1)
#include "../../dbgprint.h"

/* Used to read from GDROM instead of cdrom */
#define GDROM_FS (1)
#include "../../gdrom/gdrom_fs.h"

/* Changes from using Polys to native Sprites */
//#define KOS_SPRITE (1)

#define PRINT_MEMBER(struct, member)                        \
  do {                                                      \
    DBG_PRINT("%s: %d\n", #member, (int)((struct).member)); \
  } while (0)

extern int round(float x);

/* BMFont implementation */
#define BMF_MAGIC (54938946) /* BMF(0x3) */

typedef struct bm_header {
  char bmf[3];
  uint8_t version;
} bm_header;

/* Binary font info structures */

// Tag header for each block type
typedef enum BM_BLOCK_TYPE { INFO = 1,
                             COMMON,
                             PAGES,
                             CHARS,
                             KERNING } BM_BLOCK_TYPE;

typedef struct __attribute__((__packed__)) bm_block_tag {
  uint8_t type;
  uint32_t size;
} bm_block_tag;

// Block type 1: info
#define INFO_NAME_OFFSET (14)
typedef struct __attribute__((__packed__)) bm_info {
  int16_t fontSize;
  uint8_t bitField;
  uint8_t charSet;
  uint16_t stretchH;
  uint8_t aa;
  uint8_t paddingUp;
  uint8_t paddingRight;
  uint8_t paddingDown;
  uint8_t paddingLeft;
  uint8_t spacingHoriz;
  uint8_t spacingVert;
  uint8_t outline;
  char fontName[/*n+1*/]; /* VLA, size of block length - 14, null terminated string with length n */
} bm_info;

// Block type 2: common
typedef struct __attribute__((__packed__)) bm_common {
  uint16_t lineHeight;
  uint16_t base;
  uint16_t scaleW;
  uint16_t scaleH;
  uint16_t pages;
  uint8_t bitField;  //bits 0-6: reserved, bit 7: packed
  uint8_t alphaChnl;
  uint8_t redChnl;
  uint8_t greenChnl;
  uint8_t blueChnl;
} bm_common;

// Block type 3: pages
typedef struct __attribute__((__packed__)) bm_pages {
  int name_len;
  char pageNames[]; /*	p*(n+1)	strings	0	p null terminated strings, each with length n */
} bm_pages;

// Block type 4: chars
typedef struct __attribute__((__packed__)) bm_char {
  uint32_t id;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  int16_t xoffset;
  int16_t yoffset;
  int16_t xadvance;
  uint8_t page;
  uint8_t chnl;
} bm_char;

// Block type 5: kerning pairs
typedef struct __attribute__((__packed__)) bm_kern_pair {
  uint32_t first;
  uint32_t second;
  int16_t amount;
} bm_kern_pair;

typedef struct bm_font {
  uint16_t height;
  uint16_t width;
  uint32_t lineHeight;
  uint32_t fontSize;
  uint32_t num_chars;
  uint32_t num_kerns;
  bm_char chars[256];
  bm_kern_pair *kerns;
} bm_font;

static bm_font font_basilea;

static int BMF_parse_info(FD_TYPE fd, size_t block_size, bm_font *font) {
  DBG_PRINT("BMF found info block!\n");
  bm_info *temp_info = malloc(sizeof(bm_info) + sizeof(char) * (block_size - INFO_NAME_OFFSET));
  fread(temp_info, block_size, 1, fd);

  /* Fix fontSize, its negative ? */
  temp_info->fontSize *= -1;

  font->fontSize = temp_info->fontSize;

  PRINT_MEMBER(*temp_info, fontSize);
  PRINT_MEMBER(*temp_info, bitField);
  PRINT_MEMBER(*temp_info, charSet);
  PRINT_MEMBER(*temp_info, stretchH);
  PRINT_MEMBER(*temp_info, aa);
  PRINT_MEMBER(*temp_info, paddingUp);
  PRINT_MEMBER(*temp_info, paddingRight);
  PRINT_MEMBER(*temp_info, paddingDown);
  PRINT_MEMBER(*temp_info, paddingLeft);
  PRINT_MEMBER(*temp_info, spacingHoriz);
  PRINT_MEMBER(*temp_info, spacingVert);
  PRINT_MEMBER(*temp_info, outline);
  DBG_PRINT("name: %s\n", temp_info->fontName);
  DBG_PRINT("\n");

  free(temp_info);
  return 0;
}

static int BMF_parse_common(FD_TYPE fd, size_t block_size, bm_font *font) {
  DBG_PRINT("BMF found common block!\n");
  bm_common temp_common;
  fread(&temp_common, block_size, 1, fd);

  font->width = temp_common.scaleW;
  font->height = temp_common.scaleH;
  font->lineHeight = temp_common.lineHeight;

  PRINT_MEMBER(temp_common, lineHeight);
  PRINT_MEMBER(temp_common, base);
  PRINT_MEMBER(temp_common, scaleW);
  PRINT_MEMBER(temp_common, scaleH);
  PRINT_MEMBER(temp_common, pages);
  PRINT_MEMBER(temp_common, bitField);
  PRINT_MEMBER(temp_common, alphaChnl);
  PRINT_MEMBER(temp_common, redChnl);
  PRINT_MEMBER(temp_common, greenChnl);
  PRINT_MEMBER(temp_common, blueChnl);

  DBG_PRINT("\n");
  return 0;
}

static int BMF_parse_pages(FD_TYPE fd, size_t block_size, bm_font *font) {
  DBG_PRINT("BMF found pages block!\n");
  (void)font;

  /*
  bm_pages *temp_pages = malloc(sizeof(bm_pages) + sizeof(char) * (block_size));
  temp_pages->name_len = block_size;
  fread(&temp_pages->pageNames, block_size, 1, fd);
  free(temp_pages);
  */

  /* Do nothing! */
  fseek(fd, block_size, SEEK_CUR);

  DBG_PRINT("\n");

  return 0;
}

static int BMF_parse_chars(FD_TYPE fd, size_t block_size, bm_font *font) {
  DBG_PRINT("BMF found char block!\n");

  int num_chars = block_size / sizeof(bm_char);
  bm_char temp_char;

  font->num_chars = num_chars;

  DBG_PRINT("BMF %d chars present\n", num_chars);

  /* Read one at a time to font charset */
  for (int i = 0; i < num_chars; i++) {
    fread(&temp_char, sizeof(bm_char), 1, fd);
    if (temp_char.id < 256) {
      /* Optionally print out info for each char parsed */
#if DBG_CHAR_INFO
      char temp = (char)temp_char.id;
      DBG_PRINT("Char: %c\n", temp);
      PRINT_MEMBER(temp_char, id);
      PRINT_MEMBER(temp_char, x);
      PRINT_MEMBER(temp_char, y);
      PRINT_MEMBER(temp_char, width);
      PRINT_MEMBER(temp_char, height);
      PRINT_MEMBER(temp_char, xoffset);
      PRINT_MEMBER(temp_char, yoffset);
      PRINT_MEMBER(temp_char, xadvance);
      DBG_PRINT("\n");
#endif

      font->chars[temp_char.id] = temp_char;
    }
  }

  DBG_PRINT("\n");
  return 0;
}

static int BMF_parse_kerning(FD_TYPE fd, size_t block_size, bm_font *font) {
  DBG_PRINT("BMF found kerning block!\n");

  /* Do nothing! */
  //fseek(fd, block_size, SEEK_CUR);

  /* Parse and save */
  int num_pairs = block_size / sizeof(bm_kern_pair);
  bm_kern_pair temp_pair;

  DBG_PRINT("BMF %d kerning pairs present\n", num_pairs);

  font->kerns = malloc(sizeof(bm_kern_pair) * num_pairs);
  font->num_kerns = num_pairs;

  /* Read one at a time to font kerning set */
  for (int i = 0; i < num_pairs; i++) {
    fread(&temp_pair, sizeof(temp_pair), 1, fd);

#if DBG_KERN_INFO
    char first = (char)temp_pair.first;
    char second = (char)temp_pair.second;
    DBG_PRINT("First: %c\n", first);
    DBG_PRINT("Second: %c\n", second);
    PRINT_MEMBER(temp_pair, amount);
    DBG_PRINT("\n");
#endif

    font->kerns[i] = temp_pair;
  }

  DBG_PRINT("\n");
  return 0;
}

/* BMFont specifics */
static int BMF_load(const char *file, bm_font *font) {
  // Zero out
  memset(font, '\0', sizeof(bm_font));

  FD_TYPE fd = (FD_TYPE)NULL;
  bool parsing = true;
  fd = fopen(file, "rb");

  if (fd <= 0) {
    printf("ERR: font \"%s\" missing err(%d)!\n", file, fd);
    return 1;
  }

  /*
  uint32_t file_magic = 0;
  fread(&file_magic, sizeof(file_magic), 1, fd);
  if (file_magic != BMF_MAGIC) {
    fclose(fd);
    printf("ERR: font magic wrong %8x!\n", &file_magic);
    return 1;
  }
  */
  bm_header file_header;
  fread(&file_header, sizeof(bm_header), 1, fd);
  if (file_header.version != 3) {
    fclose(fd);
    printf("ERR: font magic wrong %3s!\n", file_header.bmf);
    return 1;
  }

  bm_block_tag next_block = {0, 0};
  size_t ele_read;
  while (parsing) {
    ele_read = fread(&next_block, sizeof(bm_block_tag), 1, fd);
    DBG_PRINT("Found block type %d of size %ld\n", next_block.type, next_block.size);
#ifdef GDROM_FS
    if (ele_read != (sizeof(bm_block_tag) * 1)) {
      break;
    }
#else
    if (ele_read != 1) {
      break;
    }
#endif

    switch (next_block.type) {
      case INFO:
        BMF_parse_info(fd, next_block.size, font);
        break;
      case COMMON:
        BMF_parse_common(fd, next_block.size, font);
        break;
      case PAGES:
        BMF_parse_pages(fd, next_block.size, font);
        break;
      case CHARS:
        BMF_parse_chars(fd, next_block.size, font);
        break;
      case KERNING:
        BMF_parse_kerning(fd, next_block.size, font);
        parsing = false; /* should be end of file */
        break;
    }
  }

  fclose(fd);
  return 0;
}

int BMF_adjust_kerning(unsigned char first, unsigned char second, bm_font *font) {
  const int32_t num_kerns = font->num_kerns;
  for (int i = 0; i < num_kerns; i++) {
    if (font->kerns[i].first == first && font->kerns[i].second == second) {
      return font->kerns[i].amount;
    }
  }
  return 0;
}

static float current_height;
static float current_scale;

void font_set_height_default(void) {
  current_height = font_basilea.fontSize;
  current_scale = 1.0f;
}
void font_set_height(float height) {
  current_height = height;
  current_scale = height / font_basilea.fontSize;
}

#ifdef KOS_SPRITE
static pvr_sprite_hdr_t font_header;
#else
static pvr_poly_hdr_t font_header;
#endif
static image font_texture;

/* Font prototype generics */
int font_init(void) {
  pvr_ptr_t txr;
  int ret = 0;

  ret += BMF_load(DISC_PREFIX "FONT.FNT", &font_basilea);

  if (!(txr = load_pvr("FONT_0.PVR", &font_texture.width, &font_texture.height, &font_texture.format)))
    return 1;
  font_texture.texture = txr;

  /* Make a polygon header */
#ifdef KOS_SPRITE
  pvr_sprite_cxt_t tmp;
  pvr_sprite_cxt_txr(&tmp, PVR_LIST_TR_POLY, font_texture.format, font_texture.width, font_texture.height, font_texture.texture, PVR_FILTER_BILINEAR);
  pvr_sprite_compile(&font_header, &tmp);
#else
  pvr_poly_cxt_t tmp;
  pvr_poly_cxt_txr(&tmp, PVR_LIST_TR_POLY, font_texture.format, font_texture.width, font_texture.height, font_texture.texture, PVR_FILTER_BILINEAR);
  pvr_poly_compile(&font_header, &tmp);
#endif

  return ret;
}

void font_destroy(void) {
  /*@Todo: release safely */
}

void font_begin_draw(void) {
  font_set_height_default();
  pvr_prim(&font_header, sizeof(font_header));
}

/* Draws a font letter using two triangle strips */
int font_draw_char(int x, int y, float color, unsigned char chr) {
  (void)color;
  bm_font *font = &font_basilea;

  /* Upper left */
  const float x1 = round(x + current_scale * (float)font->chars[chr].xoffset);
  const float y1 = round(y + current_scale * (float)font->chars[chr].yoffset);
  const float u1 = (float)font->chars[chr].x / (float)font->width;
  const float v1 = (float)font->chars[chr].y / (float)font->height;

  /* Lower right */
  const float x2 = round(x + current_scale * ((float)font->chars[chr].width + font->chars[chr].xoffset));
  const float y2 = round(y + current_scale * ((float)font->chars[chr].height + font->chars[chr].yoffset));
  const float u2 = (float)(font->chars[chr].x + font->chars[chr].width) / (float)font->width;
  const float v2 = (float)(font->chars[chr].y + font->chars[chr].height) / (float)font->height;

  const float z = z_get();

#ifdef KOS_SPRITE
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
      /* interpolatied */
      .dx = x1,
      .dy = y2,
      .auv = PVR_PACK_16BIT_UV(u1, v1), /* UVS */
      .buv = PVR_PACK_16BIT_UV(u2, v1), /* UVS */
      .cuv = PVR_PACK_16BIT_UV(u2, v2), /* UVS */
  };
  pvr_prim(&vert, sizeof(vert));
#else
  pvr_vertex_t vert;

  vert.flags = PVR_CMD_VERTEX;
  vert.x = x1;
  vert.y = y2;
  vert.z = z;
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

  const float temp_advance = round(current_scale * font->chars[chr].xadvance);
  const float ret_advance = (temp_advance < 1.0f ? 1.0f : temp_advance);
  return ret_advance;
}

void _font_draw_string(int x1, int y1, float color, const char *str, char font) {
  z_inc();
  unsigned char prev = 0;
  while (*str) {
    unsigned char chr = (*str);
    /* Add possible kerning adjustment */
    x1 += round(current_scale * BMF_adjust_kerning(prev, chr, &font_basilea));
    x1 += font_draw_char(x1, y1, color, chr);

    prev = chr;
    (void)*str++;
  }

  (void)font;
}

static float _font_calculate_length(const char *str) {
  float width = 0;
  unsigned char prev = 0;
  bm_font *font = &font_basilea;

  while (*str) {
    unsigned char chr = (*str);
    /* Add possible kerning adjustment */
    width += BMF_adjust_kerning(prev, chr, &font_basilea);
    width += font->chars[chr].xadvance;

    prev = chr;
    (void)*str++;
  }

  return round(width);
}

void font_draw_auto_size(int x1, int y1, float color, const char *str, int width) {
  float temp = _font_calculate_length(str);
  font_set_height(width / temp * font_basilea.fontSize);
  _font_draw_string(x1, y1, color, str, 0);
}

void font_draw_centered(int x1, int y1, float color, const char *str) {
  int temp = (int)_font_calculate_length(str);
  _font_draw_string(x1 - (temp / 2), y1, color, str, 0);
}

void font_draw_main(int x1, int y1, float color, const char *str) {
  font_set_height_default();
  _font_draw_string(x1, y1, color, str, 0);
}

void font_draw_sub(int x1, int y1, float color, const char *str) {
  font_set_height(16.0f);
  _font_draw_string(x1, y1, color, str, 1);
}

void font_draw_sub_wrap(int x1, int y1, float color, const char *str, int width) {
  (void)x1;
  (void)y1;
  (void)color;
  (void)str;

  (void)width;

  font_draw_sub(x1, y1, color, str);
}