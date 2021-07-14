/*
 * File: ui_menu_credits.h
 * Project: ui
 * File Created: Monday, 12th July 2021 11:40:41 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include "common.h"
#include "global_settings.h"

void menu_setup(enum draw_state* state, uint32_t _text_color, uint32_t _highlight_color);

void handle_input_menu(enum control input);
void handle_input_credits(enum control input);

void draw_menu_op(void);
void draw_menu_tr(void);

void draw_credits_op(void);
void draw_credits_tr(void);
