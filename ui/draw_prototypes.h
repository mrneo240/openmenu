/*
 * File: draw_prototypes.h
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:33:13 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

/* Called only once at start */
void draw_init(void);
/* called at the start of each frame */
void draw_setup(void);
/* Throws ID into id and returns something if needs to*/
void *draw_load_texture(const char filename, unsigned int *id);
/* draws an image at coords of a given size */
void draw_draw_image(unsigned int id, unsigned int x, unsigned int y, float width, float height);