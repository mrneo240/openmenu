/*
 * File: ui_line_desc.c
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:08:07 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "ui_line_desc.h"

#include <stdio.h>

#include "../backend/gd_list.h"

FUNCTION(UI_NAME, init) {
  printf("%s()\n", __func__);
}

FUNCTION(UI_NAME, setup) {
  printf("%s()\n", __func__);
}

FUNCTION_INPUT(UI_NAME, handle_input) {
  printf("%s(%d)\n", __func__, button);
}

FUNCTION(UI_NAME, draw) {
  printf("%s()\n", __func__);
}
