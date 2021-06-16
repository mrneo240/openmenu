/*
 * File: ui_line_desc.h
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:29:03 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

/* buttons:
1 LEFT
2 RIGHT
3 UP
4 DOWN

5 A
6 B

7 START
*/

#define UI_NAME LIST_DESC

#define FUNC_NAME(name, func) name##_##func

#define MAKE_FN(name, func) void name##_##func(void)
#define FUNCTION(signal, func) MAKE_FN(signal, func)

#define MAKE_FN_INPUT(name, func) void name##_##func(unsigned int button)
#define FUNCTION_INPUT(signal, func) MAKE_FN_INPUT(signal, func)

FUNCTION(UI_NAME, init);
FUNCTION(UI_NAME, setup);
FUNCTION_INPUT(UI_NAME, handle_input);
FUNCTION(UI_NAME, drawOP);
FUNCTION(UI_NAME, drawTR);
