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

void font_draw_main(int x, int y, float color, const char *str);
void font_draw_sub(int x, int y, float color, const char *str);
void font_draw_sub_wrap(int x, int y, float color, const char *str, int width);
void font_draw_auto_size(int x, int y, float color, const char *str, int width);
void font_draw_centered(int x, int y, float color, const char *str);
