/*
 * File: draw_console.h
 * Project: ui
 * File Created: Wednesday, 19th May 2021 11:59:38 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

typedef struct image {
  char name[16];
  unsigned int width, height;
  unsigned int format;
  unsigned int texture;
} image;