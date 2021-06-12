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
#define INPUT_TIMEOUT (10)
#define FOCUSED_HIRES_FRAMES (60 * 1) /* 1 second load in */

static int screen_row = 0;
static int screen_column = 0;
static int current_starting_index = 0;
static int navigate_timeout = INPUT_TIMEOUT;
static int frames_focused = 0;

static const int items_per_row = 3;
static const int rows = 3;

static bool showing_large_art = false;

/* For drawing */
//static image txr_highlight, txr_bg; /* Highlight square and Background */
static image txr_icon_list[9]; /* Lower list of 9 icons */
static image txr_focus;

extern image txr_highlight, txr_bg; /* Highlight square and Background */
extern image img_empty_boxart;

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

static void draw_large_art(int x, int y, int width, int height) {
  if (showing_large_art) {
    txr_get_large(list_current[current_selected()]->product, &txr_focus);
    if (txr_focus.texture == img_empty_boxart.texture) {
      /* Only draw if large is present */
      return;
    }
    draw_draw_image(x, y, width, height, 1.0f, &txr_focus);
  }
}

  int gutter_top = 20;

  int idx;

static void draw_grid_boxes(void) {
  const int tile_size = 120;
  const int gutter_side = 100;
  const int gutter_top = 20;
  const int horizontal_spacing = 40;
  const int vertical_spacing = 10;
  const int highlight_overhang = 4;

  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < items_per_row; column++) {
      int idx = (row * items_per_row) + column;

      if (current_starting_index + idx < 0) {
        continue;
      }
      if (current_starting_index + idx >= list_len) {
        continue;
      }
      int x_pos = gutter_side + ((horizontal_spacing + tile_size) * column); /* 100 + ((40 + 120)*{0,1,2}) */
      int y_pos = gutter_top + ((vertical_spacing + tile_size) * row);       /* 20 + ((10 + 120)*{0,1,2}) */

      txr_get_small(list_current[current_starting_index + idx]->product, &txr_icon_list[idx]);
      draw_draw_square(x_pos, y_pos, tile_size, 1.0f, &txr_icon_list[idx]);

      if ((current_starting_index + idx) == selected) {
        /* Highlight */
        draw_draw_square(x_pos - 4.0f, y_pos - 4.0f, tile_size + 8, 1.0f, &txr_highlight);
      }
    }
  }
}

  /* If focused, draw large cover art */
  draw_large_art(gutter_side - 4, gutter_top - 4, (tile_size * items_per_row) + (horizontal_spacing * (items_per_row - 1)) + 8, (tile_size * items_per_row) + (vertical_spacing * (rows - 1)) + 8);
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


  frames_focused = 0;
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

  frames_focused = 0;
  screen_column = screen_row = 0;
  current_starting_index = 0;
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

static void menu_show_large_art(void) {
  showing_large_art = true;
}

/* Base UI Methods */

FUNCTION(UI_NAME, init) {
  /* Moved to global init */
  /*
  draw_load_texture("HIGHLIGHT.PVR", &txr_highlight);
  draw_load_texture("BG_RIGHT.PVR", &txr_bg);
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

FUNCTION_INPUT(UI_NAME, handle_input) {
  showing_large_art = false;
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
    case X:
      menu_show_large_art();
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
  font_draw_centered(320, 430, 1.0f, list_current[current_selected()]->name);
}
