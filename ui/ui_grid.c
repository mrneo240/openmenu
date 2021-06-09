/*
 * File: ui_grid.c
 * Project: ui
 * File Created: Saturday, 5th June 2021 10:07:38 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */
#include "ui_grid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/gd_item.h"
#include "../backend/gd_list.h"
#include "../texture/txr_manager.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

extern void ui_cycle_next(void);

/* List managment */
static int screen_row = 0;
static int screen_column = 0;
static int current_starting_index = 0;
#define INPUT_TIMEOUT (10)
static int navigate_timeout = INPUT_TIMEOUT;

static int items_per_row = 3;
static int rows = 3;

/* For drawing */
//static image txr_highlight, txr_bg; /* Highlight square and Background */
static image txr_icon_list[9]; /* Lower list of 9 icons */

extern image txr_highlight, txr_bg; /* Highlight square and Background */

/* Our actual gdemu items */
static const gd_item **list_current;
static int list_len;
enum sort_type { DEFAULT,
                 ALPHA,
                 DATE,
                 PRODUCT,
                 SORT_END };
static enum sort_type sort_current = DEFAULT;

static void draw_bg_layers(void) {
  draw_draw_image(0, 0, 640, 480, 1.0f, &txr_bg);
}

static inline int current_selected(void) {
  return current_starting_index + (screen_row * items_per_row) + (screen_column);
}

static void draw_grid_boxes(void) {
  int tile_size = 120;
  int gutter_side = 100;
  int horizontal_spacing = 40;
  int vertical_spacing = 10;

  int gutter_top = 20;

  int idx;

  int selected = current_selected();

  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < items_per_row; column++) {
      idx = (row * items_per_row) + column;

      if (current_starting_index + idx < 0) {
        continue;
      }
      if (current_starting_index + idx >= list_len) {
        return;
      }

      int x_pos = gutter_side + ((horizontal_spacing + tile_size) * column);
      int y_pos = gutter_top + ((vertical_spacing + tile_size) * row);

      txr_get_small(list_current[current_starting_index + idx]->product, &txr_icon_list[idx]);
      draw_draw_square(x_pos, y_pos, tile_size, 1.0f, &txr_icon_list[idx]);

      if ((current_starting_index + idx) == selected) {
        /* Highlight */
        draw_draw_square(x_pos - 4.0f, y_pos - 4.0f, tile_size + 8, 1.0f, &txr_highlight);
      }
    }
  }
}

FUNCTION(UI_NAME, init) {
  /* Moved to global init */
  /*
  draw_load_texture("/cd/highlight.pvr", &txr_highlight);
  draw_load_texture("/cd/bg_right.pvr", &txr_bg);

  font_init();
  */
}

/* Reset variables sensibly */
FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  screen_column = screen_row = 0;
  current_starting_index = 0;
  navigate_timeout = INPUT_TIMEOUT;
  sort_current = DEFAULT;
}

static void menu_row_up(void) {
  screen_row--;
  if (screen_row < 0) {
    screen_row = 0;
    current_starting_index -= items_per_row;
    if (current_starting_index < 0) {
      current_starting_index = 0;
    }
  }
}

static void menu_row_down(void) {
  screen_row++;
  if (screen_row >= rows) {
    screen_row--;
    current_starting_index += items_per_row;
    if (current_selected() > list_len) {
      current_starting_index -= items_per_row;
    }
  }
  while (current_selected() >= list_len) {
    screen_column--;
    if (screen_column < 0) {
      screen_column = items_per_row - 1;
      menu_row_up();
    }
  }
}

static void menu_up(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  menu_row_up();

  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_down(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  menu_row_down();

  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_left(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }

  screen_column--;
  if (current_starting_index == 0 && screen_column < 0) {
    screen_column = 0;
  }
  if (screen_column < 0) {
    screen_column = items_per_row - 1;
    menu_row_up();
  }
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_right(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  screen_column++;
  if (current_starting_index + (screen_row * items_per_row) + screen_column >= list_len) {
    screen_column--;
  }
  if (screen_column >= items_per_row) {
    screen_column = 0;
    menu_row_down();
  }

  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_accept(void) {
  dreamcast_rungd(list_current[current_selected()]->slot_num);
}

static void menu_swap_sort(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  sort_current++;
  if (sort_current == SORT_END) {
    sort_current = DEFAULT;
  }
  switch (sort_current) {
    case ALPHA:
      list_current = list_get_sort_name();
      break;
    case DATE:
      list_current = list_get_sort_date();
      break;
    case PRODUCT:
      list_current = list_get_sort_product();
      break;
    case DEFAULT:
    default:
      list_current = list_get_sort_default();
      break;
  }
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_cycle_ui(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  ui_cycle_next();
  navigate_timeout = INPUT_TIMEOUT;
}

FUNCTION_INPUT(UI_NAME, handle_input) {
  enum control input_current = button;
  switch (input_current) {
    case LEFT:
      menu_left();
      break;
    case RIGHT:
      menu_right();
      break;
    case UP:
      menu_up();
      break;
    case DOWN:
      menu_down();
      break;
    case A:
      menu_accept();
      break;
    case START:
      menu_swap_sort();
      break;
    case Y:
      menu_cycle_ui();
      break;

    /* These dont do anything */
    case B:
      break;
    case X:
      break;
    /* Always nothing */
    case NONE:
    default:
      break;
  }
  if (screen_row < 0) {
    screen_row = 0;
  }
  if (screen_column < 0) {
    screen_column = 0;
  }
}

FUNCTION(UI_NAME, draw) {
  draw_bg_layers();
  draw_grid_boxes();

  font_begin_draw();
  font_draw_main(20, 430, 1.0f, list_current[current_selected()]->name);
}
