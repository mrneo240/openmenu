/*
 * File: draw_prototypes.h
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:33:13 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "common.h"

#ifdef _arch_dreamcast
#include "draw_kos.h"
#else
#include "draw_console.h"
#endif

struct dat_file;

/* Called only once at start */
void draw_init(void);

/* called at the start of each frame */
void draw_setup(void);

/* returns default missing texture */
void *draw_load_missing_icon(void *user);
/* Throws pass whatever is relevant to your platform as a pointer and it will filled + returned if successfull, otherwise NULL */
void *draw_load_texture(const char *filename, void *user);
void *draw_load_texture_buffer(const char *filename, void *user, void *buffer);
/* Loads from new DAT file using struct + ID of file requested */
void *draw_load_texture_from_DAT_to_buffer(struct dat_file *bin, const char *ID, void *user, void *buffer);

/* draws an image at coords of a given size */
void draw_draw_image(unsigned int x, unsigned int y, float width, float height, float alpha, void *user);
/* draws an image at coords as a square */
void draw_draw_square(unsigned int x, unsigned int y, float size, float alpha, void *user);

/* exec proto */
void dreamcast_rungd(unsigned int slot_num);
