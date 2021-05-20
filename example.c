/*
 * File: example.c
 * Project: ini_parse
 * File Created: Wednesday, 19th May 2021 2:50:50 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */
/*

gcc example.c ini.c -Os -s  -Xlinker -Map=output.map -ffunction-sections -fdata-sections -Xlinker --gc-sections  -o example

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "backend/gd_list.h"
#include "ui/ui_line_large.h"
#undef UI_NAME
#include "ui/ui_line_desc.h"
#undef UI_NAME

int main(int argc, char** argv) {
  list_read();
  //list_print_temp();
  const struct gd_item** list = list_get_sort_name();
  //list_print(list);
  //list_destroy();

  /*
  LIST_LARGE_setup();
  LIST_LARGE_handle_input();
  LIST_LARGE_draw();
  */

  LIST_DESC_setup();
  LIST_DESC_handle_input();
  LIST_DESC_draw();
}
