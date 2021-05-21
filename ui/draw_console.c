/*
 * File: draw_console.c
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:40:19 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include <stdio.h>

#include "draw_prototypes.h"

static int texture_id = 0;

/* Called only once at start */
void draw_init(void) {
}

/* called at the start of each frame */
void draw_setup(void) {
}

/* Throws ID into id and returns something if needs to*/
void *draw_load_texture(const char *filename, void *user) {
  unsigned int *id = (unsigned int *)user;
  printf("draw_load_texture(%s)\n", filename);
  *id = ++texture_id;
  return NULL;
}

/* draws an image at coords of a given size */
void draw_draw_image(unsigned int x, unsigned int y, float width, float height, float alpha, void *user) {
  unsigned int id = *(unsigned int *)user;
  printf("draw_image(%d, %d, %.0f, %.0f, %u)\n", x, y, width, height, id);
}

/* draws an image at coords as a square */
void draw_draw_square(unsigned int x, unsigned int y, float size, float alpha, void *user) {
  unsigned int id = *(unsigned int *)user;
  printf("draw_image(%d, %d, %.0f, %.0f, %u)\n", x, y, size, size, id);
}