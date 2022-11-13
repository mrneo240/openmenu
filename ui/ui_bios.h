/*
 * File: ui_bios.h
 * Project: ui
 * File Created: Thursday, 13th October 2022 6:31:04 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2022 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-Clause "New" or "Revised" License, https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#define UI_NAME BIOS

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
FUNCTION(UI_NAME, drawOP);
FUNCTION(UI_NAME, drawTR);
