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

#define UI_NAME LIST_DESC

#define MAKE_FN(name, func) void name##_##func(void)
#define FUNCTION(signal, func) MAKE_FN(signal, func)

FUNCTION(UI_NAME, setup);
FUNCTION(UI_NAME, handle_input);
FUNCTION(UI_NAME, draw);
