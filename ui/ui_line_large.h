/*
 * File: ui_line_large.h
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:09:42 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

/* buttons are defined in an enum `control` and have these values and names
0 NONE
1 LEFT
2 RIGHT
3 UP
4 DOWN

5 A
6 B
7 X
8 Y

9 START
*/

#define UI_NAME LIST_LARGE

#define MAKE_FN(name, func) void name##_##func(void)
#define FUNCTION(signal, func) MAKE_FN(signal, func)

#define MAKE_FN_INPUT(name, func) void name##_##func(unsigned int button)
#define FUNCTION_INPUT(signal, func) MAKE_FN_INPUT(signal, func)

/* Called once on boot */
FUNCTION(UI_NAME, init);
/* Similar to above for now but may change when swapping interfaces is added */
FUNCTION(UI_NAME, setup);
/* Handles incoming input each frame, your job to manage */
FUNCTION_INPUT(UI_NAME, handle_input);
/* Called per frame to draw your UI */
FUNCTION(UI_NAME, draw);
