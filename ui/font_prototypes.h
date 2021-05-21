/*
 * File: font_prototypes.h
 * Project: ui
 * File Created: Thursday, 20th May 2021 12:35:54 am
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once
int font_init(void);
void font_destroy(void);

void font_begin_draw(void);

void font_draw_main(int x1, int y1, float color, const char *str);
void font_draw_sub(int x1, int y1, float color, const char *str);
void font_draw_sub_wrap(int x1, int y1, float color, int width, const char *str);
