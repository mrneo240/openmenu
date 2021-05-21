/*
 * File: font.h
 * Project: ui
 * File Created: Monday, 3rd June 2019 1:00:21 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */
#pragma once

#ifndef FONT_H
#define FONT_H
#include <dc/pvr.h>

#include "../draw_kos.h"

#define FONT_WIDTH 16
#define FONT_HEIGHT 16
#define FONT_PIC_WIDTH 256
#define FONT_PIC_HEIGHT 256
#define FONT_DOUBLE 2.2
#define FONT_15 1.5
#define FONT_SIZE_MAIN 2.0
#define FONT_SIZE_SUB 1.0
#define FONT_HAS_LC

#define FONT_PERROW (FONT_PIC_WIDTH / FONT_WIDTH)
#define FONT_ROWS ((FONT_PIC_HEIGHT / 2) / FONT_HEIGHT)

pvr_poly_hdr_t font_header;
image font_texture;

#endif /* FONT_H */
