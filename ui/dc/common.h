/*
 * File: common.h
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:01:31 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

#include <dc/pvr.h>
#include <dc/fmath.h>

/* Offset and dimensions of each sprite within a spritesheet (romdisk/foo.txt file) */
typedef struct image {
	char        name[16];
	uint32    width, height;
    uint32    format;
    pvr_ptr_t   texture;
} image;

pvr_ptr_t load_pvr(char const* filename, uint32* w, uint32* h, uint32* txrFormat);

#endif /* COMMON_H */
