/*
 * File: common.c
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:25:54 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */
#include "pvr_texture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../gdrom/gdrom_fs.h"

#define PVR_HDR_SIZE 0x20

static unsigned char* _internal_buf = NULL;
static char filename_safe[128];

static uint32_t pvr_get_texture_size(const void* input, uint32_t* w, uint32_t* h, uint32_t* txrFormat) {
  unsigned char* texBuf = (unsigned char*)input;

  const int texW = texBuf[PVR_HDR_SIZE - 4] | texBuf[PVR_HDR_SIZE - 3] << 8;
  const int texH = texBuf[PVR_HDR_SIZE - 2] | texBuf[PVR_HDR_SIZE - 1] << 8;
  int texFormat = 0, texColor = 0;
  int bpp = 2;

  switch ((unsigned int)texBuf[PVR_HDR_SIZE - 8]) {
    case 0x00:
      texColor = PVR_TXRFMT_ARGB1555;
      bpp = 2;
      break;  //(bilevel translucent alpha 0,255)

    case 0x01:
      texColor = PVR_TXRFMT_RGB565;
      bpp = 2;
      break;  //(non translucent RGB565 )

    case 0x02:
      texColor = PVR_TXRFMT_ARGB4444;
      bpp = 2;
      break;  //(translucent alpha 0-255)

    case 0x03:
      texColor = PVR_TXRFMT_YUV422;
      bpp = 1;
      break;  //(non translucent UYVY )

    case 0x04:
      texColor = PVR_TXRFMT_BUMP;
      bpp = 2;
      break;  //(special bump-mapping format)

    case 0x05:
      texColor = PVR_TXRFMT_PAL4BPP;
      bpp = 1;
      break;  //(4-bit palleted texture)

    case 0x06:
      texColor = PVR_TXRFMT_PAL8BPP;
      bpp = 1;
      break;  //(8-bit palleted texture)

    default:
      texColor = PVR_TXRFMT_RGB565;
      bpp = 2;
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

  const int txr_size = texW * texH * bpp;
  *w = texW;
  *h = texH;
  *txrFormat = texFormat | texColor;

  return txr_size;
}

pvr_ptr_t load_pvr_from_buffer_to_buffer(const void* input, uint32_t* w, uint32_t* h, uint32_t* txrFormat, void* buffer) {
  unsigned char* texBuf = (unsigned char*)input;
  uint32_t txr_size = pvr_get_texture_size(input, w, h, txrFormat);

  pvr_txr_load(texBuf + PVR_HDR_SIZE, (pvr_ptr_t)buffer, txr_size);

  return buffer;
}

pvr_ptr_t load_pvr_from_buffer(const void* input, uint32_t* w, uint32_t* h, uint32_t* txrFormat) {
  pvr_ptr_t rv;
  unsigned char* texBuf = (unsigned char*)input;
  uint32_t txr_size = pvr_get_texture_size(input, w, h, txrFormat);

  if (!txr_size)
    return NULL;

  if (!(rv = pvr_mem_malloc(txr_size))) {
    printf("PVR: Couldn't allocate memory for texture!\n");
    return NULL;
  }
  pvr_txr_load(texBuf + PVR_HDR_SIZE, rv, txr_size);

  return rv;
}

void* pvr_get_internal_buffer(void) {
  if (!_internal_buf) {
    _internal_buf = malloc(512 * 512 * 2);
    if (!_internal_buf) {
	  printf("%s no free memory\n", __func__);
	  return NULL;
    }
  }
  return _internal_buf;
}

static void pvr_read_to_internal(const char* filename) {
  uint32_t texSize;
  file_t tex_fd;
  //memcpy(filename_safe, DISC_PREFIX, strlen(DISC_PREFIX) + 1);
  //strcat(filename_safe, filename);
  snprintf(filename_safe, 127, "%s%s", DISC_PREFIX, filename);

  /* replace all - with _ */
  char* iter = filename_safe;
  while (*iter++) {
    if (*iter == '-')
      *iter = '_';
  }

  unsigned char* texBuf = pvr_get_internal_buffer();
  memset(texBuf, '\0', PVR_HDR_SIZE);

  tex_fd = fs_open(filename_safe, O_RDONLY);
  if (tex_fd == -1) {
    printf("PVR:Error opening %s!\n", filename_safe);
    return;
  }

  fs_seek(tex_fd, 0, SEEK_END);
  texSize = fs_tell(tex_fd);

  fs_seek(tex_fd, 0, SEEK_SET);
  fs_read(tex_fd, texBuf, PVR_HDR_SIZE/2);
  
  if (!strncmp((char *)texBuf,"GBIX", 4))
  {
	   fs_read(tex_fd, texBuf+(PVR_HDR_SIZE/2), texSize - (PVR_HDR_SIZE/2));
  }
  else
  {
	  fs_seek(tex_fd, 0, SEEK_SET);
	  fs_read(tex_fd, texBuf+(PVR_HDR_SIZE/2), texSize);
  }
  
  fs_close(tex_fd);
}

pvr_ptr_t load_pvr(const char* filename, uint32_t* w, uint32_t* h, uint32_t* txrFormat) {
  pvr_read_to_internal(filename);

  return load_pvr_from_buffer(_internal_buf, w, h, txrFormat);
}

pvr_ptr_t load_pvr_to_buffer(const char* filename, uint32_t* w, uint32_t* h, uint32_t* txrFormat, void* buffer) {
  pvr_read_to_internal(filename);

  return load_pvr_from_buffer_to_buffer(_internal_buf, w, h, txrFormat, buffer);
}
