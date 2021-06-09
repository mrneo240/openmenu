/*
 * File: common.c
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:25:54 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */
#include <string.h>

#include "common.h"

static unsigned char* texBuf = NULL;

pvr_ptr_t load_pvr(char const* filename, uint32* w, uint32* h, uint32* txrFormat) {
  pvr_ptr_t rv;

#define PVR_HDR_SIZE 0x20
  FILE* tex = NULL;
  //unsigned char* texBuf;
  unsigned int texSize;
  const size_t filename_len = strlen(filename) + 1;

  /* replace all - with _ */
  char* filename_safe = malloc(filename_len);
  memcpy(filename_safe, filename, filename_len);
  char* iter = filename_safe;
  while (*iter++) {
    if (*iter == '-')
      *iter = '_';
  }

  tex = fopen(filename_safe, "rb");
  if (!tex) {
    printf("Failed to load image file: %s\n", filename_safe);
    return NULL;
  }

  if (!texBuf) {
    texBuf = malloc(512 * 512 * 2);
  }

  fseek(tex, 0, SEEK_END);
  texSize = ftell(tex);

  //texBuf = (unsigned char*)malloc(texSize);
  fseek(tex, 0, SEEK_SET);
  fread(texBuf, 1, texSize, tex);
  fclose(tex);

  int texW = texBuf[PVR_HDR_SIZE - 4] | texBuf[PVR_HDR_SIZE - 3] << 8;
  int texH = texBuf[PVR_HDR_SIZE - 2] | texBuf[PVR_HDR_SIZE - 1] << 8;
  int texFormat, texColor;
  int Bpp = 2;  // in Bytes

  switch ((unsigned int)texBuf[PVR_HDR_SIZE - 8]) {
    case 0x00:
      texColor = PVR_TXRFMT_ARGB1555;
      Bpp = 2;
      break;  //(bilevel translucent alpha 0,255)

    case 0x01:
      texColor = PVR_TXRFMT_RGB565;
      Bpp = 2;
      break;  //(non translucent RGB565 )

    case 0x02:
      texColor = PVR_TXRFMT_ARGB4444;
      Bpp = 2;
      break;  //(translucent alpha 0-255)

    case 0x03:
      texColor = PVR_TXRFMT_YUV422;
      Bpp = 1;
      break;  //(non translucent UYVY )

    case 0x04:
      texColor = PVR_TXRFMT_BUMP;
      Bpp = 2;
      break;  //(special bump-mapping format)

    case 0x05:
      texColor = PVR_TXRFMT_PAL4BPP;
      Bpp = 1;
      break;  //(4-bit palleted texture)

    case 0x06:
      texColor = PVR_TXRFMT_PAL8BPP;
      Bpp = 1;
      break;  //(8-bit palleted texture)

    default:
      texColor = PVR_TXRFMT_RGB565;
      Bpp = 2;
      break;
  }

  switch ((unsigned int)texBuf[PVR_HDR_SIZE - 7]) {
    case 0x01:
      texFormat = PVR_TXRFMT_TWIDDLED;
      break;  //SQUARE TWIDDLED

    case 0x03:
      texFormat = PVR_TXRFMT_VQ_ENABLE;
      break;  //VQ TWIDDLED

    case 0x09:
      texFormat = PVR_TXRFMT_NONTWIDDLED;
      break;  //RECTANGLE

    case 0x0B:
      texFormat = PVR_TXRFMT_STRIDE | PVR_TXRFMT_NONTWIDDLED;
      break;  //RECTANGULAR STRIDE

    case 0x0D:
      texFormat = PVR_TXRFMT_TWIDDLED;
      break;  //RECTANGULAR TWIDDLED

    case 0x10:
      texFormat = PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED;
      break;  //SMALL VQ

    default:
      texFormat = PVR_TXRFMT_NONE;
      break;
  }

  int txr_size = texW * texH * Bpp;

  fflush(stdout);

  if (!(rv = pvr_mem_malloc(txr_size))) {
    printf("Couldn't allocate memory for texture!\n");
    //free(texBuf);
    return NULL;
  }
  pvr_txr_load(texBuf + PVR_HDR_SIZE, rv, texW * texH * 2);

  //free(texBuf);

  *w = texW;
  *h = texH;
  *txrFormat = texFormat | texColor;

  return rv;
}

pvr_ptr_t load_pvr_to_buffer(char const* filename, uint32* w, uint32* h, uint32* txrFormat, void* buffer) {
#define PVR_HDR_SIZE 0x20
  FILE* tex = NULL;
  unsigned int texSize;
  const size_t filename_len = strlen(filename) + 1;

  /* replace all - with _ */
  char* filename_safe = malloc(filename_len);
  memcpy(filename_safe, filename, filename_len);
  char* iter = filename_safe;
  while (*iter++) {
    if (*iter == '-')
      *iter = '_';
  }

  tex = fopen(filename_safe, "rb");
  if (!tex) {
    printf("Failed to load image file: %s\n", filename_safe);
    return NULL;
  }

  if (!texBuf) {
    texBuf = malloc(512 * 512 * 2);
  }

  fseek(tex, 0, SEEK_END);
  texSize = ftell(tex);

  fseek(tex, 0, SEEK_SET);
  fread(texBuf, 1, texSize, tex);
  fclose(tex);

  int texW = texBuf[PVR_HDR_SIZE - 4] | texBuf[PVR_HDR_SIZE - 3] << 8;
  int texH = texBuf[PVR_HDR_SIZE - 2] | texBuf[PVR_HDR_SIZE - 1] << 8;
  int texFormat, texColor;
  int Bpp = 2;  // in Bytes

  switch ((unsigned int)texBuf[PVR_HDR_SIZE - 8]) {
    case 0x00:
      texColor = PVR_TXRFMT_ARGB1555;
      Bpp = 2;
      break;  //(bilevel translucent alpha 0,255)

    case 0x01:
      texColor = PVR_TXRFMT_RGB565;
      Bpp = 2;
      break;  //(non translucent RGB565 )

    case 0x02:
      texColor = PVR_TXRFMT_ARGB4444;
      Bpp = 2;
      break;  //(translucent alpha 0-255)

    case 0x03:
      texColor = PVR_TXRFMT_YUV422;
      Bpp = 1;
      break;  //(non translucent UYVY )

    case 0x04:
      texColor = PVR_TXRFMT_BUMP;
      Bpp = 2;
      break;  //(special bump-mapping format)

    case 0x05:
      texColor = PVR_TXRFMT_PAL4BPP;
      Bpp = 1;
      break;  //(4-bit palleted texture)

    case 0x06:
      texColor = PVR_TXRFMT_PAL8BPP;
      Bpp = 1;
      break;  //(8-bit palleted texture)

    default:
      texColor = PVR_TXRFMT_RGB565;
      Bpp = 2;
      break;
  }

  switch ((unsigned int)texBuf[PVR_HDR_SIZE - 7]) {
    case 0x01:
      texFormat = PVR_TXRFMT_TWIDDLED;
      break;  //SQUARE TWIDDLED

    case 0x03:
      texFormat = PVR_TXRFMT_VQ_ENABLE;
      break;  //VQ TWIDDLED

    case 0x09:
      texFormat = PVR_TXRFMT_NONTWIDDLED;
      break;  //RECTANGLE

    case 0x0B:
      texFormat = PVR_TXRFMT_STRIDE | PVR_TXRFMT_NONTWIDDLED;
      break;  //RECTANGULAR STRIDE

    case 0x0D:
      texFormat = PVR_TXRFMT_TWIDDLED;
      break;  //RECTANGULAR TWIDDLED

    case 0x10:
      texFormat = PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED;
      break;  //SMALL VQ

    default:
      texFormat = PVR_TXRFMT_NONE;
      break;
  }

  const int txr_size = texW * texH * Bpp;

  pvr_txr_load(texBuf + PVR_HDR_SIZE, (pvr_ptr_t)buffer, txr_size);

  *w = texW;
  *h = texH;
  *txrFormat = texFormat | texColor;

  return buffer;
}